/*
___         _______                                 _______                 ___________________
   |       |       |       :       :       :       |       |       :       |                   
   | start | bit 0 | bit 1 : bit 2 : bit 3 : bit 4 | bit 5 | bit 6 : bit 7 | stop              
   |_______|       |_______:_______:_______:_______|       |_______:_______|                   

*/
/*
    readme: 8bit,no parity,1 stopbit
*/
#ifndef _GPIO_UART_RX_H_
#define _GPIO_UART_RX_H_

#include "LPC11xx.h"
#include "gpio.h"

#define _USE_KEIL_OPTIMIZE_ 1

#define   TimerClkFrequency (48000000)
#define   GPIOUartRxBufferSize (128)
#if      (GPIOUartRxBufferSize&(GPIOUartRxBufferSize-1))
    #warning [GPIOUartRxBufferSize] SHOULD BE POWER OF 2 FOR OPTIMISING
#endif
#if      (GPIOUartRxBufferSize>256)
    #error   [GPIOUartRxBufferSize] SHOULD <= 256
#endif

#define   GPIOUartRxIoIrqSet()        GPIOSetInterrupt(3,4,0,0,0);LPC_GPIO3->IC|=(0x1<<4);LPC_GPIO3->IE|=(1<<4);
#define   GPIOUartRxIoIrqDisable()    LPC_GPIO3->IE&=~(1<<4);
#define   GPIOUartRxIoIrqEnable()     LPC_GPIO3->IC|=(0x1<<4);LPC_GPIO3->IE|=(1<<4);
#define   GPIOUartRxTimerIrqEnable(t) LPC_TMR32B0->MR0=t; LPC_TMR32B0->MCR=3;LPC_TMR32B0->TCR=1;    
#define   GPIOUartRxTimerIrqAdjust(t) LPC_TMR32B0->TCR=0;LPC_TMR32B0->TC=0;LPC_TMR32B0->MR0=t;LPC_TMR32B0->TCR=1;
#define   GPIOUartRxTimerIrqDisable() LPC_TMR32B0->TCR=0;
#define   GPIOUartRxReadPin()         ((LPC_GPIO3->DATA)&(1<<4));

#define GPIOUartTxSetIoLow()          LPC_GPIO1->MASKED_ACCESS[1<<9]=0;
#define GPIOUartTxSetIoHigh()         LPC_GPIO1->MASKED_ACCESS[1<<9]=1<<9;

#define GPIOUartTxIoInit()            LPC_GPIO1->DIR|=1<<9;GPIOUartTxSetIoHigh();
#define GPIOUartTxTimerInit(t);       LPC_TMR32B1->MR0=t;LPC_TMR32B1->MCR = 3;    
#define GPIOUartTxTimerEnable();      LPC_TMR32B1->TCR=1;
#define GPIOUartTxTimerDisable();     LPC_TMR32B1->TCR=0;

void GPIOUartRxInit(int bandrate);
int  GPIOUartRxBufferRead(unsigned char *);
void GPIOUartTxInit(int bandrate);
void GPIOUartTxSend(unsigned char data);

#endif
