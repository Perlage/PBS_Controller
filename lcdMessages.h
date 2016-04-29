// ====================================================================================
// LCD MESSAGES
// ====================================================================================

void messageInitial()
{
  lcd.setCursor (0, 0);  lcd.print (F("FIZZIQ Cocktail     "));
  printLcd (1, "Bottling System " + versionSoftwareTag);
	//lcd.setCursor (0, 2);  lcd.print (F("By APPLIED FIZZICS  "));
	//lcd.setCursor (0, 3);  lcd.print (F("Initializing        "));
}

void messageGasLow()
{
  lcd.setCursor (0, 0); lcd.print (F("CO2 pressure low;   "));
  lcd.setCursor (0, 1); lcd.print (F("check tank & hoses. "));
}

void messageGasLowCarb()
{
	lcd.setCursor (0, 0); lcd.print (F("CO2 PRESSURE DROPPED"));
	lcd.setCursor (0, 1); lcd.print (F("Check CO2 source... "));
}

void messageLcdOpenDoor()
{
	lcd.setCursor (0, 2); lcd.print (F("B3 opens door.      "));
}

void messageB2B3Toggles()
{
  lcd.setCursor (0, 0); lcd.print (F("B2 toggles filling; "));
  lcd.setCursor (0, 1); lcd.print (F("B3 toggles exhaust. "));
}
/*
void messageInsertBottle()
{
  lcd.setCursor (0, 0); lcd.print (F("Insert bottle;      "));
  lcd.setCursor (0, 1); lcd.print (F("B1 raises platform. "));
}
*/
void messageInsertBottle()
{
	lcd.setCursor (0, 0); lcd.print (F("Insert bottle...    "));
	lcd.setCursor (0, 1); lcd.print (F("B1 raises platform; "));
	//lcd.setCursor (0, 2); lcd.print (F("Hold B1 until beep. ")); //Temporary to be able to see analog fill value
}

void messageLcdBlank(byte line)
{
  lcd.setCursor (0, line); lcd.print (F("                    "));
}

void messageLcdWaiting()
{
	lcd.setCursor (0, 2); lcd.print (F("Waiting...          "));
}

void messageLcdReady(byte line)
{
	lcd.setCursor (0, line); lcd.print (F("Ready...            "));
}
