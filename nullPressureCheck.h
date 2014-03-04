  //============================================================================
  // NULL PRESSURE ROUTINES
  // Check for stuck pressurized bottle or implausibly low gas pressure at start 
  //============================================================================

  boolean inPressurizedBottleLoop = false;
  boolean inPressureNullLoop = false;

  // CASE 1: PRESSURIZED BOTTLE (Bottle is already depressurizing because S3 opened above)
  if (P1 - offsetP1 > pressureDeltaDown)
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
  if (P2 - offsetP2 < pressureNull) 
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
