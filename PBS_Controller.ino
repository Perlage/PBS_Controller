/*
//===========================================================================  

FIZZIQ Cocktail Bottling System v1.1

Author: 
  Evan Wallace
  Perlage Systems, Inc.
  Seattle, WA 98101 USA
  
Copyright 2013, 2014  All rights reserved

//===========================================================================  
*/

//Version control variables
String (versionSoftwareTag)   = "v1.1"   ;     
// v1.1 includes numerous bug fixes and P1 has been moved AFTER check valve 1. This required no software changes.
// String (versionHardwareTag) = "v1.0.0.0" ;     

//Library includes
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "floatToString.h"               
#include <EEPROM.h>
//#include <avr/pgmspace.h>                      // 1-26 Added for PROGMEM function // UNUSED now
//#include <math.h>                              // Unused

LiquidCrystal_I2C lcd(0x3F,20,4);  

// Pin assignments
const int button1Pin                 =  0;     // pin for button1 B1 (Raise platform) RX=0;
const int button2Pin                 =  1;     // pin for button2 B2 (Fill/purge)     TX=1; 
// 2 SDA for LCD                     =  2;  
// 3 SDL for LCD                     =  3;
const int button3Pin                 =  4;     // pin for button3 B3 (Depressurize and lower) 
const int relay1Pin                  =  5;     // pin for relay1 S1 (liquid fill)
const int relay2Pin                  =  6;     // pin for relay2 S2 (bottle gas inlet)
const int relay3Pin                  =  7;     // pin for relay3 S3 (bottle gas vent)
const int relay4Pin                  =  8;     // pin for relay4 S4 (pneumatic lift gas in)
const int relay5Pin                  =  9;     // pin for relay5 S5 (pneumatic lift gas out)
const int relay6Pin                  = 10;     // pin for relay6 S6 (door lock solenoid)
const int sensorFillPin              = 11;     // pin for fill sensor F1 // DO NOT PUT THIS ON PIN 13!!
const int switchDoorPin              = 12;     // pin for door switch
const int buzzerPin                  = 13;     // pin for buzzer

const int sensorP2Pin                = A0;     // pin for pressure sensor 2 // 1-23 NOW REGULATOR PRESSURE. 
const int sensorP1Pin                = A1;     // pin for pressure sensor 1 // 1-23 NOW BOTTLE PRESSURE
const int switchModePin              = A2;     // New pin for Mode switch (formerly Clean Switch)
const int light1Pin                  = A3;     // pin for button1 light 
const int light2Pin                  = A4;     // pin for button2 light
const int light3Pin                  = A5;     // pin for button3 light

// A0 formerly was sensor1, which is closest to what we now call bottle pressure, and A1 was unused. 
// So switched sensor1 to A1 to make code changes easier. All reads of sensor1 will now read bottle pressure

// Declare variables and initialize states
// ===========================================================================  

//Component states //FEB 1: Changed from int to boolean. 300 byte drop in program size
boolean button1State                 = HIGH;
boolean button2State                 = HIGH; 
boolean button3State                 = HIGH;
boolean light1State                  = LOW;
boolean light2State                  = LOW;
boolean light3State                  = LOW;
boolean relay1State                  = HIGH;
boolean relay2State                  = HIGH;
boolean relay3State                  = HIGH;
boolean relay4State                  = HIGH;
boolean relay5State                  = HIGH;
boolean relay6State                  = HIGH;
boolean sensorFillState              = HIGH; 
boolean sensorFillStateTEMP          = HIGH;
boolean switchDoorState              = HIGH;
boolean switchModeState              = HIGH; //LOW is Manual, HIGH (or up) is auto (normal)

//State variables 
boolean inPressureNullLoop           = false;
boolean inPlatformUpLoop             = false;
boolean inPurgeLoop                  = false;
boolean inPressurizeLoop             = false;
boolean inFillLoop                   = false;
boolean inOverFillLoop               = false;
boolean inDepressurizeLoop           = false;
boolean inPlatformLowerLoop          = false;
boolean inDoorOpenLoop               = false;
boolean inPressureSaggedLoop         = false;
boolean inMenuLoop                   = false;
boolean inManualModeLoop             = false;
boolean inManualModeLoop1            = false;
boolean inCleaningMode               = false;
boolean inPressureDropLoop           = false;
boolean inPlatformEmergencyLock      = false;
boolean inDiagnosticMode             = false;

//Key logical states
boolean autoMode_1                   = false;   // Variable to help differentiate between states reached automatically vs manually 
boolean platformLockedNew            = false;   // Variable to be set when platfrom first locks up. This is for autofill-on-doorclose function
boolean platformStateUp              = false;   // true means platform locked in UP; toggled false when S5 opens

//Pressure constants                            // NOTE: 1 psi = 12.6 units. 
int offsetP1;                                   // Zero offset for pressure sensor1 (bottle). Set through EEPROM in initial factory calibration, and read into pressureOffset during Setup loop
int offsetP2;                                   // Zero offest for pressure sensor2 (input regulator). Ditto above.
const int pressureDeltaUp            = 50;      // Pressure at which, during pressurization, full pressure is considered to have been reached // Tried 10, 38; went back to 50 to prevent repressurizing after fill button cycle
const int pressureDeltaDown          = 38;      // Pressure at which, during depressurizing, pressure considered to be close enough to zero // 38 works out to 3.0 psi 
const int pressureDeltaMax           = 100;     // This is max pressure difference allowed on filling (i.e., limits fill speed) //v1.1 Now can reduce this significantly from 250 since have two pressure sensors
const int pressureNull               = 200;     // This is the threshold for the controller deciding that no gas source is attached. 
const int pressureDropAllowed        = 100;     // Max pressure drop allowed in session before alarm sounds
int pressureRegStartUp;                         // Starting regulator pressure. Will use to detect pressure sag during session; and to find proportional values for key pressure variables (e.g. pressureDeltaMax)

//Pressure conversion and output variables
int P1;                                         // Current pressure reading from sensor1 (BOTTLE)
int P2;                                         // Current pressure reading from sensor2 (REGULATOR/KEG)
float PSI1;
float PSI2;
float PSIdiff;
String (convPSI1);
String (convPSI2);
String (convPSIdiff);
String (outputPSI_rbd);                         // Input, bottle, difference
String (outputPSI_rb);                          // Input, bottle
String (outputPSI_b);                           // Bottle
String (outputPSI_r);                           // Regulator
String (outputPSI_d);                           // Difference

//Variables for platform function and timing
int timePlatformInit;                           // Time in ms going into loop
int timePlatformCurrent;                        // Time in ms currently in loop
int timePlatformRising               = 0;       // Time diffence between Init and Current
const int timePlatformLock           = 1250;    // Time in ms before platform locks in up position
const int autoPlatformDropDuration   = 1500;    // Duration of platform autodrop in ms

//Key performance parameters
int autoSiphonDuration;                         // Duration of autosiphon function in ms
byte autoSiphonDuration10s;                     // Duration of autosiphon function in 10ths of sec
float autoSiphonDurationSec;                    // Duration of autosiphon function in sec
const int antiDripDuration           =  500;    // Duration of anti-drip autosiphon
const int foamDetectionDelay         = 2000;    // Amount of time to pause after foam detection
const int pausePlatformDrop          = 1000;    // Pause before platform drops after door automatically opens
 
//Variables for button toggle states 
boolean button2StateTEMP             = HIGH;
boolean button3StateTEMP             = HIGH;
boolean button2ToggleState           = false;
boolean button3ToggleState           = false;

//System variables      
int numberCycles;                                // Number of cycles since factory reset, measured by number of platform PLUS fill loop being executed
byte numberCycles01;                             // Ones digit for numberCycles in EEPROM in base 255
byte numberCycles10;                             // Tens digit for numberCycles in EEPROM in base 255
int numberCyclesSession              = 0;        // Number of session cycles
boolean buzzedOnce                   = false;
boolean inFillLoopExecuted           = false;    // True of FillLoop is dirty. Used to compute numberCycles
char buffer[25];                                 // Used in float to string conv // 1-26 Changed from 25 to 20 //CHANGED BACK TO 25!! SEEMS TO BE IMPORTANT!

//======================================================================================
// PROCESS LOCAL INCLUDES
//======================================================================================

// Order is important!!!! Must include an include before it is referenced by other includes
#include "functions.h"			//Functions
#include "lcdMessages.h"		//User messages
#include "loops.h"				//Major resused loops
#include "manualMode.h"			//Manual Mode function
#include "menuShell.h"			//Menu shell
#include "nullPressureCheck.h"	//Null Pressure routines

//=====================================================================================
// SETUP LOOP
//=====================================================================================

void setup()
{
  //Setup pins
  pinMode(button1Pin, INPUT);  //Changed buttons from INPUT_PULLUP to PULLUP when installed touchbuttons, which use a pulldown resistor. 
  pinMode(button2Pin, INPUT);  //Unpushed state is LOW with buttons 1,2,3
  pinMode(button3Pin, INPUT);  
  pinMode(relay1Pin, OUTPUT);      
  pinMode(relay2Pin, OUTPUT);
  pinMode(relay3Pin, OUTPUT);      
  pinMode(relay4Pin, OUTPUT);
  pinMode(relay5Pin, OUTPUT);  
  pinMode(relay6Pin, OUTPUT);  
  pinMode(light1Pin, OUTPUT); 
  pinMode(light2Pin, OUTPUT);
  pinMode(light3Pin, OUTPUT);
  pinMode(sensorFillPin, INPUT_PULLUP); 
  pinMode(switchDoorPin, INPUT_PULLUP); 
  pinMode(switchModePin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  
  //set all relay pins to high which is "off" for this relay
  digitalWrite(relay1Pin, HIGH);
  digitalWrite(relay2Pin, HIGH);
  digitalWrite(relay3Pin, LOW);    //OPEN ASAP
  digitalWrite(relay4Pin, HIGH);   
  digitalWrite(relay5Pin, HIGH); 
  digitalWrite(relay6Pin, HIGH);
  
  //set to HIGH which open (off) for touchbuttons 
  digitalWrite(button1Pin, HIGH); 
  digitalWrite(button2Pin, HIGH);
  digitalWrite(button3Pin, HIGH);
  
  //Serial.begin(9600); // TO DO: remove when done
  
  // Initialize LCD - Absolutely necessary
  lcd.init();
  lcd.backlight();
  

  //===================================================================================
  // STARTUP ROUTINE
  //===================================================================================

  //===================================================================================
  // EEPROM factory set-reset routine 
  // For some reason, have to include it here and not above with others
  
  messageInitial();
  delay(1000);
  
  #include "EEPROMset.h"   
  
  // Read EEPROM
  offsetP1               = EEPROM.read(0);
  offsetP2               = EEPROM.read(1);
  autoSiphonDuration10s  = EEPROM.read(3);    
  numberCycles01         = EEPROM.read(4);  //Write "ones" digit base 255    
  numberCycles10         = EEPROM.read(5);  //Write "tens" digit base 255
  
  //Read routine for autoSiphon  
  autoSiphonDuration = autoSiphonDuration10s * 100; // Convert 10ths of sec to millisec

  //Read routine for numberCycles  
  numberCycles = numberCycles10 * 255 + numberCycles01;
  //numberCycles = nnnn; // Use to set numberCyles to some value for debug

  // Read States. Get initial pressure readings from sensor. 
  switchDoorState = digitalRead(switchDoorPin);  
  P1 = analogRead(sensorP1Pin); // Read the initial bottle pressure and assign to P1
  P2 = analogRead(sensorP2Pin); // Read the initial regulator pressure and assign to P2
  pressureRegStartUp = P2;      // Read and store initial regulator pressure to test for pressure drop during session
  
  //Allow user to invoke Manual Mode from bootup before anything else happens
  while (button1State == LOW)
  {
    inManualModeLoop = true;
    button1State = !digitalRead(button1Pin); 
    lcd.setCursor (0, 0); lcd.print (F("Entering Manual Mode"));
    buzzOnce(500, light2Pin);
  }  
  buzzedOnce = false;

  if (inManualModeLoop == true){
    manualModeLoop();
  }
  
  //===================================================================================
  //NULL PRESSURE: Check for stuck pressurized bottle or implausibly low gas pressure at start 
  
  nullPressureStartup();

  //Initial user message 
  messageInitial();
  
  //UNCOMMENT OUT NEXT LINE FOR SHOW
  //relayOn(relay4Pin, true);  // Now Raise platform     
  
  // Leave blink loop and light all lights 
  digitalWrite(light1Pin, HIGH); delay(500);
  digitalWrite(light2Pin, HIGH); delay(500);
  digitalWrite(light3Pin, HIGH); delay(500); 

  // Continue with normal ending when pressure is OK
  // ====================================================================================

  //NOW print lifetime fills and autosiphon duration 
  autoSiphonDurationSec = float(autoSiphonDuration10s) / 10;
  String (convInt) = floatToString(buffer, autoSiphonDurationSec, 1);
  String (outputInt) = "Autosiphon: " + convInt + " sec";
  printLcd (2, outputInt); 
  delay(1500); 

  convInt = floatToString(buffer, numberCycles, 0);
  outputInt = "Total fills: " + convInt;
  printLcd (2, outputInt);  
  delay (1000);

  // Open door if closed. Moved this to make sure door opens before plaform drops in case of unpressurized stuck bottle
  doorOpen();
  
  // Drop platform in case it is up, which had been raised before nullpressure checks
  relayOn(relay4Pin, false);  
  relayOn(relay5Pin, true);
  delay(2000);  
  relayOn(relay5Pin, false); 
  relayOn(relay3Pin, false); // Close this so solenoid doesn't overhead on normal startup

  delay(500); digitalWrite(light3Pin, LOW);
  delay(100); digitalWrite(light2Pin, LOW);
  delay(100); digitalWrite(light1Pin, LOW);
  
  buzzer (1000);
}
  
//====================================================================================================================================
// MAIN LOOP =========================================================================================================================
//====================================================================================================================================

void loop()
{
  
  //MAIN LOOP IDLE FUNCTIONS
  //=====================================================================

  // Read the state of buttons and sensors
  // Boolean NOT (!) added for touchbuttons so that HIGH button state (i.e., button pressed) reads as LOW (this was easiest way to change software from physical buttons, where pressed = LOW) 
  button1State			= !digitalRead(button1Pin); 
  button2StateTEMP		= !digitalRead(button2Pin); 
  button3StateTEMP		= !digitalRead(button3Pin); 
  switchDoorState		=  digitalRead(switchDoorPin);
  switchModeState		=  digitalRead(switchModePin); 
  sensorFillStateTEMP   =  digitalRead(sensorFillPin); // Maybe we don't need to measure this
  
  sensorFillState		=  HIGH; //Instead we SET the sensorState HIGH.

  //Check Button2 toggle state
  //======================================================================
  if (button2StateTEMP == LOW && button2ToggleState == false){  //ON push
    button2State = LOW;         //goto while loop
  }
  if (button2StateTEMP == LOW && button2ToggleState == true){   //button still being held down after OFF push in while loop. 
    button2State = HIGH;        //nothing happens--buttonState remains HIGH
  } 
  if (button2StateTEMP == HIGH && button2ToggleState == true){  //OFF release
    button2ToggleState = false; //buttonState remains HIGH
  }
  //Check Button3 toggle state
  //======================================================================
  if (button3StateTEMP == LOW && button3ToggleState == false){  //ON push
    button3State = LOW;         //goto while loop
  }
  if (button3StateTEMP == LOW && button3ToggleState == true){   //button still being held down after OFF push in while loop. 
    button3State = HIGH;        //nothing happens--buttonState remains HIGH
  } 
  if (button3StateTEMP == HIGH && button3ToggleState == true){  //OFF release
    button3ToggleState = false; //buttonState remains HIGH
  }
   
  // Give relevant instructions in null loop
  //======================================================================
  if (switchDoorState == LOW && platformStateUp == false)
  {
    lcd.setCursor (0, 0); lcd.print (F("B3 opens door;      ")); 
    lcd.setCursor (0, 1); lcd.print (F("Insert bottle...    "));
  }
  if (switchDoorState == HIGH && platformStateUp == false)
  {
    lcd.setCursor (0, 0); lcd.print (F("Insert bottle;      "));
    lcd.setCursor (0, 1); lcd.print (F("B1 raises platform. "));  
  }  
  /*
  if (platformStateUp == true)
  {
    lcd.setCursor (0, 0); lcd.print (F("B2 toggles filling; ")); //This will rarely get hit
    lcd.setCursor (0, 1); lcd.print (F("B3 toggles exhaust. "));
  }
  */
  lcd.setCursor (0, 2); lcd.print (F("Ready...            "));  
 

  //======================================================================
  //MULTI BUTTON COMBO ROUTINES
  //======================================================================

  boolean inMenuLoop = false;
  boolean inPurgeLoop = false;
  boolean inMultiButtonLockLoop = false;
  
  while (!digitalRead(button2Pin) == LOW && platformStateUp == false)
  {
    inMultiButtonLockLoop = true;
    digitalWrite (light2Pin, HIGH);
    
    //PURGE ROUTINE
    //====================================================================
    while (!digitalRead(button1Pin) == LOW && switchDoorState == HIGH)
    {
      inPurgeLoop = true;
      lcd.setCursor (0, 2); lcd.print (F("Purging...          "));
      digitalWrite(light1Pin, HIGH); 
      relayOn(relay2Pin, true); //open S2 to purge
    }
    if(inPurgeLoop)
    {
      relayOn(relay2Pin, false); //Close relay when B1 and B2 not pushed
      inPurgeLoop = false;
      digitalWrite(light1Pin, LOW); 
    }
 
    //MENU ROUTINE
    //====================================================================
    while (!digitalRead(button3Pin) == LOW)
    {
      inMenuLoop = true;
      lcd.setCursor (0, 2); lcd.print (F("Entering Menu...    ")); 
      buzzOnce(500, light3Pin);
    }
  }  
  if (inMultiButtonLockLoop)
  {
    inMultiButtonLockLoop = false;
    buzzedOnce = false;
    
    //Enter menuLoop once B3 released
    if (inMenuLoop){
      digitalWrite (light2Pin, LOW);
      menuShell(inMenuLoop);
    }  
    
    lcd.setCursor (0, 2); lcd.print (F("Wait...             ")); 
    delay(750);  //This is to prevent nullPressure loop from kicking in
    digitalWrite (light2Pin, LOW);
  }
  
  // Main Loop idle pressure measurement and LCD output
  //======================================================================

  pressureOutput();
  if (P1 - offsetP1 > pressureDeltaDown){
    printLcd(3, outputPSI_rb); // Print reg and bottle if bottle pressurized
  }
  else{
    printLcd(3, outputPSI_r);  // Print only reg if bottle not present or at zero pressure
  }  
  

  // v1.1 Door opened while bottle pressurized...emergency dump of pressure  
  //========================================================================
  if (platformStateUp == true && switchDoorState == HIGH && (P1 - offsetP1 >= pressureDeltaDown + 50)) // May need to raise this threshold
  { 
    emergencyDepressurize();
  }  


  //  This routine takes action if pressure drops in idle loop. TO DO: NEEDS WORK
  idleLoopPressureDrop();
  
  
  // =====================================================================================  
  // PLATFORM RAISING LOOP
  // while B1 is pressed, platform is not UP, and door is open, raise bottle platform.
  // =====================================================================================  

  platformUpLoop();

  //============================================================================================
  // PRESSURIZE LOOP
  // Pressurization will start automatically when door closes IF platfromLockedNew is true
  //============================================================================================
    
  int PTest1;
  int PTest2;
  int pressurizeStartTime;
  int pressurizeDuration = 0;
  int pressurizeCurrentTime = 0;
  boolean PTestFail = false;

  // Get fresh pressure readings  
  P1 = analogRead(sensorP1Pin);
  P2 = analogRead(sensorP2Pin);       
  
  while((button2State == LOW || platformLockedNew == true) && switchDoorState == LOW && platformStateUp == true && (P1 - offsetP1 <= P2 - offsetP2 - pressureDeltaUp)) 
  { 
    inPressurizeLoop = true;

    // Only want to run this once per platformUp--not on subsequent repressurizations within a cycle 
    if (platformLockedNew == true)
    {
      //delay(250);                                         //Make a slight delay in starting pressuriztion when door is first closed
      pressurizeStartTime = millis();                       //startTime for no bottle test below. 
    }
    
    //Open gas-in relay, close gas-out
    relayOn(relay3Pin, false);                              //close S3 if not already 
    relayOn(relay2Pin, true);                               //open S2 to pressurize
    digitalWrite(light2Pin, HIGH);                       

    //NO BOTTLE TEST: Check to see if bottle is pressurizing; if PTest not falling, must be no bottle 
    while (platformLockedNew == true)                       //First time through loop with platform up 
    {
      pressurizeDuration = millis() - pressurizeStartTime;  //Get the duration
      if (pressurizeDuration < 50){
        PTest1 = analogRead(sensorP1Pin);                   //Take a reading at 50ms after pressurization begins. 50 ms gives time for pressure to settle down after S2 opens
      }
      if (pressurizeDuration < 200){
        PTest2 = analogRead(sensorP1Pin);                   //Take a reading at 100ms after pressurization begins
      }  
      if (pressurizeDuration > 200){                        //After 100ms, test
        if (PTest2 - PTest1 < 20){                          //If there is less than a 15 unit difference, must be no bottle in place
          button2State = HIGH; 
          relayOn(relay2Pin, false);                        //close S2 immediately
          button2ToggleState = true;                        //Need this to keep toggle routine below from changing button2state back to LOW 
          PTestFail = true;                                 //If true, no bottle. EXIT 
        }  
        platformLockedNew = false;
      }
      /* This code works better than breakpoint. BP can't process messages fast enough
      Serial.print ("T= "); 
      Serial.print (pressurizeDuration);
      Serial.print (" P1= ");  
      Serial.print (PTest1);
      Serial.print (" P2= "); 
      Serial.print (PTest2);
      Serial.println (); 
      */
    }
    
    //Read sensors
    switchDoorState = digitalRead(switchDoorPin); //Check door switch    
    P1 = analogRead(sensorP1Pin); // Don't really even need to read P2; read it going into loop?
   
    //Check toggle state of B2
    button2StateTEMP = !digitalRead(button2Pin);

    if (button2StateTEMP == HIGH && button2ToggleState == false){  //ON release
      button2ToggleState = true;                                   //leave button state LOW
      button2State = LOW;                                          //or make it low if it isn't yet
    }
    if (button2StateTEMP == LOW && button2ToggleState == true){    //OFF push
      button2State = HIGH;                                         //exit WHILE loop
    }  
      
	messageB2B3Toggles();
    lcd.setCursor (0, 2); lcd.print (F("Pressurizing...     "));

    // Pressure output
    pressureOutput();
    printLcd(3, outputPSI_b); 
  }
    
  // PRESSURIZE LOOP EXIT ROUTINES
  //=======================================================================================
  
  if(inPressurizeLoop)
  {
    inPressurizeLoop = false;

    relayOn(relay2Pin, false);       // close S2 because we left PressureLoop
    digitalWrite(light2Pin, LOW);    // Turn off B2 light
    
    //PRESSURE TEST EXIT ROUTINE
    if (PTestFail == true)
    {
      lcd.setCursor (0, 2); lcd.print (F("No bottle, or leak. "));
      lcd.setCursor (0, 3); lcd.print (F("Wait...             "));
      digitalWrite(light1Pin, LOW); 

      pressureDump();   // Dump any pressure that built up
      doorOpen();       // Open door
      platformDrop();   // Drop platform
      
      //Reset variables
      PTestFail = false;      
      timePlatformRising = 0;
      platformStateUp = false;      
    }
    
    //Door opened while bottle pressurized...emergency dump of pressure  
    if (platformStateUp == true && switchDoorState == HIGH)
    { 
      emergencyDepressurize();
    }  
  }

  // END PRESSURIZE LOOP
  //========================================================================================
  
  //========================================================================================
  // FILL LOOP
  //========================================================================================

  // Get fresh pressure readings
  P1 = analogRead (sensorP1Pin);
  P2 = analogRead (sensorP2Pin);

  //pinMode(sensorFillPin, INPUT_PULLUP); //Probably no longer necessary since FillSwitch was moved off Pin13 (Zach proposed this Oct-7)

  // First pressureDeltaMax condition ensures filling can't go too fast (unless in cleaning mode); second ensures can't dispense w/out pressurized bottle
  // v1.1 Changed the pressureDeltaMax condition to reflect fact that there are two sensors
  while(button2State == LOW && (sensorFillState == HIGH || inCleaningMode == true) && switchDoorState == LOW && (((P2 - offsetP2) - (P1 - offsetP1) < pressureDeltaMax) || inCleaningMode == true) && (P1 - offsetP1) > pressureDeltaMax) 
  {     
    inFillLoop = true;
    inFillLoopExecuted = true; //This is an "is dirty" variable for counting lifetime bottles. Reset in platformUpLoop.

    //while the button is  pressed, fill the bottle: open S1 and S3
    relayOn(relay1Pin, true);
    relayOn(relay3Pin, true);
    digitalWrite(light2Pin, HIGH); 

    //Check toggle state of B2    
    button2StateTEMP = !digitalRead(button2Pin);
    //delay(25);
    if (button2StateTEMP == HIGH && button2ToggleState == false){  //ON release
      button2ToggleState = true;                                   //Leaves buttonState LOW
      button2State = LOW;                                          //Or makes it low
    }
    if (button2StateTEMP == LOW && button2ToggleState == true){    // OFF push
      button2State = HIGH;                                         //exit WHILE loop
    }
    
    //Read and output pressure
    pressureOutput();
    printLcd (3, outputPSI_rb); 
    
    // CLEANING MODE: If in Cleaning Mode, set FillState HIGH to disable sensor
    if (inCleaningMode == true)
    {
      sensorFillState = HIGH;      
      lcd.setCursor (0, 2); lcd.print (F("Filling--Cleaning ON"));
    }
    else
    {
      //Read sensors
      sensorFillState = digitalRead(sensorFillPin); //Check fill sensor
      switchDoorState = digitalRead(switchDoorPin); //Check door switch 
      lcd.setCursor (0, 2); lcd.print (F("Filling...          "));
    }
  }

  // FILL LOOP EXIT ROUTINES
  // Either 1) B2 was released, or 2) FillSwitch was tripped, or 
  // 3) pressure dipped too much from filling too fast. Repressurize in 2 and 3
  //===============================================================================================================
  
  if(inFillLoop)
  {
    digitalWrite(light2Pin, LOW);
    relayOn(relay1Pin, false);
    relayOn(relay3Pin, false);
    printLcd(2,""); 
    
    // Check which condition caused filling to stop
    // CASE 1: Button2 pressed when filling (B2 low and toggle state true). Run Anti-Drip
    if (button2State == HIGH && !inCleaningMode == true)
    {
      // Anti-drip rountine
      relayOn(relay1Pin, true);
      relayOn(relay2Pin, true);
      
      //lcd.setCursor (0, 2); lcd.print (F("Preventing drip... "));
      delay(antiDripDuration); // This setting determines duration of autosiphon 
      //lcd.setCursor (0, 2); lcd.print (F("                   "));

      relayOn(relay1Pin, false);
      relayOn(relay2Pin, false);
      
      //delay (25);
      //sensorFillState = digitalRead(sensorFillPin); //v1.1 This doesn't do anything
    }
    
    // CASE 2: FillSensor tripped--Overfill condition
    else if (inFillLoop && sensorFillState == LOW) 
    {
      relayOn(relay1Pin, true);
      relayOn(relay2Pin, true);
      
      lcd.setCursor (0, 2); lcd.print (F("Adjusting level...  "));
      delay(autoSiphonDuration); // This setting determines duration of autosiphon 
      relayOn(relay1Pin, false);
      relayOn(relay2Pin, false);

      delay (1000); //Added this to create a slight delay to remedy the immediate false foam detection 
      lcd.setCursor (0, 2); lcd.print (F("                    "));
      
      //v1.1 Clear Sensor routine under development 
      if (digitalRead(sensorFillPin) == LOW)
      {
        lcd.setCursor (0, 2); lcd.print (F("Clearing Fill Sensor"));
        relayOn(relay3Pin, true); delay (1000); relayOn(relay3Pin, false);
        relayOn(relay2Pin, true); delay (250); relayOn(relay2Pin, false);
      }  
      //sensorFillState = digitalRead(sensorFillPin); //TO DO: SET sensorFillState to LOW rather than meassuring it for force into depressurize loop. This is what we do after detecting foam--not measuring But think about case where liquid doesn't siphone down and stays in contact it
      sensorFillState = HIGH; // Changed it back to this

      button3State = LOW; // This make AUTO-depressurize after overfill // TO DO: shouldn't this be AutoMode_1?
    }
    else 
    {
      // CASE 3: Bottle pressure dropped below Deltatamp threshold; i.e., filling too fast
      // FILLING TOO FAST LOOP    
      while (platformStateUp == true && switchDoorState == LOW && (P1 - offsetP1) <= (P2 - offsetP2) - pressureDeltaUp)
      {
        inPressurizeLoop = true;
        digitalWrite(light2Pin, HIGH); 
        relayOn(relay2Pin, true);       //open S2 to equalize
        P1 = analogRead(sensorP1Pin);    //do sensor read again to check
        P2 = analogRead(sensorP2Pin);    //do sensor read again to check

        lcd.setCursor (0, 0); lcd.print (F("Filling too fast... "));
        lcd.setCursor (0, 1); lcd.print (F("Close exhaust valve,"));
        lcd.setCursor (0, 2); lcd.print (F("Press B2 to resume  "));
        delay (500); // This seems to be necessary
      }
	  
      // FILLING TOO FAST LOOP EXIT ROUTINES      
      if(inPressurizeLoop)
      { 
        relayOn(relay2Pin, false);  
        digitalWrite(light2Pin, LOW);
        
        //This is a little trap for B2 button to demand user input; doesn't change button states. Just wants user input.
        while(!digitalRead(button2Pin) == HIGH){
          digitalWrite(light2Pin, HIGH);
          buzzer (150);
          digitalWrite(light2Pin, LOW);
          delay (350);
        }
        button2ToggleState = false; // button2State is still LOW, with toggleState true. Setting toggleState to false should be like pressing Fill again

        printLcd(2,"");
        inPressurizeLoop = false; 
      } 
      // END FILLING TOO FAST LOOP   
      //=============================================     
    } 
   
    //Door opened while bottle filling...emergency dump of pressure  
    if (platformStateUp == true && switchDoorState == HIGH)
    { 
      emergencyDepressurize();
    }      
    inFillLoop = false;
	messageB2B3Toggles();
  }

  // END FILL LOOP EXIT ROUTINES
  //========================================================================================

  // #include "autoCarbonator.h"

  //========================================================================================
  // DEPRESSURIZE LOOP
  //========================================================================================
  
  while(button3State == LOW && (sensorFillState == HIGH || inCleaningMode == true) && (P1 - offsetP1 >= pressureDeltaDown)) 
  {  
    inDepressurizeLoop = true;
    digitalWrite(light3Pin, HIGH);

    relayOn(relay3Pin, true); //Open Gas Out solenoid
    
    // Pressure output    
    pressureOutput();
    printLcd(3, outputPSI_rb); 
    
    //Allow momentary "burst" foam tamping
    button2State = !digitalRead(button2Pin);
      if(button2State == LOW)
      {
        digitalWrite(light2Pin, HIGH); 
        relayOn(relay2Pin, true);
        delay(50);  // Burst duration
        relayOn(relay2Pin, false);
        digitalWrite(light2Pin, LOW);
      }
    
    // CLEANING MODE: If in cleaning mode, set FillState HIGH
    if (inCleaningMode == true)
    {
      sensorFillState = HIGH;      
      lcd.setCursor (0, 2); lcd.print (F("Venting--Cleaning ON"));
    }
    else
    {
      sensorFillState = digitalRead(sensorFillPin); //Check fill sensor
      switchDoorState = digitalRead(switchDoorPin); //Check door switch // Not using this
      lcd.setCursor (0, 2); lcd.print (F("Depressurizing...   "));
    }   
    
    //switchModeState = digitalRead(switchModePin); //Check cleaning switch //Not using this
    
    //Check toggle state of B3
    button3StateTEMP = !digitalRead(button3Pin);
    if (button3StateTEMP == HIGH && button3ToggleState == false){  // ON release
      button3ToggleState = true;                                   // Leaves buttonState LOW
      button3State = LOW; 
    }
    if (button3StateTEMP == LOW && button3ToggleState == true){    // OFF push
      button3State = HIGH;                                         // Exit WHILE loop
    }
}

  // DEPRESSURIZE LOOP EXIT ROUTINES 
  // If in this loop, Button3 was released, or Fill/Foam Sensor tripped, 
  // or bottle is propeRly pressurized. Find out which reason and reSpond.
  //========================================================================
  
  if(inDepressurizeLoop)
  { 
    printLcd(2, "");
    //relayOn(relay3Pin, false);     //Used to close S3 here. Try leaving it open when in auto mode until platform drops
    
    // CASE 1: Button3 released
    if (button3State == HIGH)  
    { 
      relayOn(relay3Pin, false);     //Used to repressurize here; now we don't  
      //TO DO: either a quick burst to clear the sensor, or check for overfill and tamp in case bottle foams up
    }
    
    // CASE 2: Foam tripped sensor
    if (sensorFillState == LOW)
    {
      lcd.setCursor (0, 2); lcd.print (F("Foam detected...wait"));
      relayOn(relay3Pin, false);  
      relayOn(relay2Pin, true);
      delay(foamDetectionDelay);    // Wait a bit before proceeding    
      relayOn(relay2Pin, false);
      
      //v1.1 Clear Sensor routine under development //I don't think this does anything. Functionally the same as running another cycle.
      if (digitalRead(sensorFillPin) == LOW)
      {
        lcd.setCursor (0, 2); lcd.print (F("Clearing Foam Sensor"));
        delay(1000); //Decided to just have a delay instead
        //relayOn(relay3Pin, true); delay (750); relayOn(relay3Pin, false);
        //relayOn(relay2Pin, true); delay (250); relayOn(relay2Pin, false);
      }
      
      lcd.setCursor (0, 2); lcd.print (F("                    "));

      sensorFillState = HIGH; //v1.1 Set the sensorState to HIGH
      //sensorFillState = digitalRead(sensorFillPin); 
    }

    //CASE 3: Bottle was properly depressurized. If we reach here, the pressure must have reached threshold. Go to Platform lower loop
    if (P1 - offsetP1 <= pressureDeltaDown)
    {
      buzzer(100);
      autoMode_1 = true;  //Going to platform loop automatically, so set this var to partially drop platform 
    }
    
    digitalWrite(light3Pin, LOW);
    inDepressurizeLoop = false;
   }
  
  // END DEPRESSURIZE LOOP
  //============================================================================================

  // ===========================================================================================
  // DOOR OPENING LOOP
  // ===========================================================================================
 
  // Only activate door solenoid if door is already closed
  
  while((button3State == LOW || autoMode_1 == true) && switchDoorState == LOW && (P1 - offsetP1) <= pressureDeltaDown)
  {
    inDoorOpenLoop = true;
    digitalWrite(light3Pin, HIGH);
    doorOpen(); // Run door open function

    //TO DO: Add timer: if door not open in 2 sec, must be stuck; show error message
  }

  // DOOR OPEN LOOP EXIT ROUTINES
  //============================================================================================

  if(inDoorOpenLoop)
  {
    while(button3State == LOW)
    {
      button3State = !digitalRead(button3Pin); //This is a little trap for B3 button push so platform lower loop doesn't kick in
    }
    delay(500); // This gives a pause so platform lower loop doesn't kick in
    inDoorOpenLoop = false;
    digitalWrite(light3Pin, LOW);
    button3State = HIGH; 
  }
  
  // END DOOR OPEN LOOP
  //============================================================================================
    
  // ===========================================================================================
  // PLATFORM LOWER LOOP 
  // Platform will not lower with door closed. This prevents someone from defeating door switch
  // ===========================================================================================

  LABEL_PlatformLowerLoop:
  
  // 1-24 Added sensorFillState == HIGH to prevent lowering plaform is foam is hitting sensor
  while((button3State == LOW || autoMode_1 == true) && switchDoorState == HIGH && sensorFillState == HIGH && (P1 - offsetP1) <= pressureDeltaDown)
  {
    inPlatformLowerLoop = true;
    relayOn(relay4Pin, false); // Finally can close platform up solenoid
    lcd.setCursor (0, 2); lcd.print (F("Lowering platform..."));
    
    if(autoMode_1 == true)
    {
      relayOn(relay5Pin, true);
      delay(autoPlatformDropDuration);  //Can adjust this value to determine how much to drop the platform in full auto mode
      relayOn(relay5Pin, false);
      button3State = HIGH;
      autoMode_1 = false;
    }
    else
    {
      //relayOn(relay3Pin, true);  // May as well leave this open? YES. Liquid is still outgassing.//TO DO: Don't need this here?? it's already open?
      relayOn(relay5Pin, true);  // Open cylinder exhaust
      digitalWrite(light3Pin, HIGH); 
    }

    P1 = analogRead(sensorP1Pin);
    button3State = !digitalRead(button3Pin);
    switchDoorState =  digitalRead(switchDoorPin);
    sensorFillState =  digitalRead(sensorFillPin);
  }

  // PLATFORM LOWER LOOP EXIT ROUTINES
  // ===========================================================================================
  
  if(inPlatformLowerLoop)
  {
    //close platform release 
    lcd.setCursor (0, 2); lcd.print (F("                    "));
    relayOn(relay3Pin, false); 
    relayOn(relay5Pin, false);
    relayOn(relay6Pin, false); //Release door solenoid
    digitalWrite(light3Pin, LOW); 

    inPlatformLowerLoop = false;   
    platformStateUp = false;

    //Prepare for next cycle
    lcd.setCursor (0, 0); lcd.print (F("Insert bottle;      "));
    lcd.setCursor (0, 1); lcd.print (F("B1 raises platform  "));

    // Calculate lifetime and session fills    
    if (inFillLoopExecuted)
    {
      numberCycles = numberCycles + 1;                   //Increment lifetime cycles
      numberCyclesSession = numberCyclesSession + 1;     //Increment session cycles
      
      // Write lifetime cycles back to EEPROM
      numberCycles01 = numberCycles % 255;
      numberCycles10 = (numberCycles - numberCycles01)/255;
      EEPROM.write(4, numberCycles01);
      EEPROM.write(5, numberCycles10);

      // Write session/total fills to screen
      //String (convNumberCycles) = floatToString(buffer, numberCycles, 0);
      String (convNumberCyclesSession) = floatToString(buffer, numberCyclesSession, 0);
      String outputInt = "Session fills: " + convNumberCyclesSession;
      printLcd(2, outputInt); 
      delay(2000);
    
      inFillLoopExecuted = false;
    }      
  } 
  
  // END PLAFORM LOWER LOOP
  //============================================================================================

}

//=======================================================================================================
// END MAIN LOOP ========================================================================================
//=======================================================================================================
     
