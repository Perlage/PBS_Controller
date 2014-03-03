// =======================================================================================
// MANUAL MODE
// =======================================================================================

void manualModeLoop()
{
  // Manual Mode Entrance Routines
  // ==================================================

  boolean inManualModeLoop1 = false;
  
  if (inManualModeLoop == true)
  inManualModeLoop1 = true;
  {
    sensorFillState = HIGH; // Set it high and don't read it anymore
  
    lcd.setCursor (0, 0); lcd.print (F(" ***MANUAL MODE***  "));
    lcd.setCursor (0, 1); lcd.print (F("Use for diagnostics "));
    lcd.setCursor (0, 2); lcd.print (F("and troubleshooting."));
    lcd.setCursor (0, 2); lcd.print (F("Press B2 to exit.   "));
    buzzer (1000);
    
    // FUNCTION Dump pressure
    pressureDump();
    
    // FUNCTION doorOpen
    doorOpen();
  
    // FUNCTION Drop platform if up
    platformDrop();
  
  }
  
  // END Manual Mode Entrance Routines
  // ========================================================
  
  //======================================================================================
  // MANUAL MODE LOOP
  //======================================================================================
      
  while (inManualModeLoop1 == true)
  {    
    // FUNCTION: PlatformUpLoop
    platformUpLoop();     
   
    // FUNCTION: Read all states of buttons, sensors
    readButtons();
    readStates();
    
    // FUNCTION: Read and output pressure
    pressureOutput();
    printLcd(3, outputPSI_rbd);    
  
    if (platformStateUp == false && switchDoorState == HIGH)
    {
      lcd.setCursor (0, 0); lcd.print (F(" ***MANUAL MODE***  "));
      lcd.setCursor (0, 1); lcd.print (F("B1 raises platform; "));
      lcd.setCursor (0, 2); lcd.print (F("B2 exits manual mode"));
    }
    if (platformStateUp == false && switchDoorState == LOW)
    {
      lcd.setCursor (0, 0); lcd.print (F(" ***MANUAL MODE***  "));
      lcd.setCursor (0, 1); lcd.print (F("B3 opens door;      "));
      lcd.setCursor (0, 2); lcd.print (F("B2 exits manual mode"));
    }
    if (platformStateUp == true && switchDoorState == HIGH)
    {
      lcd.setCursor (0, 0); lcd.print (F(" ***MANUAL MODE***  "));
      lcd.setCursor (0, 1); lcd.print (F("Close door to start;"));
      lcd.setCursor (0, 2); lcd.print (F("B3 to lower platform"));
    }
    if (platformStateUp == true && switchDoorState == LOW)
    {
      lcd.setCursor (0, 0); lcd.print (F("B1: Gas IN          "));
      lcd.setCursor (0, 1); lcd.print (F("B2: Liquid IN       "));
      lcd.setCursor (0, 2); lcd.print (F("B3: Gas OUT/Open dr."));
    }
      
    // B1: GAS IN ================================================================
    if (button1State == LOW && platformStateUp == true && switchDoorState == LOW){
      relayOn (relay2Pin, true);}  
    else{
      relayOn (relay2Pin, false);}
      
    // B2 LIQUID IN ==============================================================
    if (button2State == LOW && platformStateUp == true && switchDoorState == LOW){
      // TO DO: ADD WARNING
      relayOn (relay1Pin, true);}  
    else{
      relayOn (relay1Pin, false);}
      
    // B3 GAS OUT ================================================================
    if (button3State == LOW && platformStateUp == true && switchDoorState == LOW && (P1 - offsetP1 >= pressureDeltaDown)){
      relayOn (relay3Pin, true);}  
    else{
      relayOn (relay3Pin, false);}
      
    // B3: Open Door or drops platform if pressure low============================
    if (button3State == LOW && switchDoorState == LOW && (P1 - offsetP1 < pressureDeltaDown))
    {
      doorOpen();
    }  
    if (button3State == LOW && switchDoorState == HIGH && (P1 - offsetP1 < pressureDeltaDown))
    {
      while (button3State == LOW)
      {
        button3State = !digitalRead(button3Pin); 
        delay(10);
        relayOn(relay4Pin, false);   
        relayOn(relay5Pin, true); 
      }  
      relayOn(relay5Pin, false); 
      platformStateUp = false;
    }  

    // B2 EXITS if pressure low and platform down
    while (button2State == LOW && platformStateUp == false && (P1 - offsetP1 < pressureDeltaDown))
    {
      button2State = !digitalRead(button2Pin); 
      inManualModeLoop1 = false;
      buzzOnce (1000, light2Pin);
    }  
  }
    
  //END MANUAL MODE LOOP 
  //=================================================   
   
  //MANUAL MODE LOOP EXIT ROUTINES 
  //==================================================
  if (inManualModeLoop)
  {
    inManualModeLoop = false;
    
    lcd.setCursor (0, 0); lcd.print (F("EXITING MANUAL MODE "));
    lcd.setCursor (0, 1); lcd.print (F("                    "));
    lcd.setCursor (0, 2); lcd.print (F("Continuing...       "));
    
    while (P1 - offsetP1 > pressureDeltaDown)
    {
      relayOn (relay3Pin, true);
      
      pressureOutput();
      printLcd(3, outputPSI_rbd); 
     
      lcd.setCursor (0, 2); lcd.print (F("Depressurizing...   "));
    }
    
    doorOpen();
    platformDrop();
    relayOn (relay3Pin, false);
  }  
  
  // END MANUAL MODE
  //===========================================================================================

}
