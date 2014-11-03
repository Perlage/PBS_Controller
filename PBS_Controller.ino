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

//Version control variable
String (versionSoftwareTag)   = "v1.1"   ;       

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
const int switchModePin              = A2;     // OPEN PIN
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
boolean platformLockedNew            = false;   // Variable to be set when platform first locks up. This is for autofill-on-door close function
boolean platformStateUp              = false;   // true means platform locked in UP; toggled false when S5 opens

//Pressure constants                            // NOTE: 1 psi = 12.71 units. 
int offsetP1;                                   // Zero offset for pressure sensor1 (bottle). Set through EEPROM in initial factory calibration, and read into pressureOffset during Setup loop
int offsetP2;                                   // Zero offset for pressure sensor2 (input regulator). Ditto above.
const int pressureDeltaUp            =  50;     // Pressure at which, during pressurization, full pressure is considered to have been reached // Tried 10, 38; went back to 50 to prevent repressurizing after fill button cycle
const int pressureDeltaDown          =  38;     // Pressure at which, during depressurizing, pressure considered to be close enough to zero // 38 works out to 3.0 psi 
const int pressureDeltaMax           = 150;     // FILLING TOO FAST criterion //v1.1 Now can reduce this significantly from 250 since have two pressure sensors. 100 is good.
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
String (outputPSI_rb);                          // Keg, bottle
String (outputPSI_b);                           // Bottle
String (outputPSI_r);                           // Keg

//Variables for platform function and timing
int timePlatformInit;                           // Time in ms going into loop
int timePlatformCurrent;                        // Time in ms currently in loop
int timePlatformRising               = 0;       // Time difference between Init and Current
const int timePlatformLock           = 1250;    // Time in ms before platform locks in up position
const int autoPlatformDropDuration   = 1500;    // Duration of platform auto drop in ms

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

//For display throttling messages
int Nf1;
int Nf2 = 0;
boolean PFlag	= true;			// Print flag
int throttleVal = 500;		// This is a global value; indidual functions can overwritten. The smaller the number, the higher the frequency of positive PFlags, given by (2000/throttleInt)

//======================================================================================
// PROCESS LOCAL INCLUDES
//======================================================================================

// Order is important!!!! Must include an include before it is referenced by other includes
#include "functions.h"					//Functions
#include "lcdMessages.h"				//User messages
#include "loops.h"							//Major reused loops
#include "manualMode.h"					//Manual Mode function
#include "menuShell.h"					//Menu shell
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
  pinMode(sensorFillPin, INPUT_PULLUP);  //Experimenting with INPUT and external pullup resistor
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

  // Read States. Get initial pressure readings from sensor. 
  switchDoorState = digitalRead(switchDoorPin);  
  P1 = analogRead(sensorP1Pin); // Read the initial bottle pressure and assign to P1
  P2 = analogRead(sensorP2Pin); // Read the initial regulator pressure and assign to P2
  pressureRegStartUp = P2;      // Read and store initial regulator pressure to test for pressure drop during session

	// MENU MODE: Allow user to invoke Menu Mode from bootup before anything else happens (buttons get read in EEPROM include)
  while (button2State == LOW && button3State == LOW)
  {
	  inMenuLoop = true;
	  button2State = !digitalRead(button2Pin);
		button3State = !digitalRead(button3Pin);
	  lcd.setCursor (0, 0); lcd.print (F("Entering Menu Mode  "));
	  buzzOnce(500, light2Pin);
  }
  buzzedOnce = false;

  if (inMenuLoop == true){
	  menuShell(inMenuLoop);
  }
    
  //NULL PRESSURE: Check for stuck pressurized bottle or implausibly low gas pressure at start 
	pressurizedBottleStartup();
  nullPressureStartup();

  messageInitial(); //Rewrite initial user message, in case pressure routines above wrote to screen
  
  //UNCOMMENT OUT NEXT 2 LINEs FOR SHOW
  //relayOn(relay4Pin, true);  // Now Raise platform
	//delay (1000);     
  
  // Leave blink loop and light all lights 
  digitalWrite(light1Pin, HIGH); delay(400);
  digitalWrite(light2Pin, HIGH); delay(200);
  digitalWrite(light3Pin, HIGH); 

  // Open door if closed. Moved this to make sure door opens before platform drops in case of unpressurized stuck bottle
	doorOpen(); 
  
  // Drop platform in case it is up, which had been raised before nullpressure checks
  relayOn(relay4Pin, false);  
  relayOn(relay5Pin, true);
  
	String convInt = floatToString(buffer, numberCycles, 0);
  String outputInt = "Total fills: " + convInt;
  printLcd (2, outputInt);
	delay(2000);  // Time to let platform drop
  relayOn(relay5Pin, false); 
  relayOn(relay3Pin, false); // Close this so solenoid doesn't overhead on normal startup

  digitalWrite(light3Pin, LOW);
  delay(200); digitalWrite(light2Pin, LOW);
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
  button1State				= !digitalRead(button1Pin); 
  button2StateTEMP		= !digitalRead(button2Pin); 
  button3StateTEMP		= !digitalRead(button3Pin); 
  switchDoorState			=  digitalRead(switchDoorPin);
  switchModeState			=  digitalRead(switchModePin); 
  sensorFillStateTEMP =  digitalRead(sensorFillPin); // Maybe we don't need to measure this

  sensorFillState		=  HIGH; //v1.1: We now SET the sensorState HIGH.

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
		lcd.setCursor (0, 1); lcd.print (F("B2+B3 loads Menu.   "));
  }
  
	if (switchDoorState == HIGH && platformStateUp == false)
  {
		messageRotator(6000, .25, 1500);
		if (messageID){
			messageInsertBottle(); // MESSAGE: Insert Bottle; B1 raises Platform
		}else{
			lcd.setCursor (0, 1); lcd.print (F("B2+B1 purges bottle."));
		}
  }  
	
	if (P1 - offsetP1 > pressureDeltaDown)
	{
		messageRotator(10000, .15, 0);
		if (messageID){
			lcd.setCursor (0, 1); lcd.print (F("B3 toggles exhaust; "));
		}else{
			lcd.setCursor (0, 1); lcd.print (F("B1 adjusts overfill."));
		}
	}
  
	messageLcdReady(); // MESSAGE: "Ready...        "  
	
  // Main Loop idle pressure measurement and LCD output
  //======================================================================

  pressureOutput();
  if (P1 - offsetP1 > pressureDeltaDown)
  {
	  printLcd2(3, outputPSI_rb, throttleVal); // Print reg and bottle if bottle pressurized
  }
  else
  {
		printLcd2(3, outputPSI_r, throttleVal); // Print only reg if bottle unpressurized
  }
	
	// Autosiphon in Main loop
	//=====================================================================
	while (!digitalRead(button1Pin) == LOW && (P1 - offsetP1 > pressureDeltaDown) && platformStateUp ==  true)
	{
		 relayOn(relay1Pin, true);                            
		 relayOn(relay2Pin, true);                               
		 digitalWrite(light1Pin, HIGH);
	}
	relayOn(relay1Pin, false);
	relayOn(relay2Pin, false);
	digitalWrite(light1Pin, LOW);
	
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
    
    //PURGE ROUTINE Entrance
    //====================================================================
    while (!digitalRead(button1Pin) == LOW && switchDoorState == HIGH)
    {
      inPurgeLoop = true;
      lcd.setCursor (0, 2); lcd.print (F("Purging...          "));
      digitalWrite(light1Pin, HIGH); 
      relayOn(relay2Pin, true); //open S2 to purge
    }
    //PURGE ROUTINE Exit
		if(inPurgeLoop)
    {
      relayOn(relay2Pin, false); //Close relay when B1 and B2 not pushed
      inPurgeLoop = false;
      digitalWrite(light1Pin, LOW); 
    }
 
    //MENU ROUTINE Entrance
    //====================================================================
    while (!digitalRead(button3Pin) == LOW)
    {
      inMenuLoop = true;
      lcd.setCursor (0, 2); lcd.print (F("Entering Menu...    ")); 
      buzzOnce(500, light3Pin);
    }
  }  
  //MULTIBUTTON LOCK LOOP Exit
	if (inMultiButtonLockLoop)
  {
    inMultiButtonLockLoop = false;
    buzzedOnce = false;
    
    //Enter menuLoop once B3 released
    if (inMenuLoop)
		{
      digitalWrite (light2Pin, LOW);
      menuShell(inMenuLoop);
    }  
    
    lcd.setCursor (0, 2); lcd.print (F("Wait...             ")); 
    //delay(750);  //This is to prevent nullPressure loop from kicking in
    digitalWrite (light2Pin, LOW);
  }
  
  // DOOR OPEN EMERGENCY PRESSURE DUMP (v1.1) 
	// Can adjust this threshold by adding a value to pressureDeltaDown 
  //========================================================================
  if (platformStateUp == true && switchDoorState == HIGH && (P1 - offsetP1 >= pressureDeltaDown + 50)) 
  { 
    emergencyDepressurize();
  }  

  //  This routine takes action if pressure drops in idle loop.
  idleLoopPressureDrop(); //v1.1 Better to put this is pressurize loop?
  
  // =====================================================================================  
  // PLATFORM RAISING LOOP
  // =====================================================================================  

  platformUpLoop();

	//======================================================================================
	// PRESSURIZE LOOP
	//======================================================================================
    
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
      /* 
	    //This code works better than breakpoint. BP can't process messages fast enough
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
    
		
		//v1.1 Pressure test. If bottle hasn't reached pressureDeltaUp in 4 sec, something is wrong
		pressurizeDuration = millis() - pressurizeStartTime;
		if (pressurizeDuration > 4000 && (P1 - offsetP1 <= P2 - offsetP2 - pressureDeltaUp))
		{
			messageGasLow();
			messageLcdWaiting(); // MESSAGE: "Waiting...          "
		}		  
		else
		{
			messageB2B3Toggles();
			lcd.setCursor (0, 2); lcd.print (F("Pressurizing...     "));
		}

    // Pressure output
    pressureOutput();
    printLcd2(3, outputPSI_b, throttleVal); 
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
      lcd.setCursor (0, 0); lcd.print (F("No bottle, or leak. "));
      lcd.setCursor (0, 1); lcd.print (F("Wait...             "));
      digitalWrite(light1Pin, LOW); 
      
			pressureDump();							// Dump any pressure that built up
      doorOpen();									// Open door
			
			while (!digitalRead(button3Pin) == HIGH)
			{
				lcd.setCursor (0, 1); lcd.print (F("Press B3 to continue"));
			}
			
			//Working on this for v1.1	
			//delay(2000);								// Give time to degas
			platformDrop();								// Drop platform
			//relayOn(relay3Pin, false);  // Now close S3
      
      //Reset variables
      PTestFail = false;      
      timePlatformRising = 0;
      platformStateUp = false;      
    }
  }

  // END PRESSURIZE LOOP
  //========================================================================================
  
  //========================================================================================
  // FILL LOOP
  //========================================================================================

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
    if (button2StateTEMP == HIGH && button2ToggleState == false){  //ON release
      button2ToggleState = true;                                   //Leaves buttonState LOW
      button2State = LOW;                                          //Or makes it low
    }
    if (button2StateTEMP == LOW && button2ToggleState == true){    //OFF push
      button2State = HIGH;                                         //exit WHILE loop
    }
		
    //Read and output pressure
		pressureOutput();
    printLcd2 (3, outputPSI_rb, throttleVal); 
    
    // CLEANING MODE: If in Cleaning Mode, set FillState HIGH to disable sensor
    if (inCleaningMode == true)
    {
      sensorFillState = HIGH;      
      lcd.setCursor (0, 2); lcd.print (F("Filling--SENSOR OFF "));
    }
    else
    {
      //Read sensors
      sensorFillState = digitalRead(sensorFillPin); //Check fill sensor
      switchDoorState = digitalRead(switchDoorPin); //Check door switch 
      lcd.setCursor (0, 2); lcd.print (F("Filling...          "));
    }
		lcd.setCursor (0, 0); lcd.print (F("B2 toggles filling; "));
		lcd.setCursor (0, 1); lcd.print (F("                    "));
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
      // Anti-drip routine
      relayOn(relay1Pin, true);
      relayOn(relay2Pin, true);
      
      delay(antiDripDuration); 

      relayOn(relay1Pin, false);
      relayOn(relay2Pin, false);
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
      messageLcdBlankLn2(); // MESSAGE: "                    "
      
      //v1.1 Clear Sensor routine under development 
      if (digitalRead(sensorFillPin) == LOW)
      {
        lcd.setCursor (0, 2); lcd.print (F("Clearing Fill Sensor"));
        relayOn(relay3Pin, true); delay (1000); relayOn(relay3Pin, false);
        relayOn(relay2Pin, true); delay (250); relayOn(relay2Pin, false);
      }  
      sensorFillState = HIGH; // v1.1 Changed it back to this
      button3State = LOW; // This make AUTO-depressurize after overfill
    }
    else 
    {
      // CASE 3: Bottle pressure dropped below Deltatamp threshold; i.e., filling too fast
      // FILLING TOO FAST LOOP    
      while (platformStateUp == true && switchDoorState == LOW && (P1 - offsetP1) <= (P2 - offsetP2) - pressureDeltaUp)
      {
        inPressurizeLoop = true;
        digitalWrite(light2Pin, HIGH); 
        relayOn(relay2Pin, true);        //open S2 to equalize
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
        buzzOnce(1000, light2Pin);
        
				//This is a little trap for B2 button to demand user input; doesn't change button states. Just wants user input.
        while(!digitalRead(button2Pin) == HIGH)
				{
          digitalWrite(light2Pin, HIGH);
          delay (150);
          digitalWrite(light2Pin, LOW);
          delay (350);
        }
        buzzedOnce = false;
				button2ToggleState = false; // button2State is still LOW, with toggleState true. Setting toggleState to false should be like pressing Fill again
        printLcd(2,"");
        inPressurizeLoop = false; 
      } 
      // END FILLING TOO FAST LOOP   
      //=============================================     
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
  
  while(button3State == LOW && (sensorFillState == HIGH || !digitalRead(button1Pin) == LOW || inCleaningMode == true) && switchDoorState == LOW && (P1 - offsetP1 >= pressureDeltaDown)) //v1.1 added sensor override
  {  
    inDepressurizeLoop = true;
    relayOn(relay3Pin, true); //Open Gas Out solenoid
		digitalWrite(light3Pin, HIGH);
	
	//v1.1 added as part of sensor override
	if (!digitalRead(button1Pin) == LOW)
	{
		digitalWrite(light1Pin, HIGH);
	}
	else 
	{
		digitalWrite(light1Pin, LOW);
	}
  
	// Pressure output
	pressureOutput();
	printLcd2 (3, outputPSI_b, throttleVal);
  
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
    lcd.setCursor (0, 2); lcd.print (F("Venting--SENSOR OFF"));
  }
  else
  {
    sensorFillState = digitalRead(sensorFillPin); //Check fill sensor
    switchDoorState = digitalRead(switchDoorPin); //Check door switch // Not using this
    lcd.setCursor (0, 2); lcd.print (F("Depressurizing...   "));
  }   
	lcd.setCursor (0, 0); lcd.print (F("B3 toggles venting; "));

	messageRotator(10000, .5, 0);
	if (messageID){
		lcd.setCursor (0, 1); lcd.print (F("B2: Burst tamping   "));
	}
	else{
		lcd.setCursor (0, 1); lcd.print (F("B1: Overrides sensor"));
	}
		
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
    
		// Turn off Light 1 if on
		digitalWrite(light1Pin, LOW);
    
		// CASE 1: Button3 released
    if (button3State == HIGH)  
    { 
      relayOn(relay3Pin, false);     //Used to repressurize here; now we don't  
			lcd.setCursor (0, 1); lcd.print (F("B2 toggles filling. ")); //Overwrites second line on exit
    }
    
    // CASE 2: Foam tripped sensor
    if (sensorFillState == LOW && !digitalRead(button1Pin) == HIGH) // The second condition is part of fillSensor override
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
      }
      lcd.setCursor (0, 2); lcd.print (F("                    "));
      sensorFillState = HIGH; //v1.1 Set the sensorState to HIGH
    }

    //CASE 3: Bottle was properly depressurized. If we reach here, the pressure must have reached threshold. Go to Platform lower loop. Note S3 still open
    if (P1 - offsetP1 <= pressureDeltaDown)
    {
      buzzer(500);
      autoMode_1 = true;  //Going to platform loop automatically, so set this var to partially drop platform 
	  	lcd.setCursor (0, 0); lcd.print (F("Grasp bottle;       "));
	  	lcd.setCursor (0, 1); lcd.print (F("B3 lowers platform. "));
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
  
  // v1.1 This is a catch routine to remind the user to open the exhaust if there is foam present in the neck of bottle
  if (digitalRead(sensorFillPin) == LOW && switchDoorState == HIGH && platformStateUp == true && inFillLoopExecuted == true)
  {
		lcd.setCursor (0, 0); lcd.print (F("Open Exhaust valve  "));
		lcd.setCursor (0, 1); lcd.print (F("to prevent spray;   "));
		lcd.setCursor (0, 2); lcd.print (F("press B3 to resume. "));
	
		while(!digitalRead(button3Pin) == HIGH)
		{
			pressureOutput();
			printLcd2(3, outputPSI_b, throttleVal);
			digitalWrite(light3Pin, HIGH);
			buzzer (75);
			digitalWrite(light3Pin, LOW);
			delay (350);
		}
  }

	// v1.1 Removed "|| autoMode_1 == true and  && (digitalRead(sensorFillPin) == HIGH)" so platform no longer drops automatically
  while((!digitalRead(button3Pin) == LOW) && switchDoorState == HIGH && (P1 - offsetP1) <= pressureDeltaDown) 
  {
    inPlatformLowerLoop = true;
    digitalWrite(light3Pin, HIGH);
		relayOn(relay4Pin, false); // Finally can close platform up solenoid
    lcd.setCursor (0, 2); lcd.print (F("Lowering platform..."));
		lcd.setCursor (0, 3); lcd.print (F("Regulator...        "));
    
    if(autoMode_1 == true)
    {
      digitalWrite(light3Pin, HIGH);
			relayOn(relay5Pin, true);
      delay(autoPlatformDropDuration);  
      relayOn(relay5Pin, false);
      button3State = HIGH;
      autoMode_1 = false;
    }
    else
    {
			//relayOn(relay3Pin, true);  // May as well leave this open? YES. Liquid is still outgassing.//TO DO: Don't need this here?? it's already open?
			digitalWrite(light3Pin, HIGH); 
			relayOn(relay5Pin, true);  // Open cylinder exhaust
    }
    P1 = analogRead(sensorP1Pin);
    button3State = !digitalRead(button3Pin);
    switchDoorState =  digitalRead(switchDoorPin);
    sensorFillState =  digitalRead(sensorFillPin);
		digitalWrite(light3Pin, LOW);
  }

  // PLATFORM LOWER LOOP EXIT ROUTINES
  // ===========================================================================================
  
  if(inPlatformLowerLoop)
  {
	//close platform release 
    messageLcdBlankLn2(); // MESSAGE: "                    "
    relayOn(relay3Pin, false); 
    relayOn(relay5Pin, false);
    relayOn(relay6Pin, false); //Release door solenoid
    digitalWrite(light3Pin, LOW); 

    inPlatformLowerLoop = false;   
    platformStateUp = false;

    //Prepare for next cycle
		messageInsertBottle();

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
      delay(1500);
    
      inFillLoopExecuted = false;
    }      
  } 
  
  // END PLAFORM LOWER LOOP
  //============================================================================================
}

//=======================================================================================================
// END MAIN LOOP ========================================================================================
//=======================================================================================================
     
