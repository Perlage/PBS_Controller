// ====================================================================================
// MENU SHELL
// ====================================================================================

void menuExit()
{
	inMenuLoop = false;
	lcd.clear();
	printLcd(0, "Firmware " + versionSoftwareTag);

	//Print total fills (odometer reading)
	String convInt = floatToString(buffer, numberCycles, 0);
	String outputInt = "Total fills: " + convInt;
	printLcd(1, outputInt);

	//Print session fills
	String(convNumberCyclesSession) = floatToString(buffer, numberCyclesSession, 0);
	outputInt = "Session fills: " + convNumberCyclesSession;
	printLcd(2, outputInt);						

	lcd.setCursor(0, 3); lcd.print(F("Exiting menu...     "));
	buzzer(250);
	delay(1500);  //To show message
}

void menuShell(boolean inMenuLoop)
{
	boolean inMenuLoop1 = false;
	boolean inMenuLoop2 = false;
	boolean menuOption11 = false;	//Carbonation
	boolean menuOption12 = false;	//Cleaning//Cleaning
	boolean menuOption21 = false;	//Autosiphon (or AutoLevel)
	boolean menuOption22 = false;	//Manual Diagnostic Mode

	if (inMenuLoop == true)
	{
		inMenuLoop1 = true;

		lcd.setCursor(0, 0); lcd.print(F("   *** MENU 1 ***   "));
		lcd.setCursor(0, 1); lcd.print(F("B1: Carbonation Mode"));
		lcd.setCursor(0, 2); lcd.print(F("B2: Cleaning Mode   "));
		lcd.setCursor(0, 3); lcd.print(F("B3: More...         "));
	}
	buzzedOnce = false;

	while (inMenuLoop1 == true)
	{
		readButtons();

		while (button1State == LOW)
		{
			menuOption11 = true;
			inMenuLoop1 = false;
			button1State = !digitalRead(button1Pin);

			lcd.clear();
			lcd.setCursor(0, 0); lcd.print(F("**CARBONATION MODE**"));
			lcd.setCursor(0, 1); lcd.print(F("Press B1 to begin.  "));
			//lcd.setCursor (0, 3); lcd.print (F("                    "));   
			buzzOnce(500, light1Pin);
		}
		buzzedOnce = false; //Not sure if this is part of Carbonation mode

		while (button2State == LOW)
		{
			menuOption12 = true;
			inMenuLoop1 = false;
			buttonPush(button2Pin, light2Pin, 250);
			button2State = !digitalRead(button2Pin);

			lcd.setCursor(0, 0); lcd.print(F("***CLEANING MODE*** "));
			
			if (inCleaningMode == true) //PBSFIRM-136
			{
				lcd.setCursor(0, 1); lcd.print(F("B3: Exit Cleaning   "));
			}
			else
			{
				lcd.setCursor(0, 1); lcd.print(F("B1: Start Cleaning  "));
			}
			messageLcdBlank(2);
			messageLcdBlank(3);
		}

		while (button3State == LOW)
		{
			inMenuLoop2 = true;
			inMenuLoop1 = false;
			buttonPush(button3Pin, light3Pin, 250); //v1.1 new function
			button3State = !digitalRead(button3Pin);

			lcd.setCursor(0, 0); lcd.print(F("   *** MENU 2 ***   "));
			lcd.setCursor(0, 1); lcd.print(F("B1: Set Auto Level  "));
			lcd.setCursor(0, 2); lcd.print(F("B2: Manual Mode     "));
			lcd.setCursor(0, 3); lcd.print(F("B3: Exit...         "));
		}
	}

	while (inMenuLoop2 == true)
	{
		readButtons();

		while (button1State == LOW)
		{
			menuOption21 = true;
			inMenuLoop2 = false;
			buttonPush(button1Pin, light1Pin, 250);
			button1State = !digitalRead(button1Pin);

			lcd.setCursor(0, 0); lcd.print(F("B1: Incr. by .1 sec "));
			lcd.setCursor(0, 1); lcd.print(F("B3: Decr. by .1 sec "));
			lcd.setCursor(0, 2); lcd.print(F("B2: Set value & exit"));
			// Current value is printed to screen on line 3
		}

		while (button2State == LOW)
		{
			menuOption22 = true;
			inMenuLoop2 = false;
			button2State = !digitalRead(button2Pin);

			lcd.clear();
			lcd.setCursor(0, 0); lcd.print(F(" ***MANUAL MODE***  "));
			lcd.setCursor(0, 1); lcd.print(F("Press B1 to begin.  "));
			//lcd.setCursor (0, 2); lcd.print (F("CAUTION: Fill Sensor"));
			//lcd.setCursor (0, 3); lcd.print (F("disabled in Manual. "));
			buzzOnce(500, light2Pin);
		}
		buzzedOnce = false;

		while (button3State == LOW)
		{
			inMenuLoop2 = false;
			menuExit();
			while (button3State == LOW)
			{
				button3State = !digitalRead(button3Pin); //This catches loop until release 
			} 
		}
	}

	// ===============================================================================
	// MENU OPTIONS
	// ===============================================================================
	
	// Menu Option 21: AUTOSIPHON SET //FIZFIRM-12: These options are disordered now that menu has been rearranged
	// ===============================================================================

	while (menuOption21 == true)
	{
		inMenuOption11Loop = true;
		autoSiphonSet();

		if (!inMenuOption11Loop)
		{
			menuOption21 = false;
			//menuExit();			//FIZFIRM-15
			inMenuLoop = false;		//FIZFIRM-15
		}
	}

	// Menu Option 12: CLEANING MODE SET
	// ===============================================================================

	while (menuOption12 == true)
	{
		readButtons();
		if (inCleaningMode == false)  //PBSFIRM-136: Only the relevant button should be active
		{
			while (button1State == LOW)
			{
				button1State = !digitalRead(button1Pin);
				inCleaningMode = true;
				lcd.setCursor(0, 3); lcd.print(F("Cleaning Mode *ON*  "));
				buzzOnce(500, light1Pin);
				delay(500);
				menuOption12 = false;
			}
		}
		else
		{
			while (button3State == LOW)
			{
				button3State = !digitalRead(button3Pin);
				inCleaningMode = false;
				lcd.setCursor(0, 3); lcd.print(F("Cleaning Mode *OFF* "));
				buzzOnce(500, light3Pin);
				delay(500);
				menuOption12 = false;
			}
		}
		if (menuOption12 == false) {
			//menuExit();			//FIZFIRM-15
			inMenuLoop = false;		//FIZFIRM-15
			buzzedOnce = false;		//FIZFIRM-15
		}
	}

	// Menu Option 11: CARBONATION MODE
	// ===============================================================================

	while (menuOption11 == true)
	{
		#include "CarbonationMode.h" 
		//TODO Carbonation Mode has been commented out. FIX BEFORE RELEASE!
	}

	// Menu Option 22: MANUAL MODE
	// ===============================================================================

	while (menuOption22 == true)
	{
		readButtons();
		while (!digitalRead(button1Pin) == LOW)
		{
			inManualModeLoop = true;
			menuOption22 = false;
			lcd.setCursor(0, 3); lcd.print(F("Manual Mode *ON*    "));
			buzzOnce(500, light1Pin);
			digitalWrite(light1Pin, HIGH); //Keeps light on while
			//delay(500);

			// This a secret diagnostic mode (B1 + B2) that disables all pressure checks! DOOR MUST BE OPEN
			// A 187ml bottle will fill in 4 seconds in this mode
			while (!digitalRead(button2Pin) == LOW) 
			{
				inDiagnosticMode = true;
				digitalWrite(light2Pin, HIGH);
				lcd.setCursor(0, 3); lcd.print(F("DIAGNOSTIC MODE *ON*"));
				buzzer(500); 
				//delay(500);
			}
			digitalWrite(light2Pin, LOW);
		}
		digitalWrite(light1Pin, LOW);
		buzzedOnce = false;
		if (menuOption22 == false)
		{
			manualModeLoop();
			inMenuLoop = false;
		}
	}
}
