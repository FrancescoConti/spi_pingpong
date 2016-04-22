/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    21-April-2016
  * @brief   Default main function.
  ******************************************************************************
*/

#define PING 0
#define PONG 1
#define PING_PONG PING
#define BUFFERSIZE 1

volatile int tick_counter;
volatile int tick_status;

#include "stm32f4xx.h"

void SysTick_Handler(void) {
  if(tick_counter < 12) {
    tick_counter++;
  }
  else {
    tick_counter = 0;
    while(!SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE));
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);
    SPI_I2S_SendData(SPI2, tick_status);
    while(!SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE));
    SPI_I2S_ReceiveData(SPI2);
    GPIO_SetBits(GPIOB, GPIO_Pin_12);
    if(tick_status == 0) {
      GPIO_SetBits(GPIOA, GPIO_Pin_5);
    }
    else {
      GPIO_ResetBits(GPIOA, GPIO_Pin_5);
    }
    tick_status = (tick_status == 0) ? 1 : 0;
  }
}

void SPI2_IRQHandler(void) {
  if (SPI_I2S_GetITStatus(SPI2, SPI_I2S_IT_TXE) == SET) {
    SPI_I2S_SendData(SPI2, 0);
  }
  else if(SPI_I2S_GetITStatus(SPI2, SPI_I2S_IT_RXNE) == SET) {
    uint16_t data = SPI_I2S_ReceiveData(SPI2);
    if(data == 1) {
      GPIO_SetBits(GPIOA, GPIO_Pin_5);
    }
    else {
      GPIO_ResetBits(GPIOA, GPIO_Pin_5);
    }
  }
}

int main(void) {

  // initialize helper variables
  tick_counter = 0;

  // declare data structures
  SPI_InitTypeDef   SPI_InitStructure;
  GPIO_InitTypeDef  GPIO_InitStructure;
  NVIC_InitTypeDef  NVIC_InitStructure;
  RCC_ClocksTypeDef RCC_Clocks;

  // enable SPI clock
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

  // enable GPIO clock
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  // de-initialize GPIO
  GPIO_DeInit(GPIOA);
  GPIO_DeInit(GPIOB);
   
  // connect SPI pins (except NSS) to AF5
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);
   
  // initialize GPIO data structure
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
#if PING_PONG == PING
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
#else
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
#endif
   
  // SPI SCK pin configuration
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // SPI MISO pin configuration
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // SPI MOSI pin configuration
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // SPI NSS pin configuration
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
#if PING_PONG == PING
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
#else
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
#endif
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // led configuration
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // SPI peripheral configuration
  SPI_I2S_DeInit(SPI2);
  SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL              = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
  SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial     = 7;
#if PING_PONG == PING
  SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
#else
  SPI_InitStructure.SPI_Mode              = SPI_Mode_Slave;
#endif
  SPI_Init(SPI2, &SPI_InitStructure);

  // enable SPI peripheral
  SPI_Cmd(SPI2, ENABLE);

#if PING_PONG == PING
  // get current frequency
  RCC_GetClocksFreq(&RCC_Clocks);
  // setup SysTick Timer for 40ms interrupts (should be 10ms in theory...)
  if (SysTick_Config(RCC_Clocks.HCLK_Frequency / 100)) {
    // capture error
    while (1);
  }
  // configure the SysTick handler priority
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  NVIC_InitStructure.NVIC_IRQChannel = SysTick_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
#else
  // enable SPI interrupt
  SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_TXE, ENABLE);
  SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);
  // configure the SPI handler priority
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  NVIC_InitStructure.NVIC_IRQChannel = SPI2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
#endif

  // infinite loop
  for(;;)
    __WFI();

}
