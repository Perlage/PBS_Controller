// =======================================================================================
// MANUAL MODE
// =======================================================================================

void manualModeLoop()
{
  if (inManualModeLoop == true)
  {
    inManualModeLoop1 = true;
    sensorFillState = HIGH; // Set it high and don't read it anymore
    //buzzer (1000);
    
    // FUNCTION Dump pressure
    pressureDump();
    
    // FUNCTION doorOpen
    doorOpen();
  
    // FUNCTION Drop platform if up
    platformStateUp = true;
    platformDrop();
  }
      
  while (inManualModeLoop1 == true)
  {    
    // FUNCTION: Read all states of buttons, sensors
    readButtons();
    readStates();
    
    // FUNCTION: Read and output pressure
    pressureOutput();
    printLcd2(3, outputPSI_rb, throttleVal);    
  
    if (platformStateUp == false && switchDoorState == HIGH && !inDiagnosticMode)
    {
      lcd.setCursor (0, 1); lcd.print (F("B1 raises platform; "));
      lcd.setCursor (0, 2); lcd.print (F("B2 exits Manual Mode"));
    }
    if (platformStateUp == false && switchDoorState == LOW && !inDiagnosticMode)
    {
      lcd.setCursor (0, 1); lcd.print (F("B3 opens door;      "));
      lcd.setCursor (0, 2); lcd.print (F("B2 exits Manual Mode"));
    }
    if (platformStateUp == true && switchDoorState == HIGH && !inDiagnosticMode)
    {
      lcd.setCursor (0, 1); lcd.print (F("Close door to start;"));
      lcd.setCursor (0, 2); lcd.print (F("B3 lowers platform  "));
    }

    if (inDiagnosticMode)
    {
      lcd.setCursor (0, 1); lcd.print (F("B2: LIQUID-CAUTION! "));
      lcd.setCursor (0, 2); lcd.print (F("Close door to exit. "));
    }

    if (platformStateUp == true && switchDoorState == LOW && !inDiagnosticMode) //Add a pressure check to this
    {
      lcd.setCursor (0, 0); lcd.print (F("B1: Gas IN          "));
      lcd.setCursor (0, 1); lcd.print (F("B2: Liquid IN       "));
      lcd.setCursor (0, 2); lcd.print (F("B3: Gas OUT/Open Dr."));
    }
    else
    {
      lcd.setCursor (0, 0); lcd.print (F(" ***MANUAL MODE***  "));
    }
      
    // B1: FUNCTION PlatformUpLoop
    platformUpLoop();     
   
    // B1: GAS IN ================================================================
    if (button1State == LOW && platformStateUp == true && switchDoorState == LOW){
			digitalWrite(light1Pin, HIGH);
      relayOn (relay2Pin, true);}  
    else{
			digitalWrite(light1Pin, LOW);
      relayOn (relay2Pin, false);}
      
    // B2 LIQUID IN ==============================================================
    if ((button2State == LOW && platformStateUp == true && switchDoorState == LOW && (P1 - offsetP1 >= pressureDeltaDown)) || (button2State == LOW) && inDiagnosticMode == true){ 
			digitalWrite(light2Pin, HIGH);
      relayOn (relay1Pin, true);}  
    else{
			digitalWrite(light2Pin, LOW);
      relayOn (relay1Pin, false);}
      
    // B3 GAS OUT ================================================================
    if (button3State == LOW && platformStateUp == true && switchDoorState == LOW && (P1 - offsetP1 >= pressureDeltaDown)){
			digitalWrite(light3Pin, HIGH);
      relayOn (relay3Pin, true);}  
    else{
			digitalWrite(light3Pin, LOW);
      relayOn (relay3Pin, false);}
      
    // B3: OPEN DOOR =============================================================
    if (button3State == LOW && switchDoorState == LOW && (P1 - offsetP1 < pressureDeltaDown))
    {
			digitalWrite(light3Pin, HIGH);
      relayOn (relay3Pin, true);
      doorOpen();
      button3State = HIGH; //This fixed a bug where opening door passed button state into next loop and was setting platform state to down when still up
			digitalWrite(light3Pin, LOW);
    }  

    // B3: DROP PLATFORM =========================================================
    if (button3State == LOW && switchDoorState == HIGH && (P1 - offsetP1 < pressureDeltaDown))
    {
      while (button3State == LOW)
      {
        button3State = !digitalRead(button3Pin); 
				digitalWrite(light3Pin, HIGH);        
				relayOn (relay3Pin, true);
				relayOn(relay4Pin, false);   
        relayOn(relay5Pin, true); 
      }  
			digitalWrite(light3Pin, LOW);      
			relayOn (relay3Pin, true);
			relayOn(relay5Pin, false); 
      platformStateUp = false;
    }  

    // B2 EXITS if pressure low and platform down
    while (button2State == LOW && platformStateUp == false && (P1 - offsetP1 < pressureDeltaDown) && !inDiagnosticMode)
    {
      button2State = !digitalRead(button2Pin); 
      inManualModeLoop1 = false;
      buzzOnce (1000, light2Pin);
    }
    
    //Exit DiagnosticMode back to Manual Mode
    if (inDiagnosticMode && switchDoorState == LOW)
    {
      inDiagnosticMode = false;
    }  
  }
   
  //MANUAL MODE LOOP EXIT ROUTINES 
  //==================================================
  if (inManualModeLoop)
  {
    inManualModeLoop = false;
    lcd.clear();
    lcd.setCursor (0, 0); lcd.print (F("EXITING MANUAL MODE "));
  }  
}
