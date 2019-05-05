// ====================================================================================
// MENU SHELL
// ====================================================================================

void menuExit()
{
	inMenuLoop = false;
	lcd.clear();
	printLcd(0, "FIZZIQ " + versionSoftwareTag);
	lcd.setCursor(0, 2); lcd.print(F("Exiting menu...     "));
	buzzer(250);
	delay(750);  //To show message
}

void menuShell(boolean inMenuLoop)
{
	boolean inMenuLoop1 = false;
	boolean inMenuLoop2 = false;
	boolean menuOption11 = false; //AutoSiphon
	boolean menuOption12 = false; //Cleaning
	boolean menuOption21 = false; //Carbonation
	boolean menuOption22 = false; //Manual Diagnostic Mode

	if (inMenuLoop == true)
	{
		inMenuLoop1 = true;

		lcd.setCursor(0, 0); lcd.print(F("   *** MENU 1 ***   "));
		lcd.setCursor(0, 1); lcd.print(F("B1: Set AutoSiphon  "));
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
			buttonPush(button1Pin, light1Pin, 250);
			button1State = !digitalRead(button1Pin);

			lcd.setCursor(0, 0); lcd.print(F("B1: Incr. by .1 sec "));
			lcd.setCursor(0, 1); lcd.print(F("B2: Decr. by .1 sec "));
			lcd.setCursor(0, 2); lcd.print(F("B3: Set value & exit"));
			// Current value is printed to screen on line 3
		}

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
				lcd.setCursor(0, 1); lcd.print(F("B1: Enter Cleaning  "));
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
			lcd.setCursor(0, 1); lcd.print(F("B1: Carbonation Mode"));
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
			button1State = !digitalRead(button1Pin);

			lcd.clear();
			lcd.setCursor(0, 0); lcd.print(F("**CARBONATION MODE**"));
			lcd.setCursor(0, 1); lcd.print(F("Press B1 to begin.  "));
			//lcd.setCursor (0, 3); lcd.print (F("                    "));   
			buzzOnce(500, light1Pin);
		}
		buzzedOnce = false;

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

	// Menu Option 11: AUTOSIPHON SET
	// ===============================================================================

	while (menuOption11 == true)
	{
		inMenuOption11Loop = true;
		autoSiphonSet();

		if (!inMenuOption11Loop)
		{
			menuOption11 = false;
			menuExit();
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
			menuExit();
			buzzedOnce = false;
		}
	}

	// Menu Option 21: CARBONATION MODE
	// ===============================================================================

	while (menuOption21 == true)
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
