

// =============================================================================================
// CARBONATION MODE
// =============================================================================================

boolean inTimingLoop      = false;   
float timerTime               = 0;					//Timer in sec   
unsigned long timerStart      = 0;					//Init value of millis() going into loop in ms. MUST BE LONG INT!
unsigned long timerTimeMs     = 0;					//Timer value in ms
byte timerTimeMin             = 0;					//Timer value, minutes part
byte timerTimeSec             = 0;					//Timer value, seconds part
int P2Start;																//Initial regulator reading going into loop
float PSIStart;
float timerShakeTime;												//Time of shaking in sec, less the number of 15 second rest periods               
boolean inShakeState;												//Whether or not should be shaking in given interval
boolean inShakeStateNEW;                  

String shakeState;													//REST or SHAKE
float pressureDipTargetInit  = 6 * 12.71;		//Initial pressure slump user is trying to hit at T=0 (12.7 units per psi)
float pressureDipTarget;										//Target pressure dip goes down over time as liquid saturates. dipTarget decreases by factor of 1, 1/2, 1/3, 1/4...
float percentEffort; 

button1State = !digitalRead(button1Pin); delay(10);
while (button1State == LOW)
{
  inTimingLoop = true;
  button1State = !digitalRead(button1Pin); delay(10);
  timerStart = millis();
  P2Start = analogRead(sensorP2Pin);
  PSIStart = pressureConv2(P2Start);
  lcd.setCursor (0, 1); lcd.print (F("Press B3 to exit.   "));
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
      inShakeStateNEW = false;
      shakeState = "REST...";
    } 
    else
    {
      inShakeStateNEW = true;
      
			if (percentEffort < 100)
			{
				shakeState = "SHAKE HARDER...";
			}
			else
			{
				shakeState = "GOOD!"; 
			}

			//This turned out to be problematic
			/*
			if (inShakeStateNEW != inShakeState)
			{
				 P2Start = analogRead(sensorP2Pin); // get a fresh reading every 30 sec
			}
			*/
    }
  }
  else
  {
    shakeState = "DONE.";
		buzzOnce(1500, light3Pin);
		inTimingLoop = false;
		menuOption21 = false;
  }
  inShakeState = inShakeStateNEW;
	
	//Write initial message...
	if (timerTime < 15)
	{
		shakeState = "GET READY...";
	}
	  
  //Need this IF to account for the fact that seconds 0-9 don't have leading zero
  if (timerTimeSec <= 9){
    printLcd(2, convTimeMin + ":" + "0" + convTimeSec + " " + shakeState);}
  else{
    printLcd(2, convTimeMin + ":" + convTimeSec + " " + shakeState);}  

  //Buzz every 15 sec  
  if (timerTimeSec % 15 == 0){
    buzzer(1000);}  
  
  //Get pressure reading
  P2 = analogRead(sensorP2Pin);
  	
	int timerShakeTimeSegment = int ((timerTime) / 30) + 1; //Get the number of the 15sec shake interval
	int pressureDiff = constrain ((P2Start - P2), 0, 999);
	
	float pressureDipTarget = float (pressureDipTargetInit / timerShakeTimeSegment);   //So, dipTarget decreases by factor of 1, 1/2, 1/3, 1/4...
	percentEffort =  constrain(((P2Start - P2) / pressureDipTarget) * 100, 0, 999);				 //Just use raw integer readings--don't need to worry about offsets. Constrain keeps above 0
	
	String stringPercentEffort = floatToString (buffer, percentEffort, 0);
	String strPressureDipTarget = floatToString(buffer, pressureDipTarget, 0);
	String strPressureDip = floatToString(buffer, pressureDiff, 0);
	
	if (inShakeState)
	{
		String outputPSI_td  = "Goal:" + strPressureDipTarget + " > " + strPressureDip + " (" + stringPercentEffort + "%)";
		printLcd2 (3, outputPSI_td, 1000); 	
	}
	else
	{
		String outputPSI_td  = "Next Goal: " + strPressureDipTarget + " units";
		printLcd2 (3, outputPSI_td, 1000); 
	}
	
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