//============================================================================
// NULL PRESSURE ROUTINES
// Check for stuck pressurized bottle or implausibly low gas pressure at start 
//============================================================================

boolean inPressurizedBottleLoop = false;
boolean inPressureNullLoop = false;

// CASE 1: PRESSURIZED BOTTLE (Bottle is already depressurizing because S3 opened above)
while (P1 - offsetP1 > pressureDeltaDown)
{
  relayOn (relay3Pin, true);   // Vent bottle immediately
  inPressurizedBottleLoop = true;
  
  lcd.setCursor (0, 0); lcd.print (F("Pressurized bottle  "));
  lcd.setCursor (0, 1); lcd.print (F("Found. Open valve   "));
  lcd.setCursor (0, 2); lcd.print (F("to contol venting.  "));
  pressureOutput(); printLcd(3, outputPSI_rb); 

  buzzOnce(1000, light2Pin);
 
  //PLATFORM SUPPORT:
  //If regulator pressure is higher than bottle pressure, immediately give platform support;
  //Else, close cylinder solenoids to conserve gas
  if (P2 - offsetP2 > P1 - offsetP1) 
  {
    relayOn (relay4Pin, true);    // Support platform by opening S4
    relayOn (relay5Pin, false);  
  }
  else
  {
    relayOn (relay4Pin, false);   // Lock platform so platform doesn't creep down with pressurized bottle
    relayOn (relay5Pin, false);  
  }  
}
buzzedOnce = false;

// CASE 2: GASS OFF OR LOW       
while (P2 - offsetP2 < pressureNull) 
{
  inPressureNullLoop = true;
  buzzOnce(1000, light2Pin);

  // Write Null Pressure warning 
  lcd.setCursor (0, 0); lcd.print (F("Gas off or empty;   "));
  lcd.setCursor (0, 1); lcd.print (F("check tank & hoses. "));
  lcd.setCursor (0, 2); lcd.print (F("B3 opens door.      "));
  pressureOutput(); printLcd(3, outputPSI_r);  

  button3State = !digitalRead(button3Pin);
  delay(10); 
        
  if (button3State == LOW){
    doorOpen();
  }  
}
buzzedOnce = false;

// NULL PRESSURE EXIT ROUTINES
// No longer closing S3 at end of this to prevent popping in some edge cases of a very foamy bottle
// ==================================================================================

if (inPressurizedBottleLoop || inPressureNullLoop)
{
  delay(2000); // Delay gives time for getting accurate pressure reading
  pressureRegStartUp = analogRead (sensorP2Pin); // Get GOOD start pressure for emergency lock loop
  lcd.clear();
  doorOpen(); 
  platformStateUp = true;
  platformDrop();
  
  inPressurizedBottleLoop = false;
  inPressureNullLoop = false;

  button1State = !digitalRead(button1Pin);
  while (button1State == HIGH)
  {
    button1State = !digitalRead(button1Pin);    
    lcd.setCursor (0, 3); lcd.print (F("Press B1 to continue"));
  }
  button1State == HIGH;    
} 

// END NULL PRESSURE ROUTINE
// ===================================================================================
