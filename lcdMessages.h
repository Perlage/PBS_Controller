// ====================================================================================
// LCD MESSAGES
// ====================================================================================

void messageInitial()
{
  lcd.setCursor (0, 0);  lcd.print (F("FIZZIQ Cocktail     "));
  printLcd (1, "Bottling System " + versionSoftwareTag);
  lcd.setCursor (0, 3);  lcd.print (F("Initializing...     "));
}

void messageGasLow()
{
  lcd.setCursor (0, 0); lcd.print (F("CO2 pressure low;   "));
  lcd.setCursor (0, 1); lcd.print (F("check tank & hoses. "));
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
	lcd.setCursor (0, 0); lcd.print (F("Insert bottle;      "));
	lcd.setCursor (0, 1); lcd.print (F("B1 raises platform. "));
	lcd.setCursor (0, 2); lcd.print (F("Hold B1 until beep. "));
}

// messageLcdBlankLn2(); // MESSAGE: "                    "
void messageLcdBlankLn2()
{
  lcd.setCursor (0, 2); lcd.print (F("                    "));
}

void messageLcdWaiting()
{
	lcd.setCursor (0, 2); lcd.print (F("Waiting...          "));
}

void messageLcdReady()
{
	lcd.setCursor (0, 2); lcd.print (F("Ready...            "));
}
