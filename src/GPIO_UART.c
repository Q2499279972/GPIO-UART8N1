#include "GPIO_UART.h"

//rx
typedef struct UartRxBufferType
{
    unsigned char writeindex;
    unsigned char readindex;
    unsigned char buffer[GPIOUartRxBufferSize];
}UartRxBufferType;

typedef enum GPIOUartRxStateType
{
    GPIOUartRxState_Idle,
    GPIOUartRxState_ReadingStart,
    GPIOUartRxState_ReadingData,
    GPIOUartRxState_ReadingStop,
}GPIOUartRxStateType;

static unsigned int UartRxSampleTimeFirst;
static unsigned int UartRxSampleTimePeriod;
static UartRxBufferType UartRxBuffer;
volatile GPIOUartRxStateType GPIOUartRxState;
static unsigned int GPIOUartErrorFlag;

void GPIOUartRxBufferInit()
{
    UartRxBuffer.readindex=0;
    UartRxBuffer.writeindex=0;
}

int GPIOUartRxBufferWrite(unsigned char data)
{
    unsigned int i = (UartRxBuffer.writeindex+1)%GPIOUartRxBufferSize;
    if(i==UartRxBuffer.readindex)
    {
        return 0;
    }
    else
    {
        UartRxBuffer.buffer[UartRxBuffer.writeindex]=data;
        UartRxBuffer.writeindex=i;
        return 1;
    }
}

int GPIOUartRxBufferRead(unsigned char *data)
{
    if(UartRxBuffer.writeindex==UartRxBuffer.readindex)
    {
        return 0;
    }
    else
    {
        *data=UartRxBuffer.buffer[UartRxBuffer.readindex];
        UartRxBuffer.readindex++;
        UartRxBuffer.readindex%=GPIOUartRxBufferSize;
        return 1;
    }
}

static void InitSampleTimeList(int band)
{
    int T=(TimerClkFrequency+band/2)/band;
    UartRxSampleTimeFirst = T/2;//To Read Start[0]
    UartRxSampleTimePeriod = T; //To Read Bit0-Bit7 and Stop
    //Most of the time, cumulative errors can be ignore ...
}

void GPIOUartRxInit(int bandrate)
{
    GPIOUartRxIoIrqSet();  
    GPIOUartRxIoIrqDisable();
    GPIOUartRxTimerIrqDisable();

    GPIOUartRxBufferInit();
    InitSampleTimeList(bandrate);
    GPIOUartRxState = GPIOUartRxState_Idle;
    GPIOUartErrorFlag=0;  
    
    GPIOUartRxIoIrqEnable();
}   

void GPIOUartRxGPIOIrqHandle(void)
{
    if(GPIOUartRxState == GPIOUartRxState_Idle)
    {
        GPIOUartRxState = GPIOUartRxState_ReadingStart;
        GPIOUartRxIoIrqDisable();
        GPIOUartRxTimerIrqEnable(UartRxSampleTimeFirst);
    }
}

#if _USE_KEIL_OPTIMIZE_
#pragma push
#pragma Otime O3
#endif
int debug_cnt0;
int debug_cnt1;
int debug_cnt2;
void GPIOUartRxTimerIrqHandler(void)
{
    static int read_cnt;
    static unsigned char read_data;
    int PIN=GPIOUartRxReadPin();
    
    if(GPIOUartRxState==GPIOUartRxState_ReadingStart)
    {
        if(!PIN)
        {
            debug_cnt0++;
            GPIOUartRxState=GPIOUartRxState_ReadingData;
            read_data=0;
            read_cnt=0;
            GPIOUartRxTimerIrqAdjust(UartRxSampleTimePeriod);
        }
        else
        {
            debug_cnt1++;
            GPIOUartErrorFlag|=1;
            GPIOUartRxTimerIrqDisable();
            GPIOUartRxState=GPIOUartRxState_Idle;
            GPIOUartRxIoIrqEnable();
        }
    }
    else if(GPIOUartRxState==GPIOUartRxState_ReadingData)
    {
        read_data>>=1;
        if(PIN)
        {
            read_data|=0x80;
        }
        read_cnt++;
        if(read_cnt==8)
        {
            GPIOUartRxState=GPIOUartRxState_ReadingStop;
        }
    }
    else if(GPIOUartRxState==GPIOUartRxState_ReadingStop)
    {
        if(PIN)
        {
            GPIOUartRxTimerIrqDisable();
            GPIOUartRxState=GPIOUartRxState_Idle;
            GPIOUartRxBufferWrite(read_data);
            GPIOUartRxIoIrqEnable();
        }
        else
        {
            GPIOUartErrorFlag|=2;
            GPIOUartRxTimerIrqDisable();
            GPIOUartRxState=GPIOUartRxState_Idle;
            GPIOUartRxIoIrqEnable();
        }
    }
}
#if _USE_KEIL_OPTIMIZE_
#pragma pop
#endif


//tx
static volatile unsigned int GPIOUartTxSendBuffer;

void GPIOUartTxInit(int bandrate)
{
    GPIOUartTxIoInit();
    GPIOUartTxTimerInit((TimerClkFrequency+bandrate/2)/bandrate);
}

void GPIOUartTxSend(unsigned char data)
{
    GPIOUartTxSendBuffer = data;
    GPIOUartTxSendBuffer <<= 1;
    GPIOUartTxSendBuffer |= 0x00000200;
    GPIOUartTxTimerEnable();
    while(GPIOUartTxSendBuffer!=0);
}
#if _USE_KEIL_OPTIMIZE_
#pragma push
#pragma Otime O3
#endif
void GPIOUartTxTimerHandler()
{
    if(GPIOUartTxSendBuffer == 0)
    {    
        GPIOUartTxTimerDisable();
        return;
    }
    if(GPIOUartTxSendBuffer & 1)
    {
        GPIOUartTxSetIoHigh();
    }
    else
    {
        GPIOUartTxSetIoLow();
    }
    GPIOUartTxSendBuffer >>=1;
}
#if _USE_KEIL_OPTIMIZE_
#pragma pop
#endif

//IRQ  implementation
void TIMER32_0_IRQHandler(void) 
{
    if ( LPC_TMR32B0->IR & 0x01 )
    {
        GPIOUartRxTimerIrqHandler();
        LPC_TMR32B0->IR = 1;
        return;
    }
}

void TIMER32_1_IRQHandler(void)
{
    if ( LPC_TMR32B1->IR & 0x01 )
    {
        GPIOUartTxTimerHandler();
        LPC_TMR32B1->IR = 1;
        return;
    }
}

void PIOINT3_IRQHandler(void)
{
    uint32_t regVal;
    regVal = GPIOIntStatus( 3, 4 );
    if ( regVal )
    {
        debug_cnt2++;
        GPIOUartRxGPIOIrqHandle();
        GPIOIntClear( 3, 4 );
    }        
    return;
}

