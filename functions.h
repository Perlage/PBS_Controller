// ======================================================================================
// DECLARE FUNCTIONS
// ======================================================================================

//This throttles the number of times the display updates per second
//ThrottleInt is in ms. The duration between positive PFlags is given by (throttleInt/1000)/2
// ======================================================================================

void displayThrottle(float throttleInt)
{
	Nf1  = int((millis()/throttleInt - int(millis()/throttleInt)) * 2); // For display timing
	if (Nf1 != Nf2)
	{
		PFlag = true;
		Nf2 = Nf1;
	}
}

// FUNCTION printLcd2:
//This throttles the number of times the display updates per second
//ThrottleInt is in ms. The duration between positive PFlags is given by (throttleInt/1000)/2
// =======================================================================================

void printLcd2 (int line, String newString, float throttleInt)
{

	Nf1  = int((millis()/throttleInt - int(millis()/throttleInt)) * 2); // For display timing
	if (Nf1 != Nf2)
	{
		PFlag = true;
		Nf2 = Nf1;
	}
	if (PFlag == true)
	{
    lcd.setCursor(0,line);
    lcd.print("                    ");
    lcd.setCursor(0,line);
    lcd.print(newString);
		PFlag = false;
  }
}

//String currentLcdString[3]; // Jeremy's original
String currentLcdString;

void printLcd (int line, String newString)
{
  //if(!currentLcdString[line].equals(newString)) // see Arduino reference, Data types / String object / equals() function
  if(!currentLcdString.equals(newString)) // see Arduino reference, Data types / String object / equals() function
	{
    //currentLcdString[line] = newString;
		currentLcdString = newString;
    lcd.setCursor(0,line);
    lcd.print("                    ");
    lcd.setCursor(0,line);
    lcd.print(newString);
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
  }
}  

// FUNCTION: Read Buttons
// ======================================================================================
void readButtons()
{
  button1State = !digitalRead(button1Pin); 
  button2State = !digitalRead(button2Pin); 
  button3State = !digitalRead(button3Pin); 
  //delay (10); //debounce
}

// FUNCTION: Read Sensors
// =======================================================================================
void readStates()
{
  switchDoorState = digitalRead(switchDoorPin);
  switchModeState = digitalRead(switchModePin);
  sensorFillState = digitalRead(sensorFillPin);
  //delay (25); //debounce
}

// FUNCTION: Pressure reading/conversion/output
// ====================================================================================
void pressureOutput()
{
  P1 = analogRead(sensorP1Pin); 
  P2 = analogRead(sensorP2Pin);
        
  PSI1     = pressureConv1(P1); 
  PSI2     = pressureConv2(P2); 
  PSIdiff  = PSI2 - PSI1;
  
  (convPSI1)      = floatToString(buffer, PSI1, 1);
  (convPSI2)      = floatToString(buffer, PSI2, 1);
  (convPSIdiff)   = floatToString(buffer, PSIdiff, 1);

  (outputPSI_rb)  = "Keg:" + convPSI2 + " Bottle:" + convPSI1; //was Reg
  (outputPSI_b)   = "Bottle: " + convPSI1 + " psi"; 
  (outputPSI_r)   = "Keg: " + convPSI2 + " psi"; //Was "Regulator"
}  

// FUNCTION: pressureDump()
// =====================================================================================
void pressureDump()
{
  while (P1 - offsetP1 > pressureDeltaDown)
  {
    relayOn(relay3Pin, true); 
    P1 = analogRead (sensorP1Pin);
    pressureOutput();
    printLcd (3, outputPSI_b);
  }  
  relayOn(relay3Pin, false);  
}