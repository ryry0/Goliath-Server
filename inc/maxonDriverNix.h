
//#include "stdafx.h"
//#include <windows.h>
#include <stdio.h>
#include <ftd2xx.h>
#include <iostream>
#include <cstring>
//#include <conio.h>


#define MAX_DEVICES		5

//FTDI_STATUS dStatus;
FT_HANDLE ftHandle_MAXON;
FT_STATUS ftStatus;

//unsigned char  TxBuffer[14]  = {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x0f,0x00,0x00,0x00,0x8d,0x69};
//unsigned char  TxBuffer1[14] = {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x7f,0x00,0x00,0x00,0x85,0xb1};
unsigned short int CalFieldCRC(unsigned short int *pDataArray, int number_of_data);
void Command_Maxon_Driver4w(unsigned char, unsigned char *);
void Command_Maxon_Driver(unsigned char * command_buffer,
                          unsigned char * read_buffer = NULL,
                          int length_to_read = 0);

void List_Devices(void)
{
  int num_devices;
	char * 	pcBufLD[MAX_DEVICES + 1];
	char 	cBufLD[MAX_DEVICES][64];

	for(int i = 0; i < MAX_DEVICES; i++) //initialize the information buffers.
		pcBufLD[i] = cBufLD[i];

	pcBufLD[MAX_DEVICES] = NULL;

	ftStatus = FT_ListDevices(pcBufLD, &num_devices, FT_LIST_NUMBER_ONLY);
	//FT_LIST_ALL | FT_OPEN_BY_SERIAL_NUMBER);

	if(ftStatus != FT_OK)
		printf("Error: FT_ListDevices(%d)\n", (int)ftStatus);

	for(int i = 0; ( (i <MAX_DEVICES) && (i < num_devices) ); i++)
		printf("Device %d Serial Number - %d\n", i, cBufLD[i]);

}

void Init_Maxon_Motor_Driver(void)
{
  char * serialNum = "602095000131";
  /*
     This can fail if the ftdi_sio driver is loaded
     use lsmod to check this and rmmod ftdi_sio to remove
     also rmmod usbserial
     */

  ftStatus = FT_SetVIDPID(0x0403, 0xa8b0);
  if (ftStatus != FT_OK)
    printf("Could not set device VID and PID");

  ftStatus = FT_OpenEx(serialNum,FT_OPEN_BY_SERIAL_NUMBER,&ftHandle_MAXON);
  ftStatus = FT_SetBaudRate(ftHandle_MAXON, 1000000);

  if (ftStatus == FT_OK)
    printf("Device Successfully Opened\n");

  else
    printf("Failure to Open\n");

}

void Enable_Maxon_Motor_Driver(void)
{
  DWORD BytesWritten_Enable;
  unsigned short int pDataArray[6] = {0};
  unsigned short int CRCTemp;
  unsigned char fault_reset[10] =
    {0x11, 0x04, 0x40, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  unsigned char shutdown[10] =
    {0x11, 0x04, 0x40, 0x60, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00};
  unsigned char switch_on[10] =
    {0x11, 0x04, 0x40, 0x60, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00};
  unsigned char enable[10] =
    {0x11, 0x04, 0x40, 0x60, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00};
  unsigned char set_mode[10] =
    {0x11, 0x03, 0x60, 0x60, 0x00, 0x00, 0x01, 0x00};

  unsigned char check_status[6] =
    {0x10, 0x02, 0x41, 0x60, 0x00, 0x00};
  unsigned char status_buffer[14] = {0};

  Command_Maxon_Driver(check_status, status_buffer, 14);
  for (int i = 0; i < 14; i ++)
    printf("%x ", status_buffer[i]);

  /*
  //shutdown
  unsigned char TxBuffer_Enable1[14] = {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x06,0x00,0x00,0x00,0x1c,0xf7};
  //switch on and enable operation
  unsigned char TxBuffer_Enable2[14] = {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x0f,0x00,0x00,0x00,0x8d,0x69};
  unsigned char TxBuffer_Enable3[14] = {0x90,0x02,0x11,0x04,0x40,0x60,0x00,0x00,0x0f,0x01,0x00,0x00,0x39,0x1f};

  ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_Enable1,14,&BytesWritten_Enable);
  ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_Enable2,14,&BytesWritten_Enable);
  ftStatus = FT_Write(ftHandle_MAXON,TxBuffer_Enable3,14,&BytesWritten_Enable);
  ftStatus = FT_Read(ftHandle_MAXON,TxBuffer_Enable3,10,&BytesWritten_Enable);
  */

  //fault reset
  Command_Maxon_Driver(shutdown);
  Command_Maxon_Driver(enable);
  printf("Motor Driver Enable\n");
}

//                    [ byte ][ byte ][ length *words ]
//a command buffer is [opcode][length][     data      ]
void Command_Maxon_Driver(unsigned char * command_buffer,
                          unsigned char * read_buffer,
                          int length_to_read)
{
  //length is number of words the data portion of the buffer has
  unsigned char lengthw = command_buffer[1];
  unsigned char Tx_Sizeb = lengthw*2 + 6; //total tx frame size in bytes
  //header + data size in bytes
  unsigned char header_data_sizeb = lengthw * 2 + 2;
  //add a zero word on the end
  unsigned char CRC_Buffer_Sizeb = header_data_sizeb + 2;
  unsigned char CRC_Buffer_Sizew = CRC_Buffer_Sizeb / 2;

  //               [word][ byte ][ byte ][ length *words ][  word  ]
  //a Tx buffer is [SYNC][opcode][length][     data      ][CRC Code]
  //thus length of data*2 + 6 bytes.
  unsigned char * Tx_Buffer = new unsigned char[Tx_Sizeb];
  const unsigned char SYNC[2] = {0x90, 0x02};

  unsigned char * CRC_Buffer = new unsigned char[CRC_Buffer_Sizeb];
  memset(CRC_Buffer, 0, CRC_Buffer_Sizeb);
  unsigned short int CRC_Code;
  unsigned int BytesWritten_Enable;

  //copy sync header to Tx_Buffer
  memcpy(Tx_Buffer, SYNC, 2);
  //copy data to Tx_Buffer which is length of data*2 + opcode and length
  memcpy((Tx_Buffer + 2), command_buffer, header_data_sizeb);

  /*
  for (int i = 0; i < header_data_sizeb/2; i++)
  {
    CRC_Buffer[i*2] = command_buffer[i*2 +1];
    CRC_Buffer[i*2 +1] = command_buffer[i*2];
  }
  */
  memcpy(CRC_Buffer, command_buffer, header_data_sizeb);

  CRC_Code = CalFieldCRC((short unsigned int *) CRC_Buffer,
      CRC_Buffer_Sizew)&0xffff;

  Tx_Buffer[Tx_Sizeb-2] = CRC_Code& 0xff;
  Tx_Buffer[Tx_Sizeb-1] = (CRC_Code>>8) &0xff;

  /*
  //for (int i = 0;i < CRC_Buffer_Sizew; i++)
    //printf("%02X\n",*(short unsigned int *)(CRC_Buffer + 2*i));

  //for (int i = 0; i < CRC_Buffer_Sizeb; i++)
    //printf("%02X\n",*(CRC_Buffer + i));

  printf("\n\n%d\n\n", CRC_Buffer_Sizew);
  for (int i = 0;i < Tx_Sizeb; i++)
    printf("%02X\n",Tx_Buffer[i]);
  */

  ftStatus = FT_Write(ftHandle_MAXON, Tx_Buffer, Tx_Sizeb, &BytesWritten_Enable);
  if (read_buffer != NULL)
    ftStatus = FT_Read(ftHandle_MAXON, read_buffer, length_to_read,
        &BytesWritten_Enable);

  delete Tx_Buffer;
  delete CRC_Buffer;
}

//                    [ byte ][ byte ][ length *words ]
//a command buffer is [opcode][length][     data      ]
//this assumes that the length is 4 words
void Command_Maxon_Driver4w(unsigned char opcode, unsigned char * data_buffer)
{
  //               [word][ byte ][ byte ][ length *words ][  word  ]
  //a Tx buffer is [SYNC][opcode][length][     data      ][CRC Code]
  //thus length of data*2 + 6 bytes.
  unsigned char Tx_Buffer[14] = {0x90, 0x02, 0x11, 0x04};
  Tx_Buffer[2] = opcode;
  unsigned short int CRCTemp;
  DWORD BytesWritten_Enable;
  unsigned short int pDataArray[6]= {0};

  //copy the data to the buffer
  memcpy((Tx_Buffer + 4), data_buffer, 4*sizeof(short int));

  for (int i=0; i<5; i++)
    pDataArray[i] =(Tx_Buffer[2*i+3] << 8)  | (Tx_Buffer[2*i+2] & 0xff);

  CRCTemp=CalFieldCRC(pDataArray,6)&0xffff;

  Tx_Buffer[12] = CRCTemp & 0xff;
  Tx_Buffer[13] = (CRCTemp>>8) &0xff;

  //for (int i=0;i<14;i++)
    //printf("%02X\n",Tx_Buffer[i]);
  for (int i=0;i<6;i++)
    printf("%X\n",pDataArray[i]);

  ftStatus = FT_Write(ftHandle_MAXON, Tx_Buffer,14,&BytesWritten_Enable);
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
