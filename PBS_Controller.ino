/*
//===========================================================================  

Perlini Bottling System v1.0

Author: 
  Evan Wallace
  Perlage Systems, Inc.
  Seattle WA USA
  
Copyright 2013, 2014  All rights reserved

//===========================================================================  
*/

//Version control variables
String (versionSoftwareTag) = "v0.6.0.0" ; // 2nd pressure sensor rev'd to 0.6
String (versionHardwareTag) = "v0.6.0.0" ; // 2nd sensor rev'd to v0.6

//Library includes
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <math.h> //pressureOffset
#include "floatToString.h"
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x3F,20,4);  

// Pin assignments
const int button1Pin    =  0;     // pin for button 1 B1 (Raise platform) RX=0;
const int button2Pin    =  1;     // pin for button 2 B2 (Fill/purge)     TX=1; 
// 2 SDA for LCD           2;  
// 3 SDL for LCD           3;
const int button3Pin    =  4;     // pin for button 3 B3 (Depressurize and lower) 
const int relay1Pin     =  5;     // pin for relay1 S1 (liquid fill)
const int relay2Pin     =  6;     // pin for relay2 S2 (bottle gas inlet)
const int relay3Pin     =  7;     // pin for relay3 S3 (bottle gas vent)
const int relay4Pin     =  8;     // pin for relay4 S4 (pneumatic lift gas in)
const int relay5Pin     =  9;     // pin for relay5 S5 (pneumatic lift gas in)
const int relay6Pin     = 10;     // pin for relay6 S6 (door lock solenoid)
const int switchFillPin = 11;     // pin for fill sensor F1 -- DO NOT PUT THIS ON PIN 13
const int switchDoorPin = 12;     // pin for finger switch
const int buzzerPin     = 13;     // pin for buzzer

// A0 formerly was sensor1, which is closest to what we now call bottle pressure, and A1 was unused. 
// So switched sensor1 to A1 to make code changes easier. All reads of sensor1 will now read bottle pressure
// sensor1 measures bottle. "1" or "" refers to bottle
// sensor2 measures regulator. "2" refers to regulator

const int sensor1Pin    = A1;     // pin for preasure sensor1 // 1-23 NOW BOTTLE
const int sensor2Pin    = A0;     // pin for pressure sensor2 // 1-23 NOW REGULATOR. 
const int switchCleanPin= A2;     // pin for cleaning mode switch 
const int light1Pin     = A3;     // pin for button 1 light 
const int light2Pin     = A4;     // pin for button 2 light
const int light3Pin     = A5;     // pin for button 3 light


// Declare variables and inititialize states
// ===========================================================================  

//Component states
int button1State     = HIGH;
int button2State     = HIGH; 
int button3State     = HIGH;
int light1State      = LOW;
int light2State      = LOW;
int light3State      = LOW;
int relay1State      = HIGH;
int relay2State      = HIGH;
int relay3State      = HIGH;
int relay4State      = HIGH;
int relay5State      = HIGH;
int relay6State      = HIGH;
int switchFillState  = HIGH; 
int switchDoorState  = HIGH;
int switchCleanState = HIGH; 

//State variables set in loops
boolean inPressureNullLoop           = false;
boolean inPlatformLoop               = false;
boolean inPurgeLoopInterlock         = false;
boolean inPurgeLoop                  = false;
boolean inPressurizeLoop             = false;
boolean inFillLoop                   = false;
boolean inOverFillLoop               = false;
boolean inDepressurizeLoop           = false;
boolean inPlatformLowerLoop          = false;
boolean inDoorOpenLoop               = false;
boolean inCleanLoop                  = false; 
boolean inPressureSaggedLoop         = false;

//Key logical states
boolean autoMode_1                   = false;   // Variable to help differentiate between states reached automatically vs manually 
boolean platformLockedNew            = false;   // Variable to be set when platfrom first locks up. This is for autofill-on-doorclose function
boolean platformStateUp              = false;   // true means platform locked in UP; toggled anytime S5 opens

//Pressure variables
int P1                               = 0;       // Current pressure reading from sensor1
int P2                               = 0;       // Current pressure reading from sensor2 
int pressureOffset;                             // Zero offset for bottle sensor1. Set through EEPROM in initial factory calibration, and read into pressureOffset during Setup loop
int pressureOffset2;                            // zero offest for regulator sensor2
int pressureDeltaUp                  = 50;      // Pressure at which, during pressurization, full pressure is considered to have been reached // Tried 10; went back to 50 to prevent repressurizing after fill button cycle
int pressureDeltaDown                = 40;      // Pressure at which, during depressurizing, pressure considered to be close enough to zero
int pressureDeltaAutotamp            = 250;     // This is max pressure difference allowed on filling (i.e., limits fill speed)
int pressureNull                     = 200;     // This is the threshold for the controller deciding that no gas source is attached. 
float PSI                            = 0;       // Current pressure reading P1 in PSI
float PSI2                           = 0;       // Current pressure reading of P2 in PSI 
float bottlePressurePSI              = 0;       // Bottle pressure in PSI (i.e., StartPressure - PSI)

//variables for platform function and timing
int timePlatformInit;                           // Time in ms going into loop
int timePlatformCurrent;                        // Time in ms currently in loop
int timePlatformRising               = 0;       // Time diffence between Init and Current
int timePlatformLock                 = 1250;    // Time in ms before platform locks in up position
int autoPlatformDropDuration         = 1500;    // Duration of platform autodrop in ms

//Key performance parameters
int autoSiphonDuration               = 3000;    // Duration of autosiphon function in ms //SACC: CHANGED FROM 2500 TO 5500 WITH LONGER TUBE FOR 750S
int antiDripDuration                 = 1000;    // Duration of anti-drip autosiphon
int foamDetectionDelay               = 2000;    // Amount of time to pause after foam detection
int pausePlatformDrop                = 1000;    // Pause before platform drops after door automatically opens
 
//Variables for button toggle states 
int button2StateTEMP                 = HIGH;
int button3StateTEMP                 = HIGH;
boolean button2ToggleState           = false;
boolean button3ToggleState           = false;

char buffer[25]; // Used in LCD write routines

// Declare functions
// =====================================================================

// FUNCTION printLcd: 
// To simplify string output
String currentLcdString[3];
void printLcd (int line, String newString){
  if(!currentLcdString[line].equals(newString)){
    currentLcdString[line] = newString;
    lcd.setCursor(0,line);
    lcd.print("                    ");
    lcd.setCursor(0,line);
    lcd.print(newString);
  }
}

// FUNCTION relayOn: 
// Allows relay states to be easily be changed from HI=on to LOW=on
void relayOn(int pinNum, boolean on){
  if(on){
    digitalWrite(pinNum, LOW); //turn relay on
  }
  else{
    digitalWrite(pinNum, HIGH); //turn relay off
  }
}

// FUNCTION pressureConv: 
// Routine to convert pressure from parts in 1024 to psi
// pressurePSI = (((P1 - pressureOffset) * 0.0048828)/0.009) * 0.145037738; //This was original equation
// 1 PSI = 12.7084 units; 1 unit = 0.078688 PSI; 40 psi = 543 units

// Function for P1
float pressureConv(int P1) 
{
  float pressurePSI;
  pressurePSI = (P1 - pressureOffset) * 0.078688; 
  return pressurePSI;
}

// Function for P2 (potentially different offset)
float pressureConv2(int P2) 
{
  float pressurePSI2;
  pressurePSI2 = (P2 - pressureOffset2) * 0.078688; 
  return pressurePSI2;
}

//=======================================================================================
// SETUP LOOP
//=======================================================================================

void setup()
{
  //Setup pins
  pinMode(button1Pin, INPUT);  //Changed buttons 1,2,3 from PULLUP to regular when starting to use touchbuttons, which use a pulldown resistor
  pinMode(button2Pin, INPUT);
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
  pinMode(switchFillPin, INPUT_PULLUP); 
  pinMode(switchDoorPin, INPUT_PULLUP); 
  pinMode(switchCleanPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  
  //set all relay pins to high which is "off" for this relay
  digitalWrite(relay1Pin, HIGH);
  digitalWrite(relay2Pin, HIGH);
  digitalWrite(relay3Pin, HIGH);
  digitalWrite(relay4Pin, HIGH);   
  digitalWrite(relay5Pin, HIGH); 
  digitalWrite(relay6Pin, HIGH);
  
  //set to HIGH which "open" for touchbuttons
  digitalWrite(button1Pin, HIGH); 
  digitalWrite(button2Pin, HIGH);
  digitalWrite(button3Pin, HIGH);
  
  Serial.begin(9600); // TO DO: remove when done
  
  //setup LCD (TO DO: still needed?
  lcd.init();
  lcd.backlight();
  
  // ==============================================================================
  // EEPROM CALIBRATION 
  // Write null pressure to EEPROM. 
  // WITH GAS OFF AND PRESSURE RELEASED FROM HOSES, Hold down all three buttons on startup 
  // ==============================================================================

  boolean button1State = !digitalRead(button1Pin); 
  boolean button2State = !digitalRead(button2Pin); 
  boolean button3State = !digitalRead(button3Pin); 
  
  if (button1State == LOW && button2State == LOW && button3State == LOW)
  {
    pressureOffset  = analogRead(sensor1Pin); 
    pressureOffset2 = analogRead(sensor2Pin); 
    delay(100);

    EEPROM.write(0, pressureOffset);  //zero offset for sensor1 (bottle)
    EEPROM.write(1, pressureOffset2); //Zero offset for sensor2 (regulator)

    String (convOffset) = floatToString(buffer, pressureOffset, 1);
    String (convOffset2) = floatToString(buffer, pressureOffset2, 1);

    String (outputOffset) = "Offset1: " + convOffset; 
    String (outputOffset2) = "Offset2: " + convOffset2; 

    printLcd(2, outputOffset);
    printLcd(3, outputOffset2);
    delay(5000);

    printLcd(2, "");
    printLcd(3, "");
  }    

  //Write the actual nullPressure to pressureNull
  pressureOffset = EEPROM.read(0);
  pressureOffset2 = EEPROM.read(1);

  // END EEPROM 
  // ==============================================================================
    
  //===================================================================================
  // Startup Routine
  //===================================================================================
  
  // Read States. Get initial pressure difference reading from sensor. 
  // High pressure reading means unpressurized bottle; low means pressurized bottle or no gas
  
  P1 = analogRead(sensor1Pin); // Read the initial bottle pressure and assign to P1
  P2 = analogRead(sensor2Pin); // Read the initial regulator pressure and assign to P2
  switchDoorState = digitalRead(switchDoorPin);  

  relayOn(relay3Pin, true); // Open at earliest possibility to vent  
  
  // Note start time for depressurization of stuck bottle
  unsigned long pressurizeStartTime = millis(); 
  unsigned long pressurizeDuration = 0;
  
  //Initial user message
  printLcd (0, "Perlini Bottling");
  printLcd (1, "System, " + versionSoftwareTag);
  printLcd (2, "");
  printLcd (3, "Initializing...");
  delay(1000); //Just to give a little time before platform goes up

  //=================================================================================
  // MENU ROUTINE
  //=================================================================================
 
  button1State     = !digitalRead(button1Pin); 
  boolean button1StateMENU = !digitalRead(button1Pin); 
  boolean button2StateMENU = !digitalRead(button2Pin); 
  boolean button3StateMENU = !digitalRead(button3Pin); 
  delay(100);
  boolean inMenuLoop = false;

  while (button1State == LOW)
  {
    inMenuLoop = true;
    
    printLcd (0, "Setup Menu. Press...");
    printLcd (1, "");
    printLcd (2, "B2: Autosiphon time");
    printLcd (3, "B3: Exit");
    
    button1StateMENU = !digitalRead(button1Pin); 
    button2StateMENU = !digitalRead(button2Pin); 
    button3StateMENU = !digitalRead(button3Pin); 
    
    if (button2StateMENU == LOW)
    {
      autoSiphonDuration = 7500;
      printLcd (2, "Inc. to 7500 ms");
      delay(2000);
    }  
    if (button3StateMENU == LOW){
      button1State = HIGH;
    }  
  }
  while (inMenuLoop)
  {
    inMenuLoop = false;
    printLcd (3, "Exiting menu...");  
    delay(1000);  
    printLcd (0, "Perlini Bottling");
    printLcd (1, "System, " + versionSoftwareTag);
    printLcd (2, "");
    printLcd (3, "Initializing...");
    delay(1000); //Just to give a little time before platform goes up    
  }  
  // END MENU ROUTINE
  //=================================================================================  

  // Turn on platform support immediately, but make sure door is closed so no pinching!
  if (switchDoorState == LOW)
  {
    relayOn(relay4Pin, true); // Turn on platform support immediately. Raises platform if no bottle; keeps stuck bottle in place
  }
  else
  {
    while (switchDoorState == HIGH)
    {
      printLcd (3, "PLEASE CLOSE DOOR");
      switchDoorState = digitalRead(switchDoorPin); 
      
      digitalWrite(buzzerPin, HIGH); 
      delay(100);
      digitalWrite(buzzerPin, LOW);
      delay(100);
    }

    delay(500);
    relayOn(relay4Pin, true); // Raise platform     
    printLcd (3, "Initializing...");        
  }    

  // Blinks lights and give time to measure any degassing
  for (int n = 0; n < 1; n++)
  {
    digitalWrite(light1Pin, HIGH);
    delay(500);
    //digitalWrite(light1Pin, LOW);
    digitalWrite(light2Pin, HIGH);
    delay(500);
    //digitalWrite(light2Pin, LOW);
    digitalWrite(light3Pin, HIGH);
    delay(500);
    digitalWrite(light1Pin, LOW);
    digitalWrite(light2Pin, LOW);
    digitalWrite(light3Pin, LOW);
    delay(325);
  }

  // Leave loop and light all lights 
  digitalWrite(light1Pin, HIGH);
  delay(500);
  //digitalWrite(light1Pin, LOW);
  digitalWrite(light2Pin, HIGH);
  delay(500);
  //digitalWrite(light2Pin, LOW);
  digitalWrite(light3Pin, HIGH);
  delay(500);

  //============================================================================
  // NULL PRESSURE ROUTINES
  // Check for stuck pressurized bottle or implausibly low gas pressure at start 
  //============================================================================

  boolean inPressurizedBottleLoop = false;
  boolean inPressureNullLoop = false;

  // CASE 1: PRESSURIZED BOTTLE (Bottle is already depressurizing because S3 opened above)
  if (P1 - pressureOffset > pressureDeltaDown) 
  {
    inPressurizedBottleLoop = true;

    printLcd(0, "Pressurized bottle");
    printLcd(1, "detected. Open valve");
    printLcd(2, "Depressurizing...");
    printLcd(3, "");
   
    digitalWrite(buzzerPin, HIGH); 
    delay(250);
    digitalWrite(buzzerPin, LOW);

    while (P1 - pressureOffset > pressureDeltaDown)
    {
      P1 = analogRead(sensor1Pin);

      bottlePressurePSI = pressureConv(P1);
      String (convPSI) = floatToString(buffer, bottlePressurePSI, 1);
      String (outputPSI) = "Pressure: " + convPSI + " psi";  
      printLcd(3, outputPSI);  
    } 
  }
      
  // CASE 2: GASS OFF OR LOW       
  if (P2 < pressureNull)
  {
    inPressureNullLoop = true;

    printLcd(0, "Gas off or empty;");
    printLcd(1, "check tank & hoses.");
    printLcd(2, "B3 opens door.");
    printLcd(3, "Waiting...");
        
    digitalWrite(buzzerPin, HIGH); 
    delay(250);
    digitalWrite(buzzerPin, LOW);
    
    while (P2 < pressureNull)
    {
      //Read sensors
      P2 = analogRead(sensor2Pin); 
      button3State = !digitalRead(button3Pin);
      switchDoorState = digitalRead(switchDoorPin);  
            
      while(button3State == LOW && switchDoorState == LOW)
      {
        inDoorOpenLoop = true;
        relayOn(relay6Pin, true);
        switchDoorState = digitalRead(switchDoorPin);  
      }
      
      if (inDoorOpenLoop == true)
      {
        inDoorOpenLoop = false;
        relayOn(relay6Pin, false);
      }  
    }  
  }
  
  // NULL PRESSURE EXIT ROUTINES
  // No longer closing S3 at end of this to prevent popping in some edge cases of a very foamy bottle
  // ==================================================================================
  
  if (inPressurizedBottleLoop)
  {
    //Open door, but don't drop platform--allow to degas
    while (switchDoorState == LOW)
    {
      inDoorOpenLoop = true;
      relayOn(relay6Pin, true);
      switchDoorState = digitalRead(switchDoorPin);
    }
    
    if (inDoorOpenLoop == true)
    {
      inDoorOpenLoop = false;
      relayOn(relay6Pin, false);
    }      

    while (button3State == HIGH)
    {  
      button3State = !digitalRead(button3Pin);

      printLcd(0, "Press B3 to lower");
      printLcd(1, "platform.");
      printLcd(2, "Depressurized.");
    }
  } 

  if (inPressureNullLoop)
  {
    //Open door, drop platform
    while(switchDoorState == LOW)
    {
      inDoorOpenLoop = true;
      relayOn(relay6Pin, true);
      switchDoorState = digitalRead(switchDoorPin);  
    }
    if (inDoorOpenLoop == true)
    {
      inDoorOpenLoop = false;
      relayOn(relay6Pin, false);
    }  

    //Drop plaform 
    relayOn(relay4Pin, false);
    relayOn(relay5Pin, true);
    delay (3000);
    relayOn(relay5Pin, false);
  }

  // END NULL PRESSURE LOOP
  // ====================================================================================

  // Do the normal ending when pressure is OK
  if (inPressureNullLoop == false && inPressurizedBottleLoop == false && switchDoorState == LOW) 
  {
    
    // Drop platform, which had been raised before nullpressure checks
    relayOn(relay4Pin, false);  
    relayOn(relay5Pin, true);
    delay(3000);  
    relayOn(relay5Pin, false); 
      
    relayOn(relay3Pin, false); // Close this so solenoid doesn't overhead on normal startup
    
    // Open door if closed
    while (switchDoorState == LOW)
    {
      inDoorOpenLoop = true;
      relayOn(relay6Pin, true);
      switchDoorState = digitalRead(switchDoorPin);  
    }
    if (inDoorOpenLoop == true)
    {
      inDoorOpenLoop = false;
      relayOn(relay6Pin, false);
    }
  }
 
  // User instructions
  printLcd(0, "Insert bottle;");
  printLcd(1, "B1 raises platform");
  printLcd(2, "Ready.");

  delay(500);
  digitalWrite(light3Pin, LOW);
  delay(100);
  digitalWrite(light2Pin, LOW);
  delay(100);
  digitalWrite(light1Pin, LOW);
  
}

//====================================================================================================================================
// MAIN LOOP =========================================================================================================================
//====================================================================================================================================

void loop()
{
  
  //MAIN LOOP IDLE FUNCTIONS
  //=====================================================================
  
  // Read the state of buttons and sensors
  // Boolean NOT (!) added for touchbuttons so that HIGH button state (i.e., button pressed) reads as LOW (this was easiest way to change software from physical buttons, where pressed = LOW 
  button1State =     !digitalRead(button1Pin); 
  button2StateTEMP = !digitalRead(button2Pin); 
  button3StateTEMP = !digitalRead(button3Pin); 
  switchDoorState  =  digitalRead(switchDoorPin);
  switchCleanState =  digitalRead(switchCleanPin);
  delay(25);  

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
  if (button3StateTEMP == LOW && button3ToggleState == false){  //ON push
    button3State = LOW;         //goto while loop
  }
  if (button3StateTEMP == LOW && button3ToggleState == true){   //button still being held down after OFF push in while loop. 
    button3State = HIGH;        //nothing happens--buttonState remains HIGH
  } 
  if (button3StateTEMP == HIGH && button3ToggleState == true){  //OFF release
    button3ToggleState = false; //buttonState remains HIGH
  }
   
  // Main Loop pressure measurement
  // Will only get continuous measurement if platform state is down, so we aren't overwriting current state informtion during press/fill/depress
  int pressureIdle;  
  int pressure2Idle;  
  float pressureIdlePSI;  
  float pressure2IdlePSI;  

  if (platformStateUp == false) 
  {
    pressureIdle = analogRead(sensor1Pin); 
    pressure2Idle = analogRead(sensor2Pin);
          
    pressureIdlePSI = pressureConv(pressureIdle); 
    pressure2IdlePSI = pressureConv2(pressure2Idle); 
    
    String (convPSI) = floatToString(buffer, pressureIdlePSI, 1);
    String (convPSI2) = floatToString(buffer, pressure2IdlePSI, 1);

    //String (outputPSI) = "Pb:" + convPSI + "Pr:" + convPSI2 + " psi";
    String (outputPSI) = "Pressure: " + convPSI2 + " psi";
    printLcd(3, outputPSI); 
  }
  
  // =====================================================================================  
  // CLEANING ROUTINES
  // =====================================================================================  
  
  if (switchCleanState == LOW)
  {
    //inCleanLoop = true; //TO DO: IS THIS NEEDED?
    printLcd (2, "IN CLEANING MODE...");
    switchFillState = HIGH;
  }
  else
  {
    //inCleanLoop = false;
    printLcd (2, "Ready.");
  }

  // END CLEANING ROUTINES
  // =====================================================================================  

  // =====================================================================================  
  // PLATFORM RAISING LOOP
  // while B1 is pressed, Raise bottle platform.
  // Conditions look for no bottle or bottle unpressurized, and door must be open. Added pressure condition 
  // to make sure this button couldn't accidentally lower platform when pressurized
  // =====================================================================================  
  
  timePlatformInit = millis(); // Inititalize time for platform lockin routine
  
  while (button1State == LOW && platformStateUp == false && switchDoorState == HIGH && (timePlatformRising < timePlatformLock)) 
  { 
    inPlatformLoop = true; 
    digitalWrite(light1Pin, HIGH);
    
    relayOn(relay5Pin, false);    //closes vent
    relayOn(relay4Pin, true);     //raises plaform
    
    timePlatformCurrent = millis();
    delay(10); //added this to make sure time gets above threshold before loop exits--not sure why works
    timePlatformRising = timePlatformCurrent - timePlatformInit;
    
    String platformStatus = ("Raising... " + String(timePlatformRising) + " ms");
    printLcd(2, platformStatus);  

    button1State = !digitalRead(button1Pin); 
  }  

  // PLATFORM RAISING LOOP EXIT ROUTINES
  // =======================================================================================
  if (inPlatformLoop)
  {
    if (timePlatformRising >= timePlatformLock)
    {
      // PlatformLoop lockin reached
      digitalWrite(buzzerPin, HIGH); 
      delay(100);
      digitalWrite(buzzerPin, LOW);
  
      printLcd(0, "To fill, close door"); 
      printLcd(1, "B2 toggles filling"); 
      printLcd(2, "");

      relayOn(relay4Pin, true);  // Slight leak causes platform to fall over time--so leave open 
      relayOn(relay5Pin, false); 
      digitalWrite(light1Pin, LOW); //Decided to turn this off. Lights should be lit only if pressing button or releasing it can change a state. 
      platformStateUp = true;
      platformLockedNew = true; //Pass this to PressureLoop for autopressurize on door close--better than trying to pass button2State = LOW, which causes problems
      
      // Read new starting pressure 
      // This used to be in the platform DOWN exit loop
      
      delay(50); // Brief delay to account for moving gas
      P1 = analogRead(sensor1Pin); //Bottle Pressure
      P2 = analogRead(sensor2Pin); //Regulator Pressure
      
      PSI2 = pressureConv2(P2); 
      String (convPSI) = floatToString(buffer, PSI2, 1);
      String (outputPSI) = "Pressure: " + convPSI + " psi";
      printLcd(3, outputPSI); 
      
    }  
    else    
    {
      // Drop Platform
      relayOn(relay4Pin, false);
      relayOn(relay5Pin, true);
      digitalWrite(light1Pin, LOW); 
      platformStateUp = false;
      printLcd(2, "");
    }

    timePlatformRising = 0;
    inPlatformLoop = false;
  }
  
  // END PLATFORM RAISING LOOP
  //=============================================================================================


  //=============================================================================================
  // PURGE LOOP
  // While B2 and then B1 is pressed, and door is open, and platform is down, purge the bottle
  //=============================================================================================
  
  // PURGE INTERLOCK LOOP
  //=============================================================================================
  // Door must be OPEN, platform DOWN, and pressure DIFF near max (i.e. no bottle)
  
  while(button2State == LOW && switchDoorState == HIGH && platformStateUp == false && (P1 - pressureOffset <= pressureDeltaDown)) //Pressure condx is an extra safety measure
  {
    inPurgeLoopInterlock = true;
    digitalWrite(light2Pin, HIGH);
    
    // PURGE LOOP
    //=========================
    while (button1State == LOW) // Require two button pushes, B2 then B1, to purge
    { 
      inPurgeLoop = true;
      printLcd(2,"Purging..."); 
      digitalWrite(light1Pin, HIGH); 
      relayOn(relay2Pin, true); //open S2 to purge
      button1State = !digitalRead(button1Pin); 
      delay(25);  
    }
    
    // PURGE LOOP EXIT ROUTINES
    //=========================
    
    if(inPurgeLoop)
    {
      relayOn(relay2Pin, false); //Close relay when B1 and B2 not pushed
      digitalWrite(light1Pin, LOW); 
  
      inPurgeLoop = false;
      printLcd(2,""); 
    }
    
    button1State = !digitalRead(button1Pin); 
    button2State = !digitalRead(button2Pin); 
    delay(25);
  }
  
  // PURGE LOOP INTERLOCK EXIT ROUTINES
  //===================================
  
  if (inPurgeLoopInterlock)
  {
    digitalWrite(light2Pin, LOW); 
    inPurgeLoopInterlock = false;
  }  
 
  // END PURGE LOOP
  //============================================================================================
   
  //============================================================================================
  // PRESSURIZE LOOP
  // Pressurization will start automatically when door closes IF platfromLockedNew is true
  //============================================================================================
  
  LABEL_inPressurizeLoop:
  
  int PTest1;
  int PTest2;
  int PTestFail = false;
  int pressurizeStartTime;
  int pressurizeDuration = 0;
  int pressurizeCurrentTime = 0;

  // Get fresh pressure readings  
  P1 = analogRead(sensor1Pin);
  P2 = analogRead(sensor2Pin);       
  
  while((button2State == LOW || platformLockedNew == true) && switchDoorState == LOW && platformStateUp == true && (P1 - pressureOffset <= P2 - pressureOffset2 - pressureDeltaUp)) 
  { 
    inPressurizeLoop = true;
    
    if (platformLockedNew == true)
    {
      delay(250);   //Make a slight delay in starting pressuriztion when door is first closed
      pressurizeStartTime = millis(); //startTime for no bottle test below
    }
    
    //Open gas-in relay, close gas-out
    relayOn(relay3Pin, false);      //close S3 if not already 
    relayOn(relay2Pin, true);       //open S2 to pressurize
    digitalWrite(light2Pin, HIGH);  //Light button when button state is low

    //NO BOTTLE TEST: Check to see if bottle is pressurizing; if PTest not falling, must be no bottle 
    while (platformLockedNew == true)
    {
      pressurizeDuration = millis() - pressurizeStartTime; // Get the duration
      if (pressurizeDuration < 50){
        PTest1 = analogRead(sensor1Pin);                   //Take a reading at 50ms after pressurization begins
      }
      if (pressurizeDuration < 100){
        PTest2 = analogRead(sensor1Pin);                   //Take a reading at 100ms after pressurization begins
      }  
      if (pressurizeDuration > 100){                        //After 100ms, test
        if (PTest2 - PTest1 < 10){                          //If there is less than a 10 unit difference, must be no bottle in place
          button2State = HIGH; 
          button2ToggleState = true;                        //Need this to keep toggle routine below from changing button2state back to LOW 
          PTestFail = true;
        }  
        platformLockedNew = false;
      }
    }
    
    //Read sensors
    switchDoorState = digitalRead(switchDoorPin); //Check door switch    
    P1 = analogRead(sensor1Pin); // Don't really even need to read P2; read it going into loop?
   
    //Check toggle state of B2
    button2StateTEMP = !digitalRead(button2Pin);
    delay(25);  //Debounce
    if (button2StateTEMP == HIGH && button2ToggleState == false){  //ON release
      button2ToggleState = true;                                   //leave button state LOW
      button2State = LOW;                                          //or make it low if it isn't yet
    }
    if (button2StateTEMP == LOW && button2ToggleState == true){    //OFF push
      button2State = HIGH;                                         //exit WHILE loop
    }  
      
    printLcd(2,"Pressurizing..."); 
    bottlePressurePSI = pressureConv(P1);
    String (convPSI) = floatToString(buffer, bottlePressurePSI, 1);
    String (outputPSI) = "Bottle Press: " + convPSI; + "psi";
    printLcd(3, outputPSI); 
  }
    
  // PRESSURIZE LOOP EXIT ROUTINES
  //===================================
  
  if(inPressurizeLoop)
  {
    inPressurizeLoop = false;

    relayOn(relay2Pin, false);       // close S2 because we left PressureLoop
    digitalWrite(light2Pin, LOW);    // Turn off B2 light
    
    //PRESSURE TEST EXIT ROUTINE
    if (PTestFail == true)
    {
      printLcd (2, "No bottle, or leak");
      printLcd (3, "Wait...");

      digitalWrite(light1Pin, LOW); 
      delay (2000);

      //Open door
      while (switchDoorState == LOW)
      {
        switchDoorState = digitalRead (switchDoorPin);
        digitalWrite (relay6Pin, LOW);
      }
      digitalWrite (relay6Pin, HIGH);
      switchDoorState = HIGH;
      
      //Make platform drop
      digitalWrite(relay4Pin, HIGH); 
      digitalWrite(relay5Pin, LOW); 
      delay (3000);
      digitalWrite(relay5Pin, HIGH); 
      
      //Reset variables
      PTestFail = false;      
      timePlatformRising = 0;
      platformStateUp = false;      
    }
    
    //Door opened while bottle pressurized...emergency dump of pressure  
    if (switchDoorState == HIGH)
    { 
      goto LABEL_inEmergencyDepressurizeLoop;
    }  
  }

  // END PRESSURIZE LOOP
  //========================================================================================
  
  //========================================================================================
  // FILL LOOP
  //========================================================================================

  // Get fresh pressure readings
  P1 = analogRead (sensor1Pin);
  P2 = analogRead (sensor2Pin);

  //pinMode(switchFillPin, INPUT_PULLUP); //Probably no longer necessary since FillSwitch was moved off Pin13 (Zach proposed this Oct-7)

  // With two sensors, the pressure condition should absolutely prevent any liquid sprewing. pressureDeltaAutotamp ensures: a) filling can't go too fast; b) can't dispense w/ no bottle
  while(button2State == LOW && switchFillState == HIGH && switchDoorState == LOW && (P1 - pressureOffset) > pressureDeltaAutotamp) 
  {     
    inFillLoop = true;
  
    //pinMode(switchFillPin, INPUT_PULLUP); // TO DO: See above comment above...is this still needed?
    
    //while the button is  pressed, fill the bottle: open S1 and S3
    relayOn(relay1Pin, true);
    relayOn(relay3Pin, true);
    digitalWrite(light2Pin, HIGH); 

    //Check toggle state of B2    
    button2StateTEMP = !digitalRead(button2Pin);
    delay(25);
    if (button2StateTEMP == HIGH && button2ToggleState == false){  // ON release
      button2ToggleState = true;                                   //Leaves buttonState LOW
      button2State = LOW;                                          //Or makes it low
    }
    if (button2StateTEMP == LOW && button2ToggleState == true){    // OFF push
      button2State = HIGH;                                         //exit WHILE loop
    }
    
    //Check pressure
    P1 = analogRead(sensor1Pin);
    P2 = analogRead(sensor2Pin); 

    float pressurePSI = pressureConv(P1);
    float pressurePSI2 = pressureConv2(P2);
    String (convPSI) = floatToString(buffer, (pressurePSI2 - pressurePSI), 1);
    String (outputPSI) = "Press diff: " + convPSI + " psi";
    printLcd (3, outputPSI); 
    
    // CLEANING MODE: If clean switch closed, set FillState HIGH
    if (switchCleanState == LOW)
    {
      switchFillState = HIGH;
      printLcd (2, "Filling...CLEAN MODE");
      //switchCleanState = digitalRead(switchCleanPin);
    }
    else
    {
      //Read sensors
      switchFillState = digitalRead(switchFillPin); //Check fill sensor
      switchDoorState = digitalRead(switchDoorPin); //Check door switch 
      printLcd (2, "Filling..."); 
    }   
    
    switchCleanState = digitalRead(switchCleanPin); //Check cleaning switch 
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
    if (button2State == HIGH)
    {
      
      // Anti-drip rountine
      //digitalWrite(light2Pin, HIGH); //Don't light button for automatic processes
      relayOn(relay1Pin, true);
      relayOn(relay2Pin, true);
      
      printLcd(2,"Anti drip..."); 
      delay(antiDripDuration); // This setting determines duration of autosiphon 
      printLcd(2,"");       
      relayOn(relay1Pin, false);
      relayOn(relay2Pin, false);
      //digitalWrite(light2Pin, LOW); 
      
      switchFillState = digitalRead(switchFillPin); 
      delay (50);
    }
    
    // CASE 2: FillSwitch tripped--Overfill condition
    else if (inFillLoop && switchFillState == LOW) 
    {
      //digitalWrite(light2Pin, HIGH); //Don't light buttons for automatic processes
      relayOn(relay1Pin, true);
      relayOn(relay2Pin, true);
      
      printLcd(2,"Adjusting level..."); 
      delay(autoSiphonDuration); // This setting determines duration of autosiphon 
      printLcd(2,"");       
      relayOn(relay1Pin, false);
      relayOn(relay2Pin, false);
      //digitalWrite(light2Pin, LOW); 
      
      switchFillState = digitalRead(switchFillPin); 
      delay (50); 
      
      button3State = LOW; // This make AUTO-depressurize after overfill
    }
    
    // CASE 3: Bottle pressure dropped below Deltatamp threshold
    else 
    {

      //=============================================
      // REPRESSURIZE LOOP
      //=============================================
      
      while ((P1 - pressureOffset) <= (P2 - pressureOffset2) - pressureDeltaUp)
      {
        inPressurizeLoop = true;
        digitalWrite(light2Pin, HIGH); 
        relayOn(relay2Pin, true);       //open S2 to equalize
        P1 = analogRead(sensor1Pin);    //do sensor read again to check
        P2 = analogRead(sensor2Pin);    //do sensor read again to check
        printLcd(2,"Repressurizing...");
      }
      
      // REPRESSURIZE LOOP EXIT ROUTINES
      //=============================================
      
      if(inPressurizeLoop)
      { 
        relayOn(relay2Pin, false);  //close S2 because the preasures are equal
        digitalWrite(light2Pin, LOW);
        printLcd(2,"");
        inPressurizeLoop = false; 
      } 
      
      // END REPRESSUIZE LOOP   
      //=============================================     
    }  
    
    inFillLoop = false;
    printLcd(0, "B2 toggles fill;");  
    printLcd(1, "B3 toggles exhaust");  
  }

  // END FILL LOOP EXIT ROUTINE
  //========================================================================================

  //========================================================================================
  // EMERGENCY DEPRESSURIZE LOOP
  // Depressurize anytime door opens when pressurized above low threshold
  //========================================================================================

  LABEL_inEmergencyDepressurizeLoop:    
  int inEmergencyDepressurizeLoop = false;
  
  
  while (switchDoorState == HIGH && (P1 - pressureOffset > pressureDeltaDown))
  {

    inEmergencyDepressurizeLoop = true;
    relayOn(relay3Pin, true);  
    printLcd (2, "Close door...");
    
    float pressurePSI = pressureConv(P1);
    String (convPSI) = floatToString(buffer, pressurePSI, 1);
    String (outputPSI) = "Bottle Press: " + convPSI;
    printLcd(3, outputPSI);     
    
    switchDoorState = digitalRead(switchDoorPin); //Check door switch    
    P1 = analogRead(sensor1Pin);
  }  
  
  // EMERGENCY DEPRESSURIZE LOOP EXIT ROUTINES
  //========================================================================================
  if (inEmergencyDepressurizeLoop)
  {
    printLcd (2, "");
    button2State = LOW;  //TO DO: SHOULDN'T THIS BE HIGH? This might happen after a post-door open foam up--dont want to continue
    relayOn(relay3Pin, false);  
    inEmergencyDepressurizeLoop = false;
  }  
  
  // END EMERGENCY DEPRESSURIZE LOOP
  //========================================================================================


  //================================================================================
  // DEPRESSURIZE LOOP
  //================================================================================

  LABEL_inDepressurizeLoop:
  
  while(button3State == LOW && switchFillState == HIGH && (P1 - pressureOffset >= pressureDeltaDown)) 
  {  

    inDepressurizeLoop = true;
    digitalWrite(light3Pin, HIGH);

    relayOn(relay3Pin, true); //Open Gas Out solenoid
    
    float pressurePSI = pressureConv(P1);
    String (convPSI) = floatToString(buffer, pressurePSI, 1);
    String (outputPSI) = "Bottle Press: " + convPSI;
    printLcd(3, outputPSI); 
    
    //Allow momentary "burst" foam tamping
    button2State = !digitalRead(button2Pin);
      if(button2State == LOW)
      {
        digitalWrite(light2Pin, HIGH); 
        relayOn(relay2Pin, true);
        delay(50);
        relayOn(relay2Pin, false);
        digitalWrite(light2Pin, LOW);
      }

    // Read sensors
    P1 = analogRead(sensor1Pin);
    //P2 = analogRead(sensor2Pin);
    
    // CLEANING MODE 
    if (switchCleanState == LOW)
    {
      switchFillState = HIGH;
      printLcd (2, "Venting...CLEAN MODE");
    }
    else
    {
      switchFillState = digitalRead(switchFillPin); //Check fill sensor
      switchDoorState = digitalRead(switchDoorPin); //Check door switch 
      printLcd(2, "Depressurizing...");  
    }   
    
    switchCleanState = digitalRead(switchCleanPin); //Check cleaning switch 
    
    //Check toggle state of B3
    button3StateTEMP = !digitalRead(button3Pin);
    delay(25);
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
  // or bottle is propely pressurized. Find out which reason and repond.
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
    if (switchFillState == LOW)
    {
      relayOn(relay3Pin, false);     //
      printLcd(2, "Foam... wait");
      relayOn(relay2Pin, true);
      delay(foamDetectionDelay); // Wait a bit before proceeding    

      relayOn(relay2Pin, false);
      switchFillState = HIGH;
      printLcd(2, "");
      //TO DO: we seem to have ended up with auto-resume of depressurization with new toggle routine. Not sure if want this?
    }

    //CASE 3: Bottle was properly depressurized. If we reach here, the pressure must have reached threshold. Go to Platform lower loop
    if (P1 - pressureOffset <= pressureDeltaDown)
    {
      digitalWrite(buzzerPin, HIGH); 
      delay(100);
      digitalWrite(buzzerPin, LOW);     
      autoMode_1 = true;  //Going to platform loop automatically, so set this var to partially drop platform 
    }
    
    digitalWrite(light3Pin, LOW);
    inDepressurizeLoop = false;
   }
  
  // END DEPRESSURIZE LOOP
  //=======================================================================

  // ======================================================================
  // DOOR OPENING LOOP
  // ======================================================================
 
  // Only activate door solenoid if door is already closed
  
  while((button3State == LOW || autoMode_1 == true) && switchDoorState == LOW && (P1 - pressureOffset) <= pressureDeltaDown)
  {
    inDoorOpenLoop = true;

    printLcd(2,"Opening door..."); 
    digitalWrite(light3Pin, HIGH);

    relayOn(relay6Pin, true);  

    P1 = analogRead(sensor1Pin);
    switchDoorState =  digitalRead(switchDoorPin);
    delay(50); //Debounce

    //TO DO: Add timer: if door not open in 2 sec, must be stuck; show error message
  }

  // DOOR OPEN LOOP EXIT ROUTINES
  //=======================================================================

  if(inDoorOpenLoop)
  {
    printLcd(2,""); 
    relayOn(relay6Pin, false);
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

  while((button3State == LOW || autoMode_1 == true) && switchDoorState == HIGH && (P1 - pressureOffset) <= pressureDeltaDown)
  {
    inPlatformLowerLoop = true;
    relayOn(relay4Pin, false); // Finally can close platform up solenoid
    printLcd(2, "Lowering platform..."); 
    
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
      relayOn(relay3Pin, true);  // May as well leave this open? YES. Liquid is still outgassing.
      relayOn(relay5Pin, true);  // Open cylinder exhaust
      digitalWrite(light3Pin, HIGH); 
    }

    P1 = analogRead(sensor1Pin);
    button3State = !digitalRead(button3Pin);
    switchDoorState =  digitalRead(switchDoorPin);
    delay(50);
  }

  // PLATFORM LOWER LOOP EXIT ROUTINES
  // ===========================================================================================
  
  if(inPlatformLowerLoop)
  {
    //close platform release 
    printLcd(2,"");
    relayOn(relay3Pin, false);
    relayOn(relay5Pin, false);
    relayOn(relay6Pin, false); //Release door solenoid
    digitalWrite(light3Pin, LOW); //TOTC

    inPlatformLowerLoop = false;   
    platformStateUp = false;

    //Prepare for next cycle
    printLcd(0, "Insert bottle,");
    printLcd(1, "B1 raises platform");

    //Add the following to the platform UP routine, to get a more current value of the startPressure
    P1 = analogRead(sensor1Pin); //If we've lowered the platform, assume we've gone through a cycle and store the startPressure again
    P2 = analogRead(sensor2Pin); 
    
    float startPressurePSI = pressureConv(P2); 
    String (convPSI) = floatToString(buffer, startPressurePSI, 1);
    String (outputPSI) = "Reg. Press: " + convPSI + " psi";
    printLcd(3, outputPSI); 
    
    digitalWrite(light1Pin, LOW); 
  } 
  
  // END PLAFORM LOWER LOOP
  //=====================================================================================================
}

//=======================================================================================================
// END MAIN LOOP ========================================================================================
//=======================================================================================================


/*
// ======================================================================================================
// CODE FRAGMENTS 
// ======================================================================================================
  

/* THIS FUNCTION ADDED BY MATT
int ReadButton(const int buttonId)
{
if (buttonId == button3Pin)
{
!digitaRead(buttonId);
}
else
{
digitalRead(buttonId);
}
}
//example:
//button1State = ReadButton(button1Pin);
*/

/* OLD STUCK BOTTLE ROUTINE
// Timing routine to sample pressure difference every 1000ms, and compare with previous reading
unsigned long T1 = millis(); 
PTest1 = startPressure; //Assign PTest1 smallest possible value--i.e., the very first reading--to start
int PTest2 = analogRead(sensor1Pin);   
      
//Continue to depressurize if PTest2 less than pressureNull OR pressure is drrpping by specified # of units per interval
//First condition prevents release of bottle if user closes needle valve, thus preventing venting, but bottle still high P
while ((PTest2 < pressureNull) || (PTest2 >= PTest1 + 2)) 
{ 
  //Every 1000ms, make PTest1 equal to current PTest2 and get new pressure for PTest2
  if (millis() > T1 + 1000)
  {
    T1 = millis(); //Make T1 equal new start time
    PTest1 = PTest2; 
    PTest2 = analogRead(sensor1Pin);
  }  
}
*/

/*
Serial.print ("BEFORE DEPRESS LOOP : B3 State= ");
Serial.print (button3State);
Serial.print ("; B3 toggleState= ");
Serial.print (button3ToggleState);
Serial.print ("; switchFillState=  ");
Serial.print (switchFillState);
Serial.println("");
*/
     
