/*
  Author: Ryan - David Reyes
  Co-Author: Dr. Oscar Chuy (maxonDriver.h)
*/

#include <iostream>
#include <stdlib.h>
#include <string>
#include <cstring>

#include <tcpipnix.h>
#include <serialnix.h>

//These definitions enable motors, extra debug, etc at compile time.
#define ENABLE_STEERING
#define DEBUG
#define ENABLE_MOTORS
#define ENABLE_SERIAL

#ifdef ENABLE_STEERING
#include <WinTypes.h>
#include <ftd2xx.h>
#include <maxonDriverNix.h>
#endif


const int PORT = 65534;
const int SOCKET_ERROR  = -1;
const int DATA_ERROR    = -1;
const int SERIAL_ERROR  = -1;

//quick reference variables for ascii characters
const char CTRL_J   = 10;
const char SPACE    = ' ';
char MA[2]          = {'m','a'};  //command for motor move absolute

const int STEERING_CENTER = 0;


//*FUNCTION PROTOTYPES
//initializes all connections and motors
bool initSerial(  unsigned int & serialPort, char * serialAddr );
bool initMotors();

void motorControl(unsigned int & serialPort, char messageType, int value);
void resetMotors(unsigned int & serialPort);

void debugprint(const char messageType, const int value);
//*END FUNCTION PROTOTYPES



//*MAIN*////////////////////////////////////////////////////////
int main(int argc, char * argv[])
{
  bool active = true; //flag that controls main loop
  int value;          //value of received message
  char messageType;
  int pipefd;

  unsigned int  serialPort;

  char * serialAddr = "/dev/ser1";
  char * pipeAddr = "../pipe";

  //Initialization
  //Parse the arguments
  if ( argc >= 2 )
  {
    serialAddr = argv[1];
    if ( initSerial(serialPort, serialAddr) == false )
    {
      std::cout << "Serial port initialization failure.\n";
      return 0;
    }
    if ( initMotors() == false )
    {
      std::cout << "Motor initialization failure.\n";
      return 0;
    }
  }

  pipefd = open_port(pipeAddr);

  while (active)
  {
    //read letter
    read(pipefd, (char *) &messageType, sizeof(messageType));

    //read number
    int numread = read(pipefd, (char *) &value, sizeof(value));
    if(numread == DATA_ERROR)
    {
      active = false;
      std::cout << "Client Disconnected!" << std::endl;
    }

#ifdef DEBUG
    debugprint(messageType, value);
#endif

    if(messageType == 'X')
      active = false;

    else if (active == true)
    {

#ifdef ENABLE_STEERING
      if (messageType == 'S')
        Move_Motor_Abs(value);
      else
#endif
        motorControl(serialPort, messageType, value);
        //watch out for this!
    }
  }
  std::cout << "Resetting motors..." << std::endl;
  resetMotors(serialPort);

#ifdef ENABLE_STEERING
  Close_Maxon_Motor_Driver();
#endif

  std::cout << "Terminating..." << std::endl;
  close(pipefd);
  return 0;
}
//*END MAIN////////////////////////////////////////////////



//*FUNCTION DEFINITIONS
bool initSerial( unsigned int & serialPort, char * serialAddr )
{
#ifdef ENABLE_SERIAL
  //initialize serial port
  serialPort = open_port(serialAddr);

  if (init_serial_port(serialPort) == SERIAL_ERROR)
    return false;
  else
    return true;
#endif
}

bool initMotors()
{
#ifdef ENABLE_STEERING
  //initialize Maxon motors
  //List_Devices();
  Init_Maxon_Motor_Driver();

  if (ftStatus != FT_OK)
    return false;

  Enable_Maxon_Motor_Driver();
  Set_Traj_Params();
  return true;
#endif
#ifndef ENABLE_STEERING
  return true;
#endif
}

void motorControl(unsigned int & serialPort, char messageType, int value)
{
#ifdef ENABLE_MOTORS
  //buffer that holds string of commanded motor value
  char buffer[15] = "\0";
  char motorvalue[10] = {0};

  //write message type to the buffer
  //all messages to schneider motors must be encapsulated by ^J (\n)
  sprintf(buffer, "\n%cma %d\n", messageType, value);

  //send only the ascii text, up to null delimiter
  write(serialPort, buffer, strlen(buffer));
#endif
}


//reset the position of all the motors
void resetMotors(unsigned int & serialPort)
{
#ifdef ENABLE_MOTORS
  motorControl(serialPort, 'G', 0);
  motorControl(serialPort, 'T', 0);
  motorControl(serialPort, 'B', 0);
#endif

#ifdef ENABLE_STEERING
  Move_Motor_Abs(STEERING_CENTER);
#endif
}


//prints all the values of each component to screen
void debugprint(const char messageType, const int value)
{
  int steerVal = 0, throttleVal = 0, brakeVal = 0, gearVal = 0;
  switch(messageType)
  {
    case 'G':
      gearVal = value;
      break;

    case 'T':
      throttleVal = value;
      break;

    case 'B':
      brakeVal = value;
      break;

    case 'S':
      steerVal = value;
      break;
  }
  std::cout << "Msg: " << messageType << "\tThrottle: " << throttleVal;
  std::cout << "\tGear: " << gearVal << "\tBrake: " << brakeVal;
  std::cout << "\tSteering: " << steerVal << std::endl;
}
