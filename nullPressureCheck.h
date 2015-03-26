//============================================================================
// ANOMOLOUS PRESSURE ROUTINES
// Check for stuck pressurized bottle, implausibly low gas pressure at start, sagging pressure
//============================================================================

// PRESSURIZED BOTTLE AT STARTUP
void pressurizedBottleStartup()
{	
	delay (1000); //This gives a chance for initial message to show
	boolean inPressurizedBottleLoop = false;

	// PRESSURIZED BOTTLE LOOP
	// =========================================================================
	while (P1 - offsetP1 > pressureDeltaDown)
	{
	  //TEMPORARY Temperature measurement in null pressure loop
	  delay(500);
	  sensors.requestTemperatures();
		getTemperature(liquidThermometer);inPressurizedBottleLoop = true;
		
		// Relay3 already opened in variable initialization
	  
		lcd.setCursor (0, 0); lcd.print (F("Pressurized bottle  "));
	  lcd.setCursor (0, 1); lcd.print (F("found. Open exhaust "));
	  lcd.setCursor (0, 2); lcd.print (F("valve to vent...    "));
	  
		pressureOutput(); 
		//printLcd (3, outputPSI_b); //TEMPORARY FOR Temperature

	  buzzOnce(1000, light2Pin);
 
	  //If regulator pressure is LOWER than bottle pressure, close cylinder solenoids to conserve gas; ELSE open S4 to support platform
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

	// PRESSURIZED BOTTLE LOOP EXIT ROUTINES
	// ========================================================================
	if (inPressurizedBottleLoop)
	{
	  //lcd.clear();
	  lcd.setCursor (0, 0); lcd.print (F("Bottle depressurized"));
	  lcd.setCursor (0, 1); lcd.print (F("Press B3 to continue"));
		
		buzzer (1000);
		doorOpen();
		messageLcdReady(2);

		while (!digitalRead(button3Pin) == HIGH)
	  {
			digitalWrite(light3Pin, HIGH);
			delay(500);
			digitalWrite(light3Pin, LOW);
			delay(500);
	  }

		platformStateUp = true; //Need to pass this into function
	  platformDrop();
	  
		inPressurizedBottleLoop = false;
		button3State = HIGH;   
	} 
}
// END PRESSURIZED BOTTLE ROUTINE
// =======================================================================

// GAS OFF OR LOW AT STARTUP
// =======================================================================
void nullPressureStartup()
{
	delay (1000); //This gives a chance for initial message to show
	boolean inPressureNullLoop = false;
	
	// NULL PRESSURE LOOP
	while (P2 - offsetP2 < pressureNull)
	{
		
		//TEMPORARY Temperature measurement in null pressure loop 
		delay(500);
		sensors.requestTemperatures();
		getTemperature(liquidThermometer);
		
		inPressureNullLoop = true;
		buzzOnce(1000, light2Pin);

		// Write Null Pressure warning
		messageGasLow();
		messageLcdOpenDoor();
		
		pressureOutput();
		//printLcd (3, outputPSI_r); //TEMPORARY
		//lcd.setCursor (0, 3); lcd.print (outputPSI_r);

		// Program will loop here until gas pressure fixed. 
		while (!digitalRead(button3Pin) == LOW){
			digitalWrite(light3Pin, HIGH);
			doorOpen();
			relayOn (relay4Pin, true); //This allows us to be able to manually move platform up and down with no gas connected
			relayOn (relay5Pin, true);
		}
		relayOn (relay4Pin, false);
		relayOn (relay5Pin, false);
		digitalWrite(light3Pin, LOW);
	}

	// NULL PRESSURE LOOP EXIT
	if (inPressureNullLoop)
	{
		lcd.clear();
		lcd.setCursor (0, 2); lcd.print (F("Problem corrected..."));
		delay(2000); // Delay gives time for getting accurate pressure reading
		pressureRegStartUp = analogRead (sensorP2Pin); // Get GOOD start pressure for emergency lock loop

		inPressureNullLoop = false;
	}
}
// END NULL PRESSURE ROUTINE
// =======================================================================

//========================================================================
// PRESSURE DROP ROUTINES IN MAIN LOOP:
// Lock platform if gas pressure drops while bottle pressurized
// This function takes action if pressure drops in idle loop
//========================================================================

void idleLoopPressureDrop()
{	  
		// PRESSURE DROP LOOP
		// ===================================================================
	  while (P2 - offsetP2 < pressureRegStartUp - pressureDropAllowed) // Number to determine what constitutes a pressure drop.// 2-18 Changed from 75 to 100
	  {
		  inPressureDropLoop = true;
			
			messageGasLow();
		  messageLcdWaiting();
			
		  // Pressure measurement and output
		  pressureOutput();
		  printLcd (3, outputPSI_r);
			
			if (!digitalRead(button3Pin) == LOW)
			{
				doorOpen();
			}
			
			//If bottle is pressurized (along with pressure sagging), also lock the platform
			while (P1 - offsetP1 > pressureDeltaDown)
			{
				inPlatformEmergencyLock = true;
				relayOn (relay4Pin, false);   // Lock platform so platform doesn't creep down with pressurized bottle
				buzzOnce(2000, light2Pin);
				lcd.setCursor (0, 2); lcd.print (F("Depressurizing...   "));
				pressureDump();
				doorOpen();
				while (!digitalRead(button3Pin) == HIGH)
				{
					lcd.setCursor (0, 2); lcd.print (F("Press B3 to continue"));
				}
				platformDrop();
				//lcd.setCursor (0, 2); lcd.print (F("Platform locked...  ")); //This doesn't really tell user anything
			}
			//Only sound buzzer once
			buzzOnce(1500, light2Pin);
	  }
	  //buzzedOnce = false;
	  
		//PRESSURE DROP LOOP EXIT
		// =====================================================================
	  if (inPressureDropLoop)
	  {
		  inPressureDropLoop = false;
		  buzzedOnce = false;
		  
		  // Run this condition if had pressurized bottle
		  if (inPlatformEmergencyLock == true)
		  {
			  relayOn (relay3Pin, false);      // Re close vent if opened
			  inPlatformEmergencyLock = false;
			  messageB2B3Toggles();
		  }
		  //lcd.setCursor (0, 2); lcd.print (F("Problem corrected..."));
		  buzzOnce(1500, light2Pin);
	  }
}
//END PRESSURE DROP LOOP
//==========================================================================
