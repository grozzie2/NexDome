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
 *   This file conatins:-
 *   Firmware for the aruduino in the NexDome shutter controller
 *   Hardware is the leonardo clone with an xbee series 1 module
 *   and a TB6600 based stepper motor driver
 */
 
//  Libraries we need to include
#include <EEPROM.h>
#include <AccelStepper.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 10


/*
 * Allow for bench testing without magnetic sensors
 * giving absolute position for open and closed
 * when this is defined, the firmware will assume the
 * limit switches are active at the correct spots, this
 * allows for bench testing drivers without physical hardware
 * set up
 */
#define BENCH_TEST 1

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
#define BUTTON_OPEN 5
#define BUTTON_CLOSE 6

#define CLOSED_SWITCH 2
#define OPEN_SWITCH 3

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

#define REDUCTION_GEAR 15.3
//#define REDUCTION_GEAR 77
#define DOME_TEETH 77
#define GEAR_TEETH 7
#ifdef BENCH_TEST
#define SHUTTER_MOVE_TIME 30
#else
#define SHUTTER_MOVE_TIME 90
#endif
// set this to match the type of steps configured on the
// stepper controller
#define STEP_TYPE 8

//  Set up a default for the communication timeout
//  for 5 minutes, we get 300000 millis
unsigned long int ShutterCommunicationTimeout=300000;
//  This timer is used for the communication loss timer
//  When we reach the timeout threshold with no communications at all
//  then the shutter will be closed
unsigned long int LastCommandTime=0;
//  we want to start up not hibernated
//  and if the state flag says we are
//  Wireless configuration will kick in right away
bool HibernateRadio=false;
bool RadioLongSleep=true;
unsigned long int LastBatteryCheck=0;
unsigned long int HibernateTimeout=60000;
unsigned long int MotorOffTime;
//  Initialize the serial target to true
//  so when we come from power up, our drop dead timers
//  become active, unless somebody pushes a button and puts
//  us in manual control mode
//  We used to have another state variable for shutter safety modes
//  but they all trigger at the same time as this one
//  So this doubles as the safety trigger too
bool SerialTarget=true;

int BatteryVolts=0;
int CutoffVolts=0;
int LowVoltCount=0;

//  The shutter class will use an accel stepper object to run the motor
AccelStepper accelStepper(AccelStepper::DRIVER, STP, DIR);

#define EEPROM_LOCATION 10
#define SIGNATURE 1037
typedef struct ShutterConfiguration {
  int signature;
  unsigned long int StepsToFullOpen;
  unsigned long int HibernateTimeout;
  int CutoffVolts;
} shutter_config;

void ConfigureWireless();

class NexShutter
{
  
  public:
    NexShutter();
    ~NexShutter(){};  //  Empty destructor because it'll never get used

    //  we need a faster deceleration when we
    //  hit limit switches
    long int AccelSpeed;
    long int DecelSpeed;
    bool Active;
    //  direction of current movement
    bool Opening;
    bool Closing;
    //  Flag for current move to continue
    //  until a limit switch is detected, irrelavent
    //  of step counts
    bool OpeningFull;
    bool ClosingFull;
    bool isOpen;
    bool isOpenFull;
    bool isClosed;
    bool HaveDetectedClose;
    
    long int StepsToFullOpen;
    long int CurrentPosition();
    void EnableMotor();
    void DisableMotor();
    void Stop();
    bool Run();
    void MoveTo(long int);
    void SetStepsToFullOpen(long int);

    void OpenShutter();
    void CloseShutter();
    int getShutterState();
    float getShutterPosition();
    bool setShutterPosition(float);
    bool SaveConfig();
    bool ReadConfig();
};

NexShutter::NexShutter()
{
  Active=false;
  Opening=false;
  OpeningFull=false;
  Closing=false;
  ClosingFull=false;
  isOpen=false;
  isOpenFull=false;
  isClosed=false;
  HaveDetectedClose=false;
}

bool NexShutter::SaveConfig()
{
  shutter_config cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.signature=SIGNATURE;
  cfg.StepsToFullOpen=StepsToFullOpen;
  cfg.HibernateTimeout=HibernateTimeout;
  cfg.CutoffVolts=CutoffVolts;
  EEPROM.put(EEPROM_LOCATION,cfg);
  return true;
}

bool NexShutter::ReadConfig()
{
  shutter_config cfg;
  
  memset(&cfg,0,sizeof(cfg));
  EEPROM.get(EEPROM_LOCATION,cfg);
  if(cfg.signature != SIGNATURE) {
    //Computer.println("Invalid EEPROM data");
    return false;
  }
  //Computer.println("Using eeprom data");
  //StepsToFullOpen=cfg.StepsToFullOpen;
  HibernateTimeout=cfg.HibernateTimeout;
  CutoffVolts=cfg.CutoffVolts;
  SetStepsToFullOpen(cfg.StepsToFullOpen);
  //Computer.println(StepsToFullOpen);
  return true;
}

void NexShutter::EnableMotor()
{
  //Computer.print("Enable at ");
  //Computer.println(CurrentPosition());
//#ifdef BIG_EASY
digitalWrite(EN,LOW);
//#else
//digitalWrite(EN,HIGH);
//#endif 
  delay(100);
  Active=true;
  HibernateRadio=false;  
}

void NexShutter::DisableMotor()
{
//#ifdef BIG_EASY
digitalWrite(EN,HIGH);
//#else
//digitalWrite(EN,LOW);
//#endif  

  //digitalWrite(DIR,LOW);
  //digitalWrite(STP,LOW);
  //digitalWrite(DIR,HIGH);
  //digitalWrite(STP,HIGH);


  //Computer.print("Disable at ");
  //Computer.println(CurrentPosition());
  Active=false;
  // start our hibernate timer
  MotorOffTime=millis();
  //HibernateRadio=true;
}

bool NexShutter::Run()
{
  bool r;
  r=accelStepper.run();
  if(!r) {
    //  The move is finished
    long int pos;
    int l;
    
    pos=accelStepper.currentPosition();
    l=pos%STEP_TYPE;
     //  We want it to stop on an even step boundary
     //  and if not, round it off
    if(l==0) {
      //Computer.print("Stopped at ");
      //Computer.println(pos);
      DisableMotor();
      Opening=false;
      //Closing=false;
#ifdef BENCH_TEST
      if(pos <= 0) isClosed=true;
      if(pos >= StepsToFullOpen) {
        isOpenFull=true;
      }
#endif
      if(isClosed) {
        //  We have a sensor that says we are closed
        //Computer.println("At closed sensor");
        accelStepper.setCurrentPosition(0);
        HaveDetectedClose=true;
        Closing=false;
      }
      if(isOpenFull) {
        long int pos;
        //Computer.print("At open sensor ");
        pos=accelStepper.currentPosition();
        //Computer.println(pos);
        if(HaveDetectedClose) {
          //  we have detected a closed position
          //  So this is our step count to full open
          long int delta;
          delta=StepsToFullOpen-pos;
          //Computer.println(delta);
          if(delta < 0) delta=delta*-1;
          //  if our difference is more than
          //  2% of what we have saved 
          if(delta > StepsToFullOpen/50) {
            Computer.println("Calibrating Shutter");
            StepsToFullOpen=pos;
            SaveConfig();
          } else {
            //  dont have to do anything, calibration data is good
            //Computer.println("Calibration ok");
            //Computer.print(StepsToFullOpen);
            //Computer.print(" ");
            //Computer.println(pos);
          }
        } else {
          //  We are full open based on a sensor
          //  
          accelStepper.setCurrentPosition(StepsToFullOpen);
        }
      }
    } else {
      //  this is what we need to do to round the move
      //  to an even step
      pos=pos-l;
      MoveTo(pos);
    }

    //Computer.print("Stopped at ");
    //Computer.println(pos);
    //if(pos==0) isClosed=true;
    //else isOpen=true;
  } else {
    //  if we are processing a close command, we need to keep going until
    //  we hit the sensor, but dont keep doing this if we have hit the sensor
    if((ClosingFull)&&(!isClosed)) {
      long int pos;
      pos=accelStepper.currentPosition();
      MoveTo(pos - 5000);
    }
    //  if we are processing a command to fully open, we need to keep going
    //  until we hit the open sensor, but dont do this if the sensor has been hit
    if((OpeningFull)&&(!isOpenFull)) {
      long int pos;
      pos=accelStepper.currentPosition();
      MoveTo(pos + 5000);
    }
#ifdef BENCH_TEST
    long int pos;
     pos=accelStepper.currentPosition();
  
    if((pos > StepsToFullOpen)&&(!Closing)) {
      isOpen=true;
      Stop();
      //Computer.println("hit open");
    }
    if((pos <= 0)&&(!Opening)) {
      //Computer.println("hit closed");
      isClosed=true;
      Stop();
    }
#endif
  }
  return r;
}

long int NexShutter::CurrentPosition()
{
  return accelStepper.currentPosition();
}

//  for the stop function, we want to abandon a target
//  and just start the deceleration, but we want it to stop
//  on an even step boundary, so we do some math hoops to
//  ensure our stop point is on a whole step, not a microstep
void NexShutter::Stop()
{
  long int current;
  int r;


  ClosingFull=false;
  OpeningFull=false;
  
  if(!Active) return;
  
  //  round to even steps
  current=accelStepper.currentPosition();
  r=current%STEP_TYPE;
  current=current-r;
  accelStepper.moveTo(current);

  //accelStepper.stop();

  //Computer.println(current);

  return;
  
}

void NexShutter::SetStepsToFullOpen(long int s)
{
  long int StepsPerSecond;
  
  StepsToFullOpen=s;
  StepsPerSecond=StepsToFullOpen/SHUTTER_MOVE_TIME;
  //  if we let the stop rate get to high then accelstepper takes to long
  //  we start missing the xbee timings on the xbee serial port
  if(StepsPerSecond > 2500) StepsPerSecond=2500;
  AccelSpeed=StepsPerSecond*2;
  DecelSpeed=StepsPerSecond*4;
  accelStepper.setMaxSpeed(StepsPerSecond);
  //  we need to stop fairly quickly when a sensor is hit
  accelStepper.setAcceleration(StepsPerSecond);

  //Computer.print(StepsPerSecond);
  //Computer.println(" steps per second"); 
  
  return;
}

void NexShutter::MoveTo(long int t)
{
  long int p;
  //  if motor is not active
  //  enable it
  if(!Active) EnableMotor();

  //  Set the acceleration to use for ramping up
  accelStepper.setAcceleration(AccelSpeed);

  //  get our current position
  p=accelStepper.currentPosition();
  //  if we are headed to a bigger number, we are opening
  if(t > p) {
    Opening=true;
    isOpen=true;
    Closing=false;
#ifdef BENCH_TEST
    isClosed=false;
#endif
  } else {
    Closing=true;
    Opening=false;
#ifdef BENCH_TEST
    isOpenFull=false;
#endif
  }
  isClosed=false;
  accelStepper.moveTo(t);
  
  return;
}

void NexShutter::OpenShutter()
{
  //Computer.println("Enter open shutter");
  Opening=true;
  OpeningFull=true;
  Closing=false;
  //  We want to make sure we are going far enough to reach the
  //  fully open sensor
  MoveTo(StepsToFullOpen);
}

void NexShutter::CloseShutter()
{
  //Computer.println("Enter close shutter");
  Closing=true;
  ClosingFull=true;
  Opening=false;
  //  We want to make sure we are going far enough to reach the closed sensor
  MoveTo(-10000);
}

int NexShutter::getShutterState()
{
  int state;
  state=SHUTTER_STATE_UNKNOWN;

//Computer.print(Active);
//Computer.print(isOpen);
//Computer.println(isClosed);
  
  if(Active) {
    if(Opening) state=SHUTTER_STATE_OPENING;
    if(Closing) state=SHUTTER_STATE_CLOSING;
  } else {
    if(isOpenFull) isOpen=true;
    if(isOpen) state=SHUTTER_STATE_OPEN;
    if(isClosed) state=SHUTTER_STATE_CLOSED;
  }
  return state;
}

float NexShutter::getShutterPosition()
{
  float pos;
  float offset;
  float range;

  offset=StepsToFullOpen;
  //  offset is how much we have to open to have the shutter
  //  up the width of the shutter creating an opening centered at
  //  zero
  offset=offset*0.2;
  range=StepsToFullOpen-offset;
  
  pos=accelStepper.currentPosition();
  pos=pos-offset;

  //Computer.println(pos);
  //Computer.println(offset);
  //Computer.println(range);
  //  now convert to a percentage of the range
  pos=pos/range;
  //  and now make it degrees
  pos=pos*90;
  //if(pos < 0) pos=0;
  return pos;
  
}

/*  this is the reverse math of get position  */

bool NexShutter::setShutterPosition(float target)
{
  float pos;
  float offset;
  float range;
  long int s;
  int r;

  offset=StepsToFullOpen;
  offset=offset=offset*0.2;
  range=StepsToFullOpen-offset;

  pos=target*range/90;
  pos=pos+offset;
  if(pos > StepsToFullOpen) pos=StepsToFullOpen;
  s=pos;

  
  // round to even steps
  r=s%STEP_TYPE;
  s=s-r;
  
  //Computer.print("set position goes to ");
  //Computer.println(s);
  MoveTo(s);
  
}

//  we have defined the shutter
//  now create one
NexShutter Shutter;

/*  this interrupt will get called when the closed limit switch changes state  */
void ClosedInterrupt()
{
  int a;
  a=digitalRead(CLOSED_SWITCH);
  if(a==0) Shutter.isClosed=true;
  else Shutter.isClosed=false;

  if(Shutter.isClosed) {
    Computer.print("Closed irq at ");
    Computer.println(accelStepper.currentPosition());
  }
  //  if we have detected the closed switch
  //  and we are in the process of closing
  //  Stop the movement now, irrelavent of the
  //  Step count
  if(Shutter.isClosed) {
    Shutter.HaveDetectedClose=true;
    if(Shutter.Closing) {
      //  We need to stop quickly, we have hit the switch
      accelStepper.setAcceleration(Shutter.DecelSpeed);
      Shutter.Stop();
    }
  }
  
}
/*  this interrupt will get called when the open limit switch changes state  */
void OpenInterrupt()
{
  int a;
  a=digitalRead(OPEN_SWITCH);
  if(a==0) Shutter.isOpenFull=true;
  else Shutter.isOpenFull=false;

  //  If we have detected the 'wide open' switch, and we are processing an open command
  //  Stop the movement now
  if(Shutter.isOpenFull) {
    Computer.print("Open Irq at ");
    Computer.println(accelStepper.currentPosition());
    if(Shutter.Opening) {
      accelStepper.setAcceleration(Shutter.DecelSpeed);
      Shutter.Stop();
    }
  }
  
}

void setup() {
  // put your setup code here, to run once:
  int a;
  
  // We need the internal pullups on our input pins
  pinMode(OPEN_SWITCH,INPUT_PULLUP);
  pinMode(CLOSED_SWITCH,INPUT_PULLUP);
  pinMode(BUTTON_OPEN,INPUT_PULLUP);
  pinMode(BUTTON_CLOSE,INPUT_PULLUP);

  pinMode(STP, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(EN, OUTPUT);
  

  Computer.begin(9600);
  Wireless.begin(9600);

  delay(200);
  //  now read both limit switches
  ClosedInterrupt();
  OpenInterrupt();

  attachInterrupt(digitalPinToInterrupt(CLOSED_SWITCH),ClosedInterrupt,CHANGE);
  attachInterrupt(digitalPinToInterrupt(OPEN_SWITCH),OpenInterrupt,CHANGE);
  
   //  hack for development, we want it to wait now for the serial port to be connected
  //  so we see the output in the monitor during inti
  //while(!Serial) {
  //}

  //Computer.println("Starting NexShutter"); 

  long int MotorTurnsPerShutterMove;
  long int StepsPerGearTurn;
  long int StepsPerShutterMove;

  MotorTurnsPerShutterMove=(float)DOME_TEETH / (float) GEAR_TEETH;
  StepsPerGearTurn=200.0*(float)REDUCTION_GEAR*STEP_TYPE;
  StepsPerShutterMove=MotorTurnsPerShutterMove*StepsPerGearTurn;

  //Computer.print(StepsPerShutterMove);
  //Computer.println(" Steps per shutter move");


  Shutter.EnableMotor();
  Shutter.DisableMotor();
  Shutter.SetStepsToFullOpen(StepsPerShutterMove);
  //  We have calculated defaults, now see if we have actual calibration info in the eeprom
  //  which will overwrite our defaults if they exist
  Shutter.ReadConfig();

#ifdef BENCH_TEST
  //  lets make the bench test open/close a little faster
  Shutter.SetStepsToFullOpen(60000);
#endif

  //  test if we are at a limit switch during startup
  //  to set our full open/close state flags
  a=digitalRead(OPEN_SWITCH);
  if(a==0) {
    Shutter.isOpenFull=true;
    accelStepper.setCurrentPosition(StepsPerShutterMove);
    //Computer.println("Shutter is open full");
  }
  else Shutter.isOpenFull=false;
  
  a=digitalRead(CLOSED_SWITCH);
  if(a==0) {
    Shutter.isClosed=true;
    Shutter.HaveDetectedClose=true;
    //Computer.println("Shutter is closed");
  }
  else Shutter.isClosed=false;

#ifdef BENCH_TEST
  // for bench test, we assume it starts up closed
  Shutter.isClosed=true;
#endif

   //  prime our last command time so our timeouts
   //  can work
   LastCommandTime=millis();
   LastBatteryCheck=0;
   //  and configure the xbee
   //  if it has not been configured at all
   //  then it will respond right away
   //  if the xbee is in a hibernate because it's already been configured
   //  our code to reconfigure it will happen immediately upon the first incoming
   //  character that wakes it up
   ConfigureWireless();
   CheckBattery();

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
  bw=digitalRead(BUTTON_OPEN);
  be=digitalRead(BUTTON_CLOSE);
  buttonstate=bw+(be<<1);
  buttonstate=buttonstate ^ 0x03;

 
  return buttonstate;
  
}


//  define a buffer and pointers for our serial data state machine
// this definition conflicts with arduino headers in later versions
// we can just run with what they have defined for now
//#define SERIAL_BUFFER_SIZE 20
char SerialBuffer[SERIAL_BUFFER_SIZE];
int SerialPointer=0;

//  Now define a buffer and pointers for our serial link over wireless
//  for talking to the rotator
char WirelessBuffer[SERIAL_BUFFER_SIZE];
int WirelessPointer=0;
bool DoingWirelessConfig=false;
int WirelessConfigState=0;
bool MasterAlive=false;
bool FoundXbee=false;


/*
 *   We want to handle commands from both the wireless and the serial port
 *   with the exception of xbee setup, they are handled the same
 *   and we want to respond on the port the command came from
 *   so pass a port by reference to this handler
 *   and responses will go out whichever port the command came from
 */
bool ProcessCommandBuffer(char *cbuf, Serial_ *responder)
{
  char buf[20];
//Computer.println(cbuf);

  //  We always need to process an abort
  if(cbuf[0]=='a') {
    //Computer.println("Sending abort response");
    responder->println("A");
    //SerialTarget=false;
    Shutter.Stop();
    return true;
  }
  /* we do want a motion status while moving  */
    if(cbuf[0]=='s') {
      if(Shutter.Active) Shutter.Run();
      int state=SHUTTER_STATE_UNKNOWN;
      //  for now, just send an unknown state response
      //Computer.println("Sending response to wireless");
      responder->write("S",1);
      state=Shutter.getShutterState();
      //Computer.println(state);
      
      switch(state) {
        case SHUTTER_STATE_OPENING:
          responder->write("P",1);
          break;
        case SHUTTER_STATE_CLOSING:
          responder->write("D",1);
          break;
        case SHUTTER_STATE_CLOSED:
          responder->write("C",1);
          break;
        case SHUTTER_STATE_OPEN:
          responder->write("O",1);
          break;
        default:
          responder->write("U",1);
          break;
      }
      //  tell the host if we have the radio in hibernation mode
      if(RadioLongSleep) responder->write("1",1);
      else responder->write("0",1);
      responder->write("\n",1);
      if(Shutter.Active) Shutter.Run();
      return true;
    }

  if(cbuf[0]=='p') {
    if(Shutter.Active) Shutter.Run();
    dtostrf(Shutter.getShutterPosition(),2,1,buf);
    if(Shutter.Active) Shutter.Run();
    responder->write("P ");
    responder->write(buf,strlen(buf));
    responder->write("\n",1);
    return true;
  }
    
  
  if(cbuf[0]=='q') {  
    dtostrf(Shutter.CurrentPosition(),2,1,buf);
    responder->write("Q ",2);
    responder->write(buf,strlen(buf));
    responder->write("\n",1);
    return true;
  }
  //  if we are moving the shutter
  //  dont process anything else, it causes
  //  to many jitters on the stepper
  if(Shutter.Active) return true;

  
  /*  restart xbee wireless */
  if(cbuf[0]=='w') {
    responder->println("W");
    ConfigureWireless();
    return true;
  }
  if(cbuf[0]=='o') {
    responder->println("O");
    SerialTarget=true;
    //ShutterSafety=true;
    Shutter.OpenShutter();
    //delay(100);
    //HibernateRadio=false;
    //ConfigureWireless();
    return true;
  }
  if(cbuf[0]=='c') {
    responder->println("C");
    SerialTarget=true;
    //ShutterSafety=true;
    Shutter.CloseShutter();
    return true;
  }
  if(cbuf[0]=='f') {
    float alt;
    alt=atoi(&cbuf[1]);
    SerialTarget=true;
    //ShutterSafety=true;
    Shutter.setShutterPosition(alt);
    return true;
  }
  if(cbuf[0]=='b') {
    if(cbuf[1]==' ') {
      int newcutoff;
      newcutoff=atoi(&cbuf[1]);
      if(newcutoff != CutoffVolts) {
        CutoffVolts=newcutoff;
        Shutter.SaveConfig();
      }
    }
    responder->print("B ");
    if(Shutter.Active) Shutter.Run();
    responder->print(BatteryVolts);
    responder->print(" ");
    responder->println(CutoffVolts);
    return true;
  }

  //  get shutter hibernate timer
  if(cbuf[0]=='h') {
    //Computer.println(cbuf);
    if(cbuf[1]==' ') {
      unsigned long int newhibernate;
      newhibernate=atol(&cbuf[1]);
      //Computer.println(newhibernate);
      // enforce that our hibernation timer cannot be less than
      //  60 seconds
      if(newhibernate < 60000) newhibernate=60000;
      if(newhibernate !=HibernateTimeout) {
        HibernateTimeout=newhibernate;
        Shutter.SaveConfig();
      }
    }
    responder->print("H ");
    responder->println(HibernateTimeout);
  }
  
  if(cbuf[0]=='g') {
    Computer.print("g ");
    Computer.print(LowVoltCount);

    Computer.println(" ");
  }
  if(cbuf[0]=='x') {
    //  this is a wakeup
    //  so we are more responsive when the config
    //  dialog is on screen
    //  just pretend we just turned off the motor
    //Computer.println("Waking up");
    responder->println("X");
    HibernateRadio=false;
    MotorOffTime=millis();
  }
  /* get firmware version */
  if(cbuf[0]=='v') {
    //Computer.println("Sending version");
    responder->print("VNexShutter ");
    responder->print(VERSION_MAJOR);
    responder->print(".");
    responder->println(VERSION_MINOR);
    return true;
  }
  return false;
}

void ProcessSerialCommand()
{
  
  if(ProcessCommandBuffer(SerialBuffer,&Computer)) {
    // if this was a valid command, reset the timeout
    LastCommandTime=millis();
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


void ConfigureWireless()
{

//  we need to flush any chars in the uart buffers
//  while(Wireless.available()) {
//    char a;
//    a=Wireless.read();
//    //IncomingWirelessChar(a);
//  }
  //Computer.println("Sending + to xbee");
  DoingWirelessConfig=true;
  LastCommandTime=millis(); //  reset the communication timeout
  WirelessConfigState=0;
  memset(WirelessBuffer,0,SERIAL_BUFFER_SIZE);
  WirelessPointer=0;
  //delay(1100);
  FoundXbee=false;
  Wireless.print("+++");
  //  ensure nothing else gets sent to the xbee after this +++
  //for a little more than our guard time
  delay(110);
}

void ProcessWirelessData()
{
  //Computer.print("buffer size");
  //Computer.println(WirelessPointer);
  //Computer.println(WirelessBuffer);
  /* handle xbee setup first */
  if(WirelessBuffer[0]=='O') {
    if(WirelessBuffer[1]=='K') {
      //  The xbee unit is ready for configuration
      FoundXbee=true;
      
      if(DoingWirelessConfig) {
        switch(WirelessConfigState) {
          case 0:
            Wireless.println("ATID5555");
            //Computer.println("Setting wireless id");
            break;
          case 1:
            Wireless.println("ATPL0");
            //Computer.println("Setting wireless power");
            break;
          case 2:
            Wireless.println("ATCE0");
            //Computer.println("Setting Endpoint");
            break;
          case 3:
              Wireless.println("ATSM4");
              //Wireless.println("ATSM0");
              //Computer.println("Setting sleep mode");
            break;
          case 4:
            // Set the sleep period
            // this is the endpoint, it defines how long we sleep
            // when sleep mode is activated
            //  hexadecimal number for time in tens of milliseconds
            if(HibernateRadio) {
              Wireless.println("ATSP5DC");
              Computer.println("Setting long sleep period");
              RadioLongSleep=true;
            } else {
              Wireless.println("ATSP32");
              Computer.println("Setting short sleep period");
              RadioLongSleep=false;
            }
            break;
          case 5:
            //  Sleep wait time defines how long we wait after
            //  Seeing data on serial or rf before going back to
            //  power saving sleep mode
            //  hexadecimal number for time in milliseconds
            Wireless.println("ATST100");
            //Computer.println("Setting sleep wait time");
            break;
          case 6:
            //  Set the guard time to a much lower value
            //  so we can sneak the +++ in between queries from
            //  the master unit, before our sleep timeouts hit
            Wireless.println("ATGT64");
            //Computer.println("Setting guard time");
            break;
          case 7:
            Wireless.println("ATCN");
            //Computer.println("Exit Command Mode");
            break;
          default:
            //Computer.println("Wireless config finished");
            DoingWirelessConfig=false;

            //  we have configured the wireless, so we should probably send the rotator
            //  a status command at this point too
            //  as we may have ate / missed a status query during the wireless
            //  reconfigure process
            //  The easiest way to do this, just set up the buffer as if we had
            //  just recieved a command request, and process it
            WirelessBuffer[0]='s';
            WirelessBuffer[1]=0;
            WirelessPointer=1;
            ProcessCommandBuffer(WirelessBuffer,(Serial_ *)&Wireless);          
            
            break;
        }
        WirelessConfigState++;
      }
      //Computer.println("Clear wireless buffer");
      memset(WirelessBuffer,0,SERIAL_BUFFER_SIZE);
      WirelessPointer=0;
      return;
    }
  }
  if(DoingWirelessConfig) {
    //  if we get to here, the xbee missed our +++, probably due to sleep states
    //  Restart the configuration state machine
    //Computer.println("restart wireless state machine");
    //  wait fo the xbee guard time
    delay(110);
    //  now configure it
    ConfigureWireless();
    //delay(1100);
    return;
  }
  // update our timer for the keep alive routines
  if(!MasterAlive) {
    if(WirelessBuffer[0]=='s') {
      //Computer.println(WirelessBuffer);
      //Computer.println("Dome woke up");
      MasterAlive=true;
    }
  }
  if(MasterAlive) {
    /*  process a command from the master */
    if(ProcessCommandBuffer(WirelessBuffer,(Serial_ *)&Wireless)) {
      //  if this was a valid command
      //  reset our timeout
      LastCommandTime=millis();;
    }
  }
  //if(HibernateRadio != RadioLongSleep) {
  //  Computer.println("Change sleep mode");
  //  ConfigureWireless();
 // }
  

  // clear the buffer now that it's processed
  memset(WirelessBuffer,0,SERIAL_BUFFER_SIZE);
  WirelessPointer=0;
  
  return;
  
}

void IncomingWirelessChar(char a)
{
  //return;
  if((a=='\n')||(a=='\r')) {
    if(WirelessPointer > 0 ) ProcessWirelessData();
    return;
  }
  //Computer.println(a);
  WirelessBuffer[WirelessPointer]=a;
  WirelessPointer++;
  if(WirelessPointer==SERIAL_BUFFER_SIZE) {
    memset(WirelessBuffer,0,SERIAL_BUFFER_SIZE);
    WirelessPointer=0;
  } else {
    WirelessBuffer[WirelessPointer]=0;
  }
  //  just a quick hack until we put error detection on
  //  this link
  if(a < 0) {
    //Computer.println("Clearing garbage");
    WirelessPointer=0;
    WirelessBuffer[0]=0;
  }
  //Computer.print(a);
  return;
}

void CheckBattery()
{
  int volts;
  unsigned long int now;

  now=millis();
  //  deal with rollovers
  if(now < LastBatteryCheck) LastBatteryCheck=now;
  //  if the radio is hibernated, we are not busy, check battery every 15 seconds
  //  but check it every second if we are awake and processing commands
  if(RadioLongSleep) {
    if(((now - LastBatteryCheck) < 15000)&&(BatteryVolts != 0)) return;
  } else {
    if(((now - LastBatteryCheck) < 1000)&&(BatteryVolts != 0)) return;
    
  }
  LastBatteryCheck=now;
  volts=analogRead(VPIN);
  //Computer.print("battery ");
  //Computer.println(volts);
  //  Account for our resistors
  //  to figure out the real voltage on the 12v line
  volts=volts/2;
  volts=volts*3;
  //v=(float)volts/(float)100;
  BatteryVolts=volts;
  //Computer.println(volts);
  if(BatteryVolts < CutoffVolts) {
    LowVoltCount++;
  } else {
    LowVoltCount=0;
  }
  return;
}

void loop() {
  // put your main code here, to run repeatedly:
  int buttonstate;
  unsigned long int now;

  buttonstate=CheckButtons();
  if(buttonstate != -1) {
    //Computer.println(buttonstate);
    //  if we have had motion commanded over the serial port
    //  abort it and use the on board buttons as definitive
    //  and this turns off all the safety close events
    if(buttonstate != 0) {
      SerialTarget=false;
      //ShutterSafety=false;
    }
    switch(buttonstate) {
      case 1:
        //  Dont continue the move if our limit switch is set
        if(!Shutter.isOpenFull) Shutter.MoveTo(Shutter.CurrentPosition()+5000);
        else Shutter.Stop();
        break;
      case 2:
        //  Dont continue the move past the limit switch
        if(!Shutter.isClosed) Shutter.MoveTo(Shutter.CurrentPosition()-5000);
        else Shutter.Stop();
        break;
      default:
        //  Dont stop the dome if we have motion driven by incoming
        //  serial data stream
        if(!SerialTarget) Shutter.Stop();
        break;
    }
  }

  //  is there any data from the host computer to process
  if(Computer.available()) {
    IncomingSerialChar(Computer.read());
  }
  //  now check for data over the xbee link
  if(Wireless.available()) {
    char a;
    a=Wireless.read();
    IncomingWirelessChar(a);
  }
  //  if the shutter is in motion, we have to keep it moving
  if(Shutter.Active) {
    Shutter.Run();
  } else {
    CheckBattery();
  }

/*
  if(!FoundXbee) {
    //  we are out of sync with the xbee
    now=millis();
    //  deal with rollovers of the millis return
    if(now < LastCommandTime) LastCommandTime=now;
    if(now-LastCommandTime > 3000) {
      ConfigureWireless();
    }
  }
*/

  if(!Shutter.Active) {
    //  lets see if it's time to hibernate the radio
    if(!HibernateRadio) {
      now=millis();
      if((now - MotorOffTime) > HibernateTimeout) {
        //Computer.println("Hibernate Timeout");
        HibernateRadio=true;
      }
    }
  }

  if(HibernateRadio != RadioLongSleep ) {
         now=millis();
        //  make sure we are more than guard time, and less than timeout
        //  since the last wireless char was recieved
        if((now -LastCommandTime) > 110) {
          //  the xbee has not had data for the guard time plus a bit
          if(!DoingWirelessConfig) {
            //  The xbee state machine is not in the process of changing
            ConfigureWireless();
          }
        }
      }


  //  if we are being driven remotely, ie not by direct push buttons
  if(SerialTarget) {
    if(LowVoltCount > 10) {
      if(!Shutter.isClosed) {
        if(!Shutter.Closing) {
          if(Shutter.Active) {
            Shutter.Stop();
          } else {
            //Computer.println("Low battery close shutter");
            Shutter.CloseShutter();
          }
        }
      }
    }
      now=millis();
    //  deal with rollovers of the millis return
    if(now < LastCommandTime) LastCommandTime=now;
    if(now-LastCommandTime > ShutterCommunicationTimeout) {
      //  we have been longer than timeout
      if(!Shutter.isClosed) {
        //  Shutter is not closed
        if(!Shutter.Closing) {
          //  and it is not in the process of closing
          //  so we need to close up 
          if(Shutter.Active) {
            //  Shutter is in motion, stop it first
            Shutter.Stop();
          } else {
            //  all conditions met
            //  shutter is not being driven manually
            //  we have reached timeout on communications
            //  and shutter is not in motion
            //  So close it up and protect the gear from weather
            Computer.println("Drop dead timer close shutter");
            //Computer.println(now);
            //Computer.println(LastCommandTime);
            //Computer.println(ShutterCommunicationTimeout);
            Shutter.CloseShutter();
          }
        }
      }
    }
  }
}
