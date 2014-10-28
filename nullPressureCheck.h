//============================================================================
// NULL PRESSURE ROUTINES
// Check for stuck pressurized bottle or implausibly low gas pressure at start 
//============================================================================

void nullPressureStartup()
{	
	boolean inPressurizedBottleLoop = false;
	boolean inPressureNullLoop = false;

	// CASE 1: PRESSURIZED BOTTLE (Bottle is already depressurizing because S3 opened above)
	while (P1 - offsetP1 > pressureDeltaDown)
	{
	  relayOn (relay3Pin, true);   // Vent bottle immediately
	  inPressurizedBottleLoop = true;
  
	  lcd.setCursor (0, 0); lcd.print (F("Pressurized bottle! "));
	  lcd.setCursor (0, 1); lcd.print (F("Open exhaust valve  "));
	  lcd.setCursor (0, 2); lcd.print (F("for faster venting. "));
	  pressureOutput(); 
		printLcd2(3, outputPSI_rb, throttleVal); 

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

	// CASE 2: GAS OFF OR LOW       
	while (P2 - offsetP2 < pressureNull) 
	{
	  inPressureNullLoop = true;
	  buzzOnce(1000, light2Pin);

	  // Write Null Pressure warning 
	  messageGasLow();
	  pressureOutput(); 
		printLcd2(3, outputPSI_r, throttleVal);  

	  button3State = !digitalRead(button3Pin);
        
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

	  buzzer (500);
	  doorOpen(); 

	  lcd.clear();
	  lcd.setCursor (0, 0); lcd.print (F("Bottle depressurized"));
	  lcd.setCursor (0, 1); lcd.print (F("Press B3 to continue"));
  
	  platformStateUp = true;
	  inPressurizedBottleLoop = false;
	  inPressureNullLoop = false;
  
	  button3State = !digitalRead(button3Pin);
	  while (button3State == HIGH)
	  {
		button3State = !digitalRead(button3Pin);    
	  }
  
	  digitalWrite(light3Pin, HIGH);
	  buzzer(500);
	  digitalWrite(light3Pin, LOW);

	  platformDrop();
	  button3State = HIGH;   
	} 
}
// END NULL PRESSURE ROUTINE
// =======================================================================

//========================================================================
// EMERGENCY PLATFORM LOCK LOOP:
// Lock platform if gas pressure drops while bottle pressurized
// This function takes action if pressure drops in idle loop
//========================================================================

void idleLoopPressureDrop()
{	  
		// If pressure drops, go into this loop and wait for user to fix
	  while (P2 - offsetP2 < pressureRegStartUp - pressureDropAllowed) // Number to determine what constitutes a pressure drop.// 2-18 Changed from 75 to 100
	  {
		  inPressureDropLoop = true;

		  P1 = analogRead(sensorP1Pin);
		  P2 = analogRead(sensorP2Pin);
		  
		  //If bottle is pressurized (along with pressure sagging), also lock the platform
		  if (P1 - offsetP1 > P2 - offsetP2)
		  {
			  inPlatformEmergencyLock = true;
			  relayOn (relay4Pin, false);   // Lock platform so platform doesn't creep down with pressurized bottle
			  relayOn (relay3Pin, true);    // Vent the bottle to be safe

			  lcd.setCursor (0, 2); lcd.print (F("Platform locked...  "));
			  buzzOnce(2000, light2Pin);
		  }
		  
		  lcd.setCursor (0, 0); lcd.print (F("Input pressure drop;"));
		  lcd.setCursor (0, 1); lcd.print (F(""));
		  lcd.setCursor (0, 2); lcd.print (F("Ready...            "));
		  
		  // Pressure measurement and output
		  pressureOutput();
		  printLcd (3, outputPSI_rb);

		  //Only sound buzzer once
		  buzzOnce(2000, light2Pin);
	  }
	  buzzedOnce = false;
	  
	  if (inPressureDropLoop)
	  {
		  inPressureDropLoop = false;
		  buzzedOnce = false;
		  
		  // Run this condition if had pressurized bottle
		  if (inPlatformEmergencyLock == true)
		  {
			  relayOn (relay4Pin, true);       // Re-open platform UP solenoid
			  relayOn (relay3Pin, false);      // Re close vent if opened
			  inPlatformEmergencyLock = false;
			  
			  messageB2B3Toggles();
		  }
		  
		  lcd.setCursor (0, 2); lcd.print (F("Problem corrected..."));
		  delay(1000);
	  }
}
//END EMERGENCY PLATFORM LOCK LOOP
//======================================================================================
