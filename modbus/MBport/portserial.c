/* ----------------------- Platform includes --------------------------------*/
#include "port.h"


/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- static functions ---------------------------------*/
static void     UART0Handler(u32bit Uart0IntStatus);
static void     prvvUARTTxReadyISR( void );
static void     prvvUARTRxISR( void );

/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    if(xRxEnable){
        UART0->IER.RDA_IEN = 1;
    }
    else{
        UART0->IER.RDA_IEN = 0;
    }
    if(xTxEnable){
        UART0->IER.THRE_IEN = 1;
        prvvUARTTxReadyISR();
    }
    else{
        UART0->IER.THRE_IEN = 0;
    }
}

void
vMBPortClose( void )
{
}

BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
    BOOL            bInitialized = TRUE;
    STR_RS485_T     RS485FnPara;
    STR_UART_T      Uart0Param;

    ucPORT = ucPORT;                                        //���Ϊʹ�ñ���.
    switch (ucDataBits){                                    //��������λ����,����ֲĬ��ʹ��8λ����λ
    case 8:
        break;
    default:
        bInitialized = FALSE;
    }
    Uart0Param.u32BaudRate = ulBaudRate;                        //���ò�����
    switch(eParity){                                            //�����ֽڸ�ʽ
        case MB_PAR_EVEN:                                       //żУ��
            Uart0Param.u8cDataBits      = DRVUART_DATABITS_8;   //8λ����λ
            Uart0Param.u8cStopBits      = DRVUART_STOPBITS_1;   //1λֹͣλ
            Uart0Param.u8cParity        = DRVUART_PARITY_EVEN;  //żУ��
            break;
        case MB_PAR_ODD:                                        //��У��
            Uart0Param.u8cDataBits      = DRVUART_DATABITS_8;   //8λ����λ
            Uart0Param.u8cStopBits      = DRVUART_STOPBITS_1;   //1λֹͣλ
            Uart0Param.u8cParity        = DRVUART_PARITY_ODD;   //��У��
            break;
        case MB_PAR_NONE:                                       //��У��
            Uart0Param.u8cDataBits      = DRVUART_DATABITS_8;   //8λ����λ
            Uart0Param.u8cStopBits      = DRVUART_STOPBITS_2;   //2λֹͣλ
            Uart0Param.u8cParity        = DRVUART_PARITY_NONE;  //��У��
            break;
        default:                                                //żУ��
            Uart0Param.u8cDataBits      = DRVUART_DATABITS_8;   //8λ����λ
            Uart0Param.u8cStopBits      = DRVUART_STOPBITS_1;   //1λֹͣλ
            Uart0Param.u8cParity        = DRVUART_PARITY_EVEN;  //żУ��
            break;
    }
    Uart0Param.u8cRxTriggerLevel= DRVUART_FIFO_46BYTES;          //���ý��ջ���������
//    Uart0Param.u8TimeOut = 0x7f;                                //����TIMEOUTʱ��
    if(bInitialized){                                           //��ʼ������
        DrvUART_Open(UART_PORT0, &Uart0Param);
        UART0->MCR.LEV_RTS = 0;
        RS485FnPara.u8cAddrEnable = 0;
        RS485FnPara.u8cAddrValue = 0;
        RS485FnPara.u8cDelayTime = 2;
        RS485FnPara.u8cModeSelect = MODE_RS485_AUD;             //�Զ�����ʶ��
        RS485FnPara.u8cRxDisable = 0;
        DrvUART_SetFnRS485(UART_PORT0,&RS485FnPara);
        DrvUART_EnableInt(  UART_PORT0,                         //ʹ�������ж�
                            //DRVUART_THREINT|                  //���ͱ��ּĴ������ж�
                            //DRVUART_RLSINT |                  //���Ͻ����ж�,���BIF,FEF,PEF
                            DRVUART_TOUTINT|                  //�������ݳ�ʱ�ж�
                            //DRVUART_RDAINT,                   //�������ݿ����ж�
                            0,
                            (PFN_DRVUART_CALLBACK*)UART0Handler);
    }
    return bInitialized;
}

BOOL
xMBPortSerialPutByte( CHAR *ucByte , UCHAR cnt)
{
    while(cnt){
        UART0->DATA = *ucByte;
        cnt --;
        ucByte ++;
    }
    return TRUE;
}

BOOL
xMBPortSerialGetByte( UCHAR * pucByte , UCHAR * cnt)
{
    while(UART0->FSR.RX_POINTER){
        (*cnt)++;
        *pucByte = UART0->DATA;
        pucByte++;
    }
    return TRUE;
}

static void
UART0Handler(u32bit Uart0IntStatus)
{   
    if(UART0->ISR.RLS_INT){                                 //���RLS�жϱ�־
        if(UART0->FSR.BIF)  UART0->FSR.BIF = 1;
        if(UART0->FSR.FEF)  UART0->FSR.FEF = 1;
        if(UART0->FSR.PEF)  UART0->FSR.PEF = 1;
    }
    if(UART0->ISR.TOUT_INT){
        (void)pxMBPortCBTimerExpired();
    }
    if(UART0->ISR.RDA_INT){
        prvvUARTRxISR();
    }
    if(UART0->ISR.THRE_INT){
        prvvUARTTxReadyISR();
    }
}



/* 
 * Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
static void
prvvUARTTxReadyISR(void)
{
    pxMBFrameCBTransmitterEmpty();
}

/* 
 * Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void
prvvUARTRxISR(void)
{
    pxMBFrameCBByteReceived();
}


u8bit Rs485SendChar(u8bit ch)
{
    while (UART0->FSR.TE_FLAG !=1);
    UART0->DATA = ch;                                       /* Send UART Data from buffer */
    return ch;
}
void Rs485sPrint(u8bit *Str)
{
    while(*Str){
        Rs485SendChar(*Str);
        Str++;
    }
}