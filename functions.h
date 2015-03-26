// ======================================================================================
// DECLARE FUNCTIONS
// ======================================================================================

// FUNCTION printLcd()
// Only blanks line if length changes. Prevents dangling characters when line shortens
// ======================================================================================

//This only blanks line if length changes
//String oldLcdString;
String oldLcdString;
void printLcd (int scrLine, String newLcdString)
{
	if(oldLcdString.length() != newLcdString.length())
	{
		oldLcdString = newLcdString;
		lcd.setCursor(0, scrLine); lcd.print("                    ");
		lcd.setCursor(0, scrLine); lcd.print(newLcdString);
	}
	else
	{
		lcd.setCursor(0, scrLine); lcd.print(newLcdString);
	}
}

//This only blanks line if length changes
//String oldLcdString;
String oldLcdString2[3];
void printLcdArray (int scrLine, String newLcdString)
{
	if(oldLcdString2[scrLine].length() != newLcdString.length())
	{
		oldLcdString2[scrLine] = newLcdString;
		lcd.setCursor(0, scrLine); lcd.print("                    ");
		lcd.setCursor(0, scrLine); lcd.print(newLcdString);
	}
	else
	{
		lcd.setCursor(0, scrLine); lcd.print(newLcdString);
	}
}

// FUNCTION relayOn
// Allows relay states to be easily be changed from HI=on to LOW=on
// =======================================================================================

void relayOn(int pinNum, boolean on){
  if(on){
    digitalWrite(pinNum, LOW);} //turn relay on
  else{
    digitalWrite(pinNum, HIGH); //turn relay off
  }
}

// FUNCTION pressureConv: 
// Routine to convert pressure from parts in 1024 to psi
// pressurePSI = (((P1 - offsetP1) * 0.0048828)/0.009) * 0.145037738
// 1 PSI = 12.7084 units; 1 unit = 0.078688 PSI; 40 psi = 543 units
// =======================================================================================

// Function for P1
float pressureConv1(int P1) 
{
  float pressurePSI1;
  pressurePSI1 = (P1 - offsetP1) * 0.078688; 
  return pressurePSI1;
}

// Function for P2 (potentially different offset)
float pressureConv2(int P2) 
{
  float pressurePSI2;
  pressurePSI2 = (P2 - offsetP2) * 0.078688; 
  return pressurePSI2;
}

// FUNCTION: Door Open Routine
// =======================================================================================
void doorOpen()
{
  switchDoorState = digitalRead(switchDoorPin); 
  while (switchDoorState == LOW && (P1 - offsetP1) <= pressureDeltaDown)
  {
    relayOn (relay6Pin, true);  // Open door
    lcd.setCursor (0, 2); lcd.print (F("Opening door...     "));
    switchDoorState = digitalRead(switchDoorPin); 
    P1 = analogRead(sensorP1Pin);
  }
  delay(250); // Added delay to prevent door from catching on latch
  relayOn (relay6Pin, false);
  lcd.setCursor (0, 2); lcd.print (F("                    "));
}

// FUNCTION: buzzer()
// =======================================================================================
void buzzer (int buzzDuration)
{
  digitalWrite (buzzerPin, HIGH); 
  delay (buzzDuration);
  digitalWrite (buzzerPin, LOW);
} 

// FUNCTION: buzzOnce()
// =======================================================================================

// Remember to set buzzedOnce back to false immediately after this function runs
void buzzOnce (int buzzOnceDuration, byte lightPin)
{
  if (buzzedOnce == false)
  {
    digitalWrite(lightPin, HIGH);
    buzzer(buzzOnceDuration);
    digitalWrite(lightPin, LOW);
    buzzedOnce = true;
  }    
} 

// FUNCTION: platformDrop() //TO DO: pass the duration as a parameter
// ======================================================================================
void platformDrop()
{
  // Drop platform if up
  if (platformStateUp == true)
  {
    relayOn(relay4Pin, false);   
    relayOn(relay5Pin, true); 
    delay (autoPlatformDropDuration);  
    relayOn(relay5Pin, false); 
    platformStateUp = false;
		EEPROM.write(6, platformStateUp);
  }
}  

// FUNCTION: Read Buttons
// ======================================================================================
void readButtons()
{
  button1State = !digitalRead(button1Pin); 
  button2State = !digitalRead(button2Pin); 
  button3State = !digitalRead(button3Pin); 
}

// FUNCTION: Read Sensors
// =======================================================================================
void readStates()
{
  switchDoorState = digitalRead(switchDoorPin);
  switchModeState = digitalRead(switchModePin);
  sensorFillState = digitalRead(sensorFillPin);
}

// FUNCTION: Pressure reading/conversion/output
// ====================================================================================
void pressureOutput()
{
  P1 = analogRead(sensorP1Pin); 
  P2 = analogRead(sensorP2Pin);
        
  PSI1     = abs(pressureConv1(P1)); //Added ABS() function Mar 9 2015 to keep bottle pressure from flickering negative
  PSI2     = abs(pressureConv2(P2)); //Added ABS() function Mar 9 2015 to keep
  PSIdiff  = (PSI2 - PSI1);
  
  (convPSI1)      = floatToString(buffer, PSI1, 1);
  (convPSI2)      = floatToString(buffer, PSI2, 1);
  (convPSIdiff)   = floatToString(buffer, PSIdiff, 1);

  (outputPSI_rb)  = "Keg:" + convPSI2 + " Bottle:" + convPSI1;	//19-20
  (outputPSI_r)   = "Pressure:   " + convPSI2 + " psi";					//20-19
	(outputPSI_b)   = "Bottle:     " + convPSI1 + " psi";					//19-20
	(outputPSI_d)   = "Difference: " + convPSIdiff + " psi";			//20-19
} 

//FUNCTION: padString
//====================================================================================

int padding;
void padString(String paddedString)
{
	padding = 20 - paddedString.length();
	for (int i = 0; i < padding; i++)
	{paddedString += " ";}
	Serial.print (paddedString)	;
}

// FUNCTION: pressureDump()
// =====================================================================================
void pressureDump() //Must close S3 manually
{
  while (P1 - offsetP1 > pressureDeltaDown)
  {
    relayOn(relay3Pin, true); 
    P1 = analogRead (sensorP1Pin);
    pressureOutput();
    printLcd (3, outputPSI_b);
  } 
}

// FUNCTION: messageRotator() 
// rotateRate is the time taken to cycle through both messages (in ms); 
// weight is the fraction of time spent on second message (between 0 and 1)
// timeOffset moves the sample window. This is helpful with startup messages with known first-hit time
//=======================================================================================
boolean messageID;
void messageRotator(int rotateRate, float weight, int timeOffset)
{
	if (((millis() + timeOffset) % rotateRate) > (weight * rotateRate))
	{
		messageID = true;
	}
	else
	{
		messageID = false;
	}
}

//FUNCTION: BUTTON PUSH
//This automatically handles button pushes. Just use when you want a button to make a sound and light
// Copy this: buttonPush (button3Pin, light3Pin, 250);
//========================================================================================
void buttonPush (byte buttonPin, byte lightPin, int buzzerDuration)
{
	if (!digitalRead (buttonPin) == LOW)
	{
		digitalWrite (lightPin, HIGH);
		buzzer(buzzerDuration);
		digitalWrite (lightPin, LOW);
	}
}

//FUNCTION: AUTOSIPHON SET ROUTINE
//========================================================================================
String convTime;
String outputTime;
boolean inMenuOption11Loop = false; //v1.1 Have to put this here because Functions include is invoked before MenuShell include???

void autoSiphonSet()
{
  readButtons();
    
  if (button1State == LOW)
  {
	  autoSiphonDuration10s = autoSiphonDuration10s + 1; //Add .1s
	  buzzOnce(100, light1Pin);
  }
  if (button2State == LOW)
  {
	  autoSiphonDuration10s = autoSiphonDuration10s - 1; //Subtract .1s
	  buzzOnce(100, light2Pin);
  }

  autoSiphonDuration10s = constrain(autoSiphonDuration10s, 5, 99); //Constrains autoSiphonDuration10s to between 5 and 99 tenths of sec
  autoSiphonDurationSec = float(autoSiphonDuration10s) / 10;
    
  convTime = floatToString(buffer, autoSiphonDurationSec, 1);
  outputTime = "Current value: " + convTime + "s ";
  //printLcd (3, outputTime);
	lcd.setCursor(0, 3); lcd.print(outputTime);
    
  buzzedOnce = false;
  if (button3State == LOW)
  {
	  outputTime = "New value: " + convTime + "s     ";
	  //printLcd (3, outputTime);
		lcd.setCursor(0, 3); lcd.print(outputTime);
	    
	  buzzOnce(500, light3Pin);
	  delay(1000);
	    
	  EEPROM.write (3, autoSiphonDuration10s);             // Write to EEPROM
	  autoSiphonDuration = autoSiphonDuration10s * 100;    //Convert to ms from 10ths of sec
	    
	  // This catches loop until release
	  while (!digitalRead(button3Pin) == LOW)
	  {
		  // This just catches B3 push so it doesn't flow through to Idle Loop and open door
	  }
	  inMenuOption11Loop = false;	
		button3State = HIGH;
  }
  buzzedOnce = false;
}


/*
//Template for WhileWend
//=================================================================================================

boolean inLoopLBL = false;
while (conditions are true)
{
	boolean inLoopLBL = true;
	//Put the do-while-true code here
}
if (inloopLBL)
{
	//exit routines (wend code)
	boolean inLoopLBL = false;
}
*/

//Button Read
//============================
//Under construction

byte buttonState[2];
void readButtonCatch(byte Id, byte buttonPin, byte lightPin) //pass in button1Pin 
{
	while (!digitalRead(buttonPin == LOW))
	{
			digitalWrite(lightPin, HIGH);
			buzzer(10);
	}
	digitalWrite (lightPin, LOW);
	buttonState[Id] = LOW;
}


void getTemperature(DeviceAddress deviceAddress)
{
	float tempC = sensors.getTempC(deviceAddress);
	if (tempC == -127.00) {
		lcd.setCursor (15, 3); lcd.print ("ERR");
		} else {
		lcd.setCursor (15, 3); lcd.print (tempC);
		//lcd.setCursor (15, 3); lcd.print (DallasTemperature::toFahrenheit(tempC));
	}
}
