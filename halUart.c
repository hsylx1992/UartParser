
#include "halUart.h"


BYTE serio_rxBuff[LRWPAN_ASYNC_RX_BUFSIZE];
BYTE serio_rxHead, serio_rxTail;

//get a character from serial port, uses interrupt driven IO
char halGetch(void)
{
    char x;
    do {
      x = serio_rxHead;  //use tmp because of volt decl
    }  while(serio_rxTail == x);
    serio_rxTail++;
   if (serio_rxTail == LRWPAN_ASYNC_RX_BUFSIZE) serio_rxTail = 0;
   return (serio_rxBuff[serio_rxTail]);
}


BOOL halGetchRdy(void)
{
    char x;
    x = serio_rxHead;
    return(serio_rxTail != x);
}


void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t x,y;
    
        serio_rxHead++;
        if (serio_rxHead == LRWPAN_ASYNC_RX_BUFSIZE ) {
            serio_rxHead = 0;
        }
        x = serio_rxHead;  //use tmp variables because of Volatile decl
        y = USART_ReceiveData(USART1);
        serio_rxBuff[x] = y;       
        
        USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    }
    if(USART_GetFlagStatus(USART1,USART_FLAG_ORE)==SET) {
        //溢出中断        
        USART_ClearFlag(USART1,USART_FLAG_ORE);				//清溢出位
    }
    
}

void halInitUart(void) 
{
    
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    /* System Clocks Configuration */
    /* Enable GPIO clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |RCC_APB2Periph_AFIO, ENABLE);
    /* Enable USART1 Clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); 
    
    /* NVIC configuration */
    NVIC_InitTypeDef NVIC_InitStructure;
    
    /* Enable the USART1 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 7;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    serio_rxHead = 0;
    serio_rxTail = 0;
    #endif
    
    /* Configure the GPIO ports */
    /* Configure USART1 Rx as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* Configure USART1 Tx as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    /* Configure USART1 */
    USART_Init(USART1, &USART_InitStructure);
    
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    /* Enable the USART1 */
    USART_Cmd(USART1, ENABLE);
    
}


void halInitMACTimer(void) 
{

  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  uint16_t PrescalerValue;

  /* PCLK1 = HCLK/4 */
  RCC_PCLK1Config(RCC_HCLK_Div4);
  /* TIM2 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  /* GPIOC clock enable */
 // RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);


  /* Enable the TIM2 global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) (SystemCoreClock * 8 / 1000000) - 1;
 
  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 65535;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
  TIM_PrescalerConfig(TIM2, PrescalerValue, TIM_PSCReloadMode_Immediate);

  #ifdef LRWPAN_ENABLE_SLOW_TIMER

  /* Output Compare Timing Mode configuration: Channel1 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
   // TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
  TIM_OCInitStructure.TIM_Pulse = CCR1_VAL;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

  TIM_OC1Init(TIM2, &TIM_OCInitStructure);
  TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Disable);

  #endif

  /* TIM IT enable */
  #ifdef LRWPAN_ENABLE_SLOW_TIMER
  TIM_ITConfig(TIM2, TIM_IT_Update | TIM_IT_CC1, ENABLE);
  #else
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
  #endif

  mac_timer_upper_byte = 0;

  /* TIM2 enable counter */
  TIM_Cmd(TIM2, ENABLE);

}

// MAC Timer为模拟的20位Timer，由TIM2的16位作为低16位，mac_timer_upper_byte变量的
// 低4位作为模拟Timer的最高位，当TIM2溢出时，中断中对mac_timer_upper_byte加1，
// 当mac_timer_upper_byte超过0x0F时，重置为0.

UINT32 halGetMACTimer(void) 
{
  UINT32 t;
  //BOOL gie_status;

  //SAVE_AND_DISABLE_GLOBAL_INTERRUPT(gie_status);
  t = TIM_GetCounter(TIM2);
  t += ((((UINT32) mac_timer_upper_byte) & 0x000000FF) << 16);
  //RESTORE_GLOBAL_INTERRUPT(gie_status);
  return (t & 0x00FFFFFF);
}

//only works as long as SYMBOLS_PER_MAC_TICK is not less than 1
UINT32 halMacTicksToUs(UINT32 ticks){

   UINT32 rval;

   rval =  (ticks/SYMBOLS_PER_MAC_TICK())* (1000000/LRWPAN_SYMBOLS_PER_SECOND);
   return(rval);
}