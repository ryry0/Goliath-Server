
//#include "stdafx.h"
//#include <windows.h>
#include <stdio.h>
#include <ftd2xx.h>
#include <iostream>
//#include <conio.h>


#define MAX_DEVICES		5

//FTDI_STATUS dStatus;
FT_HANDLE ftHandle_MAXON;
FT_STATUS ftStatus;

//unsigned char  TxBuffer[14]  = {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x0f,0x00,0x00,0x00,0x8d,0x69};
//unsigned char  TxBuffer1[14] = {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x7f,0x00,0x00,0x00,0x85,0xb1};



void List_Devices(void)
{
  int num_devices;
	char * 	pcBufLD[MAX_DEVICES + 1];
	char 	cBufLD[MAX_DEVICES][64];

	for(int i = 0; i < MAX_DEVICES; i++) //initialize the information buffers.
		pcBufLD[i] = cBufLD[i];

	pcBufLD[MAX_DEVICES] = NULL;

	ftStatus = FT_ListDevices(pcBufLD, &num_devices, FT_LIST_ALL | FT_OPEN_BY_SERIAL_NUMBER);

	if(ftStatus != FT_OK)
		printf("Error: FT_ListDevices(%d)\n", (int)ftStatus);

	for(int i = 0; ( (i <MAX_DEVICES) && (i < num_devices) ); i++)
		printf("Device %d Serial Number - %s\n", i, cBufLD[i]);

}

void Init_Maxon_Motor_Driver(void)
{
  char * serialNum = "602095000131";
  /*
     This can fail if the ftdi_sio driver is loaded
     use lsmod to check this and rmmod ftdi_sio to remove
     also rmmod usbserial
     */
  ftStatus = FT_OpenEx(serialNum,FT_OPEN_BY_SERIAL_NUMBER,&ftHandle_MAXON);

  if (ftStatus == FT_OK)
    printf("Device Successfully Open\n");

  else
    printf("Failure to Open\n");

}

void Enable_Maxon_Motor_Driver(void)
{
    DWORD BytesWritten_Enable;

    unsigned char TxBuffer_Enable1[14] = {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x06,0x00,0x00,0x00,0x1c,0xf7};
    unsigned char TxBuffer_Enable2[14] = {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x0f,0x00,0x00,0x00,0x8d,0x69};
    unsigned char TxBuffer_Enable3[14] = {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x0f,0x01,0x00,0x00,0x39,0x1f};


    ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_Enable1,14,&BytesWritten_Enable);
    ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_Enable2,14,&BytesWritten_Enable);
    ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_Enable3,14,&BytesWritten_Enable);
    printf("Motor Driver Enable\n");

}

void Set_Traj_Params(void)
{
    DWORD BytesWritten_Traj;
    unsigned char TxBuffer_TParams1[14]= {0x90,0x02,0x11,0x04,0x81,0x60,0x00,0x00,0xC4,0x09,0x00,0x00,0x88,0x82};
    unsigned char TxBuffer_TParams2[14]= {0x90,0x02,0x11,0x04,0x83,0x60,0x00,0x00,0x40,0x9C,0x00,0x00,0x2B,0x7F};
    unsigned char TxBuffer_TParams3[14]= {0x90,0x02,0x11,0x04,0x84,0x60,0x00,0x00,0x40,0x9C,0x00,0x00,0x6F,0x66};

    ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_TParams1,14,&BytesWritten_Traj);
    ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_TParams2,14,&BytesWritten_Traj);
    ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_TParams3,14,&BytesWritten_Traj);
}

void Disable_Maxon_Motor_Driver(void)
{
    DWORD BytesWritten_Dis;
    unsigned char TxBuffer_Disable[14] = {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0xbc,0x45};
    ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_Disable,14,&BytesWritten_Dis);

}





unsigned short int CalFieldCRC(unsigned short int *pDataArray, int number_of_data)
{

    unsigned short int shifter,c,carry,CRC = 0;
    int i=0;

    while(number_of_data--)
    {
        shifter = 0x8000;
        c= pDataArray[i];
        i++;
        do
        {
            carry = CRC &0x8000;
            CRC <<=1;

            if (c&shifter)
                CRC++;
            if (carry)
                CRC ^=0x1021;
            shifter>>=1;
        }
        while(shifter);
    }
    return (CRC);
}




void Move_Motor_Abs(int pos)
{


    unsigned char TxBuffer_Move1[14]= {0x90,0x02,0x11,0x04,0x7A,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    unsigned char TxBuffer_Move2[14]= {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x0F,0x00,0x00,0x00,0x8D,0x69};
    unsigned char TxBuffer_Move3[14]= {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x3F,0x00,0x00,0x00,0x28,0xAC };
    unsigned short int pDataArray[6]= {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000};
    unsigned short int CRCTemp;
    DWORD BytesWritten_Move;
    int i;


    for(i=8; i<12; i++)
        TxBuffer_Move1[i] = pos>>(8*(i-8))&0xff;

    for(i=0; i<5; i++)
        pDataArray[i] =(TxBuffer_Move1[2*i+3] << 8)  | (TxBuffer_Move1[2*i+2] & 0xff);

    CRCTemp=CalFieldCRC(pDataArray,6)&0xffff;

    TxBuffer_Move1[12] = CRCTemp & 0xff;
    TxBuffer_Move1[13] = (CRCTemp>>8) &0xff;


    //  for(i=0;i<14;i++)
//       printf("%02X\n",TxBuffer_Move1[i]);

    ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_Move1,14,&BytesWritten_Move);
    ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_Move2,14,&BytesWritten_Move);
    ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_Move3,14,&BytesWritten_Move);

}




void Close_Maxon_Motor_Driver(void)
{

    FT_Close(ftHandle_MAXON);
}

/*
int _tmain(int argc, _TCHAR* argv[])
{



//	unsigned char  RxBuffer[256]={};
//	DWORD BytesWritten;

//	DWORD RxBytes;
//	DWORD TxBytes;
//	DWORD EventDWord;
//	DWORD BytesReceived;
    int pos;
    char ch;



    Init_Maxon_Motor_Driver();

    if (ftStatus != FT_OK)
    {
        return 0;
    }

    Enable_Maxon_Motor_Driver();
    Set_Traj_Params();

    do
    {

        printf("Enter Desired Position: ");
        cin >> pos;


        Move_Motor_Abs(pos);

    }
    while(pos!=123);


    //Disable_Maxon_Motor_Driver();
    Close_Maxon_Motor_Driver();


    return 0;
} */
