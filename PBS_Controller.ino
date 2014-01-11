/*
**************************************************************************  
Perlini Bottling System
Arduino MICRO Controller

Author: 
  Evan Wallace
  Perlage Systems, Inc.
  
**************************************************************************  
*/

String (versionSoftwareTag) = "TEMP" ;
String (versionHardwareTag) = "v0.4.1"   ;

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <math.h> //pressureOffset
#include "floatToString.h"

LiquidCrystal_I2C lcd(0x3F,20,4);  

// ROW 1 ARDUINO MICRO
const int button1Pin    =  0;     // pin for button 1 B1 (Raise platform) RX=0; B1 on PBS is B3 on Touchsensor 
const int button2Pin    =  1;     // pin for button 2 B2 (Fill/purge)     TX=1; B2 on PBS is B2 on Touchsensor
// 2 SDA for LCD           2;  
// 3 SDL for LCD           3;
const int button3Pin    =  4;     // pin for button 3 B3 (Depressurize and lower) B3 on PBS is B1 on Touchsensor
const int relay1Pin     =  5;     // pin for relay1 S1 (liquid fill)
const int relay2Pin     =  6;     // pin for relay2 S2 (bottle gas)
const int relay3Pin     =  7;     // pin for relay3 S3 (bottle gas vent)
const int relay4Pin     =  8;     // pin for relay4 S4 (gas in lift)
const int relay5Pin     =  9;     // pin for relay5 S5 (gas out lift)
const int relay6Pin     = 10;     // pin for relay6 S6 (door lock solenoid) //Oct 18
const int switchFillPin = 11;     // pin for fill switchFill F1 // 10/7 Zack changed from 13 to 7 because of shared LED issue on 13. Light3Pin put on 13
const int switchDoorPin = 12;     // pin for finger switch

// ROW 2 ARDUINO MICRO
const int buzzerPin     = 13;     // pin for buzzer
const int sensor1Pin    = A0;     // pin for preasure sensor1 P1
const int sensor2Pin    = A1;     // pin for pressure sensor2 P2 ADDED 10/29
//                        A2;     // future cleaning switch
const int light1Pin     = A3;     // pin for button 1 light 
const int light2Pin     = A4;     // pin for button 2 light
const int light3Pin     = A5;     // pin for button 3 light


// Inititialize states
int button1State = HIGH;
int button2State = HIGH; 
int button3State = HIGH;

//button toggle states ####################################################################
int button2StateTEMP = HIGH;
int button3StateTEMP = HIGH;
boolean button2ToggleState = false;
boolean button3ToggleState = false;

int light1State = LOW;
int light2State = LOW;
int light3State = LOW;
int switchFillState = HIGH; 
int switchDoorState = HIGH;
int platformState = LOW; //LOW=Low

int relay1State = HIGH;
int relay2State = HIGH;
int relay3State = HIGH;
int relay4State = HIGH;
int relay5State = HIGH;
int relay6State = HIGH;

// Initialize variables
int P1 = 0;                         // Current pressure reading from sensor
int startPressure = 0;              // Pressure at the start of filling cycle
int pressureOffset = 55;            // changed from 37 t0 60 w new P sensor based on 0 psi measure
int pressureDeltaUp = 50;           // changed from 20 to 50 OCT 23 NOW WORKS
int pressureDeltaDown = 40;  
int pressureDeltaAutotamp = 250;    // This basically gives a margin to ensure that S1 can never open without pressurized bottle
int pressureNull = 450;             // This is the threshold for the controller deciding that no gas source is attached. 
float PSI = 0;
float startPressurePSI = 0;
float bottlePressurePSI = 0;

boolean inPlatformLoop = false;
boolean inFillLoop = false;
boolean inPressurizeLoop = false;
boolean inDepressurizeLoop = false;
boolean inPlatformLowerLoop = false;
boolean inPressureNullLoop = false;
boolean inOverFillLoop = false;
boolean inPurgeLoop = false;
boolean inDoorOpenLoop = false;

int timePlatformInit;        // Time in ms going into loop
int timePlatformCurrent;     // Time in ms currently in loop
int timePlatformRising = 0;  // Diffence between Init and Current
int timePlatformLock = 1250; // Time in ms before platform locks in up position
int autoSiphonDuration = 2500; //Duration of autosiphon function in ms
int autoPlatformDropDuration = 1500; //Duration of platform autodrop in ms

boolean platformLockedNew = false; // variable to be set when platfrom first locks up. This is for autofill-on-doorclose function
boolean autoMode_1 = false; //Variable to help differentiate between states reached automatically vs manually 

char buffer[25]; // TO DO: still needed?

//FUNCTION: To simplify string output
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

// FUNCTION: This was added so that the relay states could easily be changed from HI to LOW
void relayOn(int pinNum, boolean on)
{
  if(on){
    digitalWrite(pinNum, LOW); //turn relay on
  }
  else{
    digitalWrite(pinNum, HIGH); //turn relay off
  }
}

// FUNCTION: Routine to convert pressure from parts in 1024 to psi
float pressureConv(int P1) 
{
  float pressurePSI;
  // Subtract actual offset from P1 first before conversion:
  // pressurePSI = (((P1 - pressureOffset) * 0.0048828)/0.009) * 0.145037738; //This was original equation
  pressurePSI = (P1 - pressureOffset) * 0.078688; 
  return pressurePSI;
}

//***************************************************************************************
// SETUP LOOP
//***************************************************************************************

void setup()
{
  //setup LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); 
  lcd.print("Perlini Bottling");
  lcd.setCursor(0,1);
  lcd.print("System, " + versionSoftwareTag);
  delay(1000);  
  lcd.setCursor(0,3);
  lcd.print("Initializing...");

  Serial.begin(9600); // TO DO: remove
  
  //setup pins
  pinMode(relay1Pin, OUTPUT);      
  pinMode(relay2Pin, OUTPUT);
  pinMode(relay3Pin, OUTPUT);      
  pinMode(relay4Pin, OUTPUT);
  pinMode(relay5Pin, OUTPUT);  
  pinMode(relay6Pin, OUTPUT);  
  
  pinMode(button1Pin, INPUT);  //Changed buttons 1,2,3 from PULLUP  to regular when starting to use touchbuttons
  pinMode(button2Pin, INPUT);
  pinMode(button3Pin, INPUT);  
  pinMode(light1Pin, OUTPUT); 
  pinMode(light2Pin, OUTPUT);
  pinMode(light3Pin, OUTPUT);

  pinMode(switchFillPin, INPUT_PULLUP); 
  pinMode(switchDoorPin, INPUT_PULLUP); 
  pinMode(buzzerPin, OUTPUT);
  
  //set all pins to high which is "off" for this controller
  digitalWrite(relay1Pin, HIGH);
  digitalWrite(relay2Pin, HIGH);
  digitalWrite(relay3Pin, HIGH);
  digitalWrite(relay4Pin, HIGH);
  digitalWrite(relay5Pin, HIGH);
  digitalWrite(relay6Pin, HIGH);
  
  digitalWrite(button1Pin, HIGH); //This is new 
  digitalWrite(button2Pin, HIGH);
  digitalWrite(button3Pin, HIGH);
  
  // Flash lights
  digitalWrite(light1Pin, HIGH);
  digitalWrite(light2Pin, HIGH);
  digitalWrite(light3Pin, HIGH);
  delay(500);
  digitalWrite(light1Pin, LOW);
  digitalWrite(light2Pin, LOW);
  digitalWrite(light3Pin, LOW);
  delay(500);
  digitalWrite(light1Pin, HIGH);
  digitalWrite(light2Pin, HIGH);
  digitalWrite(light3Pin, HIGH);
  delay(500);
  digitalWrite(light1Pin, LOW);
  digitalWrite(light2Pin, LOW);
  digitalWrite(light3Pin, LOW); 

  P1 = analogRead(sensor1Pin); // Initial pressure difference reading from sensor. High = unpressurized bottle
  startPressure = P1;  
  startPressurePSI = pressureConv(P1);
  
  // Float to string conversion (had a lot of trouble with this
  String (convPSI) = floatToString(buffer, startPressurePSI, 1);
  String (outputPSI) = "Pressure: " + convPSI + " psi";
  printLcd(3, outputPSI); 
  
  //========================================================
  // Check for stuck pressurized bottle or implausibly low gas pressure at start 
  //=========================================================
  
  while ( P1 < pressureNull )
  {
    inPressureNullLoop = true;
    
    relayOn(relay3Pin, true); //Open bottle vent
    relayOn(relay4Pin, true); //Keep platform up while depressurizing
    
    printLcd(0, "Bottle pressurized");
    printLcd(1, "Or gas pressure low");
    printLcd(2, "Fix or wait...");
    
    P1 = analogRead(sensor1Pin); 
    
    bottlePressurePSI = pressureConv(P1);
    String (convPSI) = floatToString(buffer, bottlePressurePSI, 1);
    String (outputPSI) = "Bottle pressure: " + convPSI; //TO DO: Make this Pressure DIFF
    printLcd(3, outputPSI);       
  }  
  
  if (inPressureNullLoop)
  {
    relayOn(relay6Pin, true); //Open door
    delay(500);
    relayOn(relay6Pin, false);
    relayOn(relay4Pin, false); //turn off platform support
    relayOn(relay5Pin, true); //drop platform
    
    P1 = analogRead(sensor1Pin); 
    startPressure = P1;  
    String output = "Pressure NEW: " + String(startPressure); 
    inPressureNullLoop = false;
  }

  // Open door if closed
  if(switchDoorState == LOW)
  {
    relayOn(relay6Pin, true);
    delay(500);
    relayOn(relay6Pin, false);
  }
  
  //If platform is up, drop it. 
  relayOn(relay5Pin, true);
  
  printLcd(0, "Insert bottle,");
  printLcd(1, "B1 raises platform");
}

//***************************************************************************************
// MAIN LOOP
//***************************************************************************************
void loop()
{
  
  // read the state of buttons and sensors
  // Boolean NOT (!) added for touchbuttons
  button1State =     !digitalRead(button1Pin); 
  button2StateTEMP = !digitalRead(button2Pin); 
  button3StateTEMP = !digitalRead(button3Pin); 
  switchFillState =   digitalRead(switchFillPin);
  switchDoorState =   digitalRead(switchDoorPin);
  delay(50);  

//##########################################################################################################################################################################################################

  //Check Button2 toggle state
  if (button2StateTEMP == LOW && button2ToggleState == false){ //ON push
    button2State = LOW;         //goto while loop
  }
  if (button2StateTEMP == LOW && button2ToggleState == true){  //button still being held down after OFF push in while loop. 
    button2State = HIGH;        //nothing happens--buttonState remains HIGH
  } 
  if (button2StateTEMP == HIGH && button2ToggleState == true){  //OFF release
    button2ToggleState = false; //buttonState remains HIGH
  }
  
  //Check Button3 toggle state
  if (button3StateTEMP == LOW && button3ToggleState == false){ //ON push
    button3State = LOW;         //goto while loop
  }
  if (button3StateTEMP == LOW && button3ToggleState == true){  //button still being held down after OFF push in while loop. 
    button3State = HIGH;        //nothing happens--buttonState remains HIGH
  } 
  if (button3StateTEMP == HIGH && button3ToggleState == true){  //OFF release
    button3ToggleState = false; //buttonState remains HIGH
  }  
  
Serial.print("Main Loop: ");
Serial.print ("B2 State= ");
Serial.print (button2State);
Serial.print ("; toggleState= ");
Serial.print (button2ToggleState);
Serial.println("");
  
  
//###############################################################################################################################################################################################################
 
  // **************************************************************************  
  // BUTTON 1 FUNCTIONS 
  // **************************************************************************  

  // PLATFORM RAISING LOOP ****************************************************
  
  timePlatformInit = millis(); // Inititalize time for platform lockin routine
  
  //while B1 is pressed, Raise bottle platform
  while (button1State == LOW && (timePlatformRising < timePlatformLock) && platformState == LOW && (P1 >= startPressure - pressureDeltaUp)) // I.e. no bottle or bottle unpressurized. Added pressure condition to make sure this button doesn't act when bottle is pressurized--else could lower bottle when pressurized
  { 
    inPlatformLoop = true; 
    
    relayOn(relay4Pin, true);     //open S4
    relayOn(relay5Pin, false);
    digitalWrite(light1Pin, HIGH);
    
    //solves bug PBSFIRM-5
    timePlatformCurrent = millis();
    delay(10);
    timePlatformRising = timePlatformCurrent - timePlatformInit;
    
    String platformStatus = ("Raising... " + String(timePlatformRising) + " ms");
    printLcd(2, platformStatus);  

    //see if button is still pressed
    button1State = !digitalRead(button1Pin); 
  }  

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
      digitalWrite(light1Pin, HIGH); 
      platformState = HIGH;
      platformLockedNew = true; //Pass this to PressureLoop for autopressurize on door close--better than trying to pass button2State = LOW, which causes problems
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
    inPlatformLoop = false;
    timePlatformRising = 0;
  }

  // **************************************************************************    
  // BUTTON 2 FUNCTIONS 
  // **************************************************************************

  // PURGE LOOP***********************************************************

  // While B2 and then B1 is pressed, and door is open, and platform is down, purge the bottle
  while(button2State == LOW && switchDoorState == HIGH && (P1 >= pressureOffset + pressureDeltaUp) && platformState == LOW) // Can't purge with door closed
  {
    while(button1State == LOW) // Require two button pushes, B2 then B1, to purge
    { 
      inPurgeLoop = true;
      digitalWrite(light2Pin, HIGH); 
      relayOn(relay2Pin, true); //open S2 to purge
      button1State = !digitalRead(button1Pin); 
      delay(50);  
      printLcd(2,"Purging..."); 
    }
    
    button1State = !digitalRead(button1Pin); 
    button2State = !digitalRead(button2Pin); 
    delay(50);
  }
  if(inPurgeLoop){
    //close S2 when button lifted
    relayOn(relay2Pin, false); //Close relay when B1 not pushed
    digitalWrite(light2Pin, LOW); 

    inPurgeLoop = false;
    printLcd(2,""); 
  }
   
  // PRESSURIZE LOOP***********************************************************

  // Pressurization will start automatically when door closes IF platfromLockedNew is true
  while((button2State == LOW || platformLockedNew == true) && switchDoorState == LOW && (P1 >= pressureOffset + pressureDeltaUp) && platformState == HIGH)
  { 
    inPressurizeLoop = true;
    
    if (platformLockedNew == true)
    {
      delay(500); //Make a slight delay in starting pressuriztion when door is first closed
      platformLockedNew = false; //This is a "first pass" variable; reset to false to indicate that this is no longer the first pass
      //button2State = LOW; //set B2 low REEVALUATE THIS
    }
      
    digitalWrite(light2Pin, LOW); //Light button when button state is low
    relayOn(relay3Pin, true); //close S3 if not already
    relayOn(relay2Pin, true); //open S2 to pressurize
      
    printLcd(2,"Pressurizing..."); 
    bottlePressurePSI = startPressurePSI - pressureConv(P1);
    String (convPSI) = floatToString(buffer, bottlePressurePSI, 1);
    String (outputPSI) = "Bottle Press: " + convPSI;
    printLcd(3, outputPSI);      
   
    //####################################################################################################################
    
    //Check toggle state of B2
    button2StateTEMP = !digitalRead(button2Pin);
    delay(25);
   
    if (button2StateTEMP == HIGH && button2ToggleState == false){  // ON release
      button2ToggleState = true; //leave button state LOW
      button2State = LOW;        //or make it low if it isn't yet
    }
    if (button2StateTEMP == LOW && button2ToggleState == true){ // OFF push
      button2State = HIGH; //exit WHILE loop
    }  
      
    //other two possibilites do nothing (LOW and false, HIGH and true). button2State stays same   
    
    //####################################################################################################################   
    
    switchDoorState = digitalRead(switchDoorPin); //Check door switch    
    P1 = analogRead(sensor1Pin);
    delay(50); //Debounce
  }
  
  //inPressurizeLoop CLEANUP
  if(inPressurizeLoop){
    //close S2 because we left PressureLoop
    relayOn(relay2Pin, false);
    digitalWrite(light2Pin, LOW); //Turn off B2 light
    inPressurizeLoop = false;
    
    //button2State = LOW; //By feeding this LOW buttonState to FillLoop, FillLoop start automatically NEED TO REEVALUATE
    printLcd(2,""); 
  }

  //FILL LOOP ***************************************************
  
  pinMode(switchFillPin, INPUT_PULLUP); //Probably no longer necessary since FillSwitch was moved off Pin13 (Zach proposed this Oct-7)
  
  while(button2State == LOW && switchFillState == HIGH && switchDoorState == LOW && P1 < pressureOffset + pressureDeltaUp + pressureDeltaAutotamp) 
  { 
    pinMode(switchFillPin, INPUT_PULLUP); //See above comment

    inFillLoop = true;
    digitalWrite(light2Pin, HIGH); //Light B2
    
    //while the button is  pressed, fill the bottle: open S1 and S3
    relayOn(relay1Pin, true);
    relayOn(relay3Pin, true);
   
    printLcd(2,"Filling..."); 

    P1 = analogRead(sensor1Pin);
    float pressurePSI = pressureConv(P1);
    String (convPSI) = floatToString(buffer, pressurePSI, 1);
    String (outputPSI) = "Press diff: " + convPSI + " psi";
    printLcd(3, outputPSI); 

    //####################################################################################################################
    
    button2StateTEMP = !digitalRead(button2Pin);
    delay(25);

    if (button2StateTEMP == HIGH && button2ToggleState == false){  // ON release
      button2ToggleState = true; //Leaves buttonState LOW
      button2State = LOW;        //Or makes it low
    }
    if (button2StateTEMP == LOW && button2ToggleState == true){ // OFF push
      button2State = HIGH; //exit WHILE loop
    }
    
    //other two possibilites do nothing (LOW and false, HIGH and true). button2State stays same 
  
Serial.print("B3 toggle loop: ");
Serial.print ("B2 State= ");
Serial.print (button2State);
Serial.print ("; toggleState= ");
Serial.print (button2ToggleState);

Serial.println("");
  
    
    //####################################################################################################################
    
    switchFillState = digitalRead(switchFillPin); //Check fill sensor
    switchDoorState = digitalRead(switchDoorPin); //Check door switch    
  }

  //inFillLoop CLEANUP
  if(inFillLoop)
  {
    //Either B2 was released, or FillSwitch was tripped, or pressure dipped too much -- repressurize in all three cases
    digitalWrite(light2Pin, LOW);
    relayOn(relay1Pin, false);
    relayOn(relay3Pin, false);

    printLcd(2,""); 

    //do the preasure loop again to repressurize ///DO WE NEED TO DO THIS????
    while(P1 >= pressureOffset + pressureDeltaUp)
    {
      inPressurizeLoop = true;
      digitalWrite(light2Pin, HIGH); 
      relayOn(relay2Pin, true); //open S2 to equalize
      P1 = analogRead(sensor1Pin); //do sensor read again to check
      printLcd(2,"Repressurizing...");
    }
    
    //inPressurizeLoop CLEANUP
    if(inPressurizeLoop)
    { 
      relayOn(relay2Pin, false);  //close S2 because the preasures are equal
      digitalWrite(light2Pin, LOW);
      printLcd(2,"");
      inPressurizeLoop = false; 
    } 
    
    //Overfill condition
    if(inFillLoop && switchFillState == LOW) 
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
      button3State = LOW; // This make AUTO-depressurize after overfill
    }  

    inFillLoop = false;
    printLcd(0, "B2 toggles fill;");  
    printLcd(1, "B3 toggles exhaust");  
  }
  
  switchFillState = digitalRead(switchFillPin); 

  // *********************************************************************
  // BUTTON 3 FUNCTIONS 
  // *********************************************************************

  // DEPRESSURIZE LOOP ***************************************************

  LABEL_depressurizeLoop:
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

//###################################################################################################

    //Check toggle state of B3
    button3StateTEMP = !digitalRead(button3Pin);
    delay(25);
   
    if (button3StateTEMP == HIGH && button3ToggleState == false){  // ON release
      button3ToggleState = true; //Leaves buttonState LOW
      button3State = LOW; 
    }
    if (button3StateTEMP == LOW && button3ToggleState == true){ // OFF push
      button3State = HIGH; //exit WHILE loop
    }
    
    //other two possibilites do nothing (LOW and false, HIGH and true). button2State stays same   
    
//#####################################################################################################    
  }

  //inDepressurizeLoop CLEANUP *******************************************************************************
  // If in this loop, Button3 was released, or Fill/Foam Sensor tripped, or bottle is propely pressurized. Find out which reason and repond.
  if(inDepressurizeLoop)
  { 
    printLcd(2, "");
    relayOn(relay3Pin, false);     //Close bottle exhaust relay
    
    //Case where Button3 released
    if (button3State == HIGH)  
    { 
      //Used to repressurize here; now we don't    
    }
    
    //Case where foam trips sensor
    if (switchFillState == LOW)
    {
      printLcd(2, "Foam... wait");
      
      relayOn(relay2Pin, true);
      delay(2000); // Wait a bit before proceeding    
      relayOn(relay2Pin, false);
      switchFillState = HIGH;

      printLcd(2, "");
      
      //Foam is not a normal circumstance. So let's not make this automatically repeat
      /*
      button3State = LOW; //Feed LOW state in to make automatic platform lowering   
      inDepressurizeLoop = false;  //Leaving inDepressurizeLoop via goto statement, so need to toggle state var
      switchFillState = HIGH;
      goto LABEL_depressurizeLoop;
      */
    }

    //Case where bottle is properly depressurized. If we reach here, the pressure must have reached threshold. Go to Platform lower loop
    if (P1 >= startPressure - pressureDeltaDown)
    {
      digitalWrite(buzzerPin, HIGH); 
      delay(100);
      digitalWrite(buzzerPin, LOW);     
      button3State = LOW; //Feed LOW state in to make automatic platform lowering
      autoMode_1 = true; //Going to platform loop automatically, so set this var to partially drop platform 
    }
    
  digitalWrite(light3Pin, LOW);
  inDepressurizeLoop = false;
  }

  // DOOR OPENING LOOP **************************************************

  // TO DO: Need to add opening door loop, to accomodate door getting blocked or sticking
  // Only activate door solenoid if door is already closed
  while(button3State == LOW && switchDoorState == LOW && P1 > startPressure - pressureDeltaDown)
  {
    inDoorOpenLoop = true;
    printLcd(2,"Opening door..."); 
    digitalWrite(light3Pin, HIGH);
    relayOn(relay6Pin, true);  
    P1 = analogRead(sensor1Pin);
    switchDoorState =  digitalRead(switchDoorPin);
    delay(100);

    //TO DO: Add timer: if door not open in 2 sec, must be stuck; show error message
  }
  
  if(inDoorOpenLoop)
  {
    printLcd(2,""); 
    relayOn(relay6Pin, false);
    inDoorOpenLoop = false;
    digitalWrite(light3Pin, LOW);

    if(platformState = HIGH)
    {
      button3State == LOW;
      delay(750); //Give time to get hand on bottle before dropping platform 
    }  
    else
    {
      button3State = HIGH;
    }  
  } 
    
  // PLATFORM LOWER LOOP *************************************************
  // Platform will not lower with door closed. This prevents someone from defeating door switch
  while(button3State == LOW && switchDoorState == HIGH && P1 > startPressure - pressureDeltaDown)
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

  if(inPlatformLowerLoop){
    //close platform release 
    printLcd(2,"");
    relayOn(relay3Pin, false);
    relayOn(relay5Pin, false);
    relayOn(relay6Pin, false); //Release door solenoid
    digitalWrite(light3Pin, LOW); //TOTC

    inPlatformLowerLoop = false;   
    platformState = LOW; //TOTC

    //Prepare for next cycle
    printLcd(0, "Insert bottle,");
    printLcd(1, "B1 raises platform");

    P1 = analogRead(sensor1Pin); //If we've lowered the platform, assume we've gone through a cycle and store the startPressure again
    startPressure = P1;
    
    startPressurePSI = pressureConv(P1); 
    String (convPSI) = floatToString(buffer, startPressurePSI, 1);
    String (outputPSI) = "Reg. Press: " + convPSI + " psi";
    printLcd(3, outputPSI); 
    
    digitalWrite(light1Pin, LOW); //TOTC
  } 
}

//**********************************************************************
// END
//**********************************************************************

/*

 CODE FRAGMENTS *********************************************************
 
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
  digitalWrite(light1Pin, HIGH);
  digitalWrite(light2Pin, HIGH);
  digitalWrite(light3Pin, HIGH);
  delay(500);
  digitalWrite(light1Pin, LOW);
  digitalWrite(light2Pin, LOW);
  digitalWrite(light3Pin, LOW);
  delay(500);
  digitalWrite(light1Pin, HIGH);
  digitalWrite(light2Pin, HIGH);
  digitalWrite(light3Pin, HIGH);
  delay(500);
  digitalWrite(light1Pin, LOW);
  digitalWrite(light2Pin, LOW);
  digitalWrite(light3Pin, LOW); 
  
  delay(500);
  */
  /*
  digitalWrite(light1Pin, HIGH);
  delay(200);
  digitalWrite(light2Pin, HIGH);
  delay(400);
  digitalWrite(light3Pin, HIGH);
  delay(800);
  digitalWrite(light1Pin, LOW);
  digitalWrite(light2Pin, LOW);
  digitalWrite(light3Pin, LOW);
  delay(500);
  digitalWrite(light1Pin, HIGH);
  delay(200);
  digitalWrite(light2Pin, HIGH);
  delay(400);
  digitalWrite(light3Pin, HIGH);
  delay(800);
  digitalWrite(light1Pin, LOW);
  digitalWrite(light2Pin, LOW);
  digitalWrite(light3Pin, LOW); 
  */
  
  //Make sure bottle isn't in stuck, pressurized state
  //relayOn(relay3Pin, true);
  //delay(4000);
  //relayOn(relay5Pin, true);
  //delay(2000);
  //relayOn(relay3Pin, false);
  //relayOn(relay5Pin, false);
  
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
  
  //Test door lock and buzzer
  relayOn(relay6Pin, true);
  delay(1000);
  relayOn(relay6Pin, false);

  digitalWrite(buzzerPin, HIGH);
  delay(1000);
  digitalWrite(buzzerPin, LOW);
   
  */
  /*
  lcd.setCursor(0,2);
  lcd.clear();
  lcd.setCursor(0,2);
  lcd.print("Ready.");
  */
 
  /*
  Serial.print("Starting pressure: ");
  Serial.print (startPressurePSI);
  Serial.print (" psi");
  Serial.println("");
  
  Serial.print("Starting pressure: ");
  Serial.print (P1);
Serial.print (" units");
Serial.println("");

Serial.print("End Fill Loop Pres: ");
Serial.print (P1);
Serial.print (" units");
Serial.println("");

Serial.print("Pressure Diff: ");
Serial.print (P1);
Serial.print (" units");
Serial.println("");

Serial.print("New Start pressure: ");
Serial.print (P1);
Serial.print (" units");
Serial.println("");
========================


/* SNIPPED FROM DEPRESSURIZE CLEANUP LOOP
while(P1 >= pressureOffset + pressureDeltaUp)
{
  relayOn(relay2Pin, true);         //open S2 to repressurize and tamp down foam
  P1 = analogRead(sensor1Pin);
}  
*/

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

printLcd(2,"Repressurized");
float pressurePSI = startPressurePSI - pressureConv(P1);
String (convPSI) = floatToString(buffer, pressurePSI, 1);
String (outputPSI) = "Bottle Press: " + convPSI;
printLcd(3, outputPSI); 
delay(500);  
*/

 





