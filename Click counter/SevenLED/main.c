#include "stm32f10x.h"
#include "stm32f10x_conf.h"

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_exti.h"
#include "misc.h"

/////////////////////////////////////////////
/////////   Need change Segment PortA (Pin 0-7 to Pin) to Port B
/////////   Because Button is on PA0 pin
//////////////////////////////////////////////

#define  SegmentClearAll    			 GPIOA->ODR |=  Digits[11]
#define  SegmentSetAll          		 GPIOA->ODR &= ~Digits[11]
#define  SegmentSetDigit(digit) 		 GPIOA->ODR &= ~Digits[digit]
#define  SegmentSetDigitWithPoint(digit) GPIOA->ODR &= ~(Digits[digit] | Digits[10])
#define  Digit0On						 GPIOB->ODR &= ~GPIO_Pin_0
#define  Digit0Off  					 GPIOB->ODR |= GPIO_Pin_0
#define  Digit1On						 GPIOB->ODR &= ~GPIO_Pin_1
#define  Digit1Off  			 		 GPIOB->ODR |= GPIO_Pin_1
#define  Digit2On						 GPIOB->ODR &= ~GPIO_Pin_2
#define  Digit2Off  					 GPIOB->ODR |= GPIO_Pin_2
#define  NEGATIVE                        0x10

unsigned int volatile g7LEDCount = 0;
unsigned int volatile count = 0;
unsigned int volatile dig1 = 0;
unsigned int volatile dig2 = 0;
unsigned int volatile dig3 = 0;

const unsigned char Digits[12] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D,
		                           0x7D, 0x07, 0x7F, 0x6F, 0x80, 0xFF };


void Delay(volatile uint32_t nCount) {
	for (; nCount != 0; nCount--);
}

//Button EXTI IRQ Handler
void EXTI0_IRQHandler(void)
{
	{
		EXTI_ClearFlag(EXTI_Line0);

		g7LEDCount++;

		if(g7LEDCount > 10)
		{
			g7LEDCount = 0;
		}
	};
};

void ButtonInit(void)
{
	// Configure frequency on port A
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	// Configure port A for input button on PA0 pin
	GPIO_InitTypeDef GPIO_Config_Port_A;
	GPIO_Config_Port_A.GPIO_Pin = GPIO_Pin_0;
	GPIO_Config_Port_A.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Config_Port_A.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_Config_Port_A);

    // Configure NVIC
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    //Configure EXTI Line0 to generate an interrupt on rising or falling edge
    EXTI_InitTypeDef EXTI_Config;
    EXTI_Config.EXTI_Line = EXTI_Line0;
    EXTI_Config.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_Config.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_Config.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_Config);

    return;
}



void SevenLedSet(int iValue)
{
	dig1 = iValue / 100;
	dig2 = (iValue - dig1*100) / 10;
	dig3 = (iValue - dig1*100) % 10;
}

void SevenLedUpdate(void)
{
			Digit0Off;
	    	Digit1Off;
	    	Digit2Off;

	    	if ((count == 0) && (0 != dig1))
	    	{
	    	   SegmentClearAll;
	    	   SegmentSetDigit(dig1);
	    	   Digit0On;
	    	}
	    	 if ((count == 1) && ((0 != dig1) || (0 != dig2)))
	    	 {
	    	   SegmentClearAll;
	    	   SegmentSetDigit(dig2);
	    	   Digit1On;
	    	 }
	    	if (count == 2)
	    	 {
	    		SegmentClearAll;
	    		SegmentSetDigit(dig3);
	    		Digit2On;
	    	 }
	    	count++;
	    	if (count == 3) count = 0;
}

void Timer2Init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	//enable clocks to tim2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;

    //enable tim2 irq
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure);

    //setting timer 2 interrupt to 200Hz ((2400000/12000)*10)s
    TIM_TimeBaseStructure.TIM_Prescaler = 12000 ;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 10-1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    /* TIM IT enable */
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    /* TIM2 enable counter */
    TIM_Cmd(TIM2, ENABLE);
}

//timer 2 interrupt
void TIM2_IRQHandler(void)
{

	//if interrupt happens the do this
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
    	//clear interrupt and start counting again to get precise freq
    	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

    	SevenLedUpdate();

    }
}

void SevenLedSegmentsPinInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1|
								  GPIO_Pin_2 | GPIO_Pin_3|
								  GPIO_Pin_4 | GPIO_Pin_5|
								  GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init( GPIOA , &GPIO_InitStructure);

	GPIOA->ODR |=  Digits[11];

}

void SevenLedDigitsSelectPinInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1|
								  GPIO_Pin_2 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init( GPIOB , &GPIO_InitStructure);

	GPIOB->ODR &= ~( GPIO_Pin_0 | GPIO_Pin_1| GPIO_Pin_2) ;
}

int main(void) {

	SevenLedDigitsSelectPinInit();
	SevenLedSegmentsPinInit();
	Timer2Init();
	//ButtonInit(); //Uncomment after change

	dig1 = 5;
	dig2 = 5;
	dig3 = 5;

	while(1)
	{
		SevenLedSet(888);
		Delay(0xFFFFF);
	}

	return 0;
}
