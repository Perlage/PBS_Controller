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
String (versionSoftwareTag) = "9ca6547" ; //Current version of controller
String (versionHardwareTag) = "v0.4.0"  ; //Addition of safety door rev'd PBS from 0.4.0 to 0.5.0

//Library includes
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <math.h> //pressureOffset
#include "floatToString.h"

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
const int sensor1Pin    = A0;     // pin for preasure sensor1 
const int sensor2Pin    = A1;     // pin for pressure sensor2 ADDED 10/29
//                        A2;     // future cleaning switch
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

//Pressure variables
int P1 = 0;                         // Current pressure reading from sensor
int startPressure = 0;              // Pressure at the start of filling cycle
int pressureOffset = 35;            // Choose so that with cylinder off and IN and OUT tubes open, IDLE pressure = 0
int pressureDeltaUp = 10;           // Pressure at which, during pressurization, full pressure is considered to have been reached //1-12 was 50 //TO DO: something odd here; needle valve corroding??
int pressureDeltaDown = 40;         // Pressure at which, during depressurizing, pressure considered to be close enough to zero
int pressureDeltaAutotamp = 250;    // This basically gives a margin to ensure that S1 can never open without pressurized bottle
int pressureNull = 450;             // This is the threshold for the controller deciding that no gas source is attached. 
float PSI = 0;                      // In PSI
float startPressurePSI = 0;         // In PSI
float bottlePressurePSI = 0;        // In PSI

//State variables set in loops
boolean inPlatformLoop               = false;
boolean inFillLoop                   = false;
boolean inPressurizeLoop             = false;
boolean inDepressurizeLoop           = false;
boolean inPlatformLowerLoop          = false;
boolean inPressureNullLoop           = false;
boolean inOverFillLoop               = false;
boolean inPurgeLoop                  = false;
boolean inPurgeLoopInterlock         = false;
boolean inDoorOpenLoop               = false;

//variables for platform function and timing
int timePlatformInit;                  // Time in ms going into loop
int timePlatformCurrent;               // Time in ms currently in loop
int timePlatformRising = 0;            // Time diffence between Init and Current
int timePlatformLock = 1250;           // Time in ms before platform locks in up position
int autoPlatformDropDuration = 1500;   // Duration of platform autodrop in ms

//Key performance parameters
int autoSiphonDuration = 2500;         // Duration of autosiphon function in ms
int foamDetectionDelay = 2000;         // Amount of time to pause after foam detection
int pausePlatformDrop  = 1000;          // Pause before platform drops after door automatically opens

//Key logical states
boolean autoMode_1 = false;                // Variable to help differentiate between states reached automatically vs manually 
boolean platformLockedNew = false;         // Variable to be set when platfrom first locks up. This is for autofill-on-doorclose function
int platformState = LOW;                   // HIGH means platform locked in UP; toggled after S5 opens
boolean FLAG_firstPass = true;             // Variable to set for flagging first pass
boolean FLAG_noRepressureOnResume = false; // Variable to prevent repressurization on resuming filling after stopping with button press
 
//Variables for button toggle states 
int button2StateTEMP = HIGH;
int button3StateTEMP = HIGH;
boolean button2ToggleState = false;
boolean button3ToggleState = false;

char buffer[25]; 

// Declare functions
// =====================================================================

// FUNCTION: To simplify string output
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

// FUNCTION: Allows relay states to be easily be changed from HI=on to LOW=on
void relayOn(int pinNum, boolean on){
  if(on){
    digitalWrite(pinNum, LOW); //turn relay on
  }
  else{
    digitalWrite(pinNum, HIGH); //turn relay off
  }
}

// FUNCTION: Routine to convert pressure from parts in 1024 to psi
float pressureConv(int P1) {
  // Subtract actual offset from P1 first before conversion:
  // pressurePSI = (((P1 - pressureOffset) * 0.0048828)/0.009) * 0.145037738; //This was original equation
  // 1 PSI = 12.7084 units; 1 unit = 0.078688 PSI
  float pressurePSI;
  pressurePSI = (P1 - pressureOffset) * 0.078688; 
  return pressurePSI;
}

//=======================================================================================
// SETUP LOOP
//=======================================================================================

void setup()
{
  
  //Setup pins
  pinMode(button1Pin, INPUT);  //Changed buttons 1,2,3 from PULLUP to regular when starting to use touchbuttons
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
  pinMode(buzzerPin, OUTPUT);
  
  //set all relay pins to high which is "off" for this relay
  digitalWrite(relay1Pin, HIGH);
  digitalWrite(relay2Pin, HIGH);
  digitalWrite(relay3Pin, HIGH);
  digitalWrite(relay4Pin, HIGH);   //TO DO: Try setting this pin to LOW initially to combat popping with pressurized bottle on bootup
  digitalWrite(relay5Pin, HIGH); 
  digitalWrite(relay6Pin, HIGH);
  
  //set to HIGH which "open" for touchbuttons
  digitalWrite(button1Pin, HIGH); 
  digitalWrite(button2Pin, HIGH);
  digitalWrite(button3Pin, HIGH);
  
  // Startup behavior
  //===================================================================================

  Serial.begin(9600); // TO DO: remove when done
  relayOn(relay3Pin, true); // Open at earliest possibility to vent
    
  //setup LCD (TO DO: still needed?
  lcd.init();
  lcd.backlight();
  
  //Initial user message
  printLcd (0, "Perlini Bottling");
  printLcd (1, "System, " + versionSoftwareTag);
  printLcd (2, "");
  printLcd (3, "Initializing...");
  
  for (int n = 0; n < 2; n++)
  {
    digitalWrite(light1Pin, HIGH);
    digitalWrite(light2Pin, HIGH);
    digitalWrite(light3Pin, HIGH);
    delay(500);
    digitalWrite(light1Pin, LOW);
    digitalWrite(light2Pin, LOW);
    digitalWrite(light3Pin, LOW);
    delay(500);
  }

  // Initial pressure difference reading from sensor. High = unpressurized bottle
  //=============================================================================
  
  P1 = analogRead(sensor1Pin); 
  startPressure = P1;
  startPressurePSI = pressureConv(startPressure);
  
  // This is the starting pressure
  String (convPSI) = floatToString(buffer, startPressurePSI, 1);
  String (outputPSI) = "Pressure: " + convPSI + " psi";
  printLcd(3, outputPSI); 
  
  //============================================================================
  // NULL PRESSURE LOOP
  // Check for stuck pressurized bottle or implausibly low gas pressure at start 
  //============================================================================
  
  while ( P1 < pressureNull )
  {
    inPressureNullLoop = true;
    
    relayOn(relay3Pin, true); //Open bottle vent
    relayOn(relay4Pin, true); //Keep platform up while depressurizing
    
    printLcd(0, "Bottle pressurized");
    printLcd(1, "Or gas pressure low");
    printLcd(2, "Fix or let de-gas...");
    
    P1 = analogRead(sensor1Pin); 
    
    bottlePressurePSI = pressureConv(P1);
    String (convPSI) = floatToString(buffer, bottlePressurePSI, 1);
    String (outputPSI) = "Press. diff: " + convPSI + "psi";  //TO DO: Make this Pressure DIFF
    printLcd(3, outputPSI);     
    }  
  
  // NULL PRESSURE EXIT ROUTINES
  //================================
  
  if (inPressureNullLoop)
  {
    relayOn(relay6Pin, true);  //Open door
    delay(500);
    relayOn(relay6Pin, false);
    relayOn(relay4Pin, false); //turn off platform support
    relayOn(relay5Pin, true);  //drop platform
    
    P1 = analogRead(sensor1Pin); 
    startPressure = P1;  
    String output = "Pressure NEW: " + String(startPressure); //TO DO: this is never reported...
    inPressureNullLoop = false;
    
    printLcd(0, "");
    printLcd(1, "");
    printLcd(2, "");
  }
  
  //END NULL PRESSURE LOOP
  //====================================================================================
 
  // Open door if closed
  if(switchDoorState == LOW)
  {
    relayOn(relay6Pin, true);
    delay(500);
    relayOn(relay6Pin, false);
  }
  
  //If platform is up, drop it
  relayOn(relay5Pin, true);
  
  // User instructions
  printLcd(0, "Insert bottle,");
  printLcd(1, "B1 raises platform");
}

//====================================================================================================================================
// MAIN LOOP =========================================================================================================================
//====================================================================================================================================

void loop()
{
  
  //MAIN LOOP IDLE FUNCTIONS
  //=====================================================================
  
  // Read the state of buttons and sensors
  // Boolean NOT (!) added for touchbuttons
  button1State =     !digitalRead(button1Pin); 
  button2StateTEMP = !digitalRead(button2Pin); 
  button3StateTEMP = !digitalRead(button3Pin); 
  switchFillState =   digitalRead(switchFillPin);
  switchDoorState =   digitalRead(switchDoorPin);
  delay(50);  

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
  
  // TO DO: REMOVE THIS ######################################
  Serial.print("B2 toggle loop: ");
  Serial.print ("B2 State= ");
  Serial.print (button2State);
  Serial.print ("; toggleState= ");
  Serial.print (button2ToggleState);
  Serial.println("");
    // TO DO: REMOVE THIS
  Serial.print("B3 toggle loop: ");
  Serial.print ("B3 State= ");
  Serial.print (button3State);
  Serial.print ("; toggleState= ");
  Serial.print (button3ToggleState);
  Serial.println("");

   
  // Main Loop pressure measurement
  if (FLAG_firstPass == true) // Will only get continuous measurement IF no button has been pressed
  {
    int pressureIdle;  
    float pressureIdlePSI;
    
    pressureIdle = analogRead(sensor1Pin); 
      
    pressureIdlePSI = pressureConv(pressureIdle); 
    String (convPSI) = floatToString(buffer, pressureIdlePSI, 1);
    String (outputPSI) = "Reg. Press: " + convPSI + " psi";
    printLcd(3, outputPSI); 
  }

  // =====================================================================================  
  // PLATFORM RAISING LOOP
  // while B1 is pressed, Raise bottle platform.
  // Conditions look for no bottle or bottle unpressurized, and door must be open. Added pressure condition 
  // to make sure this button couldn't accidentally lower platform when pressurized
  // =====================================================================================  
  
  timePlatformInit = millis(); // Inititalize time for platform lockin routine
  
  while (button1State == LOW && platformState == LOW && switchDoorState == HIGH && (timePlatformRising < timePlatformLock) && (P1 >= startPressure - pressureDeltaUp)) 
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
    FLAG_firstPass = false; // Loop lost virginity
  }  

  // PLATFORM RAISING LOOP EXIT ROUTINES
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
      platformState = HIGH;
      platformLockedNew = true; //Pass this to PressureLoop for autopressurize on door close--better than trying to pass button2State = LOW, which causes problems
      
      // Read new starting pressure 
      // This used to be in the platform DOWN exit loop
      P1 = analogRead(sensor1Pin); //If we've lowered the platform, assume we've gone through a cycle and store the startPressure again
      startPressure = P1;
      
      startPressurePSI = pressureConv(P1); 
      String (convPSI) = floatToString(buffer, startPressurePSI, 1);
      String (outputPSI) = "Reg. Press: " + convPSI + " psi";
      printLcd(3, outputPSI); 
      
    }  
    else    
    {
      // Drop Platform
      relayOn(relay4Pin, false);
      relayOn(relay5Pin, true);
      digitalWrite(light1Pin, LOW); 
      platformState = LOW;
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
  // Door must be closed, platform up, and pressure at zero
  //TO DO: switch B1 and B2; toggle B1 on
  
  while(button2State == LOW && switchDoorState == HIGH && (P1 >= pressureOffset + pressureDeltaUp) && platformState == LOW)
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
  
  int P1Init = P1;
  int PTest = .5 * P1Init; //if after an interval bottle isn't 25% pressurized, exit loop
  int PTestFail = false;
  int pressurizeDuration = 0;
  int pressurizeCurrentTime = 0;
  int pressurizeStartTime = millis();
  
  while((button2State == LOW || platformLockedNew == true) && switchDoorState == LOW && platformState == HIGH && FLAG_noRepressureOnResume == false && (P1 >= pressureOffset + pressureDeltaUp))
  { 
    inPressurizeLoop = true;

    if (platformLockedNew == true)
    {
      delay(500);                   //Make a slight delay in starting pressuriztion when door is first closed
      platformLockedNew = false;    //This is a "first pass" variable; reset to false to indicate that this is no longer the first pass
    }
      
    digitalWrite(light2Pin, HIGH);  //Light button when button state is low
    relayOn(relay3Pin, true);       //close S3 if not already
    relayOn(relay2Pin, true);       //open S2 to pressurize
      
    printLcd(2,"Pressurizing..."); 
    bottlePressurePSI = startPressurePSI - pressureConv(P1);
    String (convPSI) = floatToString(buffer, bottlePressurePSI, 1);
    String (outputPSI) = "Bottle Press: " + convPSI;
    printLcd(3, outputPSI);      
    
    //Test to see if bottle is pressurizing
    pressurizeCurrentTime = millis();
    pressurizeDuration = pressurizeCurrentTime - pressurizeStartTime;
    
    if (pressurizeDuration > 1500 && P1 > PTest){
      button2State = HIGH; 
      PTestFail = true;
    }

    switchDoorState = digitalRead(switchDoorPin); //Check door switch    
    P1 = analogRead(sensor1Pin);
   
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
  }
  
  // PRESSURIZE LOOP EXIT ROUTINES
  //===================================
  
  if(inPressurizeLoop){
    relayOn(relay2Pin, false);    //close S2 because we left PressureLoop
    digitalWrite(light2Pin, LOW); //Turn off B2 light
    inPressurizeLoop = false;
    
    //PRESSURE TEST EXIT ROUTINE
    if (PTestFail == true)
    {
      printLcd (2, "No bottle, or leak");
      digitalWrite(light1Pin, LOW); 
      delay (3000);
      PTestFail = false;      
      timePlatformRising = 0;
      //TO DO: Make platform drop
    }
    
    if (switchDoorState == HIGH){ //pressureDeltaDown
      goto LABEL_inEmergencyDepressurizeLoop;
    }  
      
    printLcd(2,""); 
  }

  // END PRESSURIZE LOOP
  //========================================================================================
  
  //========================================================================================
  // EMERGENCY DEPRESSURIZE LOOP
  // Depressurize anytime door opens when pressurized above low threshold
  //========================================================================================

  LABEL_inEmergencyDepressurizeLoop:    
  int inEmergencyDepressurizeLoop = false;
  
  while (switchDoorState == HIGH && (P1 <= startPressure - pressureDeltaDown))
  {
    inEmergencyDepressurizeLoop = true;
    relayOn(relay3Pin, true);  
    printLcd (2, "Close door...");
    
    float pressurePSI = startPressurePSI - pressureConv(P1);
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
    button2State = LOW;
    inEmergencyDepressurizeLoop = false;
  }  
  
  // END EMERGENCY DEPRESSURIZE LOOP
  //========================================================================================

  //========================================================================================
  // FILL LOOP
  //========================================================================================
  
  pinMode(switchFillPin, INPUT_PULLUP); //Probably no longer necessary since FillSwitch was moved off Pin13 (Zach proposed this Oct-7)
    
  while(button2State == LOW && switchFillState == HIGH && switchDoorState == LOW && P1 < pressureOffset + pressureDeltaUp + pressureDeltaAutotamp) 
  { 
    inFillLoop = true;
    FLAG_noRepressureOnResume = false;

    digitalWrite(light2Pin, HIGH); //Light B2
    printLcd(2,"Filling..."); 
    
    //while the button is  pressed, fill the bottle: open S1 and S3
    relayOn(relay1Pin, true);
    relayOn(relay3Pin, true);

    //Check toggle state of B2    
    button2StateTEMP = !digitalRead(button2Pin);
    delay(25);
    if (button2StateTEMP == HIGH && button2ToggleState == false){  // ON release
      button2ToggleState = true; //Leaves buttonState LOW
      button2State = LOW;        //Or makes it low
    }
    if (button2StateTEMP == LOW && button2ToggleState == true){ // OFF push
      button2State = HIGH; //exit WHILE loop
    }
    
    // TO DO: REMOVE THIS
    Serial.print("B3 toggle loop: ");
    Serial.print ("B2 State= ");
    Serial.print (button2State);
    Serial.print ("; toggleState= ");
    Serial.print (button2ToggleState);
    Serial.println("");
    
    //Check pressure
    P1 = analogRead(sensor1Pin);
    float pressurePSI = pressureConv(P1);
    String (convPSI) = floatToString(buffer, pressurePSI, 1);
    String (outputPSI) = "Press diff: " + convPSI + " psi";
    printLcd(3, outputPSI); 
    
    //Read sensors
    pinMode(switchFillPin, INPUT_PULLUP); // TO DO: See above comment above...is this still needed?
    switchFillState = digitalRead(switchFillPin); //Check fill sensor
    switchDoorState = digitalRead(switchDoorPin); //Check door switch    
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
    
    // CASE 1: Button2 released
    if (button2State == HIGH)
    {
      FLAG_noRepressureOnResume = true;   
    }
    
    // CASE 2: FillSwitch tripped--Overfill condition
    else if (inFillLoop && switchFillState == LOW) 
    {
      digitalWrite(light2Pin, HIGH); 
      relayOn(relay1Pin, true);
      relayOn(relay2Pin, true);
      
      printLcd(2,"Fixing overfill..."); 
      delay(autoSiphonDuration); // This setting determines duration of autosiphon 
      printLcd(2,"");       
      relayOn(relay1Pin, false);
      relayOn(relay2Pin, false);
      digitalWrite(light2Pin, LOW); 
      
      switchFillState = digitalRead(switchFillPin); 
      delay (50);
      button3State = LOW; // This make AUTO-depressurize after overfill
    }
    // CASE 3: Bottles pressure dropped too much
    else 
    {
      //TO DO: Can we just GOTO Pressure Loop?
      //=========================
      // REPRESSURIZE LOOP
      //=========================
      while(P1 >= pressureOffset + pressureDeltaUp)
      {
        inPressurizeLoop = true;
        digitalWrite(light2Pin, HIGH); 
        relayOn(relay2Pin, true);       //open S2 to equalize
        P1 = analogRead(sensor1Pin);    //do sensor read again to check
        printLcd(2,"Repressurizing...");
      }
      
      // REPRESSURIZE LOOP EXIT ROUTINES
      //================================
      if(inPressurizeLoop)
      { 
        relayOn(relay2Pin, false);  //close S2 because the preasures are equal
        digitalWrite(light2Pin, LOW);
        printLcd(2,"");
        inPressurizeLoop = false; 
      } 
      // END REPRESSUIZE LOOP   
      //===========================     
    }  

    inFillLoop = false;
    printLcd(0, "B2 toggles fill;");  
    printLcd(1, "B3 toggles exhaust");  
  }

  //================================================================================
  // DEPRESSURIZE LOOP
  //================================================================================

  LABEL_inDepressurizeLoop:
  while(button3State == LOW && switchFillState == HIGH && P1 <= startPressure - pressureDeltaDown){  // Don't need to take into account offset, as it is in both P1 and Startpressure
    inDepressurizeLoop = true;
    digitalWrite(light3Pin, HIGH);

    relayOn(relay3Pin, true); //Open Gas Out solenoid
    
    printLcd(2, "Depressurizing...");  
    float pressurePSI = startPressurePSI - pressureConv(P1);
    String (convPSI) = floatToString(buffer, pressurePSI, 1);
    String (outputPSI) = "Bottle Press: " + convPSI;
    printLcd(3, outputPSI); 
    
    //Allow momentary "burst" foam tamping
    //TO DO: should this be a while statement to make continuous?
    button2State = !digitalRead(button2Pin);
      if(button2State == LOW)
      {
        digitalWrite(light2Pin, HIGH); 
        relayOn(relay2Pin, true);
        delay(50);
        relayOn(relay2Pin, false);
        digitalWrite(light2Pin, LOW);
      }

    P1 = analogRead(sensor1Pin);
    switchFillState = digitalRead(switchFillPin); 

    //Check toggle state of B3
    button3StateTEMP = !digitalRead(button3Pin);
    delay(25);
    if (button3StateTEMP == HIGH && button3ToggleState == false){  // ON release
      button3ToggleState = true;                                   //Leaves buttonState LOW
      button3State = LOW; 
    }
    if (button3StateTEMP == LOW && button3ToggleState == true){    // OFF push
      button3State = HIGH;                                         //exit WHILE loop
    }
  }

  // DEPRESSURIZE LOOP EXIT ROUTINES 
  // If in this loop, Button3 was released, or Fill/Foam Sensor tripped, 
  // or bottle is propely pressurized. Find out which reason and repond.
  //========================================================================
  
  if(inDepressurizeLoop)
  { 
    printLcd(2, "");
    relayOn(relay3Pin, false);     //Close bottle exhaust relay
    
    // CASE 1: Button3 released
    if (button3State == HIGH)  
    { 
      //Used to repressurize here; now we don't  
      //TO DO: either a quick burst to clear the sensor, or check for overfill and tamp in case bottle foams up
    }
    
    // CASE 2: Foam tripped sensor
    if (switchFillState == LOW)
    {
      printLcd(2, "Foam... wait");
      relayOn(relay2Pin, true);
      delay(foamDetectionDelay); // Wait a bit before proceeding    

      relayOn(relay2Pin, false);
      switchFillState = HIGH;
      printLcd(2, "");
      //TO DO: we seem to have ended up with auto-resume of depressurization with new toggle routine. Not sure if want this?
    }

    //CASE 3: Bottle was properly depressurized. If we reach here, the pressure must have reached threshold. Go to Platform lower loop
    if (P1 > startPressure - pressureDeltaDown)
    {
      digitalWrite(buzzerPin, HIGH); 
      delay(100);
      digitalWrite(buzzerPin, LOW);     
      autoMode_1 = true;  //Going to platform loop automatically, so set this var to partially drop platform 
    }
    
  digitalWrite(light3Pin, LOW);
  inDepressurizeLoop = false;


  // TO DO: REMOVE THIS #################################################
  Serial.print("End Depress Loop: ");
  Serial.print ("B3 State= ");
  Serial.print (button3State);
  Serial.print ("; toggleState= ");
  Serial.print (button3ToggleState);
  Serial.println("");  

  }
  
  // END DEPRESSURIZE LOOP
  //=======================================================================

  // ======================================================================
  // DOOR OPENING LOOP
  // ======================================================================
 
  // TO DO: Need to add opening door loop, to accomodate door getting blocked or sticking
  // Only activate door solenoid if door is already closed
  
  while((button3State == LOW || autoMode_1 == true) && switchDoorState == LOW && P1 > startPressure - pressureDeltaDown)
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
    button3State = HIGH; //TO DO: soleved one problem but caused another
    
    // TO DO: REMOVE THIS #################################################
    Serial.print("End Door Loop: ");
    Serial.print ("B3 State= ");
    Serial.print (button3State);
    Serial.print ("; toggleState= ");
    Serial.print (button3ToggleState);
    Serial.print ("; platformState= "); 
    Serial.print (platformState); 
    Serial.println(""); 

  }
  
  // END DOOR OPEN LOOP
  //======================================================================= 
    
  // ===========================================================================================
  // PLATFORM LOWER LOOP 
  // Platform will not lower with door closed. This prevents someone from defeating door switch
  // ===========================================================================================

  LABEL_PlatformLowerLoop:
  while((button3State == LOW || autoMode_1 == true) && switchDoorState == HIGH && P1 > startPressure - pressureDeltaDown)
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
  // ======================================
  
  if(inPlatformLowerLoop)
  {
    //close platform release 
    printLcd(2,"");
    relayOn(relay3Pin, false);
    relayOn(relay5Pin, false);
    relayOn(relay6Pin, false); //Release door solenoid
    digitalWrite(light3Pin, LOW); //TOTC

    inPlatformLowerLoop = false;   
    platformState = LOW;

    //Prepare for next cycle
    printLcd(0, "Insert bottle,");
    printLcd(1, "B1 raises platform");

    //Going to add the following to the platform UP routine, to get a more current value of the startPressure
    P1 = analogRead(sensor1Pin); //If we've lowered the platform, assume we've gone through a cycle and store the startPressure again
    startPressure = P1;
    
    startPressurePSI = pressureConv(P1); 
    String (convPSI) = floatToString(buffer, startPressurePSI, 1);
    String (outputPSI) = "Reg. Press: " + convPSI + " psi";
    printLcd(3, outputPSI); 
    
    digitalWrite(light1Pin, LOW); //TOTC
  } 
  
  // END PLAFORM LOWER LOOP
  //====================================================================
}

//=======================================================================================================
// END MAIN LOOP
//=======================================================================================================

/*
// CODE FRAGMENTS 
//================
 
/*  
Serial.print("Current Pressure: ");
Serial.print (P1);
Serial.print (" units");
Serial.println("");
*/  

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

/*
//This is for show--cycle platform
delay(500);
relayOn(relay4Pin, true);
delay(1000);
relayOn(relay4Pin, false);
delay(500);
relayOn(relay5Pin, true);
delay(2000);  
relayOn(relay5Pin, false);  
*/

/*
Serial.print("Starting pressure: ");
Serial.print (startPressurePSI);
Serial.print (" psi");
Serial.println("");

/* COMMENTED OUT THIS CODE TO PREVENT AUTOMATIC REPRESSURIZING -- MAY WANT TO DO PARTIAL REPRESSURIZE?
printLcd(2,"Repressurizing...");
while(P1 >= pressureOffset + pressureDeltaUp)
{
relayOn(relay2Pin, true);         //open S2 to repressurize
//digitalWrite(light2Pin, HIGH); //TOTC

P1 = analogRead(sensor1Pin);
}
relayOn(relay2Pin, false); 
//digitalWrite(light2Pin, LOW); //TOTC

*/





