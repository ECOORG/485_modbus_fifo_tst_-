#include"UartAPI.h"
#include"IO_Logic.h"
#include "mb.h"
#include "mbutils.h"

#define DATA_OK         0X00
#define TIMEOUT_Err     0x01
#define PE_Err          0x02
#define CONFIGDATA_Err  0x03

u8bit RxBuf[80],TxBuf[60],RxCnt,TxCnt;

u8bit data_in_len;              //���ݽ����������ݳ���
u8bit dil;                      //�û����������볤��
u8bit data_out_len;             //���ݽ���������ݳ���
u8bit dol;                      //�û��������������


//uart1������΢��ͨ��

void Uart1Init(u32bit BaudRate)
{
    STR_UART_T Uart1Param;
    /* UART Setting */
    Uart1Param.u32BaudRate      = BaudRate;                 //������115.2K
    Uart1Param.u8cDataBits      = DRVUART_DATABITS_8;       //8λ����λ
    Uart1Param.u8cStopBits      = DRVUART_STOPBITS_1;       //1λֹͣλ
    Uart1Param.u8cParity        = DRVUART_PARITY_EVEN;      //żУ��
    Uart1Param.u8cRxTriggerLevel= DRVUART_FIFO_1BYTES;      //1�ֽ����ݻ�����
    /* Select UART Clock Source From 12Mhz*/
    DrvSYS_SelectIPClockSource(E_SYS_UART_CLKSRC,0); 
    DrvUART_Open(UART_PORT1, &Uart1Param);
}


u8bit Uart1Send(u8bit ch)
{  
    while (UART1->FSR.TE_FLAG !=1);
    UART1->DATA = ch;                                       /* Send UART Data from buffer */
    return ch;
}


u8bit Uart1Receive(u8bit *ch)
{

    while(UART1->FSR.RX_EMPTY);

    if(UART1->FSR.PEF){
        UART1->FSR.PEF = 1;                                 //��λ��żУ���־
        return PE_Err;
    }
    *ch = UART1->DATA;                                      //����һ���ַ��ʹ������뻺����
    return DATA_OK;
}


u32bit DataExInit(void)
{
    u8bit cnt;
    u8bit err_code = 0,CheckSum;
    u32bit i,u32BaudRate;

    data_in_len = 2;
    data_out_len = 2;
    S_RTS = 0;                                              //�ӿڰ��������
    REQ_IT = 1;                                             //�ӿڰ������ʼ������

    //�ȴ����ճ�ʼ������
    for(i=0; i<49; i++){
        err_code = Uart1Receive(&RxBuf[i]);
        if(err_code) break;
    }
    S_RTS = 1;                                              //�ӿڰ��ֹ��������

    if(err_code == DATA_OK){
        //�����ۼӺ�
        CheckSum=0;
        for (cnt=0;cnt<48;cnt++)    CheckSum=CheckSum+RxBuf[cnt];

        //�����������ݱ��ĵĳ���
//        for(cnt = 0; cnt < RxBuf[3];cnt++){
//            if((RxBuf[4+cnt] >= 0x10) && (RxBuf[4+cnt] < 0x20)){
//                data_in_len = data_in_len + RxBuf[4+cnt] - 0x0F;
//            }
//            else if((RxBuf[4+cnt] >= 0x20) && (RxBuf[4+cnt] < 0x30)){
//                data_out_len = data_out_len + RxBuf[4+cnt] - 0x1F;
//            }
//            else err_code = CONFIGDATA_Err;
//        }
        data_in_len     = RxBuf[25];
        data_out_len    = RxBuf[24];
        dil=data_in_len-1;                                  //data_in_len=50,dil=49
        dol=data_out_len-1;                                 //data_out_len=50,dol==49
    }

    //���ĵ���ȷ����֤
    if(i != 49){                                            //���ĳ��ȴ���
        TxBuf[1] = 6;
    }
    else if(err_code == PE_Err){                            //��żУ�����
        TxBuf[1] = 5;
    }
    else if(CheckSum != RxBuf[48]){                         //У��ʹ���
        TxBuf[1] = 4;
    }
    else if(RxBuf[0]>126){                                  //վ�Ŵ���
        TxBuf[1] = 3;
    }
    else if(RxBuf[3]>8){                                    //�������ó��ȴ���
        TxBuf[1] = 2;
    }
    else if(err_code == CONFIGDATA_Err){
        TxBuf[1] = 1;
    }
    else{
        TxBuf[1] = 0;
    }

    //��ʼ��Ӧ��������
    TxBuf[0] = GPC_2;                                       //�������õĲ����ʴ���
    TxBuf[0] = (TxBuf[0]<<1) + GPC_1;
    TxBuf[0] = (TxBuf[0]<<1) + GPC_0;
    if(TxBuf[1]==0){                                        //������ȷӦ����
        for(i=2;i<48;i++)   TxBuf[i] = 0xaa;
        switch(RxBuf[12]){
            case 0:
                u32BaudRate = 1200;
                break;
            case 1:
                u32BaudRate = 2400;
                break;
            case 2:
                u32BaudRate = 4800;
                break;
            case 3:
                u32BaudRate = 9600;
                break;
            case 4:
                u32BaudRate = 19200;
                break;
            case 5:
                u32BaudRate = 38400;
                break;
            case 6:
                u32BaudRate = 57600;
                break;
            case 7:
                u32BaudRate = 115200;
                break;
            case 8:
                u32BaudRate = 125000;
                break;
            case 9:
                u32BaudRate = 187500;
                break;
            default:
                u32BaudRate = 9600;
                break;
        }
        eMBInit(MB_RTU,RxBuf[0],1,u32BaudRate,(eMBParity)RxBuf[13]);    //��ʼ���ӿڰ�
        eMBSetSlaveID(0x01,0xff,"BHYW-DP,CODE:0-01-00",21);
        eMBEnable();
        RDY = 1;                                            //���ճ�ʼ���������ָʾ
        REQ_IT = 0;                                         //�������ݽ���״̬
    }
    else{                                                   //���ɴ���Ӧ����
        for(i=2;i<48;i++)   TxBuf[i] = 0x55;
    }
    TxBuf[48]=0;                                            //����Ӧ�����ۼӺ�.
    for(i=0;i<48;i++)   TxBuf[48]=TxBuf[48]+TxBuf[i];

    //����Ӧ����
    for(i=0; i<49; i++){
        Uart1Send(TxBuf[i]);
    }
    DelayUs(200);
    //��շ���������
    for(i=0; i<49; i++){
        TxBuf[i] = 0;
    }
    //���س�ʼ���Ƿ���ɵ���Ϣ
    if(TxBuf[1]==0) return E_SUCCESS;
    else    return 1;
}

void DataExRoutine(void)
{
    u8bit cnt;
    u8bit err_code = 0,CheckSum;
    u32bit i;

    S_RTS = 0;                                              //�ӿڰ�������ݽ���
    REQ_IT = 0;                                             //�ӿڰ������ʼ������

    //�ȴ����ճ�ʼ������
    for(i=0; i<data_in_len; i++){
        err_code = Uart1Receive(&RxBuf[i]);
        if(err_code)
            break;
    }
    S_RTS = 1;                                              //�ӿڰ��ֹ��������

    //���ݽ������Ĳ�����
    if(err_code == DATA_OK){
        //�����ۼӺ�
        CheckSum=0;
        for (cnt=0;cnt<dil;cnt++)   CheckSum=CheckSum+RxBuf[cnt];
        TxBuf[0] = 0;
    }
    if(err_code == PE_Err){
        TxBuf[0] = 1;
    }
    else if(CheckSum != RxBuf[dil]){
        TxBuf[0] = 2;
    }
    PBF = 1;
    //����modbus�Ĵ���
    TxBuf[35] &= ~RxBuf[48];                                //������־�Ĵ���ˢ��
    TxBuf[36] &= ~RxBuf[49];                                //������־�Ĵ���ˢ��
    if(RxBuf[47] & BIT3)    TxBuf[34] &= ~BIT3;
    if(RxBuf[48] & BIT7)    TxBuf[35] &= ~BIT7;
    //TxBuf[34] &= ~(RxBuf[47] & (BIT3 | BIT4));
    if(TxBuf[0] == 0){
        //ModbusRegRefrash();
    }

    //����У���
    TxBuf[dol]=0;
    for (cnt=0;cnt<dol;cnt++)   TxBuf[dol] += TxBuf[cnt];
    //����Ӧ�����ݱ���.
    for(i=0; i<data_out_len; i++){
        Uart1Send(TxBuf[i]);
    }
    DelayUs(500);
}



/* ----------------------- Defines ------------------------------------------*/
#define REG_INPUT_START 1
#define REG_INPUT_NREGS 12
#define REG_HOLDING_START 1
#define REG_HOLDING_NREGS 28
#define REG_COILS_START 1
#define REG_COILS_NREGS 8
#define REG_DISCRETE_START 1
#define REG_DISCRETE_NREGS 48
/* ----------------------- Static variables ---------------------------------*/

static u16bit	usRegInputStart = REG_INPUT_START;
static u16bit	usRegInputBuf[REG_INPUT_NREGS];
static u16bit	usRegHoldingStart = REG_HOLDING_START;
static u16bit	usRegHoldingBuf[REG_HOLDING_NREGS];
static u16bit	usRegCoilsStart = REG_COILS_START;
static u8bit	usRegCoilsBuf[(REG_COILS_NREGS/32+1)*4];
static u16bit	usRegDiscreteStart = REG_DISCRETE_START;
static u8bit	usRegDiscreteBuf[(REG_DISCRETE_NREGS/32+1)*4];

union{
	u8bit u8bitSD;
	struct{
		u8bit SD1:1;
		u8bit SD2:1;
		u8bit SD3:1;
		u8bit SD4:1;
		u8bit SD5:1;
		u8bit SD6:1;
		u8bit SD7:1;
		u8bit SD8:1;
	}SD;
}SetDoneFlag,SetDoneFlag1;

void MBInputRegRead(void);
void MBHoldingRegRead(void);
void MBHoldingRegWrite(void);
void MBCoilsRead(void);
void MBCoilsWrite(void);
void MBDiscreteRead(void);
//����Ĵ���ˢ�»ص�����
eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if((usAddress >= REG_INPUT_START)
        && (usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS)){
        MBInputRegRead();
        iRegIndex = ( int )( usAddress - usRegInputStart );
        while( usNRegs > 0 ){
            *pucRegBuffer++ = ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ = ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
    }
    else{
        eStatus = MB_ENOREG;
    }
    return eStatus;
}


//����Ĵ���ˢ�»ص�����
eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= REG_HOLDING_START ) &&
        ( usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS ) ){
        iRegIndex = ( int )( usAddress - usRegHoldingStart );
        switch ( eMode ){
            /* Pass current register values to the protocol stack. */
        case MB_REG_READ:
            MBHoldingRegRead();
            while( usNRegs > 0 ){
                *pucRegBuffer++ = ( UCHAR ) ( usRegHoldingBuf[iRegIndex] >> 8 );
                *pucRegBuffer++ = ( UCHAR ) ( usRegHoldingBuf[iRegIndex] & 0xFF );
                iRegIndex++;
                usNRegs--;
            }
            break;

            /* Update current register values with new values from the
             * protocol stack. */
        case MB_REG_WRITE:
            while( usNRegs > 0 ){
                switch(iRegIndex){                          //�������±�ʶˢ��
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                        SetDoneFlag.SD.SD3 = 1;             //�궨�������޸�.
                        break;
                    case 8:
                    case 9:
                    case 18:
                        SetDoneFlag.SD.SD6 = 1;             //�������������޸�.
                        break;
                    case 10:
                    case 15:
                        SetDoneFlag.SD.SD1 = 1;             //�����趨ֵ,�˲�ϵ�����޸�
                        break;
                    case 11:
                    case 19:
                        SetDoneFlag.SD.SD2 = 1;             //��ϸ���л�ֵ,��ϸ���������޸�
                        break;
                    case 12:
                    case 13:
                        SetDoneFlag.SD.SD4 = 1;             //�ֶ����,���ؾ������޸�.
                        break;
                    case 14:
                    case 17:
                    case 21:
                        SetDoneFlag.SD.SD7 = 1;             //��������ֵ,�ջ�У������,�ִ�ʱ��
                        break;
                    case 16:
                    case 20:
                        SetDoneFlag.SD.SD5 = 1;             //����׷��,׷�ٴ���
                        break;
                    case 22:
                    case 23:
                    case 24:
                        SetDoneFlag1.SD.SD1 = 1;            //ʱ������Ѹ���
                        break;
				}
				usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
				usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
				iRegIndex++;
				usNRegs--;
			}
			MBHoldingRegWrite();
		}
	}
	else{
		eStatus = MB_ENOREG;
	}
	return eStatus;
}


//��Ȧ��дˢ�»ص�����
eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

	if( ( usAddress >= REG_COILS_START ) &&
		( usAddress + usNCoils <= REG_COILS_START + REG_COILS_NREGS ) ){
		iRegIndex = ( int )( usAddress - usRegCoilsStart );
		switch ( eMode ){
			/* Pass current register values to the protocol stack. */
		case MB_REG_READ:
			MBCoilsRead();
			while( usNCoils > 0 ){
				if(usNCoils >= 8){
					*pucRegBuffer++ = xMBUtilGetBits(usRegCoilsBuf,iRegIndex,8);
					iRegIndex += 8;
					usNCoils -= 8;
				}
				else{
					*pucRegBuffer++ = xMBUtilGetBits(usRegCoilsBuf,iRegIndex,usNCoils);
					usNCoils = 0;
				}
			}
			break;

            /* Update current register values with new values from the
             * protocol stack. */
		case MB_REG_WRITE:
			while( usNCoils > 0 ){
				if(usNCoils >= 8){
					xMBUtilSetBits(usRegCoilsBuf,iRegIndex,8,*pucRegBuffer++);
					iRegIndex += 8;
					usNCoils -= 8;
				}
				else{
					xMBUtilSetBits(usRegCoilsBuf,iRegIndex,usNCoils,*pucRegBuffer++);
					usNCoils = 0;
				}
			}
			MBCoilsWrite();
		}
	}
	else{
		eStatus = MB_ENOREG;
	}
	return eStatus;
}
//��ɢ������ˢ�»ص�����
eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if((usAddress >= REG_INPUT_START)
        && (usAddress + usNDiscrete <= REG_DISCRETE_START + REG_DISCRETE_NREGS)){
        MBDiscreteRead();
        iRegIndex = ( int )( usAddress - usRegDiscreteStart );
        while( usNDiscrete > 0 ){
            if(usNDiscrete >= 8){
                *pucRegBuffer++ = xMBUtilGetBits(usRegDiscreteBuf,iRegIndex,8);
                iRegIndex += 8;
                usNDiscrete -= 8;
            }
            else{
                *pucRegBuffer++ = xMBUtilGetBits(usRegDiscreteBuf,iRegIndex,usNDiscrete);
                usNDiscrete = 0;
            }
        }
    }
    else{
        eStatus = MB_ENOREG;
    }
    return eStatus;
}


//����Ĵ���������
void MBInputRegRead(void)
{
    *((u32bit *)usRegInputBuf)          = (RxBuf[1]<<24) + (RxBuf[2]<<16)
                                        + (RxBuf[3]<<8) + RxBuf[4];             //δ��Ʊ任�ľ���
    usRegInputBuf[2]                    = (RxBuf[5]<<8) + RxBuf[6];             //�ϰ�ˮ����
    usRegInputBuf[3]                    = (RxBuf[7]<<8) + RxBuf[8];             //��ǰʱ��
    usRegInputBuf[4]                    = *((u16bit *)(RxBuf + 42));            //�������󱨾�
    usRegInputBuf[5]                    = *((u16bit *)(RxBuf + 44));            //IO״̬
    usRegInputBuf[6]                    = *((u16bit *)(RxBuf + 46)) & 0x00ff;   //IO״̬
    usRegInputBuf[7]                    = (RxBuf[27]<<8) + RxBuf[28];           //����������
    usRegInputBuf[8]                    = (RxBuf[25]<<8) + RxBuf[26];
}
//���ּĴ���������
void MBHoldingRegRead(void)
{
    *((u32bit *)(usRegHoldingBuf + 0))  = (RxBuf[9]<<24) + (RxBuf[10]<<16)
                                        + (RxBuf[11]<<8) + RxBuf[12];           //����������
    *((u32bit *)(usRegHoldingBuf + 2))  = (RxBuf[13]<<24) + (RxBuf[14]<<16)
                                        + (RxBuf[15]<<8) + RxBuf[16];           //�������ֵ(AD�õ�)
    *((u32bit *)(usRegHoldingBuf + 4))  = (RxBuf[17]<<24) + (RxBuf[18]<<16)
                                        + (RxBuf[19]<<8) + RxBuf[20];           //����ֵ
    *((u32bit *)(usRegHoldingBuf + 6))  = (RxBuf[21]<<24) + (RxBuf[22]<<16)
                                        + (RxBuf[23]<<8) + RxBuf[24];           //Ƥ��
    *((u32bit *)(usRegHoldingBuf + 8))  = (RxBuf[25]<<24) + (RxBuf[26]<<16)
                                        + (RxBuf[27]<<8) + RxBuf[28];           //����������
    usRegHoldingBuf[10]                 = (RxBuf[29]<<8) + RxBuf[30];           //�����趨ֵ
    usRegHoldingBuf[11]                 = (RxBuf[31]<<8) + RxBuf[32];           //��ϸ���л�ֵ
    usRegHoldingBuf[12]                 = (RxBuf[33]<<8) + RxBuf[34];           //���ֵ
    usRegHoldingBuf[13]                 = (RxBuf[35]<<8) + RxBuf[36];           //����ֵ
    usRegHoldingBuf[14]                 = (RxBuf[37]<<8) + RxBuf[38];           //��������ֵ
    usRegHoldingBuf[15]                 = RxBuf[39];                            //�˲�ϵ��
    usRegHoldingBuf[16]                 = RxBuf[40];                            //���ٴ���
    usRegHoldingBuf[17]                 = RxBuf[41];                            //�ִ�ʱ��
    if(RxBuf[47] & BIT4)    usRegHoldingBuf[18] = 1;                            //��������������
    else                    usRegHoldingBuf[18] = 0;
    if(RxBuf[47] & BIT5)    usRegHoldingBuf[19] = 1;                            //��ϸ���л�����
    else                    usRegHoldingBuf[19] = 0;
    if(RxBuf[47] & BIT6)    usRegHoldingBuf[20] = 1;                            //����׷�ٿ���
    else                    usRegHoldingBuf[20] = 0;
    if(RxBuf[47] & BIT7)    usRegHoldingBuf[21] = 1;                            //�ջ�У������
    else                    usRegHoldingBuf[21] = 0;
    usRegHoldingBuf[22]                 = RxBuf[50];
    usRegHoldingBuf[23]                 = RxBuf[51];
    usRegHoldingBuf[24]                 = RxBuf[52];
}

//���ּĴ���д����
void MBHoldingRegWrite(void)
{
    TxBuf[1] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 0)) >> 24) & 0XFF);      //����������
    TxBuf[2] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 0)) >> 16) & 0XFF);
    TxBuf[3] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 0)) >> 8) & 0XFF);
    TxBuf[4] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 0)) >> 0) & 0XFF);

    TxBuf[5] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 2)) >> 24) & 0XFF);      //�������ֵ(AD�õ�)
    TxBuf[6] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 2)) >> 16) & 0XFF);
    TxBuf[7] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 2)) >> 8) & 0XFF);
    TxBuf[8] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 2)) >> 0) & 0XFF);

    TxBuf[9]  = (u8bit)((*((u32bit *)(usRegHoldingBuf + 4)) >> 24) & 0XFF);      //����ֵ
    TxBuf[10] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 4)) >> 16) & 0XFF);
    TxBuf[11] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 4)) >> 8) & 0XFF);
    TxBuf[12] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 4)) >> 0) & 0XFF);

    TxBuf[13] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 6)) >> 24) & 0XFF);      //Ƥ��
    TxBuf[14] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 6)) >> 16) & 0XFF);
    TxBuf[15] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 6)) >> 8) & 0XFF);
    TxBuf[16] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 6)) >> 0) & 0XFF);

    TxBuf[17] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 8)) >> 24) & 0XFF);      //����������
    TxBuf[18] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 8)) >> 16) & 0XFF);
    TxBuf[19] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 8)) >> 8) & 0XFF);
    TxBuf[20] = (u8bit)((*((u32bit *)(usRegHoldingBuf + 8)) >> 0) & 0XFF);

    TxBuf[21] = (u8bit)((*((u16bit *)(usRegHoldingBuf + 10)) >> 8) & 0XFF);
    TxBuf[22] = (u8bit)((*((u16bit *)(usRegHoldingBuf + 10)) >> 0) & 0XFF);

    TxBuf[23] = (u8bit)((*((u16bit *)(usRegHoldingBuf + 11)) >> 8) & 0XFF);
    TxBuf[24] = (u8bit)((*((u16bit *)(usRegHoldingBuf + 11)) >> 0) & 0XFF);

    TxBuf[25] = (u8bit)((*((u16bit *)(usRegHoldingBuf + 12)) >> 8) & 0XFF);
    TxBuf[26] = (u8bit)((*((u16bit *)(usRegHoldingBuf + 12)) >> 0) & 0XFF);

    TxBuf[27] = (u8bit)((*((u16bit *)(usRegHoldingBuf + 13)) >> 8) & 0XFF);
    TxBuf[28] = (u8bit)((*((u16bit *)(usRegHoldingBuf + 13)) >> 0) & 0XFF);

    TxBuf[29] = (u8bit)((*((u16bit *)(usRegHoldingBuf + 14)) >> 8) & 0XFF);
    TxBuf[30] = (u8bit)((*((u16bit *)(usRegHoldingBuf + 14)) >> 0) & 0XFF);

    TxBuf[31] = (u8bit)(usRegHoldingBuf[15] & 0XFF);

    TxBuf[32] = (u8bit)(usRegHoldingBuf[16] & 0XFF);

    TxBuf[33] = (u8bit)(usRegHoldingBuf[17] & 0XFF);

    if(usRegHoldingBuf[18])      TxBuf[34] |= (u8bit)BIT4;
    else      TxBuf[34] &= ~(u8bit)BIT4;
    if(usRegHoldingBuf[19])      TxBuf[34] |= (u8bit)BIT5;
    else      TxBuf[34] &= ~(u8bit)BIT5;
    if(usRegHoldingBuf[20])      TxBuf[34] |= (u8bit)BIT6;
    else      TxBuf[34] &= ~(u8bit)BIT6;
    if(usRegHoldingBuf[21])      TxBuf[34] |= (u8bit)BIT7;
    else      TxBuf[34] &= ~(u8bit)BIT7;

    TxBuf[35] = SetDoneFlag.u8bitSD;
    TxBuf[36] = SetDoneFlag1.u8bitSD;
    TxBuf[37] = (u8bit)(usRegHoldingBuf[22] & 0XFF);
    TxBuf[38] = (u8bit)(usRegHoldingBuf[23] & 0XFF);
    TxBuf[39] = (u8bit)(usRegHoldingBuf[24] & 0XFF);
    
}
//��Ȧ������
void MBCoilsRead(void)
{
    usRegCoilsBuf[0] = (RxBuf[47] & 0x0f) | ((RxBuf[48]>>3) & BIT4);
}
//��Ȧд����
void MBCoilsWrite(void)
{
    TxBuf[34] = usRegCoilsBuf[0] & 0X0F;
    TxBuf[35] |= ((usRegCoilsBuf[0] & BIT4)<<3);
}
//��ɢ�����������
void MBDiscreteRead(void)
{
	usRegDiscreteBuf[0] = RxBuf[43];
	usRegDiscreteBuf[1] = RxBuf[42];
	usRegDiscreteBuf[2] = RxBuf[45];
	usRegDiscreteBuf[3] = RxBuf[44];
	usRegDiscreteBuf[4] = RxBuf[46];
	usRegDiscreteBuf[5] = 0;
}

