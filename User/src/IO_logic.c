#include "DrvGPIO.h"
#include "IO_logic.h"

void GPIOInit(void)
{
    /*�����źŶ˿�,Ϊ���ģʽ*/
    DrvGPIO_Open(E_GPA,11,E_IO_OUTPUT);			//PBF
    DrvGPIO_Open(E_GPA,10,E_IO_OUTPUT);			//RDY
    DrvGPIO_Open(E_GPA,9,E_IO_OUTPUT);			//S_RTS
    DrvGPIO_Open(E_GPA,8,E_IO_OUTPUT);			//REQ_IT
    S_RTS = 1;									//�ӿڰ���������
    REQ_IT = 1;

    //DrvGPIO_Open(E_GPB,3,E_IO_OUTPUT);			//REQ_IT
    //GPB_3 = 1;
    /*���ñ������˿�,Ϊ����ģʽ*/
    DrvGPIO_Open(E_GPC,0,E_IO_INPUT);			//CSO
    DrvGPIO_Open(E_GPC,1,E_IO_INPUT);			//CS1
    DrvGPIO_Open(E_GPC,2,E_IO_INPUT);			//CS2

        
    /*����UART2�˿�,Ϊ����ģʽ*/	
    DrvGPIO_InitFunction(E_FUNC_UART0);	//TX,RX_RX_TX
    DrvGPIO_InitFunction(E_FUNC_UART1_RX_TX);	//RX,TX

    /*����CAN�˿�*/

    PBF = 0;
    RDY = 0;
}

u32bit GetUart1Baud(void)
{
	u32bit Addr = 0;
	Addr = GPC_2;
	Addr = (Addr<<1) + GPC_1;
	Addr = (Addr<<1) + GPC_0;
	switch (Addr){
	case 1:
		return 9600;
	case 2:
		return 19200;
	case 3:
		return 38400;
	case 4:
		return 57600;
	case 5:
		return 115200;
	case 6:
		return 460800;
	default:
		return 9600;
	}
}

