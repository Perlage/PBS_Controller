//========================================================================================
// CARBONATOR STUCK BOTTLE CATCH LOOP //TO DO: Do we still need this?
//========================================================================================
  
while (sensorFillState == LOW)
{    
readButtons();
lcd.setCursor (0, 0); lcd.print (F("B1: Manual Siphon  "));
lcd.setCursor (0, 1); lcd.print (F("B2: Depressurize   "));
lcd.setCursor (0, 2); lcd.print (F("Waiting...         "));

// This is manual autosiphon
while (button1State == LOW)
{
  readButtons();
  relayOn (relay1Pin, true);
  relayOn (relay2Pin, true);
}  
relayOn (relay1Pin, false);
relayOn (relay2Pin, false);

delay (25);
sensorFillState = digitalRead(sensorFillPin); 
delay (25); 

pressureOutput();
printLcd (3, outputPSI_rb); 
 
// Emergency exit
if (button2State == LOW)
{
  pressureDump();
  relayOn (relay3Pin, true);
  delay(2000);
  doorOpen();
  platformDrop();
  relayOn (relay3Pin, false);
  sensorFillState == HIGH;
}  


