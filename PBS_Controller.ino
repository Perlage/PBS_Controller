/*
//===========================================================================  

Perlini Bottling System v1.0

Author: 
  Evan Wallace
  Perlage Systems, Inc.
  Seattle, WA 98101 USA
  
Copyright 2013, 2014  All rights reserved

//===========================================================================  
*/

//Version control variables
String (versionSoftwareTag) = "v1.0"     ;     // 2nd pressure sensor rev'd to 0.6
String (versionHardwareTag) = "v0.6.0.0" ;     // 2nd sensor rev'd to v0.6

//Library includes
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <math.h> 
#include "floatToString.h"               
#include <EEPROM.h>
#include <avr/pgmspace.h>                      // 1-26 Added for PROGMEM function

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

// Declare variables and inititialize states
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
boolean switchDoorState              = HIGH;
boolean switchModeState              = HIGH; //LOW is Manual, HIGH (or up) is auto (normal)

//State variables 
boolean inPressureNullLoop           = false;
boolean inPlatformUpLoop             = false;
boolean inPurgeLoopInterlock         = false;
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

//Key logical states
boolean autoMode_1                   = false;   // Variable to help differentiate between states reached automatically vs manually 
boolean platformLockedNew            = false;   // Variable to be set when platfrom first locks up. This is for autofill-on-doorclose function
boolean platformStateUp              = false;   // true means platform locked in UP; toggled false when S5 opens

//Pressure constants
int offsetP1;                                   // Zero offset for pressure sensor1 (bottle). Set through EEPROM in initial factory calibration, and read into pressureOffset during Setup loop
int offsetP2;                                   // Zero offest for pressure sensor2 (input regulator). Ditto above.
int pressureDeltaUp                  = 50;      // Pressure at which, during pressurization, full pressure is considered to have been reached // Tried 10; went back to 50 to prevent repressurizing after fill button cycle
int pressureDeltaDown                = 38;      // Pressure at which, during depressurizing, pressure considered to be close enough to zero// 38 works out to 3.0 psi 
int pressureDeltaMax                 = 250;     // This is max pressure difference allowed on filling (i.e., limits fill speed)
int pressureNull                     = 200;     // This is the threshold for the controller deciding that no gas source is attached. 
int pressureDropAllowed              = 100;     // Max pressure drop allowed in session before alarm sounds
int pressureRegStartUp;                         // Starting regulator pressure. Will use to detect pressure sag during session; and to find proportional values for key pressure variables (e.g. pressureDeltaMax)

//Pressure conversion and output variables
int P1;                                         // Current pressure reading from sensor1
int P2;                                         // Current pressure reading from sensor2 
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
int timePlatformLock                 = 1250;    // Time in ms before platform locks in up position
int autoPlatformDropDuration         = 1500;    // Duration of platform autodrop in ms

//Key performance parameters
int autoSiphonDuration;                         // Duration of autosiphon function in ms
byte autoSiphonDuration10s;                     // Duration of autosiphon function in 10ths of sec
float autoSiphonDurationSec;                    // Duration of autosiphon function in sec
int antiDripDuration                 =  500;    // Duration of anti-drip autosiphon
int foamDetectionDelay               = 2000;    // Amount of time to pause after foam detection
int pausePlatformDrop                = 1000;    // Pause before platform drops after door automatically opens
 
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

boolean inFillLoopExecuted           = false;    // True of FillLoop is dirty. Used to compute numberCycles
char buffer[25];                                 // Used in float to string conv // 1-26 Changed from 25 to 20 //CHANGED BACK TO 25!! SEEMS TO BE IMPORTANT!
char bufferP[30];                                // make sure this is large enough for the largest string it must hold; used for PROGMEM write to LCD
byte strIndex;                                   // Used to refer to index of the string in *srtLcdTable (e.g., strLcd_0 has strLcdIndex = 0


// ======================================================================================
// Declare functions
// ======================================================================================

// FUNCTION printLcd: 
// To simplify string output. Prevents rewriting unless string has changed; 
// if string has changed, blanks out line first and reprints
// =======================================================================================
String currentLcdString[3];
void printLcd (int line, String newString)
{
  if(!currentLcdString[line].equals(newString))
  {
    currentLcdString[line] = newString;
    lcd.setCursor(0,line);
    lcd.print("                    ");
    lcd.setCursor(0,line);
    lcd.print(newString);
  }
}

// FUNCTION relayOn
// Allows relay states to be easily be changed from HI=on to LOW=on
// =======================================================================================
void relayOn(int pinNum, boolean on){
  if(on){
    digitalWrite(pinNum, LOW);} //turn relay on
  else{
    digitalWrite(pinNum, HIGH); //turn relay off
  }
}

// FUNCTION pressureConv: 
// Routine to convert pressure from parts in 1024 to psi
// pressurePSI = (((P1 - offsetP1) * 0.0048828)/0.009) * 0.145037738
// 1 PSI = 12.7084 units; 1 unit = 0.078688 PSI; 40 psi = 543 units
// =======================================================================================

// Function for P1
float pressureConv1(int P1) 
{
  float pressurePSI1;
  pressurePSI1 = (P1 - offsetP1) * 0.078688; 
  return pressurePSI1;
}

// Function for P2 (potentially different offset)
float pressureConv2(int P2) 
{
  float pressurePSI2;
  pressurePSI2 = (P2 - offsetP2) * 0.078688; 
  return pressurePSI2;
}

// FUNCTION: Door Open Routine
// =======================================================================================
void doorOpen()
{
  switchDoorState = digitalRead(switchDoorPin); 
  while (switchDoorState == LOW && (P1 - offsetP1) <= pressureDeltaDown)
  {
    relayOn (relay6Pin, true);  // Open door
    lcd.setCursor (0, 2); lcd.print (F("Opening door...     "));
    switchDoorState = digitalRead(switchDoorPin); 
    P1 = analogRead(sensorP1Pin);
  }
  relayOn (relay6Pin, false);
  lcd.setCursor (0, 2); lcd.print (F("                    "));
}

// FUNCTION: buzzer()
// =======================================================================================
void buzzer (int buzzDuration)
{
  digitalWrite (buzzerPin, HIGH); 
  delay (buzzDuration);
  digitalWrite (buzzerPin, LOW);
} 

// FUNCTION: platformDrop() //TO DO: pass the duration as a parameter
// ======================================================================================
void platformDrop()
{
  // Drop platform if up
  if (platformStateUp == true)
  {
    relayOn(relay4Pin, false);   
    relayOn(relay5Pin, true); 
    delay (autoPlatformDropDuration);  
    relayOn(relay5Pin, false); 
    platformStateUp = false;
  }
}  

// FUNCTION: pressureDump()
// Dumps pressure if pressure high
// =====================================================================================
void pressureDump()
{
  while (P1 - offsetP1 > pressureDeltaDown)
  {
    relayOn(relay3Pin, true); 
    P1 = analogRead (sensorP1Pin);
    pressureOutput();
    printLcd (3, outputPSI_b);
  }  
  relayOn(relay3Pin, false);  
}

// FUNCTION: Read Buttons
// ======================================================================================
void readButtons()
{
  button1State = !digitalRead(button1Pin); 
  button2State = !digitalRead(button2Pin); 
  button3State = !digitalRead(button3Pin); 
  delay (10); //debounce
}

// FUNCTION: Read Sensors
// =======================================================================================
void readStates()
{
  switchDoorState = digitalRead(switchDoorPin);
  switchModeState = digitalRead(switchModePin);
  sensorFillState = digitalRead(sensorFillPin);
  delay (25); //debounce
}

// FUNCTION: Read Sensors
// =======================================================================================
void readInputs()
{
  button1State = !digitalRead(button1Pin); 
  button2State = !digitalRead(button2Pin); 
  button3State = !digitalRead(button3Pin); 
  sensorFillState = digitalRead(sensorFillPin);
  switchDoorState = digitalRead(switchDoorPin);
  switchModeState = digitalRead(switchModePin);
  P1 = analogRead(sensorP1Pin);
  P2 = analogRead(sensorP2Pin); 
  delay (25); //debounce
}

// FUNCTION: Pressure reading/conversion/output
// ====================================================================================
void pressureOutput()
{
  P1 = analogRead(sensorP1Pin); 
  P2 = analogRead(sensorP2Pin);
        
  PSI1     = pressureConv1(P1); 
  PSI2     = pressureConv2(P2); 
  PSIdiff  = PSI2 - PSI1;
  
  (convPSI1)      = floatToString(buffer, PSI1, 1);
  (convPSI2)      = floatToString(buffer, PSI2, 1);
  (convPSIdiff)   = floatToString(buffer, PSIdiff, 1);

  (outputPSI_rbd) = "R:" + convPSI2 + " B:" + convPSI1 + " d:" + convPSIdiff; 
  (outputPSI_rb)  = "Reg:" + convPSI2 + " Bottle:" + convPSI1;
  (outputPSI_b)   = "Bottle: " + convPSI1 + " psi"; 
  (outputPSI_r)   = "Regulator: " + convPSI2 + " psi"; 
  (outputPSI_d)   = "Difference: " + convPSIdiff + " psi"; 
}  

// FUNCTION: Platform UP Function Loop
// =======================================================================================
void platformUpLoop()
{
  // =====================================================================================  
  // PLATFORM RAISING LOOP
  // while B1 is pressed, platform is not UP, and door is open, raise bottle platform.
  // =====================================================================================  
  
  timePlatformInit = millis(); // Inititalize time for platform lockin routine
  int n = 0;
  
  while (button1State == LOW && platformStateUp == false && switchDoorState == HIGH && (timePlatformRising < timePlatformLock)) 
  { 
    inPlatformUpLoop = true; 
    digitalWrite(light1Pin, HIGH);
    
    relayOn(relay5Pin, false);    //closes vent
    relayOn(relay4Pin, true);     //raises plaform
    
    timePlatformCurrent = millis();
    delay(10); //added this to make sure time gets above threshold before loop exits--not sure why works
    timePlatformRising = timePlatformCurrent - timePlatformInit;
    
    // Writes a lengthening line of dots
    lcd.setCursor (0, 2); lcd.print (F("Raising             "));
    lcd.setCursor (((n + 12) % 12) + 7, 2); lcd.print (F(".")); 
    n = n++;
    delay(100);  
    
    button1State = !digitalRead(button1Pin); 
    switchDoorState = digitalRead(switchDoorPin);    
  }  

  // PLATFORM RAISING LOOP EXIT ROUTINES
  // =======================================================================================

  if (inPlatformUpLoop)
  {
    if (timePlatformRising >= timePlatformLock)
    {
      // PlatformLoop lockin reached
      platformStateUp = true;
      platformLockedNew = true; //Pass this to PressureLoop for autopressurize on door close--better than trying to pass button2State = LOW, which causes problems

      lcd.setCursor (0, 0); lcd.print (F("To fill, close door "));
      lcd.setCursor (0, 1); lcd.print (F("B2 toggles filling  "));
      lcd.setCursor (0, 2); lcd.print (F("Ready...            "));

      buzzer(250);
      
      relayOn(relay4Pin, true);  // Slight leak causes platform to fall over time--so leave open 
      relayOn(relay5Pin, false); 
      digitalWrite(light1Pin, LOW); //Decided to turn this off. Lights should be lit only if pressing button or releasing it can change a state. 
    }  
    else    
    {
      // Drop Platform
      relayOn(relay4Pin, false);
      relayOn(relay5Pin, true);
      digitalWrite(light1Pin, LOW);
      delay(2000); 

      relayOn(relay5Pin, false);  // Prevents leaving S5 on if this was the last thing user did
      lcd.setCursor (0, 2); lcd.print (F("                    ")); // Blanks status line
      platformStateUp = false;
    }

    timePlatformRising = 0;
    inPlatformUpLoop = false;
  }
}
// END PLATFORM RAISING LOOP
//=============================================================================================


//========================================================================================
// EMERGENCY DEPRESSURIZE LOOP FUNCTION
// Depressurize anytime door opens when pressurized above low threshold
//========================================================================================

void emergencyDepressurize()
{
  boolean inEmergencyDepressurizeLoop = false;

  while (switchDoorState == HIGH && platformStateUp == true && (P1 - offsetP1 > pressureDeltaDown))
  {
    inEmergencyDepressurizeLoop = true;
    relayOn(relay3Pin, true);  
  
    lcd.setCursor (0, 2); lcd.print (F("CLOSE DOOR! Venting."));
    digitalWrite(buzzerPin, HIGH); //Leave buzzer on until closed
      
    pressureOutput();
    printLcd(3, outputPSI_b);     
    
    switchDoorState = digitalRead(switchDoorPin); //Check door switch    
    P1 = analogRead(sensorP1Pin);
  }  
  
  // EMERGENCY DEPRESSURIZE LOOP EXIT ROUTINES
  //========================================================================================
  if (inEmergencyDepressurizeLoop)
  {
    inEmergencyDepressurizeLoop = false;
    relayOn(relay3Pin, false);  
    digitalWrite(buzzerPin, LOW); // Now turn off
    lcd.setCursor (0, 2); lcd.print (F("                    "));
    button2State = HIGH;  // Pass HIGH state to next routine so filling doesn't automatically resume. Make user think about it!
  }  
}  
// END EMERGENCY DEPRESSURIZE LOOP FUNCTION
//========================================================================================


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
  
  Serial.begin(9600); // TO DO: remove when done
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
  // =====================================================================================
  // EEPROM CALIBRATION 
  // Write null pressure to EEPROM. 
  // WITH GAS OFF AND PRESSURE RELEASED FROM HOSES, Hold down all three buttons on startup 
  // =====================================================================================

  readButtons(); 

  if (button1State == LOW && button2State == LOW && button3State == LOW)
  {
    lcd.setCursor (0, 0); lcd.print (F("Setting EEPROM...   "));
    delay (2000);
    
    //Show old offset values
    offsetP1 = EEPROM.read(0);
    offsetP2 = EEPROM.read(1);

    String (convOffset1) = floatToString(buffer, offsetP1, 1);
    String (convOffset2) = floatToString(buffer, offsetP2, 1);

    String (outputOffset1) = "Old Offset1: " + convOffset1; 
    String (outputOffset2) = "Old Offset2: " + convOffset2; 

    printLcd(2, outputOffset1);
    printLcd(3, outputOffset2);
    delay (4000);

    //Get new offset values
    offsetP1  = analogRead(sensorP1Pin); 
    offsetP2  = analogRead(sensorP2Pin); 
    
    numberCycles = 0;                          //Set number of lifetime cycles to initial val of 0
    autoSiphonDuration10s = 25;                //Set initial duration to 25 tenths of seconds (2.5 sec)
    
    EEPROM.write (0, offsetP1);                //Write zero offset for sensor1 (bottle)
    EEPROM.write (1, offsetP2);                //Write zero offset for sensor2 (regulator)
    EEPROM.write (3, autoSiphonDuration10s);   //Write autoSiphonDuration default to 2.5 sec
    EEPROM.write (4, 0);                       //Write "ones" digit to zero in numberCycles01
    EEPROM.write (5, 0);                       //Write "tens" digit to zero in numberCycles10

    convOffset1 = floatToString(buffer, offsetP1, 1);
    convOffset2 = floatToString(buffer, offsetP2, 1);

    outputOffset1 = "New Offset1: " + convOffset1; 
    outputOffset2 = "New Offset2: " + convOffset2; 

    printLcd(2, outputOffset1);
    printLcd(3, outputOffset2);
    delay(4000);

    lcd.setCursor (0, 2); lcd.print (F("                    "));
    //lcd.setCursor (0, 3); lcd.print (F("                    ")); // Not necessary
  }    

  // END EEPROM SET
  // ==================================================================================
    
  //===================================================================================
  // STARTUP ROUTINE
  //===================================================================================
  
  relayOn(relay3Pin, true); // Open at earliest possibility to vent  
  
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

  //Initial user message
  lcd.setCursor (0, 0);  lcd.print (F("Perlini Bottling"));
  printLcd (1, "System, " + versionSoftwareTag);
  lcd.setCursor (0, 3);  lcd.print (F("Initializing..."));
  
  //=================================================================================
  // MENU ROUTINE
  //=================================================================================
   
  button1State = !digitalRead(button1Pin); 
  boolean button1StateMENU;
  boolean button2StateMENU;
  boolean button3StateMENU;
  
  while (button1State == LOW)
  {
    inMenuLoop = true;
    button1State = !digitalRead(button1Pin); 
    delay(25);   
    
    lcd.setCursor (0, 0); lcd.print (F("***MENU*** Press:   "));
    lcd.setCursor (0, 1); lcd.print (F("B1: Manual Mode     "));
    lcd.setCursor (0, 2); lcd.print (F("B2: Autosiphon time "));
    lcd.setCursor (0, 3); lcd.print (F("B3: Exit Menu       "));
  }  
    
  while (inMenuLoop == true)
  {
    button1StateMENU = !digitalRead(button1Pin); 
    button2StateMENU = !digitalRead(button2Pin); 
    button3StateMENU = !digitalRead(button3Pin); 
    switchDoorState = digitalRead(switchDoorPin);
    delay(10);
    
    // MENU1============================
    if (button1StateMENU == LOW)
    {
      inMenuLoop = false;
      inManualModeLoop = true;
      
      lcd.setCursor (0, 0); lcd.print (F("Flip switch to enter"));
      lcd.setCursor (0, 1); lcd.print (F("Cleaning/Manual mode"));
      lcd.setCursor (0, 2); lcd.print (F("now.                "));
      
      // Don't let user proceed until switch flipped
      switchModeState = digitalRead(switchModePin);
      while (switchModeState == HIGH)
      {
        switchModeState = digitalRead(switchModePin);
      }  
    } 

    // MENU2============================

    boolean autoSiphonDurationLoop = false;

    while (button2StateMENU == LOW)
    {
      //inMenuLoop = false;
      autoSiphonDurationLoop = true;
      button2StateMENU = !digitalRead(button2Pin); 

      lcd.setCursor (0, 0); lcd.print (F("B1 inc. by .1 sec   "));
      lcd.setCursor (0, 1); lcd.print (F("B3 dec. by .1 sec   "));
      lcd.setCursor (0, 2); lcd.print (F("B2 sets value; exits"));
    }  

    while (autoSiphonDurationLoop == true)
    {
      delay(10);
      button1State = !digitalRead(button1Pin); 
      button2State = !digitalRead(button2Pin); 
      button3State = !digitalRead(button3Pin); 
          
      //delay(100);
      
      if (button1State == LOW){
        autoSiphonDuration10s = autoSiphonDuration10s + 1; //Add .1s
      }  
      if (button3State == LOW){
        autoSiphonDuration10s = autoSiphonDuration10s - 1; //Subtract .1s
      }  
        
      autoSiphonDuration10s = constrain(autoSiphonDuration10s, 5, 99); //Constrains autoSiphonDuration10s to between 5 and 99 tenths of sec
      autoSiphonDurationSec = float(autoSiphonDuration10s) / 10;
      
      String (convTime) = floatToString(buffer, autoSiphonDurationSec, 1);
      printLcd (3, "Current value: " + convTime + "s");
      delay(200);

      if (button2State == LOW){
        autoSiphonDurationLoop = false;
        printLcd (3, "New value: " + convTime + " s");
        delay(2000);
        
        EEPROM.write (3, autoSiphonDuration10s);             //Write to EEPROM
        autoSiphonDuration = autoSiphonDuration10s * 100;    //Convert to ms from 10ths of sec
        
        autoSiphonDurationLoop = false;
        button3StateMENU = LOW;
      } 
    }

    // MENU3============================
    if (button3StateMENU == LOW)
    {
      //Continue with startup routine
      inMenuLoop = false;
      lcd.setCursor (0, 0); lcd.print (F("Perlini Bottling    "));
      printLcd (1, "System, " + versionSoftwareTag);
      lcd.setCursor (0, 2); lcd.print (F("Exiting menu...     "));
    }  
  }

  // END MENU ROUTINE
  //=================================================================================  
 
 
  //=================================================================================
  // RESUME NORMAL STARTUP
  //=================================================================================

  // Get fresh pressure and door state measurements
  switchDoorState = digitalRead(switchDoorPin); 
  P1 = analogRead(sensorP1Pin);
  P2 = analogRead(sensorP2Pin);

  relayOn(relay5Pin, false); // Close if not already

  // PLATFROM LOCK OR SUPPORT ROUTINE. DO THESE FIRST FOR SAFETY
  // IMMEDIATELY lock platform if P2 is low and P1 is high, or apply platform support if P1 and P2 high
  if (P1 - offsetP1 > pressureDeltaDown)
  {
    if (P2 - offsetP2 < pressureNull)
    {
      relayOn(relay4Pin, false);   // When input pressure low, close S4 to conserve gas and keep platform up (it should be closed already)
      lcd.setCursor (0, 2); lcd.print (F("Platform locked...  "));
      delay (500); // Can delete later
    }
    else
    {  
      relayOn(relay4Pin, true);    // Turn on platform support immediately if P2 high--keeps stuck bottle in place
    }
  }  
  // But if P1 is not high, then there is no bottle, or bottle pressure is low. So raise platform--but take time to make user close door, so no pinching
  else
  {
    while (switchDoorState == HIGH) // Make sure door is closed
    {
      switchDoorState = digitalRead(switchDoorPin); 
      lcd.setCursor (0, 2); lcd.print (F("PLEASE CLOSE DOOR..."));

      buzzer(100);
      delay(100);
    }
    
    delay(500);                // A little delay after closing door before raising platform
    relayOn(relay4Pin, true);  // Now Raise platform     

    lcd.setCursor (0, 0); lcd.print (F("Perlini Bottling    "));
    printLcd (1, "System, " + versionSoftwareTag);
    lcd.setCursor (0, 3); lcd.print (F("Initializing...     "));
  }

  // Blinks lights and give time to de-pressurize stuck bottle
  for (int n = 0; n < 0; n++)
  {
    digitalWrite(light1Pin, HIGH); delay(500);
    digitalWrite(light2Pin, HIGH); delay(500);
    digitalWrite(light3Pin, HIGH); delay(500);
    digitalWrite(light1Pin, LOW);
    digitalWrite(light2Pin, LOW);
    digitalWrite(light3Pin, LOW); 
    delay(325);
  }

  // Leave blink loop and light all lights 
  digitalWrite(light1Pin, HIGH); delay(500);
  digitalWrite(light2Pin, HIGH); delay(500);
  digitalWrite(light3Pin, HIGH); delay(500);
  
  //============================================================================
  // NULL PRESSURE ROUTINES
  // Check for stuck pressurized bottle or implausibly low gas pressure at start 
  //============================================================================

  boolean inPressurizedBottleLoop = false;
  boolean inPressureNullLoop = false;

  // CASE 1: PRESSURIZED BOTTLE (Bottle is already depressurizing because S3 opened above)
  if ((P1 - offsetP1 > pressureDeltaDown) && !(switchModeState == LOW && inManualModeLoop == true)) //Skip pressure check if in manual mode chosen in menu
  {
    inPressurizedBottleLoop = true;
    
    lcd.setCursor (0, 0); lcd.print (F("Pressurized bottle  "));
    lcd.setCursor (0, 1); lcd.print (F("detected. Open valve"));
    lcd.setCursor (0, 2); lcd.print (F("Depressurizing...   "));
    lcd.setCursor (0, 3); lcd.print (F("                    "));
    
    buzzer(1000);

    while (P1 - offsetP1 > pressureDeltaDown)
    {
      pressureOutput();
      printLcd(3, outputPSI_b);  
    } 
  }
      
  // CASE 2: GASS OFF OR LOW       
  if ((P2 - offsetP2 < pressureNull) && !(switchModeState == LOW && inManualModeLoop == true)) //Skip pressure check if in manual mode chosen in menu
  {
    inPressureNullLoop = true;

    // Write Null Pressure warning 
    lcd.setCursor (0, 0); lcd.print (F("Gas off or empty;   "));
    lcd.setCursor (0, 1); lcd.print (F("check tank & hoses. "));
    lcd.setCursor (0, 2); lcd.print (F("B3 opens door.      "));
    lcd.setCursor (0, 3); lcd.print (F("Waiting...          "));

    buzzer(250);
        
    while (P2 - offsetP2 < pressureNull)
    {
      //Read sensors
      P2 = analogRead(sensorP2Pin); 
      button3State = !digitalRead(button3Pin);
      switchDoorState = digitalRead(switchDoorPin); 
      delay(25); 
            
      if (button3State == LOW)
      {
        doorOpen();
      }  
    }  
  }
  
  // NULL PRESSURE EXIT ROUTINES
  // No longer closing S3 at end of this to prevent popping in some edge cases of a very foamy bottle
  // ==================================================================================
  
  if (inPressurizedBottleLoop)
  {
    //Open door
    delay(2000); // Delay gives time for getting accurate pressure reading
    doorOpen(); 
    pressureRegStartUp = analogRead (sensorP2Pin); // Get GOOD start pressure for emergency lock loop
    inPressurizedBottleLoop = false;
  } 

  if (inPressureNullLoop)
  {
    delay(2000); // Delay gives time for getting accurate pressure reading
    doorOpen();
    pressureRegStartUp = analogRead (sensorP2Pin); // Get GOOD start pressure for emergency lock loop
    inPressureNullLoop = false;
  }

  // END NULL PRESSURE ROUTINE
  // ====================================================================================

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

  // Drop platform, which had been raised before nullpressure checks
  relayOn(relay4Pin, false);  
  relayOn(relay5Pin, true);
  delay(3000);  
  relayOn(relay5Pin, false); 
  relayOn(relay3Pin, false); // Close this so solenoid doesn't overhead on normal startup
  
  // Open door if closed
  doorOpen();
 
  // Write initial instructions for normal startup
  lcd.setCursor (0, 0); lcd.print (F("Insert bottle;      "));
  lcd.setCursor (0, 1); lcd.print (F("B1 raises platform  "));
  lcd.setCursor (0, 2); lcd.print (F("Ready...            "));

  delay(500); digitalWrite(light3Pin, LOW);
  delay(100); digitalWrite(light2Pin, LOW);
  delay(100); digitalWrite(light1Pin, LOW);
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
  button1State     = !digitalRead(button1Pin); 
  button2StateTEMP = !digitalRead(button2Pin); 
  button3StateTEMP = !digitalRead(button3Pin); 
  switchDoorState  =  digitalRead(switchDoorPin);
  switchModeState  =  digitalRead(switchModePin);
  delay(10);

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
   
  // Monitor door state in null loop
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

  // Main Loop idle pressure measurement and LCD print
  // ======================================================================
  
  pressureOutput();
  if (P1 - offsetP1 > pressureDeltaDown)
  {
    printLcd(3, outputPSI_rb); // Print reg and bottle if bottle pressurized
  }
  else
  {
    printLcd(3, outputPSI_r);  // Print only reg if bottle not pressent or at zero pressure
  }  
    
  // EMERGENCY PLATFORM LOCK LOOP: 
  // Lock platform if gas pressure drops while bottle pressurized  
  //========================================================================

  /*
  Serial.print ("StartPress: "); 
  Serial.print (pressureRegStartUp);  
  Serial.print (" P1: "); 
  Serial.print (P1);  
  Serial.print (" P2: "); 
  Serial.print (P2); 
  Serial.println ();
  */
  
  boolean inEmergencyLockLoop = false;
  boolean platformEmergencyLock = false;
  boolean buzzOnce = false;
  
  // If pressure drops, go into this loop and wait for user to fix
  while (P2 -offsetP2 < pressureRegStartUp - pressureDropAllowed) // Number to determine what consitutes a pressure drop.// 2-18 Changed from 75 to 100
  {
    inEmergencyLockLoop = true;

    P1 = analogRead(sensorP1Pin); 
    P2 = analogRead(sensorP2Pin); 
    
    //If bottle is pressurized (along with pressure sagging), also lock the platform
    if (P1 - offsetP1 > pressureDeltaDown)
    {
      platformEmergencyLock = true;
      relayOn (relay4Pin, false);   // Lock platform so platform doesn't creep down with pressurized bottle
      relayOn (relay3Pin, true);    // Vent the bottle to be safe

      if (buzzOnce == false)
      {
        lcd.setCursor (0, 2); lcd.print (F("Platform locked...  "));
        buzzer(2000);
        buzzOnce = true;
      }
    }  
    
    lcd.setCursor (0, 0); lcd.print (F("Input pressure low--"));
    lcd.setCursor (0, 1); lcd.print (F("Check CO2 tank/hoses"));
    lcd.setCursor (0, 2); lcd.print (F("Waiting...          "));
    
    // Pressure measurement and output
    pressureOutput();
    printLcd (3, outputPSI_rb);

    //Only sound buzzer once
    if (buzzOnce == false)
    {
      buzzer(2000);
      buzzOnce = true;
    }
  } 
  
  if (inEmergencyLockLoop)
  {
    inEmergencyLockLoop = false;
    // Run this condition if had pressurized bottle
    if (platformEmergencyLock == true)
    {
      relayOn (relay4Pin, true);       // Re-open platform UP solenoid 
      relayOn (relay3Pin, false);      // Re close vent if opened 
      platformEmergencyLock = false;
    }
      
    delay(1000);
    lcd.setCursor (0, 0); lcd.print (F("B2 toggles filling; "));
    lcd.setCursor (0, 1); lcd.print (F("B3 toggles exhaust. "));
    lcd.setCursor (0, 2); lcd.print (F("Problem corrected..."));
    delay(1000);
  }  
  
  //END EMERGENCY PLATFORM LOCK LOOP
  //======================================================================================  


  // =====================================================================================  
  // PLATFORM RAISING LOOP
  // while B1 is pressed, platform is not UP, and door is open, raise bottle platform.
  // =====================================================================================  

  platformUpLoop();

  // END PLATFORM RAISING LOOP
  //======================================================================================


  //======================================================================================
  // PURGE LOOP
  // While B2 and then B1 is pressed, and door is open, and platform is down, purge the bottle
  //=========================================================================
  
  // PURGE INTERLOCK LOOP
  // Door must be OPEN, platform DOWN, and pressure near zero (i.e. no bottle)
  //=========================================================================

  while(button2State == LOW && switchDoorState == HIGH && platformStateUp == false && (P1 - offsetP1 <= pressureDeltaDown)) 
  {
    inPurgeLoopInterlock = true;
    digitalWrite(light2Pin, HIGH);
    
    // PURGE LOOP
    //======================================
    while (button1State == LOW) // Require two button pushes, B2 then B1, to purge
    { 
      inPurgeLoop = true;

      lcd.setCursor (0, 2); lcd.print (F("Purging...          "));
      digitalWrite(light1Pin, HIGH); 

      relayOn(relay2Pin, true); //open S2 to purge
      button1State = !digitalRead(button1Pin); 
      delay(10);  
    }
    
    // PURGE LOOP EXIT ROUTINES
    //=====================================
    
    if(inPurgeLoop)
    {
      relayOn(relay2Pin, false); //Close relay when B1 and B2 not pushed
      digitalWrite(light1Pin, LOW); 
  
      inPurgeLoop = false;
      lcd.setCursor (0, 2); lcd.print (F("                    ")); 
    }
    
    button1State = !digitalRead(button1Pin); 
    button2State = !digitalRead(button2Pin); 
    delay(10);
  }
  
  // PURGE LOOP INTERLOCK EXIT ROUTINES
  //======================================
  
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
    delay(25);  //Debounce
    if (button2StateTEMP == HIGH && button2ToggleState == false){  //ON release
      button2ToggleState = true;                                   //leave button state LOW
      button2State = LOW;                                          //or make it low if it isn't yet
    }
    if (button2StateTEMP == LOW && button2ToggleState == true){    //OFF push
      button2State = HIGH;                                         //exit WHILE loop
    }  
      
    lcd.setCursor (0, 0); lcd.print (F("B2 toggles filling; "));
    lcd.setCursor (0, 1); lcd.print (F("B3 toggles exhaust. "));
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
      platformDrop();   // Drop plaform
      
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

  pinMode(sensorFillPin, INPUT_PULLUP); //Probably no longer necessary since FillSwitch was moved off Pin13 (Zach proposed this Oct-7)

  // With two sensors, the pressure condition should absolutely prevent any liquid sprewing. pressureDeltaMax ensures: a) filling can't go too fast; b) can't dispense w/ no bottle
  while(button2State == LOW && sensorFillState == HIGH && switchDoorState == LOW && (P1 - offsetP1) > pressureDeltaMax) 
  {     
    inFillLoop = true;
    inFillLoopExecuted = true; //This is an "is dirty" variable for counting lifetime bottles. Reset in platformUpLoop.

    //while the button is  pressed, fill the bottle: open S1 and S3
    relayOn(relay1Pin, true);
    relayOn(relay3Pin, true);
    digitalWrite(light2Pin, HIGH); 

    //Check toggle state of B2    
    button2StateTEMP = !digitalRead(button2Pin);
    delay(25);
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
    
    // CLEANING MODE: If clean switch closed, set FillState HIGH
    if (switchModeState == LOW)
    {
      sensorFillState = HIGH;
      lcd.setCursor (0, 2); lcd.print (F("Fill sensor disabled"));
    }
    else
    {
      //Read sensors
      sensorFillState = digitalRead(sensorFillPin); //Check fill sensor
      switchDoorState = digitalRead(switchDoorPin); //Check door switch 
      lcd.setCursor (0, 2); lcd.print (F("Filling...          "));
    }   
    
    switchModeState = digitalRead(switchModePin); //Check cleaning switch 
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
      relayOn(relay1Pin, true);
      relayOn(relay2Pin, true);
      
      lcd.setCursor (0, 2); lcd.print (F("Preventing drip... "));
      delay(antiDripDuration); // This setting determines duration of autosiphon 
      lcd.setCursor (0, 2); lcd.print (F("                   "));

      relayOn(relay1Pin, false);
      relayOn(relay2Pin, false);
      
      delay (25);
      sensorFillState = digitalRead(sensorFillPin); 
      delay (25);
    }
    
    // CASE 2: FillSensor tripped--Overfill condition
    else if (inFillLoop && sensorFillState == LOW) 
    {
      relayOn(relay1Pin, true);
      relayOn(relay2Pin, true);
      
      lcd.setCursor (0, 2); lcd.print (F("Adjusting level...  "));
      delay(autoSiphonDuration); // This setting determines duration of autosiphon 
      lcd.setCursor (0, 2); lcd.print (F("                    "));

      relayOn(relay1Pin, false);
      relayOn(relay2Pin, false);
      
      delay (25);
      sensorFillState = digitalRead(sensorFillPin); 
      delay (25); 
             
      button3State = LOW; // This make AUTO-depressurize after overfill // TO DO: shouldn't this be AutoMode_1?
    }
    else 
    {
      // CASE 3: Bottle pressure dropped below Deltatamp threshold; i.e., filling too fast
      //=============================================
      // REPRESSURIZE LOOP
      //=============================================
      
      while (platformStateUp == true && switchDoorState == LOW && (P1 - offsetP1) <= (P2 - offsetP2) - pressureDeltaUp)
      {
        inPressurizeLoop = true;
        digitalWrite(light2Pin, HIGH); 
        relayOn(relay2Pin, true);       //open S2 to equalize
        P1 = analogRead(sensorP1Pin);    //do sensor read again to check
        P2 = analogRead(sensorP2Pin);    //do sensor read again to check

        lcd.setCursor (0, 0); lcd.print (F("Filling too fast--  "));
        lcd.setCursor (0, 1); lcd.print (F("Close exhaust valve "));
        lcd.setCursor (0, 2); lcd.print (F("Repressurizing...   "));
        delay (1000);
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
   
    //Door opened while bottle filling...emergency dump of pressure  
    if (platformStateUp == true && switchDoorState == HIGH)
    { 
      emergencyDepressurize();
    }      
    
    inFillLoop = false;

    lcd.setCursor (0, 0); lcd.print (F("B2 toggles filling; "));
    lcd.setCursor (0, 1); lcd.print (F("B3 toggles exhaust. "));
  }

  // END FILL LOOP EXIT ROUTINE
  //========================================================================================

  //========================================================================================
  // CARBONATOR STUCK BOTTLE CATCH LOOP //TO DO: Do we still need this?
  //========================================================================================

  while (sensorFillState == LOW)
  {    
    readButtons();
    lcd.setCursor (0, 0); lcd.print (F("B1: Manual Siphon  "));
    lcd.setCursor (0, 1); lcd.print (F("B2: Depressurize   "));
    lcd.setCursor (0, 2); lcd.print (F("Waiting...         "));

    // This is manual autosiphon
    while (button1State == LOW)
    {
      readButtons();
      relayOn (relay1Pin, true);
      relayOn (relay2Pin, true);
    }  
    relayOn (relay1Pin, false);
    relayOn (relay2Pin, false);

    delay (25);
    sensorFillState = digitalRead(sensorFillPin); 
    delay (25); 
    
    pressureOutput();
    printLcd (3, outputPSI_rb); 
   
    // Emergency exit
    if (button2State == LOW)
    {
      pressureDump();
      relayOn (relay3Pin, true);
      delay(2000);
      doorOpen();
      platformDrop();
      relayOn (relay3Pin, false);
      sensorFillState == HIGH;
    }  
  }
  
  //========================================================================================
  // DEPRESSURIZE LOOP
  //========================================================================================

  LABEL_inDepressurizeLoop:
  
  while(button3State == LOW && sensorFillState == HIGH && (P1 - offsetP1 >= pressureDeltaDown)) 
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
    
    // CLEANING MODE 
    if (switchModeState == LOW)
    {
      sensorFillState = HIGH;
      lcd.setCursor (0, 2); lcd.print (F("Foam sensor disabled"));
    }
    else
    {
      sensorFillState = digitalRead(sensorFillPin); //Check fill sensor
      switchDoorState = digitalRead(switchDoorPin); //Check door switch 
      lcd.setCursor (0, 2); lcd.print (F("Depressurizing...   "));
    }   
    
    switchModeState = digitalRead(switchModePin); //Check cleaning switch 
    
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
    if (sensorFillState == LOW)
    {
      lcd.setCursor (0, 2); lcd.print (F("Foam detected...wait"));
      relayOn(relay3Pin, false);  
      relayOn(relay2Pin, true);
 
      delay(foamDetectionDelay);    // Wait a bit before proceeding    

      relayOn(relay2Pin, false);
      sensorFillState = HIGH;
      lcd.setCursor (0, 2); lcd.print (F("                    "));
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
    doorOpen(); // Run door open function
    digitalWrite(light3Pin, HIGH);

    //TO DO: Add timer: if door not open in 2 sec, must be stuck; show error message
  }

  // DOOR OPEN LOOP EXIT ROUTINES
  //============================================================================================

  if(inDoorOpenLoop)
  {
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
      relayOn(relay3Pin, true);  // May as well leave this open? YES. Liquid is still outgassing.//TO DO: Don't need this here?? it's already open?
      relayOn(relay5Pin, true);  // Open cylinder exhaust
      digitalWrite(light3Pin, HIGH); 
    }

    P1 = analogRead(sensorP1Pin);
    button3State = !digitalRead(button3Pin);
    switchDoorState =  digitalRead(switchDoorPin);
    sensorFillState =  digitalRead(sensorFillPin);
    delay(25);
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
      String (convNumberCycles) = floatToString(buffer, numberCycles, 0);
      String (convNumberCyclesSession) = floatToString(buffer, numberCyclesSession, 0);
      String outputInt = "Cycles: " + convNumberCyclesSession + "/" + convNumberCycles;
      printLcd(2, outputInt); 
      delay(1000);
    
      inFillLoopExecuted = false;
    }      
  } 
  
  // END PLAFORM LOWER LOOP
  //============================================================================================


  // ===========================================================================================
  // MANUAL MODE ENTRANCE ROUTINES
  // =========================================================================================== 
  
  // Could get here from menu or switch being on (LOW)  
  if (switchModeState == LOW || inManualModeLoop == true)
  {
    inManualModeLoop = true;
    sensorFillState = HIGH;

    lcd.setCursor (0, 0); lcd.print (F(" ***MANUAL MODE***  "));
    lcd.setCursor (0, 1); lcd.print (F("Use for cleaning and"));
    lcd.setCursor (0, 2); lcd.print (F("troubleshooting.    "));
    buzzer (2000);
    
    // FUNCTION Dump pressure
    pressureDump();
    
    // FUNCTION doorOpen
    doorOpen();

    // FUNCTION Drop platform if up
    platformDrop();

  }
  else
  {
    // In standard Auto mode
    lcd.setCursor (0, 2); lcd.print (F("Ready...            "));
  }

  // END MANUAL MODE ENTRANCE ROUTINES
  // =====================================================================================  


  //======================================================================================
  // MANUAL MODE
  //======================================================================================
      
  while (switchModeState == LOW && inManualModeLoop == true)
  {
    // FUNCTION: PlatformUpLoop
    platformUpLoop();     
 
    // FUNCTION: Read all states of buttons, sensors
    readButtons();
    readStates();
    
    // FUNCTION: Read and output pressure
    pressureOutput();
    printLcd(3, outputPSI_rbd);    

    if (platformStateUp == false && switchDoorState == HIGH)
    {
      lcd.setCursor (0, 0); lcd.print (F(" ***MANUAL MODE***  "));
      lcd.setCursor (0, 1); lcd.print (F("Insert bottle;      "));
      lcd.setCursor (0, 2); lcd.print (F("B1 raises platform. "));
    }
    if (platformStateUp == false && switchDoorState == LOW)
    {
      lcd.setCursor (0, 0); lcd.print (F(" ***MANUAL MODE***  "));
      lcd.setCursor (0, 1); lcd.print (F("B3 opens door;      "));
      lcd.setCursor (0, 2); lcd.print (F("then insert bottle  "));
    }
    if (platformStateUp == true && switchDoorState == HIGH)
    {
      lcd.setCursor (0, 0); lcd.print (F(" ***MANUAL MODE***  "));
      lcd.setCursor (0, 1); lcd.print (F("Close door to start;"));
      lcd.setCursor (0, 2); lcd.print (F("B3 to lower platform"));
    }
    if (platformStateUp == true && switchDoorState == LOW)
    {
      lcd.setCursor (0, 0); lcd.print (F("B1: Gas IN          "));
      lcd.setCursor (0, 1); lcd.print (F("B2: Liquid IN       "));
      lcd.setCursor (0, 2); lcd.print (F("B3: Gas OUT/Open dr."));
    }
      
    // B1: GAS IN ================================================================
    if (button1State == LOW && platformStateUp == true && switchDoorState == LOW){
      relayOn (relay2Pin, true);}  
    else{
      relayOn (relay2Pin, false);}
      
    // B2 LIQUID IN ==============================================================
    if (button2State == LOW && platformStateUp == true && switchDoorState == LOW){
      // TO DO: ADD WARNING
      relayOn (relay1Pin, true);}  
    else{
      relayOn (relay1Pin, false);}
      
    // B3 GAS OUT ================================================================
    if (button3State == LOW && platformStateUp == true && switchDoorState == LOW){
      relayOn (relay3Pin, true);}  
    else{
      relayOn (relay3Pin, false);}
      
/*
    // B3: Open Door or drops platform ===========================================
    if (button3State == LOW && switchDoorState == LOW && (P1 - offsetP1 < pressureDeltaDown))
    {
      doorOpen();
    }  
    if (button3State == LOW && platformStateUp == true && (P1 - offsetP1 < pressureDeltaDown))
    {
      platformDrop();
    }  
*/
  }
    
  //END MANUAL MODE LOOP 
  //=================================================   
 
  //MANUAL MODE LOOP EXIT ROUTINES 
  //==================================================
  if (inManualModeLoop)
  {
    inManualModeLoop = false;
    
    lcd.setCursor (0, 0); lcd.print (F("EXITING MANUAL MODE "));
    lcd.setCursor (0, 1); lcd.print (F("                    "));
    lcd.setCursor (0, 2); lcd.print (F("Continuing...       "));
    
    while (P1 - offsetP1 > pressureDeltaDown)
    {
      relayOn (relay3Pin, true);
      
      pressureOutput();
      printLcd(3, outputPSI_rbd); 
     
      lcd.setCursor (0, 2); lcd.print (F("Depressurizing...   "));
    }
    
    doorOpen();
    platformDrop();
    relayOn (relay3Pin, false);
  }  

  // END MANUAL MODE
  //===========================================================================================

}

//=======================================================================================================
// END MAIN LOOP ========================================================================================
//=======================================================================================================


/*
// =============================================================================================
// CODE FRAGMENTS 
// =============================================================================================


//=====================================================================================
// FLASH MEMORY STRING HANDLING
//=====================================================================================

//This goes before setup()
//Write text to char strings. Previously used const_char at start of line; this didn't work

//char strLcd_0 [] PROGMEM = "Perlini Bottling    ";
//char strLcd_1 [] PROGMEM = "System, v1.0        ";
//char strLcd_2 [] PROGMEM = "                    ";
//char strLcd_3 [] PROGMEM = "Initializing...     ";

char strLcd_4 [] PROGMEM = "***MENU*** Press... ";
char strLcd_5 [] PROGMEM = "B1: Manual Mode     ";
char strLcd_6 [] PROGMEM = "B2: Autosiphon time ";
char strLcd_7 [] PROGMEM = "B3: Exit Menu       ";

char strLcd_32[] PROGMEM = "Insert bottle;      ";
char strLcd_33[] PROGMEM = "B1 raises platform  ";
char strLcd_34[] PROGMEM = "Ready...            ";
char strLcd_35[] PROGMEM = "                    ";

//Write to string table. PROGMEM moved from front of line to end; this made it work
const char *strLcdTable[] PROGMEM =  // Name of table following * is arbitrary
{   
  strLcd_20, strLcd_21, strLcd_22, strLcd_23,       // Pressurized bottle
  strLcd_24, strLcd_25, strLcd_26, strLcd_27,       // Null pressure warning
  strLcd_32, strLcd_33, strLcd_34, strLcd_35,       // Insert bottle
};

      
// Write Manual Mode intro menu text    
for (int n = 8; n <= 11; n++){
  strcpy_P(bufferP, (char*)pgm_read_word(&(strLcdTable[n])));
  printLcd (n % 4, bufferP);}
      

*/

     
