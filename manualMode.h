// =======================================================================================
// MANUAL MODE
// =======================================================================================

void manualModeLoop()
{
	if (inManualModeLoop == true)
	{
		inManualModeLoop1 = true;
		sensorFillState = HIGH; // Set it high and don't read it anymore

		if (inDiagnosticMode == false) //PBSFIRM-132: Allow user to skip pressure dump in case of faulty S3 which causes stuck pressurized bottle
		{
			// FUNCTION Dump pressure
			pressureDump();

			// FUNCTION doorOpen
			//doorOpen();

			// FUNCTION Drop platform if up
			//platformDrop();
		}
	}

	while (inManualModeLoop1 == true)
	{
		// FUNCTION: Read all states of buttons, sensors
		readButtons();
		readStates();

		// FUNCTION: Read and output pressure
		pressureOutput();

		if (platformStateUp == false && switchDoorState == HIGH && !inDiagnosticMode)
		{
			lcd.setCursor(0, 1); lcd.print(F("B1 raises platform  "));
			lcd.setCursor(0, 2); lcd.print(F("B2 exits Manual Mode"));

		}
		if (platformStateUp == false && switchDoorState == LOW && !inDiagnosticMode)
		{
			lcd.setCursor(0, 1); lcd.print(F("B3 opens door;      "));
			lcd.setCursor(0, 2); lcd.print(F("B2 exits Manual Mode"));

		}
		if (platformStateUp == true && switchDoorState == HIGH && !inDiagnosticMode)
		{
			lcd.setCursor(0, 1); lcd.print(F("Close door to start;"));
			lcd.setCursor(0, 2); lcd.print(F("B3 lowers platform  "));
		}

		if (inDiagnosticMode)
		{
			lcd.setCursor(0, 1); lcd.print(F("B2: LIQUID-CAUTION! "));
			lcd.setCursor(0, 2); lcd.print(F("Close door to exit. "));
		}

		if (platformStateUp == true && switchDoorState == LOW && !inDiagnosticMode) //TODO Add a pressure check to this?
		{
			lcd.setCursor(0, 0); lcd.print(F("B1: Gas IN          "));
			lcd.setCursor(0, 1); lcd.print(F("B2: Liquid IN       "));
			if (P1 - offsetP1 >= pressureDeltaDown)
			{
				lcd.setCursor(0, 2); lcd.print(F("B3: Gas OUT         "));
			}
			else
			{
				lcd.setCursor(0, 2); lcd.print(F("B3: Door OPEN       "));
			}
			printLcd(3, outputPSI_rb);
		}
		else
		{
			lcd.setCursor(0, 0); lcd.print(F(" ***MANUAL MODE***  "));
			messageLcdBlank(3);

		}

		// B1: FUNCTION PlatformUpLoop
		if (inDiagnosticMode == false)
		{
			platformUpLoop();
		}

		platformBoost(); //To make sure platform doesn't slump

		// B1: GAS IN ================================================================
		if ((button1State == LOW && platformStateUp == true && switchDoorState == LOW) || (button1State == LOW && inDiagnosticMode == true)) //PBSFIRM-132: Allows S2 to be opened independently in Diagnostic Mode
		{
			digitalWrite(light1Pin, HIGH);
			relayOn(relay2Pin, true);
		}
		else 
		{
			digitalWrite(light1Pin, LOW);
			relayOn(relay2Pin, false);
		}

		// B2 LIQUID IN ==============================================================
		if ((button2State == LOW && platformStateUp == true && switchDoorState == LOW && (P1 - offsetP1 >= pressureDeltaDown)) || (button2State == LOW && inDiagnosticMode == true)) 
		{
			digitalWrite(light2Pin, HIGH);
			relayOn(relay1Pin, true);
		}
		else 
		{
			digitalWrite(light2Pin, LOW);
			relayOn(relay1Pin, false);
		}

		// B3 GAS OUT ================================================================
		if ((button3State == LOW && platformStateUp == true && switchDoorState == LOW && (P1 - offsetP1 >= pressureDeltaDown - 10)) || (button3State == LOW && inDiagnosticMode == true)) //PBSFIRM-132:  Allows S3 to be opened independently in Diagnostic Mode. Subtracted 10 off from pressureDeltaDown to keep foaming from re-exceeding threshold
		{
			digitalWrite(light3Pin, HIGH);
			relayOn(relay3Pin, true);
		}
		else 
		{
			digitalWrite(light3Pin, LOW);
			relayOn(relay3Pin, false);
		}

		// B3: OPEN DOOR =============================================================
		if (button3State == LOW && switchDoorState == LOW && (P1 - offsetP1 < pressureDeltaDown))
		{
			digitalWrite(light3Pin, HIGH);
			doorOpen();
			button3State = HIGH; //This fixed a bug where opening door passed button state into next loop and was setting platform state to down when still up
			digitalWrite(light3Pin, LOW);
		}

		// B3: DROP PLATFORM =========================================================
		if (button3State == LOW && switchDoorState == HIGH && (P1 - offsetP1 < pressureDeltaDown) && !inDiagnosticMode) //PBSFIRM-132: Last condition allows you to independently open S3 with no bottle in diagnostic mode
		{
			while (button3State == LOW)
			{
				button3State = !digitalRead(button3Pin);
				digitalWrite(light3Pin, HIGH);
				relayOn(relay4Pin, false);
				relayOn(relay5Pin, true);
			}
			digitalWrite(light3Pin, LOW);
			relayOn(relay5Pin, false);
			platformStateUp = false;
			EEPROM.write(6, platformStateUp);
		}

		// B2 EXITS if pressure low and platform down
		while (button2State == LOW && platformStateUp == false && (P1 - offsetP1 < pressureDeltaDown) && !inDiagnosticMode)
		{
			button2State = !digitalRead(button2Pin);
			inManualModeLoop1 = false;
			buzzOnce(500, light2Pin);
		}
		buzzedOnce = false;

		//Exit DiagnosticMode back to Manual Mode when door closes
		if (inDiagnosticMode && switchDoorState == LOW)
		{
			inDiagnosticMode = false;
		}
	}

	//MANUAL MODE LOOP EXIT ROUTINES 
	//==================================================
	if (inManualModeLoop)
	{
		inManualModeLoop = false;
		lcd.clear();
		lcd.setCursor(0, 0); lcd.print(F("EXITING MANUAL MODE "));
	}
}
