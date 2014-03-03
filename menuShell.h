// ====================================================================================
// MENU SHELL
// ====================================================================================

void menuExit()
{
  lcd.setCursor (0, 3); lcd.print (F("Exiting menu...     "));
  delay(1000);      
  lcd.clear();
}

void menuShell()
{
  boolean inMenuLoop1 = false;
  boolean inMenuLoop2 = false;
  boolean menuOption11 = false; //AutoSiphon
  boolean menuOption12 = false; //Cleaning
  boolean menuOption21 = false; //Carbonation
  boolean menuOption22 = false; //Manual Diagnostic Mode

  while (button2State == LOW && platformStateUp == false) //or whatever button combo
  {
    buzzOnce(100, light2Pin);

    inMenuLoop1 = true;
    button2State = !digitalRead(button2Pin); 

    lcd.setCursor (0, 0); lcd.print (F("***MENU 1*** Press: "));
    lcd.setCursor (0, 1); lcd.print (F("B1: Set AutoSiphon  "));
    lcd.setCursor (0, 2); lcd.print (F("B2: Cleaning Mode   "));
    lcd.setCursor (0, 3); lcd.print (F("B3: More...         "));
  }
  buzzedOnce = false;

  while (inMenuLoop1 == true)
  {
    readButtons();
    while (button1State == LOW)
    {
      menuOption11 = true;
      inMenuLoop1 = false;   
      button1State = !digitalRead(button1Pin); 
      
      lcd.setCursor (0, 0); lcd.print (F("B1: Incr. by .1 sec "));
      lcd.setCursor (0, 1); lcd.print (F("B2: Set value & exit"));
      lcd.setCursor (0, 2); lcd.print (F("B3: Decr. by .1 sec "));
      // Current value is printed to screen on line 3
    }  

    while (button2State == LOW)
    {
      menuOption12 = true;
      inMenuLoop1 = false;   
      button2State = !digitalRead(button2Pin); 

      lcd.setCursor (0, 0); lcd.print (F("Cleaning Mode:     "));
      lcd.setCursor (0, 1); lcd.print (F("B1: Enter          "));
      lcd.setCursor (0, 2); lcd.print (F("B2: Exit           "));
      lcd.setCursor (0, 3); lcd.print (F("                   ")); 
    }  

    while (button3State == LOW)
    {
      inMenuLoop2 = true;
      inMenuLoop1 = false;
      button3State = !digitalRead(button3Pin); 
     
      lcd.setCursor (0, 0); lcd.print (F("***MENU 2*** Press: "));
      lcd.setCursor (0, 1); lcd.print (F("B1: Carbonation Mode"));
      lcd.setCursor (0, 2); lcd.print (F("B2: Manual Mode     "));
      lcd.setCursor (0, 3); lcd.print (F("B3: Exit...         "));
    }      
  }
  
  while (inMenuLoop2 == true)
  {
    readButtons();
    while (button1State == LOW)
    {
      menuOption21 = true;
      inMenuLoop2 = false;   
      button1State = !digitalRead(button1Pin); 
      
      lcd.setCursor (0, 0); lcd.print (F("CARBONATION MODE:   "));
      lcd.setCursor (0, 1); lcd.print (F("Under construction  "));
      lcd.setCursor (0, 2); lcd.print (F("                    "));
      lcd.setCursor (0, 3); lcd.print (F("                    "));   
    }  

    while (button2State == LOW)
    {
      menuOption22 = true;
      inMenuLoop2 = false;   
      button2State = !digitalRead(button2Pin); 

      lcd.setCursor (0, 0); lcd.print (F("MANUAL MODE:        "));
      lcd.setCursor (0, 1); lcd.print (F("B1: Enter           "));
      lcd.setCursor (0, 2); lcd.print (F("                    "));
      lcd.setCursor (0, 3); lcd.print (F("                    ")); 
    }  

    while (button3State == LOW)
    {
      inMenuLoop2 = false;
      button3State = !digitalRead(button3Pin); 
      menuExit();   
    }      
  }

  // ===============================================================================
  // Execute the actual functionality of the menu options
  // ===============================================================================

  // Menu Option 11: Autosiphon set
  // ===============================================================================

  while (menuOption11 == true)
  {
    readButtons();
  
    if (button1State == LOW){
      autoSiphonDuration10s = autoSiphonDuration10s + 1; //Add .1s
    }  
    if (button3State == LOW){
      autoSiphonDuration10s = autoSiphonDuration10s - 1; //Subtract .1s
    }  
      
    autoSiphonDuration10s = constrain(autoSiphonDuration10s, 5, 99); //Constrains autoSiphonDuration10s to between 5 and 99 tenths of sec
    autoSiphonDurationSec = float(autoSiphonDuration10s) / 10;
    
    String (convTime) = floatToString(buffer, autoSiphonDurationSec, 1);
    printLcd (3, "Current value: " + convTime + "s");
    delay(200);
  
    while (button2State == LOW)
    {
      button2State = !digitalRead(button2Pin); 
      printLcd (3, "New value: " + convTime + " s");
      delay(1500);
      
      EEPROM.write (3, autoSiphonDuration10s);             //Write to EEPROM
      autoSiphonDuration = autoSiphonDuration10s * 100;    //Convert to ms from 10ths of sec
      
      menuOption11 = false;
      menuExit();
    } 
  }

  // Menu Option 12: Cleaning Mode set
  // ===============================================================================

  while (menuOption12 == true)
  {
    readButtons();
    while (button1State == LOW)
    {
      button1State = !digitalRead(button1Pin); 
      inCleaningMode = true;
      lcd.setCursor (0, 3); lcd.print (F("Cleaning Mode *ON*  "));
      delay (1500);
      menuOption12 = false;
    }  
    while (button2State == LOW)
    {
      button2State = !digitalRead(button2Pin); 
      inCleaningMode = false;
      lcd.setCursor (0, 3); lcd.print (F("Cleaning Mode *OFF* "));
      delay (1500);
      menuOption12 = false;
    }  
    if (menuOption12 == false){
      menuExit();
    }
  }  

  // Menu Option 21: Carbonation Mode Set
  // ===============================================================================

  if (menuOption21 == true)
  {
    //Do menuOption11 stuff
    lcd.setCursor (0, 0); lcd.print (F("Carbonation Mode "));
    delay (500);
    menuOption21 = false;
  }  

  // Menu Option 22: Manual Mode Set
  // ===============================================================================

  while (menuOption22 == true)
  {
    readButtons();
    while (button1State == LOW)
    {
      button1State = !digitalRead(button1Pin); 
      menuOption22 = false;
      inManualModeLoop = true;

      lcd.setCursor (0, 3); lcd.print (F("Manual Mode *ON*  "));
      buzzer (1000);
    }  

    if (menuOption22 == false)
    {
      menuExit();
      manualModeLoop();
    }
  }
}  
