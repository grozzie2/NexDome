#include <EEPROM.h>


/*
 *  This package includes the drivers and sources for the NexDome
 *  Copyright (C) 2016 Rozeware Development Ltd.
 *
 *  NexDome is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NexDome is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NexDome files.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *   This file contains:-
 *   Firmware for the arduino leonardo in the NexDome rotation controller
 *   Hardware is the leonardo clone, with an xbee series 1 module
 *   and a TB6600 based stepper motor driver
 */

//  Libraries we need to include
#include <AccelStepper.h>
#include <EEPROM.h>

#define VERSION_MAJOR 0
#define VERSION_MINOR 10

/*
 *   Arduino pins
 *   
 *   0 - Serial
 *   1 - Serial
 *   2 - Irq Sensor (home for rotator, open for shutter)
 *   3 - Irq Sensor (unused rotator, closed shutter)
 *   4 -
 *   5 - Button
 *   6 - Button
 *   7 -
 *   8 - Reserved for AltSoftSerial if not on leonardo
 *   9 - Soft serial for AltSoftSerial if not on Leonardo
 *   10- Motor Enable
 *   11- Motor Dir
 *   12- Motor Step
 *   13- On board led, we dont use it
 *   
 *   These 3 used only when the small prototype hacked for the big easy driver
 *   is in use for bench testing
 *   A0- MS1
 *   A1- MS2
 *   A2- MS3
 * 
 */

//  Pin definitions
//  on the uno, ie 328 based units, only pins 2 and 3 are available for interrupts
//  for the rotator, home sensor will be on pin2 and for the shutter, both pins used for limit switches
//  home sensor is on pin 2, we use interrupts here to detect home sensor
#define HOME 2
//  We dont use the irq on pin 3 for the dome rotator
//  but we do use it for the shutter switches
#define OTHER_IRQ 3

/*
 *   Defining which pins for serial gets tricky
 *   on the leonardo, no big deal, we just use the standard hardware serial port on pins 0 and 1
 *   but for uno etc, if we intelligently choose our serial pins, we can use alternate better
 *   performing libraries
 *   
 *   SoftSerial can go on any pins
 *   AltSoftSerial can only go on tx 9 rx 8 on the uno (or other atmega328) boards
 *   There is also 'yetanothersoftserial', but it uses the two interrupt pins, 2 and 3
 *   which we want for our sensors
 *   
 */
#define SERIAL_TX 9
#define SERIAL_RX 8


//  Enable line for the stepper driver
#define EN 10
//  Direction line for the stepper driver
#define DIR 11
//  Step line for the stepper driver
#define STP 12
//  Our two buttons for manual movement
#define BUTTON_EAST 5
#define BUTTON_WEST 6


//  Physical definitions of the gears
//  This is the gearbox on the motor
//float ReductionGear=(float)15+((float)3/(float)10);
#define REDUCTION_GEAR 15.3
#define DOME_TEETH 288
#define GEAR_TEETH 15
#define DOME_TURN_TIME 90
// set this to match the type of steps configured on the
// stepper controller
#define STEP_TYPE 4


//  For reading our input voltage 
#define VPIN A0

//  if we define our serial ports this way, then re-compiling for different port assignments
//  on different processors, becomes easy and we dont have to hunt all over chaning
//  code to reference different serial definitions
#define Computer Serial
#define Wireless Serial1


#define SHUTTER_STATE_NOT_CONNECTED 0
#define SHUTTER_STATE_OPEN 1
#define SHUTTER_STATE_OPENING 2
#define SHUTTER_STATE_CLOSED 3
#define SHUTTER_STATE_CLOSING 4
#define SHUTTER_STATE_UNKNOWN 5

//  Globals needed
//  this one is set by the home sensor interrupt routines
//  needs to be volatile because of access during interrupts
volatile bool HomeSensor=false;
long int LastWirelessCheck;
int ShutterState=SHUTTER_STATE_NOT_CONNECTED;
float ShutterPosition=0;
int ShutterQueryTime;

int BatteryVolts=0;
int ShutterBatteryVolts=0;

//  The dome class will use an accel stepper object to run the motor
AccelStepper accelStepper(AccelStepper::DRIVER, STP, DIR);

#define EEPROM_LOCATION 10
#define SIGNATURE 1036
typedef struct DomeConfiguration {
  int signature;
  long int StepsPerDomeTurn;
  float HomeAzimuth;
  float ParkAzimuth;
  float Checksum;
} dome_config;

void ConfigureWireless();


//  Now declare the class for our dome
class NexDome
{
  private:
    long int TargetSteps;
    long int LastHomeCount;
  public:
    NexDome();
    ~NexDome(){}; //  empty destructor because it'll never get used

    //  exposed variables
    bool Active;  //  if the motor is in motion this is true
                  //  exposed publicly because it's performance sensative to reference
    bool FindingHome;
    bool Calibrating;
    bool isAtHome;
    
    float HomeAzimuth;
    float ParkAzimuth;
    long int StepsPerDomeTurn;
    //  public functions
    int SetStepsPerDomeTurn(long int);
    void MoveTo(unsigned long int);
    long int CurrentPosition();
    void Stop();
    bool Run();
    void EnableMotor();
    void DisableMotor();
    float GetHeading();
    int SetHeading(float);
    int AtHome();   //  called by interrupt routines when home sensor is detected
    int FindHome();
    int Calibrate();
    int Sync(float);
    //int SetHomeAzimuth(float);
    //float GetHomeAzimuth();
    long int AzimuthToTicks(int);
    bool SaveConfig();
    bool ReadConfig();

    int WriteCalibrationToEEprom();
    int ReadCalibrationFromEEprom();
};

// Every class needs a constructor
NexDome::NexDome()
{
  Active=false; //  we are not in motion when we start up
  TargetSteps=0;
  LastHomeCount=0;
  FindingHome=false;
  //SettingHome=false;
  Calibrating=false;
  isAtHome=false;
  HomeAzimuth=180;
  ParkAzimuth=180;
  //EnableMotor();
  //DisableMotor();
  accelStepper.setPinsInverted(true,false,false);
}

bool NexDome::SaveConfig()
{
  dome_config cfg;

  //  zero the structure so currently unused parts
  //  dont end up loaded with random garbage
  memset(&cfg,0,sizeof(cfg));

  cfg.signature=SIGNATURE;
  cfg.StepsPerDomeTurn=StepsPerDomeTurn;
  cfg.HomeAzimuth=HomeAzimuth;
  cfg.ParkAzimuth=ParkAzimuth;

  EEPROM.put(EEPROM_LOCATION,cfg);
  
}

bool NexDome::ReadConfig()
{
  dome_config cfg;

  //  zero the structure so currently unused parts
  //  dont end up loaded with random garbage
  memset(&cfg,0,sizeof(cfg));
  EEPROM.get(EEPROM_LOCATION,cfg);

  if(cfg.signature != SIGNATURE) return false;
  SetStepsPerDomeTurn(cfg.StepsPerDomeTurn);
  HomeAzimuth=cfg.HomeAzimuth;
  ParkAzimuth=cfg.ParkAzimuth;
  return true;
}


int NexDome::SetStepsPerDomeTurn(long int steps)
{
  long int StepsPerSecond;
  
  StepsPerDomeTurn=steps;
  StepsPerSecond=StepsPerDomeTurn/DOME_TURN_TIME;
  //Computer.print(StepsPerSecond);
  //Computer.println(" Steps per second");
  accelStepper.setMaxSpeed(StepsPerSecond);
  accelStepper.setAcceleration(StepsPerSecond*2);
  
  return 0;
}

void NexDome::EnableMotor()
{
  //Computer.print("Enable at ");
  //Computer.println(CurrentPosition());
  digitalWrite(EN,LOW);
  delay(100);
  Active=true;  
}

void NexDome::DisableMotor()
{
  digitalWrite(EN,HIGH);
  Active=false;
}

bool NexDome::Run()
{
  bool r;
  long int pos;
  r=accelStepper.run();
  if(!r) {
    //  This move has finished
    DisableMotor();
    //  Motor is stopped, so now is the time to handle wrap arounds
    pos=CurrentPosition();
    if(pos==LastHomeCount) isAtHome=true;
    if(pos < 0) {
      while(pos < 0) pos=pos+StepsPerDomeTurn;
      accelStepper.setCurrentPosition(pos);
    }
    if(pos >= StepsPerDomeTurn) {
      while(pos >= StepsPerDomeTurn) pos=pos-StepsPerDomeTurn;
      accelStepper.setCurrentPosition(pos);
    }
  }
  return r;
}

long int NexDome::CurrentPosition()
{
  return accelStepper.currentPosition();
}

//  for the stop function, we want to abandon a target
//  and just start the deceleration, but we want it to stop
//  on an even step boundary, so we do some math hoops to
//  ensure our stop point is on a whole step, not a microstep
void NexDome::Stop()
{
  long int current;
  long int nowstep;
  int r;

  FindingHome=false;
  Calibrating=false;
  if(!Active) return;   //  we are already stopped

  //  round to even steps
  current=accelStepper.currentPosition();
  r=current%STEP_TYPE;
  current=current-r;
  accelStepper.moveTo(current);

  return;
}

//  this is a raw move to a stepper count
void NexDome::MoveTo(unsigned long int p)
{
  if(!Active) EnableMotor();
  accelStepper.moveTo(p);
  isAtHome=false;
  TargetSteps=p;
  return;
}

long int NexDome::AzimuthToTicks(int az)
{
  long int ticks;
  int r;
  ticks=(float)StepsPerDomeTurn/(float)360*az;
  //  now round to whole steps, dropping microsteps
  r=ticks%STEP_TYPE;
  ticks=ticks-r;
  return ticks;
}

float NexDome::GetHeading()
{
  /* floating point math for get heading  
   *  prefer floats for accurately placing
   *  the power pickup to charge the shutter
   *  but it introduces significant processing
   *  overhead
   */
  
  float Heading;
  //Computer.print("Current ");
  //Computer.println(CurrentPosition());
  Heading=(float)CurrentPosition()/(float)StepsPerDomeTurn*360;
  //  in case we need to do another step
  //  do a run sequence now
  if(Active) Run();
  while(Heading < 0) Heading+=360;
  while(Heading >= 360) Heading-=360;
  if(Active) Run();
  return Heading;
  

/*
  long int h;

  //Computer.println(StepsPerDomeTurn);
  h=CurrentPosition();
  //Computer.println(h);
  h=h*360+StepsPerDomeTurn/2;
  //  fix for rounding issues
  //h=h+StepsPerDomeTurn/2;
  h=h/StepsPerDomeTurn;
  //Computer.println(h);
  while(h < 0) h+=360;
  while(h >= 360) h-=360;
  return (int) h;
*/

}

int NexDome::SetHeading(float h)
{
  float target;
  //long int TargetSteps;
  float current;
  int r;
  float delta;
  float newdelta;
  bool GoingBackwards;
  long int StepsToGo;

  FindingHome=false;
  //SettingHome=false;
  Calibrating=false;
  current=GetHeading();
  delta=h-current;
  if(delta == 0) return 0; //  we are already there
  
  if((delta <= 180)&&(delta >= -180)) {
    target=current+delta;
  }
  if(delta > 180) {
    //  its more than 180 degrees ahead
    newdelta=360-delta;
    target=current-newdelta;
  }
  if(delta < -180) {
    newdelta=360+delta;
    target=current+newdelta;
  }

/*
  Computer.print("heading now ");
  Computer.println(current);
  Computer.print("desired heading ");
  Computer.println(h);
  Computer.print("delta is ");
  Computer.println(delta);
  Computer.print("new target is ");
  Computer.println(target);
*/
 


  /*
  target=h/360.0*StepsPerDomeTurn;
  */
  TargetSteps=target/360*StepsPerDomeTurn;
  //Computer.println(CurrentPosition());
  //Computer.println(TargetSteps);

  
  //  now round to even steps, not microsteps
  r=TargetSteps%STEP_TYPE;
  TargetSteps=TargetSteps-r;
  
  MoveTo(TargetSteps);
  
}

int NexDome::Sync(float val)
{
  long int newaz;

  newaz=AzimuthToTicks(val);
  //Computer.print("Setting new tick count ");
  //Computer.print(newaz);
  accelStepper.setCurrentPosition(newaz);
  //Computer.print("sync with ");
  //Computer.println(val);
}


int NexDome::AtHome()
{
  int r;
  
  LastHomeCount=CurrentPosition();
  r=LastHomeCount%STEP_TYPE;
  LastHomeCount=LastHomeCount-r;
  //Computer.print("Home sensor detected at ");
  //Computer.println(LastHomeCount);
  if(FindingHome) {
    if(Calibrating) {
      //Computer.print("Full turn count ");
      //Computer.println(LastHomeCount);
      StepsPerDomeTurn=LastHomeCount;
      SaveConfig(); //  store this new value into the eeprom
      Calibrating=false;
    }
    FindingHome=false;
    //accelStepper.setCurrentPosition(0);
    MoveTo(LastHomeCount);
    //SettingHome=true;
  }
  return 0;
}

int NexDome::FindHome()
{
  FindingHome=true;
}

int NexDome::Calibrate()
{
  accelStepper.setCurrentPosition(0);
  Calibrating=true;
  FindingHome=true;
}

void HomeInterrupt()
{
  //  lets debounce this just a little
  int a,b;
  a=digitalRead(HOME);
  b=digitalRead(HOME);
  if(a==b) {
    //Computer.print("Home Sensor");
    //Computer.println(a);
    HomeSensor=true;
  }
}

//  We have defined the class, now lets create one
NexDome Dome;


void setup() {
  float MotorTurnsPerDomeTurn;
  float StepsPerGearTurn;
  long int StepsPerDomeTurn;
  float StepsPerSecond;
  long int StepsPerDegree;

  // We need the internal pullups on our input pins
  pinMode(HOME,INPUT_PULLUP);
  //  We dont need this on rotation controller
  //  but we use the same boards for testing so
  //  set the mode in case there is a switch connected
  //  because we are using a shutter board to test
  //  rotation code or vice versa
  pinMode(OTHER_IRQ,INPUT_PULLUP);
  pinMode(BUTTON_EAST,INPUT_PULLUP);
  pinMode(BUTTON_WEST,INPUT_PULLUP);

  pinMode(VPIN,INPUT);

  pinMode(STP, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(EN, OUTPUT);

  //  lets set up an interrupt from our home sensor
  attachInterrupt(digitalPinToInterrupt(HOME),HomeInterrupt,FALLING);
  
  Computer.begin(9600);
  Wireless.begin(9600);

  //  hack for development, we want it to wait now for the serial port to be connected
  //  so we see the output in the monitor during inti
  //while(!Serial) {
  //}
  Computer.println("Starting NexDome"); 

  MotorTurnsPerDomeTurn = (float)DOME_TEETH / (float) GEAR_TEETH;
  StepsPerGearTurn=200.0*(float)REDUCTION_GEAR*(float)STEP_TYPE;
  StepsPerDomeTurn=MotorTurnsPerDomeTurn*StepsPerGearTurn;
  StepsPerDegree=StepsPerDomeTurn/360;
  
  Computer.print(StepsPerDomeTurn);
  Computer.println(" steps per dome turn");
  Computer.print(StepsPerDegree);
  Computer.println(" steps per degree");
  
  Dome.EnableMotor();
  Dome.DisableMotor();
  Dome.SetStepsPerDomeTurn(StepsPerDomeTurn);

  Dome.ReadConfig();

  ShutterQueryTime=10000;
  ConfigureWireless();

}


//  We dont need to check the buttons
//  on every loop, it adds a lot of overhead
//  routing thru digitalRead functions
//  We only need to do it 10 times a second max
unsigned long int LastButton=0;
int CheckButtons()
{
  unsigned long int now;

  //  time now
  now=millis();
  if(now < LastButton) {
    //  the millisecond timer has rolled over
    LastButton=now;
  }
  if(now-LastButton < 100) {
    //  return if it's less than our time between checks
    return -1;
  }
  //Serial.println(now-LastButton);
  LastButton=now;
 
  int bw,be;
  int buttonstate;
  //Serial.println(accelStepper.currentPosition());
  //  First we check buttons for any button state
  bw=digitalRead(BUTTON_WEST);
  be=digitalRead(BUTTON_EAST);
  buttonstate=bw+(be<<1);
  buttonstate=buttonstate ^ 0x03;
  //Serial.println(buttonstate);
  return buttonstate;
  
}

//  define a buffer and pointers for our serial data state machine
// this definition conflicts with arduino headers in later versions
// we can just run with what they have defined for now
//#define SERIAL_BUFFER_SIZE 20
char SerialBuffer[SERIAL_BUFFER_SIZE];
int SerialPointer=0;
bool SerialTarget=false;

//  Now define a buffer and pointers for our serial link over wireless
//  for talking to the shutter
char WirelessBuffer[SERIAL_BUFFER_SIZE];
int WirelessPointer=0;

//  and some variables used to time out the state machines

unsigned long int LastShutterResponse=0;
unsigned long int LastShutterKeepAlive=0;
bool ShutterAlive=false;
bool DoingWirelessConfig=false;
int WirelessConfigState=0;
bool FoundXBee=false;

/*
 * When processing time sensative serial commands
 * we use sprintf and write to avoid Serial library overhead
 * which causes the stepper to stutter during command
 * exchanges if using simple print and println functions
 * 
 * On arduino, sprintf doesn't support floating point
 * so for float returns we format with dtostrf 
 * 
 * We process only a subset of commands when the dome is
 * in motion, because some of the shutter commands can tak
 * a few seconds to complete, and we dont want to make our
 * state machines so complex they dont fit in arduino memory
 * 
 * The simple solution, only process dome status commands while
 * in motion, and process the rest when dome is not running
 * 
 */
void ProcessSerialCommand()
{
  
  char buf[20];
  
  /* Abort current motion */
  if(SerialBuffer[0]=='a') {
    SerialTarget=false;
    Dome.Stop();
    //  abort any shutter movement too
    Wireless.println("a");
    Computer.println("A");
  }
  /*  Raw position count */
  if(SerialBuffer[0]=='p') {
    sprintf(buf,"P %ld\n",Dome.CurrentPosition());
    Computer.write(buf,strlen(buf));
    SerialPointer=0;
    return;
  }
  /* is dome in motion */
  if(SerialBuffer[0]=='m') {
    int state=0;
    //Computer.write("M ");
    //  if dome is in motion, figure out what kind of motion
    if(Dome.Active) {
      Dome.Run();
      //  default is just a normal move
      state=1;
      if(Dome.FindingHome) state=2;
      if(Dome.Calibrating) state=3;
    }
    sprintf(buf,"M %d\n",state);
    Computer.write(buf,strlen(buf));
    //Computer.write("\n");
    //Computer.println(state);
    SerialPointer=0;
    return;

  }
  /* query current heading */
  if(SerialBuffer[0]=='q') {
    //Computer.print("H ");
    //Computer.println(Dome.GetHeading());
    dtostrf(Dome.GetHeading(),2,1,buf);
    if(Dome.Active) Dome.Run();
    //sprintf(buf,"H %s\n",b);
    Computer.write("Q ",2);
    Computer.write(buf,strlen(buf));
    Computer.write("\n",1);
    //Computer.print("Q ");
    //Computer.println(Dome.GetHeading());
    SerialPointer=0;
    return;
  }

  
  /*
   * These commands should not be processed when the dome is in motion
   * 
   */

   /* query shutter state */
   if(SerialBuffer[0]=='u') {
    if(Dome.Active) Dome.Run();
    Computer.print("U ");
    Computer.println(ShutterState);
   }
   /*  try a reset kludge  */
   if(SerialBuffer[0]=='r') {
    if(SerialBuffer[1]=='s') {
      void(* resetFunc)(void)=0;
      resetFunc();
    }
   }
   /*  get shutter position  */
   if(SerialBuffer[0]=='b') {
    if(Dome.Active) Dome.Run();
    Computer.print("B ");
    Computer.println(ShutterPosition);
   }
   /*  Open Shutter  */
   if(SerialBuffer[0]=='d') {
    Computer.println("D");
    //  tell the shutter to open
    Wireless.println("o");
    //  now query status
    //delay(300);
    //Wireless.println("s");
      ShutterQueryTime=1000;
   }
   /*  Close Shutter  */
   if(SerialBuffer[0]=='e') {
    Computer.println("D");
    Wireless.println("c");
    //delay(300);
    //Wireless.println("s");
      ShutterQueryTime=1000;
   }
    /*  Close Shutter  */
   if(SerialBuffer[0]=='f') {
    float tt;
    
    Computer.println("F");
    tt=atof(&SerialBuffer[1]);
    Wireless.print("f ");
    Wireless.println(tt);
    //delay(300);
    //Wireless.println("s");
      ShutterQueryTime=1000;
   }
 
  /* a pass thru command to the shutter */
  if(SerialBuffer[0]=='x') {
    //  dont pass thru an empty command
    if(SerialBuffer[1] != 0) {
      if(ShutterAlive) {
        Computer.println(&SerialBuffer[1]);
        Wireless.println(&SerialBuffer[1]);
      } else {
        Computer.println("E");
      }
      
    }
  }
  /* sync on this heading */
  if(SerialBuffer[0]=='s') {
    float h;
    h=atof(&SerialBuffer[1]);
    if((h >=0)&&(h<360)) {
       Computer.print("S ");
       Computer.println(h);
       Dome.Sync(h);
    } else {
      Computer.println("E ");
    }
  }
  /* current status of steps per turn */
  if(SerialBuffer[0]=='t') {
    Computer.print("T ");
    Computer.println(Dome.StepsPerDomeTurn);
    //Computer.println(" Steps per Dome turn");
    //Computer.println(Dome.HomeAzimuth);
    //Computer.println(" Home Azimuth");
    
  }
  /* query for home azimuth */
  if(SerialBuffer[0]=='i') {
    Computer.print("I ");
    Computer.println(Dome.HomeAzimuth);
    //Computer.println(" Steps per Dome turn");
    //Computer.println(Dome.HomeAzimuth);
    //Computer.println(" Home Azimuth");
    
  }
  /* Set the home azimuth */
  if(SerialBuffer[0]=='j') {
    float newaz;
    newaz=atof(&SerialBuffer[1]);
    if((newaz >=0)&&(newaz < 360)) Dome.HomeAzimuth=newaz;
    //  Respond same as a query for home azimuth
    Computer.print("I ");
    Computer.println(Dome.HomeAzimuth);
    Dome.SaveConfig();
    //Computer.println(" Steps per Dome turn");
    //Computer.println(Dome.HomeAzimuth);
    //Computer.println(" Home Azimuth");
    
  }
  /* query for park azimuth */
  if(SerialBuffer[0]=='n') {
    Computer.print("N ");
    Computer.println(Dome.ParkAzimuth);
    //Computer.println(" Steps per Dome turn");
    //Computer.println(Dome.HomeAzimuth);
    //Computer.println(" Home Azimuth");
    
  }
  /* Set the park azimuth */
  if(SerialBuffer[0]=='l') {
    float newaz;
    newaz=atof(&SerialBuffer[1]);
    if((newaz >=0)&&(newaz < 360)) Dome.ParkAzimuth=newaz;
    //  Respond same as a query for park
    Computer.print("N ");
    Computer.println(Dome.ParkAzimuth);
    Dome.SaveConfig();
    //Computer.println(" Steps per Dome turn");
    //Computer.println(Dome.HomeAzimuth);
    //Computer.println(" Home Azimuth");
    
  }
  
/*  goto based on a heading */
  if(SerialBuffer[0]=='g') {
    float target;
    target=atof(&SerialBuffer[1]);
    if((target >=0.0)&&(target <= 360.0)) {
      SerialTarget=true;
      Dome.SetHeading(target);
      Computer.println("G");
    } else {
      Computer.println("E");
    }
  }
  /* find the home position */
  if(SerialBuffer[0]=='h') {
    Dome.FindHome();
    SerialTarget=true;
    Computer.println("H");
  }
  /* are we at the home position */
  if(SerialBuffer[0]=='z') {
    Computer.print("Z ");
    if(Dome.isAtHome) Computer.println("1");
    else Computer.println("0");
  }
  /* are we at the home position */
  if(SerialBuffer[0]=='k') {
    Computer.print("K ");
    Computer.print(BatteryVolts);
    Computer.print(" ");
    Computer.println(ShutterBatteryVolts);
  }
  /* calibrate the number of steps in a full turn */
  if(SerialBuffer[0]=='c') {
    //  calibrate the dome
    //  In order to calibrate, we need to be at the home position
    if(Dome.isAtHome) {
      Computer.println("C");
      SerialTarget=true;
      Dome.Calibrate();
      
    } else {
      Computer.println("E");
    }
  }
  /*  restart xbee wireless */
  if(SerialBuffer[0]=='w') {
    Computer.println("W");
    ConfigureWireless();
  }
  
  /* get firmware version */
  if(SerialBuffer[0]=='v') {
    Computer.print("VNexDome V ");
    Computer.print(VERSION_MAJOR);
    Computer.print(".");
    Computer.println(VERSION_MINOR);
  }

  SerialPointer=0;
  return;
}

void IncomingSerialChar(char a)
{

  if((a=='\n')||(a=='\r')) {
    return ProcessSerialCommand();
  }
  
  SerialBuffer[SerialPointer]=a;
  SerialPointer++;
  if(SerialPointer==SERIAL_BUFFER_SIZE) {
    //  we are going to overflow the buffer
    //  erase it and start over
    memset(SerialBuffer,0,SERIAL_BUFFER_SIZE);
    SerialPointer=0;
  } else {
    SerialBuffer[SerialPointer]=0;
  }
  return;
}

/*
 * Stuff we need for handling communication with the shutter
 * 
 */
 

void ConfigureWireless()
{
  //  do nothing for now
  Computer.println("Sending + to xbee");
  DoingWirelessConfig=true;
  WirelessConfigState=0;
  memset(WirelessBuffer,0,SERIAL_BUFFER_SIZE);
  delay(1100);
  WirelessPointer=0;
  Wireless.print("+++");
}


void ProcessShutterData()
{

  if(WirelessBuffer[0]=='O') {
    if(WirelessBuffer[1]=='K') {
      //  The xbee unit is ready for configuration
      if(DoingWirelessConfig) {
        switch(WirelessConfigState) {
          case 0:
            Wireless.println("ATID5555");
            Computer.println("Setting wireless id");
          
            break;
          default:
            Computer.println("Wireless config finished");
            DoingWirelessConfig=false;
            FoundXBee=true;
            break;
        }
        WirelessConfigState++;
      }
      Computer.println("Clear wireless buffer");
      memset(WirelessBuffer,0,SERIAL_BUFFER_SIZE);
      WirelessPointer=0;
      return;
    }
  }
  // update our timer for the keep alive routines
  if(!ShutterAlive) {
    if(WirelessBuffer[0]=='S') {
      //Computer.println(WirelessBuffer);
      //Computer.println("Shutter woke up");
      ShutterAlive=true;
    }
  }
  if(ShutterAlive) {
    if(WirelessBuffer[0]=='S') {
      long int newShutterState;
      LastShutterResponse=millis();
      //  the first character in the response is the shutter state
      ShutterQueryTime=10000;
      switch(WirelessBuffer[1]) {
        case 'C':
          newShutterState=SHUTTER_STATE_CLOSED;
          break;
        case 'O':
          newShutterState=SHUTTER_STATE_OPEN;
          break;
        case 'P':
          newShutterState=SHUTTER_STATE_OPENING;
          ShutterQueryTime=1000;
          break;
        case 'D':
          newShutterState=SHUTTER_STATE_CLOSING;
          ShutterQueryTime=1000;
          break;
        default:
          newShutterState=SHUTTER_STATE_UNKNOWN;
          break;
        //default:
        //  ShutterState=SHUTTER_STATE_NOT_CONNECTED;
        //  break;
      }
      if(newShutterState != ShutterState) {
        ShutterState=newShutterState;
        //Computer.print("Shutter State ");
        //Computer.println(ShutterState);
      }
      //  shutter has responded with a status
      //  lets ask it for a position
      Wireless.println("p");
    }
    if(WirelessBuffer[0]=='P') {
      ShutterPosition=atof(&WirelessBuffer[1]);
      //Computer.println("Shutter Position ");
      //Computer.println(q);
      //  We got a position
      //  lets ask for battery voltage
      Wireless.println("b");
    }
     if(WirelessBuffer[0]=='B') {
      ShutterBatteryVolts=atoi(&WirelessBuffer[1]);
     }
   
  }

  // clear the buffer now that it's processed
  memset(WirelessBuffer,0,SERIAL_BUFFER_SIZE);
  WirelessPointer=0;
  
  return;
  
}

void IncomingWirelessChar(char a)
{
  if((a=='\n')||(a=='\r')) {
    return ProcessShutterData();
  }
  //Computer.println((int)a);
  WirelessBuffer[WirelessPointer]=a;
  WirelessPointer++;
  if(WirelessPointer==SERIAL_BUFFER_SIZE) {
    memset(WirelessBuffer,0,SERIAL_BUFFER_SIZE);
    WirelessPointer=0;
  } else {
    WirelessBuffer[WirelessPointer]=0;
  }
  if(a < 0) {
    Computer.println("Clearing garbage");
    WirelessPointer=0;
    WirelessBuffer[0]=0;
  }
  //Computer.print(a);
  return;
}

unsigned long int LastBatteryCheck=0;

int CheckBattery()
{
  int volts;

  volts=analogRead(VPIN);
  //Computer.println(volts);
  volts=volts/2;
  volts=volts*3;
  //v=(float)volts/(float)100;
  BatteryVolts=volts;
  //Computer.println(volts);  
}

void loop() {
  int buttonstate;

  if(HomeSensor) {
    HomeSensor=false;
    Dome.AtHome();
  }
  if(Dome.FindingHome) {
    Dome.MoveTo(Dome.CurrentPosition()+5000);    
  }
  //  Now things are all initialized, we just get down to the nitty gritty of running a dome
  //  lets check and see if any buttons are pushed
  buttonstate=CheckButtons();
  if(buttonstate != -1) {
    //  if we have had motion commanded over the serial port
    //  abort it and use the on board buttons as definitive
    if(buttonstate != 0) SerialTarget=false;
    switch(buttonstate) {
      case 1:
        Dome.MoveTo(Dome.CurrentPosition()+5000);
        break;
      case 2:
        Dome.MoveTo(Dome.CurrentPosition()-5000);
        break;
      default:
        //  Dont stop the dome if we have motion driven by incoming
        //  serial data stream
        if(!SerialTarget) Dome.Stop();
        break;
    }
  }

  //  is there any data from the host computer to process
  if(Computer.available()) {
    IncomingSerialChar(Computer.read());
  }

  //  now see if there is any data from the wireless to process
  if(Wireless.available()) {
    char a;
    a=Wireless.read();
    IncomingWirelessChar(a);
  }

  //CheckBattery();
  
  if(Dome.Active)
  {
    Dome.Run();
  } else {
    //  When the dome is not moving
    //  we need to do a keep alive with the shutter
    //  And confirm it's still alive and running
    //  And we should check on battery voltage once in a while too
    unsigned long int now;

    //CheckBattery();
    //  time now
    now=millis();


    
    if(now < LastShutterKeepAlive) {
    //  the millisecond timer has rolled over
      LastShutterKeepAlive=now;
    }
    if((now - LastShutterKeepAlive) > ShutterQueryTime) {
      CheckBattery();
      if(!FoundXBee) {
        ConfigureWireless();
      } else {
        //  send shutter a status query
        //Computer.println("Sending shutter status request");
        Wireless.println("s");
        //  and get the battery voltage from the shutter
        //Wireless.println("b");
      }
      //Wireless.println(AliveCount++);
      LastShutterKeepAlive=now;
    }

    if(ShutterAlive) {
      //  Now lets see if it has timed out on responses, ie, gone away for some reason
      if(now < LastShutterResponse) LastShutterResponse=now;    //  deal with rollovers of unsigned
      if((now - LastShutterResponse) > 60000) {
        //  we have been 60 seconds without a response of any type
        //  we have to make sure this timing does not conflict with a keep-alive
        if(now-LastShutterKeepAlive < 5000) {
          //Computer.println("Waiting on wireless reset");
        } else {
          ShutterAlive=false;
          ShutterState=SHUTTER_STATE_NOT_CONNECTED;
          Computer.println("shutter asleep");
          ConfigureWireless();
          LastShutterResponse=millis();
        }
      
      }
    }
  }
}
