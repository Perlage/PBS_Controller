/*
//===========================================================================

FIZZIQ Cocktail Bottling System

Author:
	Evan Wallace
	Perlage Systems, Inc.
	Seattle, WA 98101 USA

Copyright 2013-2016  All rights reserved
Authored using Visual Studio Community after Apr 23, 2016

//===========================================================================
*/


//Version control variable
String(versionSoftwareTag) = "v1.4.0";		//Changed to 2-digit numbering system so fits on screen. 1.3 = 1.2.2. Changed back to three.

//Library includes
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "floatToString.h"               
#include <EEPROM.h>
//#include <avr/pgmspace.h>					// 1-26 Added for PROGMEM function // UNUSED now
//#include <math.h>							// Unused

LiquidCrystal_I2C lcd(0x27, 20, 4);		//This seems to work for new screens after Dec 12, 2014
//LiquidCrystal_I2C lcd(0x3F, 20, 4);			//This worked for original screens up until Dec, 2014 //This works again for SN033, May 2016

// Pin assignments
const int button1Pin		= 0;      // pin for button1 B1 (Raise platform) RX=0;
const int button2Pin		= 1;      // pin for button2 B2 (Fill/purge)     TX=1; 
// 2 SDA for LCD			= 2;  
// 3 SDL for LCD			= 3;
const int button3Pin		= 4;      // pin for button3 B3 (Depressurize and lower) 
const int relay1Pin			= 5;      // pin for relay1 S1 (liquid fill)
const int relay2Pin			= 6;      // pin for relay2 S2 (bottle gas inlet)
const int relay3Pin			= 7;      // pin for relay3 S3 (bottle gas vent)
const int relay4Pin			= 8;      // pin for relay4 S4 (pneumatic lift gas in)
const int relay5Pin			= 9;      // pin for relay5 S5 (pneumatic lift gas out)
const int relay6Pin			= A5;     // pin for relay6 S6 (door lock solenoid) //v2 switched from 10 to A5 for PWM
const int sensorFillPin		= 11;     // pin for fill sensor F1 // DO NOT PUT THIS ON PIN 13!!
const int switchDoorPin		= 12;     // pin for door switch

const int buzzerPin			= 13;    // pin for buzzer
const int relay8Pin			= A3;     // V2 pin for relay 8 (DEPRESSURIZE KEG)

const int sensorP2Pin		= A0;     // pin for pressure sensor 2 // REGULATOR PRESSURE--BOTTOM sensor on PCB
const int sensorP1Pin		= A1;     // pin for pressure sensor 1 // BOTTLE PRESSURE--TOP sensor on PCB

//const int switchModePin		= 101;    // V2 assign to nonexistent pin
const int relay7Pin			= A2;     // V2 REPLACEMENT OF SECONDARY REG

const int light1Pin			= 100;     // pin for button1 light //V2 assign to nonexistent pin
const int light2Pin			= A4;     // pin for button2 light
const int light3Pin			= 101;     // pin for button3 light//V2 assign to nonexistent pin
const int propSolPin		= 10;		//V2 pin for proportional solenoid//v2 switched from 10 to A5 for PWM

// A0 formerly was sensor1, which is closest to what we now call bottle pressure, and A1 was unused. 
// So switched sensor1 to A1 to make code changes easier. All reads of sensor1 will now read bottle pressure

// Declare variables and initialize states
// ===========================================================================  

//Component states //FEB 1: Changed from int to boolean. 300 byte drop in program size
boolean button1State = HIGH;
boolean button2State = HIGH;
boolean button3State = HIGH;
boolean light1State = LOW;
boolean light2State = LOW;
boolean light3State = LOW;
boolean relay1State = HIGH;
boolean relay2State = HIGH;
boolean relay3State = HIGH;
boolean relay4State = HIGH;
boolean relay5State = HIGH;
boolean relay6State = HIGH;
boolean relay7State = HIGH; //v2
boolean relay8State = HIGH; //v2
boolean sensorFillState = HIGH;
boolean sensorFillStateTEMP = HIGH;
boolean switchDoorState = HIGH;
//boolean switchModeState = HIGH; //LOW is Manual, HIGH (or up) is auto (normal)

//State variables 
boolean inPressureNullLoop = false;
boolean inPlatformUpLoop = false;
boolean inPurgeLoop = false;
boolean inPressurizeLoop = false;
boolean inFillLoop = false;
boolean inOverFillLoop = false;
boolean inDepressurizeLoop = false;
boolean inPlatformLowerLoop = false;
boolean inDoorOpenLoop = false;
boolean inPressureSaggedLoop = false;
boolean inMenuLoop = false;
boolean inManualModeLoop = false;
boolean inManualModeLoop1 = false;
boolean inCleaningMode = false;
boolean inPressureDropLoop = false;
boolean inPlatformEmergencyLock = false;
boolean inDiagnosticMode = false;

//Key logical states
boolean autoMode_1 = false;						// Variable to help differentiate between states reached automatically vs manually 
boolean platformLockedNew = false;				// Variable to be set when platform first locks up. This is for autofill-on-door close function
boolean platformStateUp = false;				// true means platform locked in UP; toggled false when S5 opens
boolean depressurizeLoopExecuted = false;		// v1.1 Let's use this to differentiate whether we got to zero pressure via depressurization vs. never being pressurized

//Pressure constants                            // NOTE: 1 psi = 12.71 units. 
int offsetP1;                                   // Zero offset for pressure sensor1 (bottle). Set through EEPROM in initial factory calibration, and read into pressureOffset during Setup loop
int offsetP2;                                   // Zero offset for pressure sensor2 (input regulator). Ditto above.
const int pressureDeltaUp = 50;					// Pressure at which, during pressurization, full pressure is considered to have been reached // Tried 10, 38; went back to 50 to prevent repressurizing after fill button cycle
const int pressureDeltaDown = 38;				// Pressure at which, during depressurizing, pressure considered to be close enough to zero // 38 works out to 3.0 psi 
const int pressureDeltaMax = 250;				// FILLING TOO FAST criterion //v1.1 Now can reduce this significantly from 250 since have two pressure sensors. 100 is good.
const int pressureNull = 200;					// This is the threshold for the controller deciding that no gas source is attached. 
const int pressureDropAllowed = 100;			// Max pressure drop allowed in session before alarm sounds
int pressureRegStartUp;                         // Starting regulator pressure. Will use to detect pressure sag during session; and to find proportional values for key pressure variables (e.g. pressureDeltaMax)

//Pressure conversion and output variables
int P1;                                         // Current pressure reading from sensor1 (BOTTLE)
int P2;                                         // Current pressure reading from sensor2 (REGULATOR/KEG)
float PSI1;
float PSI2;
float PSIdiff;
String(convPSI1);
String(convPSI2);
String(convPSIdiff);
String(outputPSI_rb);                          // Keg, bottle
String(outputPSI_b);                           // Bottle
String(outputPSI_r);                           // Keg
String(outputPSI_d);                           // Diff between bottle and keg

//Variables for platform function and timing
int timePlatformInit;                           // Time in ms going into loop
int timePlatformCurrent;                        // Time in ms currently in loop
int timePlatformRising = 0;						// Time difference between Init and Current
const int timePlatformLock = 1000;				// Time in ms before platform locks in up position
const int autoPlatformDropDuration = 500;		// Duration of platform auto drop in ms

//Key performance parameters
int autoSiphonDuration;                         // Duration of autosiphon function in ms
byte autoSiphonDuration10s;                     // Duration of autosiphon function in 10ths of sec
float autoSiphonDurationSec;                    // Duration of autosiphon function in sec
const int antiDripDuration = 500;				// Duration of anti-drip autosiphon
const int foamDetectionDelay = 2000;			// Amount of time to pause after foam detection
//const int pausePlatformDrop = 1000;				// Pause before platform drops after door automatically opens

//Variables for button toggle states 
boolean button2StateTEMP = HIGH;
boolean button3StateTEMP = HIGH;
boolean button2ToggleState = false;
boolean button3ToggleState = false;

//System variables      
int numberCycles;                                // Number of cycles since factory reset, measured by number of platform PLUS fill loop being executed
byte numberCycles01;                             // Ones digit for numberCycles in EEPROM in base 255
byte numberCycles10;                             // Tens digit for numberCycles in EEPROM in base 255
int numberCyclesSession = 0;					 // Number of session cycles
boolean buzzedOnce = false;
boolean inFillLoopExecuted = false;				 // True of FillLoop is dirty. Used to compute numberCycles
char buffer[25];                                 // Used in float to string conv // 1-26 Changed from 25 to 20 //CHANGED BACK TO 25!! SEEMS TO BE IMPORTANT!

//======================================================================================
// PROCESS LOCAL INCLUDES
//======================================================================================

// Order is important!!!! Must include an include before it is referenced by other includes
#include "functions.h"					//Functions
#include "lcdMessages.h"				//User messages
#include "loops.h"						//Major reused loops
#include "manualMode.h"					//Manual Mode function
#include "menuShell.h"					//Menu shell
#include "nullPressureCheck.h"			//Null Pressure routines

//=====================================================================================
// SETUP LOOP
//=====================================================================================

void setup()
{
	buzzer(100); delay(50); buzzer(100); delay(50); buzzer(100); //Just to give some feedback asap

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
	//pinMode(sensorFillPin, INPUT_PULLUP);  //INPUT_PULLUP uses internal Pullup and maybe additionally a larger (~65k) external pullup resistor
	pinMode(sensorFillPin, INPUT);           //INPUT and external pullup resistor
	pinMode(switchDoorPin, INPUT_PULLUP);
	//pinMode(switchModePin, INPUT_PULLUP);
	pinMode(buzzerPin, OUTPUT);
	pinMode(relay7Pin, OUTPUT); //V2 AUTOCARBONATION 2NDARY REG
	pinMode(relay8Pin, OUTPUT); //V2 AUTOCARBONATION
	pinMode(propSolPin, OUTPUT); //V2


	//set all relay pins to high which is "off" for this relay
	digitalWrite(relay1Pin, HIGH);
	digitalWrite(relay2Pin, HIGH);
	digitalWrite(relay3Pin, LOW);    //OPEN ASAP
	digitalWrite(relay4Pin, HIGH);
	digitalWrite(relay5Pin, HIGH);
	digitalWrite(relay6Pin, HIGH);
	digitalWrite(relay7Pin, HIGH); //v2 (Regulator)
	digitalWrite(relay8Pin, HIGH); //v2

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
	//buzzer(100); delay(50); buzzer(100); delay(50); buzzer(100);
	messageInitial();

	// EEPROM factory set-reset routine
	// For some reason, have to include it here and not above with others

	//#include "EEPROMset.h" //Commenting this out removes the ability to set EEPROM with production firmware. Must do in QA/Setup firmware

	// Read EEPROM
	offsetP1 = EEPROM.read(0);
	offsetP2 = EEPROM.read(1);
	autoSiphonDuration10s = EEPROM.read(3);
	numberCycles01 = EEPROM.read(4);  //Write "ones" digit base 255    
	numberCycles10 = EEPROM.read(5);  //Write "tens" digit base 255
	platformStateUp = EEPROM.read(6);  //Current valve for platformStateUp

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
	while (!digitalRead(button1Pin) == LOW)
	{
		inMenuLoop = true;
		lcd.clear();
		lcd.setCursor(0, 0); lcd.print(F("Entering Menu...    "));
		buzzOnce(1000, light1Pin);
	}
	if (inMenuLoop == true)
	{
		menuShell(inMenuLoop);
		buzzedOnce = false;
	}

	//NULL PRESSURE: Check for stuck pressurized bottle
	pressurizedBottleStartup();
	digitalWrite(relay3Pin, HIGH); //v2 Now close

	//LOW PRESSURE: THEN check for implausibly low gas pressure at start 
	//nullPressureStartup(); //v2 COMMENTED OUT FOR AUTOCARBONATION

	// v1.1 Only open door if platform state is UP (because that means there could be a bottle there)
	if (platformStateUp)
	{
		doorOpen();
		platformStateUp = false;
		EEPROM.write(6, platformStateUp);
	}

	//Rewrite initial user message, in case pressure routines above wrote to screen
	messageInitial();
	
	//UNCOMMENT "/*" FOR RELEASE==============
	/*
	//Traveling dots
	for (int n = 12; n < 20; n++)
	{
		lcd.setCursor(n, 3); lcd.print(F("."));
		delay(200);
	}

	//UNCOMMENT OUT NEXT TWO LINES FOR SHOW
	//relayOn(relay4Pin, true);  // Now Raise platform
	//delay (1000);     

	//Leave blink loop and light all lights 
	digitalWrite(light1Pin, HIGH); delay(400);
	digitalWrite(light2Pin, HIGH); delay(200);
	digitalWrite(light3Pin, HIGH);

	// Drop platform in case it is up, which had been raised before nullpressure checks
	relayOn(relay4Pin, false);
	relayOn(relay5Pin, true);

	String convInt = floatToString(buffer, numberCycles, 0);
	String outputInt = "Total fills: " + convInt;
	printLcd(3, outputInt);
	delay(2000);  // Time to let platform drop
	relayOn(relay5Pin, false);
	relayOn(relay3Pin, false); // Close this so solenoid doesn't overhead on normal startup

	digitalWrite(light3Pin, LOW);
	delay(200); digitalWrite(light2Pin, LOW);
	delay(100); digitalWrite(light1Pin, LOW);
	buzzer(1000);
	*/
	//UNCOMMENT "*/" FOR RELEASE=============
}

//====================================================================================================================================
// MAIN LOOP =========================================================================================================================
//====================================================================================================================================

void loop()
{
	int pressureReg = 40 * 12.71;
	int pressureRegDelta = 25;
	boolean inPressurizeKegLoop = false;

	while(P2 - offsetP2 <= pressureReg)
	{
		inPressurizeKegLoop = true;

		relayOn(relay7Pin, true);
		pressureOutput();
		printLcd(3, outputPSI_rb);
	}
	if (inPressurizeKegLoop)
	{
		relayOn(relay7Pin, false);
		inPressurizeKegLoop = false;
	}


	
	//MAIN LOOP IDLE FUNCTIONS
	//=====================================================================

	// Read the state of buttons and sensors
	// Boolean NOT (!) added for touchbuttons so that HIGH button state (i.e., button pressed) reads as LOW (this was easiest way to change software from physical buttons, where pressed = LOW) 
	button1State        = !digitalRead(button1Pin);
	button2StateTEMP    = !digitalRead(button2Pin);
	button3StateTEMP    = !digitalRead(button3Pin);
	switchDoorState     =  digitalRead(switchDoorPin);
	//switchModeState     =  digitalRead(switchModePin);
	sensorFillStateTEMP =  digitalRead(sensorFillPin); //TODO Maybe we don't need to measure this. Old variable??

	sensorFillState = HIGH; //v1.1: We now SET the sensorState HIGH.

	//Check Button2 toggle state
	//======================================================================

	if (button2StateTEMP == LOW && button2ToggleState == false) {	//ON push
		button2State = LOW;											//goto while loop
	}
	if (button2StateTEMP == LOW && button2ToggleState == true) {	//button still being held down after OFF push in while loop. 
		button2State = HIGH;										//nothing happens--buttonState remains HIGH
	}
	if (button2StateTEMP == HIGH && button2ToggleState == true) {	//OFF release
		button2ToggleState = false;									//buttonState remains HIGH
	}

	//Check Button3 toggle state
	//======================================================================
	if (button3StateTEMP == LOW && button3ToggleState == false) {	//ON push
		button3State = LOW;											//goto while loop
	}
	if (button3StateTEMP == LOW && button3ToggleState == true) {	//button still being held down after OFF push in while loop. 
		button3State = HIGH;										//nothing happens--buttonState remains HIGH
	}
	if (button3StateTEMP == HIGH && button3ToggleState == true) {	//OFF release
		button3ToggleState = false;									//buttonState remains HIGH
	}

	// Give relevant instructions in null loop
	//======================================================================

	// The third case, platform up and door open, is written to screen and end of plaformUP routine
	if (switchDoorState == LOW && platformStateUp == false)
	{
		lcd.setCursor(0, 0); lcd.print(F("B3 opens door       "));
		lcd.setCursor(0, 1); lcd.print(F("B2+B3 opens Menu    "));
		if (inCleaningMode == true)
		{
			messageCleanMode(2);	//PBS-FIRM 145
		}
		else
		{
			messageLcdReady(2);
		}
	}
	// MessageInsertBottle
	if (switchDoorState == HIGH && platformStateUp == false)
	{
		messageInsertBottle();
	}

	// Added last condition to differentiate between depressurized vs never pressurized, so this message doesn't get shown in a hard re-foam situation
	if (P1 - offsetP1 > pressureDeltaDown && platformStateUp == true && depressurizeLoopExecuted == false)
	{
		messageB2B3Toggles();
		lcd.setCursor(0, 2); lcd.print(F("B1 adjusts overfill "));
	}
	//v1.1 #97: this is to handle pressure buildup if user waits too long to drop platform, and valve is closed
	if (platformStateUp == true && (P1 - offsetP1) > pressureDeltaDown && depressurizeLoopExecuted == true)
	{
		buzzer(25);
		lcd.setCursor(0, 0); lcd.print(F("Open exhaust valve  "));
		lcd.setCursor(0, 1); lcd.print(F("to relieve pressure."));
		messageLcdWaiting();
		analogWrite(propSolPin, 255);
	}
	//v1.1 #97: This write the appropriate message if platform is up and user hasn't lowered platform yet
	//v1.1 #105 Swapped the hard foam catch loop for proper messages
	if (platformStateUp == true && (P1 - offsetP1) <= pressureDeltaDown && depressurizeLoopExecuted == true)
	{
		if (digitalRead(sensorFillPin) == LOW)
		{
			lcd.setCursor(0, 0); lcd.print(F("Foam detected! Open "));
			lcd.setCursor(0, 1); lcd.print(F("Exhaust valve before"));
			lcd.setCursor(0, 2); lcd.print(F("lowering platform..."));
			digitalWrite(light3Pin, LOW);
			delay(50);
			digitalWrite(light3Pin, HIGH);
			analogWrite(propSolPin, 255);
		}
		else
		{
			lcd.setCursor(0, 0); lcd.print(F("Grasp bottle;       "));
			lcd.setCursor(0, 1); lcd.print(F("B3 lowers platform. "));
			messageLcdWaiting();
			analogWrite(propSolPin, 0);
		}
	}

	// Main Loop idle pressure measurement and LCD output
	//======================================================================
	pressureOutput();
	if (P1 - offsetP1 > pressureDeltaDown || platformStateUp == true)
	{
		printLcd(3, outputPSI_rb);
	}
	else
	{
		printLcd(3, outputPSI_rb);
	}

	//=====================================================================
	// Autosiphon in Main loop
	//=====================================================================
	while (!digitalRead(button1Pin) == LOW && (P1 - offsetP1 > pressureDeltaDown) && platformStateUp == true)
	{
		relayOn(relay1Pin, true);
		relayOn(relay2Pin, true);
		digitalWrite(light1Pin, HIGH);
		lcd.setCursor(0, 2); lcd.print(F("Adjusting level...  "));
	}
	relayOn(relay1Pin, false);
	relayOn(relay2Pin, false);
	digitalWrite(light1Pin, LOW);

	//======================================================================
	// MULTI BUTTON COMBO ROUTINES
	//======================================================================

	boolean inMenuLoop = false;
	boolean inPurgeLoop = false;
	boolean inMultiButtonLockLoop = false;

	while (!digitalRead(button2Pin) == LOW && platformStateUp == false)
	{
		inMultiButtonLockLoop = true;
		digitalWrite(light2Pin, HIGH);

		while (!digitalRead(button2Pin) == LOW)
		{
			relayOn(relay8Pin, true);
		}
		relayOn(relay8Pin, false);

		//PURGE ROUTINE Entrance
		//====================================================================
		while (!digitalRead(button1Pin) == LOW && switchDoorState == HIGH)
		{
			inPurgeLoop = true;
			lcd.setCursor(0, 2); lcd.print(F("Purging...          "));
			digitalWrite(light1Pin, HIGH);
			relayOn(relay2Pin, true); //open S2 to purge
		}
		//PURGE ROUTINE Exit
		if (inPurgeLoop)
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
			lcd.setCursor(0, 2); lcd.print(F("Entering Menu...    "));
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
			digitalWrite(light2Pin, LOW);
			menuShell(inMenuLoop);
		}

		lcd.setCursor(0, 2); lcd.print(F("Wait...             "));
		digitalWrite(light2Pin, LOW);
	}

	// DOOR OPEN EMERGENCY PRESSURE DUMP (v1.1) 
	//========================================================================

	if (depressurizeLoopExecuted == false) //Don't want this kicking in if we've already reached zero pressure and foaming puts you back over threshold
	{
		emergencyDepressurize();
	}

	// This routine takes action if pressure drops in idle loop. //This is probably not needed now that Pressurize loop has a pressure check
	// idleLoopPressureDrop();

	// =====================================================================================  
	// PLATFORM RAISING LOOP
	// =====================================================================================  

	platformUpLoop();

	//======================================================================================
	// PRESSURIZE LOOP
	//======================================================================================

	int PTest1 = 0;
	int PTest2 = 0;
	int pressurizeStartTime = 0;
	int pressurizeDuration = 0;
	boolean PTestFail = false;

	// Get fresh pressure readings  
	P1 = analogRead(sensorP1Pin);
	P2 = analogRead(sensorP2Pin);

	while ((button2State == LOW || platformLockedNew == true) && switchDoorState == LOW && platformStateUp == true && (P1 - offsetP1 <= P2 - offsetP2 - pressureDeltaUp))
	{
		inPressurizeLoop = true;

		// Only want to run this once per platformUp--not on subsequent repressurizations within a cycle 
		if (platformLockedNew == true){
			pressurizeStartTime = millis();}						//startTime for no bottle test below.
		

		//Open gas-in relay, close gas-out
		relayOn(relay3Pin, false);									//close S3 if not already 
		relayOn(relay2Pin, true);									//open S2 to pressurize
		digitalWrite(light2Pin, HIGH);

		//NO BOTTLE TEST: Check to see if bottle is pressurizing; if PTest not falling, must be no bottle 
		//while (platformLockedNew == true)							//First time through loop with platform up 
		//{
		//	pressurizeDuration = millis() - pressurizeStartTime;	//Get the duration
		//	if (pressurizeDuration < 50) {
		//		PTest1 = analogRead(sensorP1Pin);					//Take a reading at 50ms after pressurization begins. 50 ms gives time for pressure to settle down after S2 opens
		//	}
		//	if (pressurizeDuration < 250) {
		//		PTest2 = analogRead(sensorP1Pin);					//Take a reading at 100ms after pressurization begins
		//	}
		//	if (pressurizeDuration > 250) {							//After 100ms, test
		//		if (PTest2 - PTest1 < 20) {							//If there is less than a 15 unit difference, must be no bottle in place
		//			button2State = HIGH;
		//			relayOn(relay2Pin, false);						//close S2 immediately
		//			button2ToggleState = true;						//Need this to keep toggle routine below from changing button2state back to LOW 
		//			PTestFail = true;								//If true, no bottle. EXIT 
		//		}
		//		platformLockedNew = false;
		//	}
		//	/*
		//	//This code works better than breakpoint. BP can't process messages fast enough
		//	Serial.print ("T= ");
		//	Serial.print (pressurizeDuration);
		//	Serial.print (" P1= ");
		//	Serial.print (PTest1);
		//	Serial.print (" P2= ");
		//	Serial.print (PTest2);
		//	Serial.println ();
		//	*/
		//}

		//Read sensors
		switchDoorState = digitalRead(switchDoorPin);					//Check door switch    
		P1 = analogRead(sensorP1Pin);									// Don't really even need to read P2; read it going into loop?

		//Check toggle state of B2
		button2StateTEMP = !digitalRead(button2Pin);

		if (button2StateTEMP == HIGH && button2ToggleState == false) {  //ON release
			button2ToggleState = true;                                  //leave button state LOW
			button2State = LOW;                                         //or make it low if it isn't yet
		}
		if (button2StateTEMP == LOW && button2ToggleState == true) {    //OFF push
			button2State = HIGH;                                        //exit WHILE loop
		}

		//v1.1 Pressure test. If bottle hasn't reached pressureDeltaUp in 4 sec, something is wrong
		pressurizeDuration = millis() - pressurizeStartTime;
		if (pressurizeDuration > 4000 && (P1 - offsetP1 <= P2 - offsetP2 - pressureDeltaUp))
		{
			messageGasLow();
			messageLcdWaiting();
		}
		else
		{
			messageB2B3Toggles();
			lcd.setCursor(0, 2); lcd.print(F("Pressurizing...     "));
		}

		// Pressure output
		pressureOutput();
		printLcd(3, outputPSI_d);
	}

	// PRESSURIZE LOOP EXIT ROUTINES
	//=======================================================================================

	if (inPressurizeLoop)
	{
		inPressurizeLoop = false;

		relayOn(relay2Pin, false);       // close S2 because we left PressureLoop
		digitalWrite(light2Pin, LOW);    // Turn off B2 light

		//PRESSURE TEST EXIT ROUTINE
		if (PTestFail == true)
		{
			lcd.setCursor(0, 0); lcd.print(F("No bottle, or leak. "));
			lcd.setCursor(0, 1); lcd.print(F("Wait...             "));
			digitalWrite(light1Pin, LOW);

			pressureDump();							// Dump any pressure that built up
			doorOpen();								// Open door

			while (!digitalRead(button3Pin) == HIGH)
			{
				lcd.setCursor(0, 1); lcd.print(F("Press B3 to continue"));
			}
			platformDrop();								// Drop platform

			//Reset variables
			PTestFail = false;
			timePlatformRising = 0;
			platformStateUp = false;
			EEPROM.write(6, platformStateUp);
		}
	}

	// END PRESSURIZE LOOP
	//========================================================================================

	//========================================================================================
	// FILL LOOP
	//========================================================================================
	
	//V2 code
	int currentSol = 200; //V2 Goes from 0 (closed) to 255 (fully open)
	int currentSolIncr = 5; //Amount to increase/decrease the solenoid current
	float pressureDiffTarget = 5; //5 psi difference target
	float pressureDiffTargetDelta = .1;
	printLcd(2, "                    ");

	// v1.1 Changed the pressureDeltaMax condition to reflect fact that there are two sensors
	// v1.1 #96: Added button3 exit shortcut
	int startSolenoidCleaningCycle = millis(); // Get the start time for the solenoid cleaning cycle
											   
	// PBSFIRM-135: Last pressure condition in while statement absolutely ensures that there can be no filling without bottle, even in cleaning mode
	while (button2State == LOW && (sensorFillState == HIGH || inCleaningMode == true) && switchDoorState == LOW && (((P2 - offsetP2) - (P1 - offsetP1) < pressureDeltaMax) || inCleaningMode == true) && (P1 - offsetP1 >= pressureDeltaDown))
	{
		inFillLoop = true;
		inFillLoopExecuted = true; //This is an "is dirty" variable for counting lifetime bottles. Reset in platformUpLoop.

		//while the button is  pressed, fill the bottle: open S1 and S3
		relayOn(relay1Pin, true);
		relayOn(relay3Pin, true);
		digitalWrite(light2Pin, HIGH);

		//V2 prop sol code
		
		if (!digitalRead(button1Pin) == LOW)
		//if (PSIdiff > pressureDiffTarget + pressureDiffTargetDelta)
		{
			currentSol = currentSol + currentSolIncr;
		}
		if (!digitalRead(button3Pin) == LOW)
		//if (PSIdiff < pressureDiffTarget - pressureDiffTargetDelta)
		{
			currentSol = currentSol - currentSolIncr;
		}

		analogWrite(propSolPin, currentSol); //V2
		lcd.setCursor(0, 2);
		lcd.print(currentSol);

		/*
		int N = 1;
		if (currentSol < 255)
		{
			analogWrite(propSolPin, currentSol);
			delay(500);
			currentSol = currentSol + N;
			pressureOutput();
			printLcd(3, outputPSI_d); //V2 This is better for checking proportional solenoid
			lcd.setCursor(0, 2);
			lcd.print(currentSol);
		}
		*/

		//Check toggle state of B2    
		button2StateTEMP = !digitalRead(button2Pin);
		if (button2StateTEMP == HIGH && button2ToggleState == false) {   //ON release
			button2ToggleState = true;                                   //Leaves buttonState LOW
			button2State = LOW;                                          //Or makes it low
		}
		if (button2StateTEMP == LOW && button2ToggleState == true) {     //OFF push
			button2State = HIGH;                                         //exit WHILE loop
		}

		//v1.1 Added shortcut to leave fill loop.
		//button3State = !digitalRead(button3Pin); //v2 comment out for now

		//Read and output pressure
		pressureOutput();
		//printLcd(3, outputPSI_rb); //V2 This is better for checking solenoid 7 and 8
		printLcd(3, outputPSI_d); //V2 This is better for checking proportional solenoid

		// CLEANING MODE: If in Cleaning Mode, set FillState HIGH to disable sensor
		if (inCleaningMode == true)
		{
			sensorFillState = HIGH;
			//lcd.setCursor(0, 2); lcd.print(F("Filling...Clean Mode")); //V2 temp

			// New cleaning routine to help prevent solenoid sticking
			// IF loop ensure that the fluttering only occurs in bursts, not continuously
			// In case of flutterCycle =  5000  and flutterDuration = 1000, fluttering would happen for 1 sec in every 5
			int flutterCycle = 5000;			// Solenoid cleaning cycle in millisec
			int flutterDuration = 1000;			// Duration that solenoid opens and closes during cycle. 
			if ((millis() - startSolenoidCleaningCycle) % flutterCycle > (flutterCycle - flutterDuration)) {
				relayOn(relay1Pin, false); delay(100);
				relayOn(relay1Pin, true);
				relayOn(relay3Pin, false); delay(100);
				relayOn(relay3Pin, true);
			}
			// End solenoid cleaning routine
		}
		else
		{
			//Read sensors
			sensorFillState = digitalRead(sensorFillPin); //Check fill sensor
			switchDoorState = digitalRead(switchDoorPin); //Check door switch 
			//lcd.setCursor(0, 2); lcd.print(F("Filling...          ")); V2
		}
		lcd.setCursor(0, 0); lcd.print(F("B2 toggles filling; "));
		lcd.setCursor(0, 1); lcd.print(F("Knob controls flow. "));
	}

	// FILL LOOP EXIT ROUTINES
	// Either 1) B2 was released, or 2) FillSwitch was tripped, or 
	// 3) pressure dipped too much from filling too fast. Repressurize in 2 and 3
	//===============================================================================================================

	if (inFillLoop)
	{
		digitalWrite(light2Pin, LOW);
		relayOn(relay1Pin, false);
		relayOn(relay3Pin, false);
		//messageLcdBlank(2); 

		// Check which condition caused filling to stop
		// CASE 1: Button2 pressed when filling (B2 low and toggle state true). Run Anti-Drip
		if ((button2State == HIGH && !inCleaningMode == true) || button3State == LOW) //#98
		{
			if (button3State == LOW)
			{
				digitalWrite(light3Pin, HIGH);
			}

			// Anti-drip routine
			relayOn(relay1Pin, true);
			relayOn(relay2Pin, true);
			lcd.setCursor(0, 2); lcd.print(F("Anti drip...        "));

			//delay(antiDripDuration);
			//delay(10);
			relayOn(relay1Pin, false);
			relayOn(relay2Pin, false);
			messageLcdBlank(2); //Need this! Do not remove
		}

		// CASE 2: FillSensor tripped--Overfill condition
		else if (inFillLoop && sensorFillState == LOW)
		{
			relayOn(relay1Pin, true);
			relayOn(relay2Pin, true);
			delay(250);
			relayOn(relay2Pin, false);
			//relayOn(relay8Pin, true);

			pressureOutput();			//V2 Added for Autocarbonation
			printLcd(3, outputPSI_rb);	//V2 Added for Autocarbonation

			lcd.setCursor(0, 2); lcd.print(F("Adjusting level...  "));
			delay(autoSiphonDuration); // This setting determines duration of autosiphon 
			relayOn(relay1Pin, false);
			//relayOn(relay2Pin, false);
			//relayOn(relay8Pin, false);

			//v1.1 Clear Sensor Routine
			if (digitalRead(sensorFillPin) == LOW)
			{
				lcd.setCursor(0, 2); lcd.print(F("Clearing Fill Sensor"));
				delay(1000); //DEBUG? Delay to show above message. 
			}
			sensorFillState = HIGH; // v1.1
			button3State = LOW; // This make AUTO-depressurize after overfill
		}
		else
		{
			// CASE 3: Bottle pressure dropped below pressureDeltaMax threshold; i.e., filling too fast
			if ((P2 - offsetP2) - (P1 - offsetP1) >= pressureDeltaMax)
			{
				// FILLING TOO FAST LOOP // Repressurize to proper pressure
				while (platformStateUp == true && switchDoorState == LOW && (P1 - offsetP1) <= (P2 - offsetP2) - pressureDeltaUp)
				{
					inPressurizeLoop = true;
					digitalWrite(light2Pin, HIGH);
					relayOn(relay2Pin, true);        //open S2 to equalize
					P1 = analogRead(sensorP1Pin);    //do sensor read again to check
					P2 = analogRead(sensorP2Pin);    //do sensor read again to check

					lcd.setCursor(0, 0); lcd.print(F("Filling too fast... "));
					lcd.setCursor(0, 1); lcd.print(F("Close exhaust valve;"));
					lcd.setCursor(0, 2); lcd.print(F("Press B2 to resume  "));

					buzzOnce(750, light2Pin);
					//delay (500); // This seems to be necessary
				}
				// FILLING TOO FAST LOOP EXIT ROUTINES    
				if (inPressurizeLoop)
				{
					relayOn(relay2Pin, false);
					digitalWrite(light2Pin, LOW);

					//This is a little trap for B2 button to demand user input; doesn't change button states. Just wants user input.
					while (!digitalRead(button2Pin) == HIGH)
					{
						//Don't like the flashing lights--slows down button inputs
					}
					buzzedOnce = false;
					button2ToggleState = false; // button2State is still LOW, with toggleState true. Setting toggleState to false should be like pressing Fill again
					messageLcdBlank(2);
					inPressurizeLoop = false;
				}
			}
		}
		inFillLoop = false;
		messageB2B3Toggles();
		analogWrite(propSolPin, 0); //V2
	}

	// END FILL LOOP EXIT ROUTINES
	//========================================================================================

	//========================================================================================
	// DEPRESSURIZE LOOP
	//========================================================================================

	pressureOutput(); //v1.1 Do we need to take a fresh reading here? Probably a good idea
	float minPressureDiffSensorClear = 10; //v1.1. In psi

	//PBSFIRM-134
	pressurizeStartTime = millis();		//Get the start time of depressurization
	unsigned long depressurizeDuration = 0;
	unsigned long timeStamp1 = 0;
	int  timeStamp2 = 0;
	int checkInterval = 2500;			//Check pressure every 2500 ms
	PTest1 = analogRead(sensorP1Pin);	//Take the first pressure reading for future comparison

	//85: Added condition (sensorFillState == LOW && PSIdiff < minPressureDiffSensorClear) to allow depressurization with sensor LOW
	while (button3State == LOW && ((sensorFillState == HIGH || (sensorFillState == LOW && PSIdiff < minPressureDiffSensorClear)) || !digitalRead(button1Pin) == LOW || inCleaningMode == true) && switchDoorState == LOW && (P1 - offsetP1 >= pressureDeltaDown)) //v1.1 added sensor override
	{
		inDepressurizeLoop = true;
		relayOn(relay3Pin, true); //Open Gas Out solenoid
		digitalWrite(light3Pin, HIGH);

		// Pressure output
		pressureOutput();
		printLcd(3, outputPSI_b);

		//V2 Prop Sol code
		analogWrite(propSolPin, 255); 

		//Allow momentary "burst" foam tamping
		button2State = !digitalRead(button2Pin);
		if (button2State == LOW)
		{
			digitalWrite(light2Pin, HIGH);
			relayOn(relay2Pin, true);
			delay(50);  // Burst duration
			relayOn(relay2Pin, false);
			digitalWrite(light2Pin, LOW);
		}

		//v1.1 added as part of sensor override
		if (!digitalRead(button1Pin) == LOW)
		{
			digitalWrite(light1Pin, HIGH);
		}
		else
		{
			digitalWrite(light1Pin, LOW);
		}

		// CLEANING MODE: If in cleaning mode, set FillState HIGH
		if (inCleaningMode == true || !digitalRead(button1Pin) == LOW)
		{
			sensorFillState = HIGH;
			lcd.setCursor(0, 2); lcd.print(F("Foam sensor OFF... "));
		}
		else
		{
			sensorFillState = digitalRead(sensorFillPin); //Check fill sensor
			switchDoorState = digitalRead(switchDoorPin); //Check door switch // Not using this
			lcd.setCursor(0, 2); lcd.print(F("Depressurizing...   "));
		}

		lcd.setCursor(0, 0); lcd.print(F("B3 toggles venting  "));
		messageRotator(10000, .5, 0);
		if (messageID) {
			lcd.setCursor(0, 1); lcd.print(F("B2 burst tamping    "));
		}
		else {
			lcd.setCursor(0, 1); lcd.print(F("B1 overrides sensor "));
		}

		//Check toggle state of B3
		button3StateTEMP = !digitalRead(button3Pin);
		if (button3StateTEMP == HIGH && button3ToggleState == false) {  // ON release
			button3ToggleState = true;                                  // Leaves buttonState LOW
			button3State = LOW;
		}
		if (button3StateTEMP == LOW && button3ToggleState == true) {    // OFF push
			button3State = HIGH;                                        // Exit WHILE loop
		}

		//PBSFIRM-134: Notify user to open exhaust knob if pressure not falling fast enough
		depressurizeDuration = millis() - pressurizeStartTime;			//Get the duration since depressure loop entered
		PTest2 = analogRead(sensorP1Pin);
		timeStamp2 = depressurizeDuration - timeStamp1;					//Get timestamp of Ptest2 reading since last reading

		if (timeStamp2 > checkInterval && timeStamp2 <120000)			//Check pressure each n checkIntervals after depressurization starts; quit at two min
		{
			timeStamp1 = timeStamp1 + checkInterval;					//Update reference time stamp each checkInterval
			//Test to see if the pressure if falling each checkInterval. Second condition allows you a way out of loop.
			while (PTest1 - PTest2 <= 1 && !digitalRead(button3Pin) == HIGH)
			{
				lcd.setCursor(0, 2); lcd.print(F("Open Exhaust knob..."));
				buzzer(10); delay(100);
				PTest2 = analogRead(sensorP1Pin);
			}
			PTest1 = PTest2;											//Update reference pressure variable each checkInterval
		}
	}

	// DEPRESSURIZE LOOP EXIT ROUTINES 
	// If in this loop, Button3 was released, or Fill/Foam Sensor tripped, 
	// or bottle is propeRly pressurized. Find out which reason and reSpond.
	//========================================================================

	if (inDepressurizeLoop)
	{
		// Turn off Light 1 if on
		digitalWrite(light1Pin, LOW);

		// CASE 1: Button3 released
		if (button3State == HIGH)
		{
			relayOn(relay3Pin, false);     //Used to repressurize here; now we don't  
			lcd.setCursor(0, 1); lcd.print(F("B2 toggles filling. ")); //Overwrites second line on exit
		}

		// CASE 2: Foam tripped sensor
		if (sensorFillState == LOW && !digitalRead(button1Pin) == HIGH) // The second condition is part of fillSensor override
		{
			lcd.setCursor(0, 2); lcd.print(F("Foam detected...wait"));
			relayOn(relay3Pin, false);
			relayOn(relay2Pin, true);
			delay(foamDetectionDelay);    // Wait a bit before proceeding    
			relayOn(relay2Pin, false);

			//v1.1 Clear Sensor Routine. This merely flashes a message
			if (digitalRead(sensorFillPin) == LOW)
			{
				lcd.setCursor(0, 2); lcd.print(F("Clearing Foam Sensor"));
				delay(1000); //DEBUG? Delay to show above message.
			}
			sensorFillState = HIGH; //v1.1 Set the sensorState to HIGH
		}

		//CASE 3: Bottle was properly depressurized. If we reach here, the pressure must have reached threshold. Go to Platform lower loop. Note S3 still open
		P1 = analogRead(sensorP1Pin); //#100
		if (P1 - offsetP1 <= pressureDeltaDown)
		{
			depressurizeLoopExecuted = true;
			buzzer(500);
			autoMode_1 = true;  //Going to platform loop manually now, but still set this var to partially drop platform when B3 is pressed
			//lcd.setCursor (0, 0); lcd.print (F("Grasp bottle;       "));
			//lcd.setCursor (0, 1); lcd.print (F("B3 lowers platform. "));
		}
		digitalWrite(light3Pin, LOW);
		inDepressurizeLoop = false;
		//V2 Prop Sol code
		analogWrite(propSolPin, 0);
	}

	// END DEPRESSURIZE LOOP
	//============================================================================================

	// ===========================================================================================
	// DOOR OPENING LOOP
	// ===========================================================================================

	// Only activate door solenoid if door is already closed
	
	//PBSFIRM-146: By taking out automode condition, can close door and resume filling after depressurizing. This is problematic--WON'T FIX IN V1.4
	//while (button3State == LOW && switchDoorState == LOW && (P1 - offsetP1) <= pressureDeltaDown)
	while ((button3State == LOW || autoMode_1 == true) && switchDoorState == LOW && (P1 - offsetP1) <= pressureDeltaDown)
	{
		inDoorOpenLoop = true;
		digitalWrite(light3Pin, HIGH);
		doorOpen(); // Run door open function
	}

	// DOOR OPEN LOOP EXIT ROUTINES
	//============================================================================================
	if (inDoorOpenLoop)
	{
		while (button3State == LOW)
		{
			button3State = !digitalRead(button3Pin); //This is a little trap for B3 button push so platform lower loop doesn't kick in
		}
		delay(500); // TODO This gives a pause so platform lower loop doesn't kick in. Is this necessary?
		inDoorOpenLoop = false;
		digitalWrite(light3Pin, LOW);
		button3State = HIGH;
		//messageLcdWaiting(); //PBSFIRM-143
	}

	// END DOOR OPEN LOOP
	//============================================================================================

	// ===========================================================================================
	// PLATFORM LOWER LOOP 
	// Platform will not lower with door closed. This prevents someone from defeating door switch
	// ===========================================================================================

	// v1.1 Removed "|| autoMode_1 == true and  && (digitalRead(sensorFillPin) == HIGH)" so platform no longer drops automatically.
	while ((!digitalRead(button3Pin) == LOW) && switchDoorState == HIGH && (P1 - offsetP1) <= pressureDeltaDown)
	{
		inPlatformLowerLoop = true;
		digitalWrite(light3Pin, HIGH);
		relayOn(relay4Pin, false); // Finally can close platform up solenoid
		lcd.setCursor(0, 2); lcd.print(F("Lowering...         "));

		if (autoMode_1 == true)
		{
			digitalWrite(light3Pin, HIGH);
			relayOn(relay5Pin, true);
			delay(autoPlatformDropDuration);
			//relayOn(relay5Pin, false);
			button3State = HIGH;
			autoMode_1 = false;
		}
		else
		{
			digitalWrite(light3Pin, HIGH);
			relayOn(relay5Pin, true);  // Open cylinder exhaust
			delay(autoPlatformDropDuration); // always drop at least a second?
		}
		P1 = analogRead(sensorP1Pin);
		button3State = !digitalRead(button3Pin);
		switchDoorState = digitalRead(switchDoorPin);
		sensorFillState = digitalRead(sensorFillPin);
		digitalWrite(light3Pin, LOW);
	}

	// PLATFORM LOWER LOOP EXIT ROUTINES
	// ===========================================================================================

	if (inPlatformLowerLoop)
	{
		//close platform release 
		relayOn(relay3Pin, false);
		relayOn(relay5Pin, false);
		relayOn(relay6Pin, false); //TODO Release door solenoid // ??? is this still needed?
		digitalWrite(light3Pin, LOW);

		inPlatformLowerLoop = false;
		platformStateUp = false;
		depressurizeLoopExecuted = false;

		EEPROM.write(6, platformStateUp);

		//Prepare for next cycle
		messageInsertBottle();

		// Calculate lifetime and session fills    
		if (inFillLoopExecuted)
		{
			numberCycles = numberCycles + 1;                   //Increment lifetime cycles
			numberCyclesSession = numberCyclesSession + 1;     //Increment session cycles

			// Write lifetime cycles back to EEPROM
			numberCycles01 = numberCycles % 255;
			numberCycles10 = (numberCycles - numberCycles01) / 255;
			EEPROM.write(4, numberCycles01);
			EEPROM.write(5, numberCycles10);

			// Write session/total fills to screen
			String(convNumberCyclesSession) = floatToString(buffer, numberCyclesSession, 0);
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

