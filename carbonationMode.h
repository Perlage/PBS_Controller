
// =============================================================================================
// CARBONATION MODE
// =============================================================================================

boolean inTimingLoop = false;
float timerTime = 0;				//Timer in sec   
unsigned long timerStart = 0;		//Init value of millis() going into loop in ms. MUST BE LONG INT!
unsigned long timerTimeMs = 0;		//Timer value in ms
byte timerTimeMin = 0;				//Timer value, minutes part
byte timerTimeSec = 0;				//Timer value, seconds part
int P2Start;						//Initial regulator reading going into loop
float timerShakeTime;				//Time of shaking in sec, less the number of 15 second rest periods               
boolean inShakeState = false;		//Whether or not should be shaking in given interval
String shakeStateMessage;			//REST or SHAKE
float pressureDipTarget;			//Target pressure dip goes down over time as liquid saturates. dipTarget decreases by factor of 1, 1/2, 1/3, 1/4...
float percentEffort;				//Shows percent of dip target
int pressureBegin;					//Pressure entering Carbonation Mode
int pressureExit;					//Pressure exiting Carbonation Mode. Will be used for pressure check
float OnePsi = 12.7084;				//1 psi in raw pressure units
float pressureDipTargetInit;		//Initial pressure slump user is trying to hit at T=0 in raw units (12.71 units per psi)

//Settable parameters
int pressureDropMax = 75;			//This is the max drop between beginning and and of Carbonation process allowed before flashing warning.
int pressureDipTargetInitPSI = 6;	//This sets the initial dip target. in V1.0, multiplier was 2.0. 
int timeShift = 60;					//v1.1 This is used for modified 1/T technique

pressureDipTargetInit = pressureDipTargetInitPSI * OnePsi;

//This starts Carbonation routine
if (!digitalRead(button1Pin) == LOW)
{
	inTimingLoop = true;
	lcd.setCursor(0, 1); lcd.print(F("Press B3 to exit.   "));
	buttonPush(button1Pin, light1Pin, 1000);
	//delay(500); //Give time to see message
}

if (inTimingLoop)
{
	timerStart = millis();
	P2Start = analogRead(sensorP2Pin);
	pressureBegin = analogRead(sensorP2Pin); //v1.1 This is start pressure for pressure check at end of routine
}

while (inTimingLoop == true)
{
	timerTimeMs = millis() - timerStart;
	timerTime = float(timerTimeMs / 1000);       //Get total seconds since start (Float)
	timerTimeSec = int(timerTime) % 60;          //Get 0-60 seconds
	timerTimeMin = int(timerTime / 60);          //Get minutes

	int timerCountdown = 15 - (int(timerTime) % 15); //Countdown clock

	String(convTimeMin) = floatToString(buffer, timerTimeMin, 0);
	String(convTimeSec) = floatToString(buffer, timerTimeSec, 0);
	String(strTimerCountDown) = floatToString(buffer, timerCountdown, 0);

	//Print out countdown timer and cumulative timer  
  //Need this IF to account for the fact that seconds 0-9 don't have leading zero
	if (timerTimeSec <= 9)
	{
		if (timerCountdown <= 9)
		{
			printLcdArray(1, "[:0" + strTimerCountDown + "]     Time: " + convTimeMin + ":0" + convTimeSec);
		}
		else
		{
			printLcdArray(1, "[:" + strTimerCountDown + "]     Time: " + convTimeMin + ":0" + convTimeSec);
		}
	}
	else
	{
		if (timerCountdown <= 9)
		{
			printLcdArray(1, "[:0" + strTimerCountDown + "]     Time: " + convTimeMin + ":" + convTimeSec);
		}
		else
		{
			printLcdArray(1, "[:" + strTimerCountDown + "]     Time: " + convTimeMin + ":" + convTimeSec);
		}
	}

	//Find shakeState and appropriate messages. Rest first and every other 15 second segment
	//Write initial message...
	if (timerTime < 15)
	{
		shakeStateMessage = "Get ready...";
		inShakeState = false;
	}
	else
	{
		if ((timerTimeSec >= 0 && timerTimeSec < 15) || (timerTimeSec >= 30 && timerTimeSec < 45))
		{
			inShakeState = false;
			shakeStateMessage = "Rest...             ";
		}
		else
		{
			inShakeState = true;
			if (percentEffort < 100)
			{
				shakeStateMessage = "Shake HARD...      ";
			}
			else
			{
				shakeStateMessage = "***GOOD!***        ";
			}
		}
	}

	//This prints current instruction on line 3
	printLcdArray(2, shakeStateMessage);

	//v1.1 Modified 1/T method with timeShift factor to help determine the slope of curve
	//The bigger timeShift, the slower the drop
	pressureDipTarget = pressureDipTargetInit * timeShift / ((timerTime - 15) + timeShift);

	// get a fresh reading every 30 sec at start of shake cycle, at 15, 45, 75
	if ((int(timerTime) - 15) % 30 == 0)
	{
		P2Start = analogRead(sensorP2Pin);
	}

	//Get current pressure reading
	P2 = analogRead(sensorP2Pin);

	//v1.1: Extra constriction of check valve means pressure diff will be larger than without, because gas can't flow to keg as fast. Any semipermanent pressure offset due to valve shouldn't matter.
	int pressureDiff = (P2Start - P2);

	percentEffort = constrain((pressureDiff / pressureDipTarget) * 100, 0, 999); //Using raw integer readings--don't need to worry about offsets because of subtraction. Constrain keeps above 0
	String stringPercentEffort = floatToString(buffer, percentEffort, 0);

	//String strPressureDipTarget = floatToString(buffer, (pressureDipTarget / OnePsi), 1);  //DEBUG CODE
	//String strPressureDip = floatToString(buffer, (pressureDiff / OnePsi), 1);						 //DEBUG CODE

	//Print out current absorption data //Don't show any output in rest phase. 
	if (inShakeState)
	{
		String outputEffort = "EFFORT: " + stringPercentEffort + "% => 100%"; // + strPressureDip + "/" + strPressureDipTarget; // Replace end of this line with "(>100%)"
		printLcdArray(3, outputEffort);
	}
	else
	{
		messageLcdBlank(3);
	}

	//Buzz every 15 sec, but not on first pas or after 240 sec
	if (timerTimeSec % 15 == 0 && timerTime > 1 && timerTime <= 240)
	{
		buzzer(1000);
	}

	//Give user positive feedback by beeping when there is a significant pressure drop while shaking 
	if (P2 < P2Start - pressureDipTarget && inShakeState == true)
	{
		buzzer(50);
	}

	//CARBONATION ROUTINE EXIT LOOP
	  //================================================================================================

	if (!digitalRead(button3Pin) == LOW || (timerTime > 240))
	{
		printLcd(2, "DONE! Exiting...    ");
		buzzOnce(1500, light3Pin);

		inTimingLoop = false;
		menuOption11 = false;
		pressureExit = analogRead(sensorP2Pin);

		//Check to see if pressure has dropped during the carbonation process
		if ((pressureBegin - pressureExit) > pressureDropMax)
		{
			messageGasLow();
			while (!digitalRead(button3Pin) == HIGH)
			{
				lcd.setCursor(0, 2); lcd.print(F("Press B3 to continue"));
			}
			buttonPush(button3Pin, light3Pin, 750);
		}
	}
	buzzedOnce = false;
}