/*
  Author: Ryan - David Reyes
  Co-Author: Dr. Oscar Chuy (maxonDriver.h)
*/

#include <iostream>
#include <stdlib.h>
#include <string>

#include "tcpipnix.h"
#include "serialnix.h"

//These definitions enable motors, extra debug, etc at compile time.
//#define ENABLE_STEERING
#define DEBUG
//#define ENABLE_MOTORS
//#define ENABLE_SERIAL

#ifdef ENABLE_STEERING
#include "maxonDriver.h"
#endif

const int PORT = 65534;

char * SERIAL_ADDR      = "/dev/ser1";
const int SOCKET_ERROR  = -1;
const int DATA_ERROR    = 0;

//quick reference variables for ascii characters
const char CTRL_J   = 10;
const char SPACE    = ' '; 
char MA[2]          = {'m','a'};  //command for motor move absolute

const int STEERING_CENTER = 0;


//*FUNCTION PROTOTYPES
//initializes all connections and motors 
bool init(  unsigned int & serialPort, 
            TCP & tcpConnection, 
            unsigned int & clientSocket);

void motorControl(unsigned int & serialPort, char messageType, int value);
void resetMotors(unsigned int & serialPort);

void debugprint(const char messageType, const int value);
//*END FUNCTION PROTOTYPES



//*MAIN*//
int main(int argc, char * argv[])
{
  bool active = true; //flag that controls main loop
  int value;
  char messageType;

  unsigned int serialPort;

  TCP tcpConnection;
  unsigned int clientSocket;

  init(serialPort, tcpConnection, clientSocket); //init everything

  while (active)
  {
    tcpConnection.receiveData(  clientSocket, 
                                (char *) &messageType, 
                                sizeof( messageType ));

    if(tcpConnection.receiveData( clientSocket, 
                                  (char *) &value, 
                                  sizeof( value )) == DATA_ERROR)
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
    }
  }
  std::cout << "Resetting motors..." << std::endl;
  resetMotors(serialPort);

#ifdef ENABLE_STEERING
  Close_Maxon_Motor_Driver();
#endif

  tcpConnection.closeSocket(clientSocket);

  std::cout << "Terminating..." << std::endl;
  return 0;
}
//*END MAIN



//*FUNCTION DEFINITIONS
bool init(  unsigned int & serialPort, 
            TCP & tcpConnection, 
            unsigned int & clientSocket)
{
#ifdef ENABLE_SERIAL
  //initialize serial port
  serialPort = open_port(SERIAL_ADDR);

  if (init_serial_port(serialPort) == DATA_ERROR)
    return false;
#endif

  //initialize TCPIP Connection
  std::cout << "Waiting for TCPIP client..." << std::endl;
  
  tcpConnection.listenToPort(PORT);
  clientSocket = tcpConnection.acceptConnection();

  if (clientSocket != SOCKET_ERROR)
    std::cout << "Connection accepted" << std::endl;
  
  else 
  {
    std::cout << "Failed to accept client" << std::endl;
    tcpConnection.closeSocket(clientSocket);
    return false;
  }

#ifdef ENABLE_STEERING
  //initialize Maxon motors
  Init_Maxon_Motor_Driver();

  if (ftStatus != FT_OK)
  {
    std::cout << "Steering Motor Failure" << std::endl;
    return false;
  }
  
  Enable_Maxon_Motor_Driver();
  //Set_Traj_Params();
#endif
}


void motorControl(unsigned int & serialPort, char messageType, int value)
{
#ifdef ENABLE_MOTORS
  //buffer that holds string of commanded motor value
  char motorvalue[10] = {0};
  sprintf(motorvalue, "%d", value);
  
  //all messages to schneider motors must be encapsulated by ^J

  write(serialPort, (char *) & CTRL_J, 1);
  write(serialPort, (char *) & messageType, 1);
  write(serialPort, MA, 2);

  //send only the ascii text, up to null delimiter
  for (int i = 0; motorvalue[i] != '\0'; i++)
    write(serialPort, (motorvalue + i), 1);

  write(serialPort, (char *) & CTRL_J, 1);
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
