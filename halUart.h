#ifndef _HALUART_H_
#define _HALUART_H_


#define UARTPARSER_BUFSIZE   150


char halGetch(void);
BOOL halGetchRdy(void);

void USART1_IRQHandler(void);
void halInitUart(void);


#endif