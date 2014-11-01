// =============================================================================================
// CODE FRAGMENTS 
// =============================================================================================

	/*
  //Convert current P reading and start pressure to float
  PSI2 = pressureConv2(P2); 
  PSIdiff = PSIStart - PSI2; // This is the key measure of how much the pressure is dipping
  
  //PSIdiff  = constrain (PSIStart - PSI2, 0, 50);
  convPSI2 = floatToString(buffer, PSI2, 1);
  convPSIdiff = floatToString(buffer, PSIdiff, 1);

  //Get the factor to reduce target dip by
  float timerShakeTimeSegment = int ((timerTime) / 30) + 1; //Get the number of the 15sec shake interval
  pressureDipTarget = pressureDipTargetInit * (1 / timerShakeTimeSegment); //So, dipTarget decreases by factor of 1, 1/2, 1/3, 1/4... 

  float convPressureDipTarget = pressureConv2(pressureDipTarget + offsetP2); //Need to add back offset because function subtracts it
  String convPSItarget = floatToString(buffer, convPressureDipTarget, 1);
  String outputPSI_td  = "Goal:" + convPSItarget + " You:" + convPSIdiff + "psi";
  printLcd(3, outputPSI_td); 
	*/

//Allow user to invoke Manual Mode from bootup before anything else happens (buttons get read in EEPROM include)
while (button1State == LOW)
{
	inManualModeLoop = true;
	button1State = !digitalRead(button1Pin);
	lcd.setCursor (0, 0); lcd.print (F("Entering Manual Mode"));
	buzzOnce(500, light1Pin);
}
buzzedOnce = false;

if (inManualModeLoop == true){
	manualModeLoop();
}

/*For testing button push
while(1==1)
{
	buttonPush(button2Pin, light2Pin, 500);
}
*/
	
// FROM BOOTUP ROUTINE
//===================================================================================


//NOW print lifetime fills and autosiphon duration
//autoSiphonDurationSec = float(autoSiphonDuration10s) / 10;
//String (convInt) = floatToString(buffer, autoSiphonDurationSec, 1);
//String (outputInt) = "Autosiphon: " + convInt + " sec";
//printLcd (2, outputInt);
//delay(1000);


/*
// FLASH MEMORY STRING HANDLING
//=====================================================================================

//char bufferP[30];                                // make sure this is large enough for the largest string it must hold; used for PROGMEM write to LCD
//byte strIndex;                                   // Used to refer to index of the string in *srtLcdTable (e.g., strLcd_0 has strLcdIndex = 0

//This goes before setup()
//Write text to char strings. Previously used const_char at start of line; this didn't work

char strLcd_0 [] PROGMEM = "Perlini Bottling    ";
char strLcd_1 [] PROGMEM = "System, v1.0        ";
char strLcd_2 [] PROGMEM = "                    ";
char strLcd_3 [] PROGMEM = "Initializing...     ";

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
  strLcd_0, strLcd_1, strLcd_2, strLcd_3,       
  strLcd_4, strLcd_5, strLcd_6, strLcd_7,     
  strLcd_32, strLcd_33, strLcd_34, strLcd_35,   
};
      
// Write Manual Mode intro menu text    
for (int n = 8; n <= 11; n++){
  strcpy_P(bufferP, (char*)pgm_read_word(&(strLcdTable[n])));
  printLcd (n % 4, bufferP);}
	

strcpy_P(bufferP, (char*)pgm_read_word(&(strLcdTable[n])));
printLcd (line, bufferP);
  
*/  
//===============================================================================================
 
  /*
  Serial.print ("StartPress: "); 
  Serial.print (pressureRegStartUp);  
  Serial.print (" P1: "); 
  Serial.print (P1);  
  Serial.print (" P2: "); 
  Serial.print (P2); 
  Serial.println ();
  */

  /*
  Serial.print ("millis(): "); 
  Serial.print (millis()); 
  Serial.print (" timerStart: "); 
  Serial.print (timerStart); 
  Serial.print (" timerTime: "); 
  Serial.print (timerTime); 
  Serial.print (" timerTimeMin: "); 
  Serial.print (timerTimeMin); 
  Serial.print (" timerTimeSec: "); 
  Serial.print (timerTimeSec); 
  Serial.println ();
  */
      

// STARTUP ROUTINE
  // /* Comment out preceding comment delimeter for normal operation--696 bytes
  // ################################################################################

  // If P1 is not high, then there is no bottle, or bottle pressure is low. So raise platform--but take time to make user close door, so no pinching

  /*
  while (switchDoorState == HIGH) // Make sure door is closed
  {
    switchDoorState = digitalRead(switchDoorPin); 
    lcd.setCursor (0, 2); lcd.print (F("PLEASE CLOSE DOOR..."));
    buzzer(100);
    delay(100);
  }
  messageLcdBlank();
  delay(500);  // A little delay after closing door before raising platform

  relayOn(relay4Pin, true);  // Now Raise platform     
  */

  /*
  // Blinks lights and give time to de-pressurize stuck bottle
  for (int n = 1; n < 0; n++)
  {
    digitalWrite(light1Pin, HIGH); delay(500);
    digitalWrite(light2Pin, HIGH); delay(500);
    digitalWrite(light3Pin, HIGH); delay(500);
    digitalWrite(light1Pin, LOW);
    digitalWrite(light2Pin, LOW);
    digitalWrite(light3Pin, LOW); delay(325);
  }
  */
    /*
  // Blinks lights and give time to de-pressurize stuck bottle
  for (int n = 1; n < 0; n++)
  {
    digitalWrite(light1Pin, HIGH); delay(500);
    digitalWrite(light2Pin, HIGH); delay(500);
    digitalWrite(light3Pin, HIGH); delay(500);
    digitalWrite(light1Pin, LOW);
    digitalWrite(light2Pin, LOW);
    digitalWrite(light3Pin, LOW); delay(325);
  }
  */
