/*  
 *   Firmware for the aruduino in the NexDome shutter controller
 *   Hardware is the leonardo clone with an xbee series 1 module
 */
 
//  Libraries we need to include
#include <AccelStepper.h>
#ifdef ARDUINO_AVR_LEONARDO
#else
// if running on an arduino with only one serial port
//  we need software serial for the xbee link
#include <SoftwareSerial.h>
#endif

#define VERSION_MAJOR 0
#define VERSION_MINOR 10


/*
 * Allow for bench testing without magnetic sensors
 * giving absolute position for open and closed
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


//  Serial1 is not available on the uno
//#ifndef ARDUINO_AVR_LEONARDO
//  SoftwareSerial Serial1(SERIAL_RX,SERIAL_TX); // RX, TX
//#endif

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
#define DOME_TEETH 77
#define GEAR_TEETH 7
#define SHUTTER_MOVE_TIME 45
// set this to match the type of steps configured on the
// stepper controller
#define STEP_TYPE 4

//  Set up a default for the communication timeout
//  for 5 minutes, we get 300000 millis
unsigned long int ShutterCommunicationTimeout=300000;

int BatteryVolts=0;

//  The shutter class will use an accel stepper object to run the motor
AccelStepper accelStepper(AccelStepper::DRIVER, STP, DIR);

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
    bool Opening;
    bool Closing;
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
};

NexShutter::NexShutter()
{
  Active=false;
  Opening=false;
  Closing=false;
  isOpen=false;
  isOpenFull=false;
  isClosed=false;
  HaveDetectedClose=false;
}

void NexShutter::EnableMotor()
{
  //Computer.print("Enable at ");
  //Computer.println(CurrentPosition());
#ifdef BIG_EASY
digitalWrite(EN,LOW);
#else
digitalWrite(EN,HIGH);
#endif  
  delay(100);
  Active=true;  
}

void NexShutter::DisableMotor()
{
#ifdef BIG_EASY
digitalWrite(EN,HIGH);
#else
digitalWrite(EN,LOW);
#endif  
  //Computer.print("Disable at ");
  //Computer.println(CurrentPosition());
  Active=false;
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
      Computer.print("Stopped at ");
      Computer.println(pos);
      DisableMotor();
      Opening=false;
      //Closing=false;
#ifdef BENCH_TEST
      if(pos == 0) isClosed=true;
      if(pos == StepsToFullOpen) isOpenFull=true;
#endif
      if(isClosed) {
        //  We have a sensor that says we are closed
        Computer.println("At closed sensor");
        accelStepper.setCurrentPosition(0);
        HaveDetectedClose=true;
        Closing=false;
      }
      if(isOpenFull) {
        long int pos;
        Computer.print("At open sensor ");
        pos=accelStepper.currentPosition();
        Computer.println(pos);
        if(HaveDetectedClose) {
          Computer.println("Calibrating Shutter");
          StepsToFullOpen=pos;
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
  AccelSpeed=StepsPerSecond*2;
  DecelSpeed=StepsPerSecond*4;
  accelStepper.setMaxSpeed(StepsPerSecond);
  accelStepper.setAcceleration(StepsPerSecond * 2);

  Computer.print(StepsPerSecond);
  Computer.println(" steps per second"); 
  
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
  accelStepper.moveTo(t);
  
  return;
}

void NexShutter::OpenShutter()
{
  Opening=true;
  Closing=false;
  //  We want to make sure we are going far enough to reach the
  //  fully open sensor
  MoveTo(StepsToFullOpen);
}

void NexShutter::CloseShutter()
{
  Closing=true;
  Opening=false;
  //  We want to make sure we are going far enough to reach the closed sensor
  MoveTo(0);
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
  
  Computer.print("set position goes to ");
  Computer.println(s);
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

//  Hack to make leonardo wait for serial port connection
//  while(!Serial) {  
//  }
  //  give pins a moment to settle
  delay(200);
  //  now read both limit switches
  ClosedInterrupt();
  OpenInterrupt();

  attachInterrupt(digitalPinToInterrupt(CLOSED_SWITCH),ClosedInterrupt,CHANGE);
  attachInterrupt(digitalPinToInterrupt(OPEN_SWITCH),OpenInterrupt,CHANGE);
  

  
  Computer.println("Starting NexShutter"); 

  long int MotorTurnsPerShutterMove;
  long int StepsPerGearTurn;
  long int StepsPerShutterMove;

  MotorTurnsPerShutterMove=(float)DOME_TEETH / (float) GEAR_TEETH;
  StepsPerGearTurn=200.0*(float)REDUCTION_GEAR*STEP_TYPE;
  StepsPerShutterMove=MotorTurnsPerShutterMove*StepsPerGearTurn;

  Computer.print(StepsPerShutterMove);
  Computer.println(" Steps per shutter move");

  Shutter.EnableMotor();
  Shutter.DisableMotor();
  Shutter.SetStepsToFullOpen(StepsPerShutterMove);
 
  ConfigureWireless();

#ifdef BENCH_TEST
  Shutter.isClosed=true;
#endif
 

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
#define SERIAL_BUFFER_SIZE 20
char SerialBuffer[SERIAL_BUFFER_SIZE];
int SerialPointer=0;
//  Initialize the serial target to true
//  so when we come from power up, our drop dead timers
//  become active, unless somebody pushes a button and puts
//  us in manual control mode
bool SerialTarget=true;

//  Now define a buffer and pointers for our serial link over wireless
//  for talking to the rotator
char WirelessBuffer[SERIAL_BUFFER_SIZE];
int WirelessPointer=0;
bool DoingWirelessConfig=false;
int WirelessConfigState=0;
bool MasterAlive=false;
bool FoundXbee=false;

//  This timer is used for the communication loss timer
//  When we reach the timeout threshold with no communications at all
//  then the shutter will be closed
unsigned long int LastCommandTime=0;

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

  //  We always need to process an abort
  if(cbuf[0]=='a') {
    Computer.println("Sending abort response");
    responder->println("A");
    //SerialTarget=false;
    Shutter.Stop();
    return true;
  }
  /* we do want a motion status while moving  */
    if(cbuf[0]=='s') {
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
      responder->write("\n",1);
      if(Shutter.Active) Shutter.Run();
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
    Shutter.OpenShutter();
    return true;
  }
  if(cbuf[0]=='c') {
    responder->println("C");
    SerialTarget=true;
    Shutter.CloseShutter();
    return true;
  }
  if(cbuf[0]=='q') {
    dtostrf(Shutter.CurrentPosition(),2,1,buf);
    responder->write("Q ",2);
    responder->write(buf,strlen(buf));
    responder->write("\n",1);
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
  if(cbuf[0]=='f') {
    float alt;
    alt=atoi(&cbuf[1]);
    Shutter.setShutterPosition(alt);
    return true;
  }
  if(cbuf[0]=='b') {
    responder->print("B ");
    if(Shutter.Active) Shutter.Run();
    responder->println(BatteryVolts);
    return true;
  }

  /* get firmware version */
  if(cbuf[0]=='v') {
    responder->print("NexShutter V ");
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
  //  do nothing for now
  Computer.println("Sending + to xbee");
  DoingWirelessConfig=true;
  LastCommandTime=millis(); //  reset the communication timeout
  WirelessConfigState=0;
  memset(WirelessBuffer,0,SERIAL_BUFFER_SIZE);
  WirelessPointer=0;
  //delay(1100);
  Wireless.print("+++");
  //delay(1100);
}

void ProcessWirelessData()
{

  /* handle xbee setup first */
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
            FoundXbee=true;
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
      //  reset out timeout
      LastCommandTime=millis();;
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
    return ProcessWirelessData();
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
  //  just a quick hack until we put error detection on
  //  this link
  if(a < 0) {
    Computer.println("Clearing garbage");
    WirelessPointer=0;
    WirelessBuffer[0]=0;
  }
  //Computer.print(a);
  return;
}

unsigned long int LastBatteryCheck=0;


void CheckBattery()
{
  int volts;
  unsigned long int now;

  now=millis();
  //  deal with rollovers
  if(now < LastBatteryCheck) LastBatteryCheck=now;
  //  only checking once every 15 seconds
  if((now - LastBatteryCheck) < 15000) return;
  LastBatteryCheck=now;
  volts=analogRead(VPIN);
  //Computer.println(volts);
  volts=volts/2;
  volts=volts*3;
  //v=(float)volts/(float)100;
  BatteryVolts=volts;
  //Computer.println(volts);  
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
    if(buttonstate != 0) SerialTarget=false;
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

  //  if we are being driven remotely, ie not by direct push buttons
  if(SerialTarget) {
    now=millis();
    //  deal with rollovers of the millis return
    if(now < LastCommandTime) LastCommandTime=now;
    if(!FoundXbee) {
      //  we have not had an ok return from xbee yet
      //if(!DoingWirelessConfig) {
        if(now-LastCommandTime > 10000) {
          //  we have gone 10 seconds with no response from the xbee
          //  and it's not been configured yet
          //  we should reconfigure the wireless link
          //  start by resetting the xbee state machine timeouts
          ShutterCommunicationTimeout=15000;
          LastCommandTime=now;
          ConfigureWireless();
        }
      //}
    }
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
            Computer.println("Drop dead timer closing shutter");
            Shutter.CloseShutter();
          }
        }
      }
    }
  }
}
