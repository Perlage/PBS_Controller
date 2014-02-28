

  boolean inMenuLoop1 = false;
  boolean inMenuLoop2 = false;
  boolean menuOption11 = false; //Carbonation
  boolean menuOption12 = false; //Cleaning
  boolean menuOption21 = false; //AutoSiphon
  boolean menuOption22 = false; //Manual Diagnostic Mode

  while (button2State == LOW) //or whatever button combo
  {
    inMenuLoop1 = true;
    button2State = !digitalRead(button2Pin); 

    lcd.setCursor (0, 0); lcd.print (F("***MENU1*** Press:  "));
    lcd.setCursor (0, 1); lcd.print (F("B1: Menu 11         "));
    lcd.setCursor (0, 2); lcd.print (F("B2: Menu 12         "));
    lcd.setCursor (0, 3); lcd.print (F("B3: More...         "));
  }
  
  while (inMenuLoop1 == true)
  {
    while (button1State == LOW)
    {
      menuOption11 = true;
      inMenuLoop1 = false;   
      button1State = !digitalRead(button1Pin); 
      
      lcd.setCursor (0, 0); lcd.print (F("***Menu Item 11*** "));
      lcd.setCursor (0, 1); lcd.print (F("Item 11 text       "));
      lcd.setCursor (0, 2); lcd.print (F("Item 11 text       "));
      lcd.setCursor (0, 3); lcd.print (F("Item 11 text       "));   
    }  

    while (button2State == LOW)
    {
      menuOption12 = true;
      inMenuLoop1 = false;   
      button2State = !digitalRead(button2Pin); 

      lcd.setCursor (0, 0); lcd.print (F("***Menu Item 12*** "));
      lcd.setCursor (0, 1); lcd.print (F("Item 12 text       "));
      lcd.setCursor (0, 2); lcd.print (F("Item 12 text       "));
      lcd.setCursor (0, 3); lcd.print (F("Item 12 text       "));      
    }  

    while (button3State == LOW)
    {
      inMenuLoop2 = true;
      inMenuLoop1 = false;
      button3State = !digitalRead(button3Pin); 
     
      lcd.setCursor (0, 0); lcd.print (F("***MENU2*** Press:  "));
      lcd.setCursor (0, 1); lcd.print (F("B1: Menu 21         "));
      lcd.setCursor (0, 2); lcd.print (F("B2: Menu 22         "));
      lcd.setCursor (0, 3); lcd.print (F("B3: Exit...         "));
    }      
  }
  
  while (inMenuLoop2 == true)
  {
    while (button1State == LOW)
    {
      menuOption21 = true;
      inMenuLoop2 = false;   
      button1State = !digitalRead(button1Pin); 
      
      lcd.setCursor (0, 0); lcd.print (F("***Menu Item 21*** "));
      lcd.setCursor (0, 1); lcd.print (F("Item 21 text       "));
      lcd.setCursor (0, 2); lcd.print (F("Item 21 text       "));
      lcd.setCursor (0, 3); lcd.print (F("Item 21 text       "));   
    }  

    while (button2State == LOW)
    {
      menuOption22 = true;
      inMenuLoop2 = false;   
      button2State = !digitalRead(button2Pin); 

      lcd.setCursor (0, 0); lcd.print (F("***Menu Item 22*** "));
      lcd.setCursor (0, 1); lcd.print (F("Item 22 text       "));
      lcd.setCursor (0, 2); lcd.print (F("Item 22 text       "));
      lcd.setCursor (0, 3); lcd.print (F("Item 22 text       "));      
    }  

    while (button3State == LOW)
    {
      inMenuLoop2 = false;
      button3State = !digitalRead(button3Pin); 
     
      lcd.clear();
      lcd.setCursor (0, 3); lcd.print (F("Exiting menu...     "));
      delay(1000);      
    }      
  }
  
  //Now we execute the actual functionality of the menu options
  if (menuOption11 == true)
  {
    //Do menuOption11 stufg
    lcd.setCursor (0, 0); lcd.print (F("menuOption11 stuff"));
    menuOption11 = false;
  }  

  if (menuOption12 == true)
  {
    //Do menuOption11 stufg
    lcd.setCursor (0, 0); lcd.print (F("menuOption12 stuff"));
    menuOption12 = false;
  }  

  if (menuOption21 == true)
  {
    //Do menuOption11 stufg
    lcd.setCursor (0, 0); lcd.print (F("menuOption21 stuff"));
    menuOption21 = false;
  }  

  if (menuOption22 == true)
  {
    //Do menuOption11 stufg
    lcd.setCursor (0, 0); lcd.print (F("menuOption22 stuff"));
    menuOption22 = false;
  }  
