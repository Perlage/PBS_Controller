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
  lcd.setCursor (0, 0); lcd.print (F("Gas off or empty;   "));
  lcd.setCursor (0, 1); lcd.print (F("check tank & hoses. "));
  lcd.setCursor (0, 2); lcd.print (F("B3 opens door.      "));
}

void messageB2B3Toggles()
{
  lcd.setCursor (0, 0); lcd.print (F("B2 toggles filling; "));
  lcd.setCursor (0, 1); lcd.print (F("B3 toggles exhaust. "));
}

void messageInsertBottle()
{
  lcd.setCursor (0, 0); lcd.print (F("Insert bottle;      "));
  lcd.setCursor (0, 1); lcd.print (F("B1 raises platform  "));
}

void messageLcdBlank()
{
  lcd.setCursor (0, 2); lcd.print (F("                    "));
}

