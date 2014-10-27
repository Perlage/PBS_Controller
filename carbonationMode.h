

// =============================================================================================
// CARBONATION MODE
// =============================================================================================

boolean inTimingLoop      = false;   
float timerTime               = 0;       //Timer in sec   
unsigned long timerStart      = 0;       //Init value of millis() going into loop in ms. MUST BE LONG INT!
unsigned long timerTimeMs     = 0;       //Timer value in ms
byte timerTimeMin             = 0;       //Timer value, minutes part
byte timerTimeSec             = 0;       //Timer value, seconds part
int P2Start;                             //Initial regulator reading going into loop
float PSIStart;
float timerShakeTime;                    //Time of shaking in sec, less the number of 15 second rest periods               
boolean inShakeState;                    //Whether r not should be shaking in given 15 sec interval
String shakeState;                       //REST or SHAKE
float pressureDipTargetInit  = 12.8 * 8; //Initial pressure slump user is trying to hit at T=0 (12.7 units per psi)
float pressureDipTarget;                 //Target pressure dip goes down over time as liquid saturates. dipTarget decreases by factor of 1, 1/2, 1/3, 1/4... 

button1State = !digitalRead(button1Pin); delay(10);
while (button1State == LOW)
{
  inTimingLoop = true;
  button1State = !digitalRead(button1Pin); delay(10);
  timerStart = millis();
  P2Start = analogRead(sensorP2Pin);
  PSIStart = pressureConv2(P2Start);
  lcd.setCursor (0, 1); lcd.print (F("Press B3 to exit.  "));
}

while (inTimingLoop == true)
{
  timerTimeMs = millis() - timerStart;
  timerTime = float(timerTimeMs) / 1000;       //Get total seconds since start
  timerTimeSec = int(timerTime) % 60;          //Get 0-60 seconds
  timerTimeMin = int(timerTime/60);            //Get minutes
  
  String (convTimeMin) = floatToString(buffer, timerTimeMin, 0);
  String (convTimeSec) = floatToString(buffer, timerTimeSec, 0);
  
  //Find shakeState. Rest first and every other 15 second segment
  if (timerTime <= 240)
  {
    if ((timerTimeSec >=  0 && timerTimeSec < 15) || (timerTimeSec >= 30 && timerTimeSec < 45))
    {
      inShakeState = false;
      shakeState = "REST...";
    } 
    else
    {
      inShakeState = true;
      shakeState = "SHAKE..."; 
    }
  }
  else
  {
    shakeState = "DONE.";
  }
    
  //Need this IF to account for the fact that seconds 0-9 don't have leading zero
  if (timerTimeSec <= 9){
    printLcd(2, "Timer: " + convTimeMin + ":" + "0" + convTimeSec + " " + shakeState);}
  else{
    printLcd(2, "Timer: " + convTimeMin + ":" + convTimeSec + " " + shakeState);}  

  //Buzz every 15 sec  
  if (timerTimeSec % 15 == 0){
    buzzer(1000);}  
  
  //Get pressure reading
  P2 = analogRead(sensorP2Pin);
  
  //Convert current P reading and start pressure to float
  PSI2 = pressureConv2(P2); 
  PSIdiff = PSIStart - PSI2; // This is the key measure of how much the pressure is dipping
  
  //PSIdiff  = constrain (PSIStart - PSI2, 0, 50);
  convPSI2 = floatToString(buffer, PSI2, 1);
  convPSIdiff = floatToString(buffer, PSIdiff, 1);

  //Get the factor to reduce target dip by
  float timerShakeTimeSegment = int ((timerTime) / 30) + 1; //Get the number of the 15sec shake interval
  pressureDipTarget = pressureDipTargetInit * (1 / timerShakeTimeSegment); //So, dipTarget decreases by factor of 1, 1/2, 1/3, 1/4... 

  float convPressureDipTarget = pressureConv2(pressureDipTarget + offsetP2); //Need to add back offset because function subtracts it
  String convPSItarget = floatToString(buffer, convPressureDipTarget, 1);
  String outputPSI_td  = "Goal:" + convPSItarget + " You:" + convPSIdiff + "psi";
  printLcd(3, outputPSI_td);  

  //Give user positive feedback by beeping when there is a significant pressure drop while shaking 
  if (P2 < P2Start - pressureDipTarget && inShakeState == true){
    buzzer(50);}  
   
  //Exit routine
  button3State = !digitalRead(button3Pin); delay(10);
  while (button3State == LOW)
  {
    button3State = !digitalRead(button3Pin); delay(10);
    inTimingLoop = false;
    menuOption21 = false;
    buzzOnce(500, light3Pin);
  }
  buzzedOnce = false;
}