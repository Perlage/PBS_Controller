// =============================================================================================
// CARBONATION MODE
// =============================================================================================

float timerTime = 0;
int timerStart = 0;
int timerTimeMs = 0;
int timerTimeMin = 0;
int timerTimeSec = 0;
int P2Start = 0;
boolean inTimingLoop = false;
String shakeState;

button1State = !digitalRead(button1Pin); delay(10);
while (button1State == LOW)
{
  inTimingLoop = true;
  button1State = !digitalRead(button1Pin); delay(10);
  timerStart = millis();
  P2Start = analogRead(sensorP2Pin);
}

while (inTimingLoop == true)
{
  timerTimeMs = millis() - timerStart;
  timerTime = float(timerTimeMs) / 1000;       //Get total seconds since start
  timerTimeSec = int(timerTime) % 60;          //Get 0-60 seconds
  timerTimeMin = int(timerTime/60);            //Get minutes
  
  /*
  Serial.print ("millis(): "); 
  Serial.print (millis()); 
  Serial.print (" timerStart: "); 
  Serial.print (timerStart); 
  Serial.print (" timerTime: "); 
  Serial.print (timerTime); 
  Serial.print (" timerTimeMin: "); 
  Serial.print (timerTimeMin); 
  Serial.print (" timerTimeSec: "); 
  Serial.print (timerTimeSec); 
  Serial.println ();
  */

  String (convTimeMin) = floatToString(buffer, timerTimeMin, 0);
  String (convTimeSec) = floatToString(buffer, timerTimeSec, 0);
  
  //Find shakeState. Rest first and every other 15 second segment
  if ((timerTimeSec >=  0 && timerTimeSec < 15) || (timerTimeSec >= 30 && timerTimeSec < 45)){
    shakeState = "REST...";}
  else{
    shakeState = "SHAKE...";}
  
  //Buzz every 15 sec  
  if (timerTimeSec % 15 == 0){
    buzzer(1000);}  
  
  //Need this IF to account for the fact that seconds 0-9 don't have leading zero
  if (timerTimeSec <= 9){
    printLcd(2, "Timer: " + convTimeMin + ":" + "0" + convTimeSec + " " + shakeState);}
  else{
    printLcd(2, "Timer: " + convTimeMin + ":" + convTimeSec + " " + shakeState);}          
  
  //Get pressure reading
  P2 = analogRead(sensorP2Pin);
  
  //Convert current P reading and start pressure to float
  PSI2 = pressureConv2(P2); 
  float PSIStart = pressureConv2(P2Start);
  PSIdiff  = constrain (PSIStart - PSI2, 0, 50);
  
  //Display
  convPSI2             = floatToString(buffer, PSI2, 1);
  convPSIdiff          = floatToString(buffer, PSIdiff, 1);
  String outputPSI_rd  = "Reg:" + convPSI2 + " Dip:" + convPSIdiff + " psi";
  printLcd(3, outputPSI_rd);  
  
  //Beep if there is a significant pressure drop of 2 psi (12.71 x 2)
  if (P2 < pressureRegStartUp - 26){
    buzzer(50);}  
 
  lcd.setCursor (0, 1); lcd.print (F("Press B3 to exit.  "));
  
  //Exit routine
  button3State = !digitalRead(button3Pin); delay(10);
  while (button3State == LOW)
  {
    button3State = !digitalRead(button3Pin); delay(10);
    inTimingLoop = false;
    menuOption21 = false;
    timerTime = 0;
    timerStart = 0;
    timerTimeMin = 0;
    timerTimeSec = 0;
    buzzOnce(500, light3Pin);
  }
  buzzedOnce = false;
}
