// =======================================================================================
// REUSED MAJOR LOOPS
// =======================================================================================

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

