// =====================================================================================
// EEPROM CALIBRATION 
// Write null pressure to EEPROM. 
// WITH GAS OFF AND PRESSURE RELEASED FROM HOSES, Hold down all three buttons on startup 
// =====================================================================================

readButtons();

if (button1State == LOW && button2State == LOW && button3State == LOW)
{
  lcd.setCursor (0, 0); lcd.print (F("Setting EEPROM...   "));
  buzzer(1000);
  delay (1000);
  
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
  
  //numberCycles = 0;                          //Set number of lifetime cycles to initial val of 0 // For EEPROM patch, don't zero (wait--nuberCycles isn't even stored as such, is it, in EEPROM reset? 
  //autoSiphonDuration10s = 25;                //Set initial duration to 25 tenths of seconds (2.5 sec) // For EEPROM patch, don't reset
  
  EEPROM.write (0, offsetP1);                //Write zero offset for sensor1 (bottle)
  EEPROM.write (1, offsetP2);                //Write zero offset for sensor2 (regulator)
  //EEPROM.write (3, autoSiphonDuration10s);   //Write autoSiphonDuration default to 2.5 sec  //Disable for EEPROM patch
  //EEPROM.write (4, 0);                       //Write "ones" digit to zero in numberCycles01 //Disable for EEPROM patch
  //EEPROM.write (5, 0);                       //Write "tens" digit to zero in numberCycles10 //Disable for EEPROM patch

  convOffset1 = floatToString(buffer, offsetP1, 1);
  convOffset2 = floatToString(buffer, offsetP2, 1);

  outputOffset1 = "New Offset1: " + convOffset1; 
  outputOffset2 = "New Offset2: " + convOffset2; 

  printLcd(2, outputOffset1);
  printLcd(3, outputOffset2);
  delay(4000);

  //messageLcdBlank(2);
}    

// END EEPROM SET
// ==================================================================================
