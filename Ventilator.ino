
#include <SD.h>

/*
 * 
 * handle 50 day rollover
 * done - add pause button
 * save and restore timing from EEPROM
 * add TFT panel
 * add sensors
 * do new modes
 * open charge valve and close discharge valve when idle
 * done - do 24 volt shutoff
 * 
 * */
 
//#include <LiquidCrystal.h>

#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#include <SPI.h>

#define VALVE_1_PIN            2
#define VALVE_2_PIN            3
//#define VALVE_3_PIN            6
//#define VALVE_4_PIN            7

#define WS2812_1_PIN           4
//#define WS2812_2_PIN           5
#define XFORMER_PIN            5

#define STATE_1_PIN            8
#define STATE_2_PIN            9

#define PUSHBUTTON1_PIN        10
#define PUSHBUTTON2_PIN        11
#define PUSHBUTTON3_PIN        12
#define PUSHBUTTON4_PIN        13

//#define START_RUN_TIME          600000

#ifdef __AVR__
  #include <avr/power.h>
#endif

//extern void EraseString( int hdl );

Adafruit_NeoPixel strip = Adafruit_NeoPixel(800, 2, NEO_RGB + NEO_KHZ800);

#define FNCFAILED              -1
#define FNCOK                  0
#define FNCENDED               -5

char obuff[64];

int curventilator;

int opmode[2];
int trigger[2];
unsigned long loopcount[2];
unsigned long lastmillis[2];
unsigned long trigger1millis[2];
unsigned long trigger2millis[2];
unsigned long trigger3millis[2];
unsigned long trigger4millis[2];
unsigned long trigger5millis[2];
unsigned long phase1millis[2];
unsigned long phase2millis[2];
unsigned long phase3millis[2];
unsigned long phase4millis[2];
unsigned long starttime[2];
int phase[2];
boolean pausestate[2];

bool chargevalvestate[2];
bool dischargevalvestate[2];
bool xformerstate = false;

boolean push1state = false;
boolean push2state = false;
boolean push3state = false;
boolean push4state = false;

void setup() 
{
  SPI.begin();

  Serial.begin(9600); // set up the monitor port for when connected via USB to computer
  
// LEDs  

  MonOut("Starting Ventilator");

  for( int i = 0; i < 2; i++ )
  {
     trigger[i] = 0;
     loopcount[i] = 0;
     opmode[i] = 1;
     chargevalvestate[i] = false;
     dischargevalvestate[i] = false;
     pausestate[i] = false;
  }
    
  curventilator = 0;
  push1state = false;
  push2state = false;
  push3state = false;
  push4state = false;

  // valves
  pinMode(VALVE_1_PIN,OUTPUT);
  pinMode(VALVE_2_PIN,OUTPUT);
  //pinMode(VALVE_3_PIN,OUTPUT);
  //pinMode(VALVE_4_PIN,OUTPUT);
  
  // ws2812
  pinMode(WS2812_1_PIN,OUTPUT);
  //pinMode(WS2812_2_PIN,OUTPUT);
  pinMode(XFORMER_PIN,OUTPUT);
  // in state
  pinMode(STATE_1_PIN,INPUT_PULLUP);
  pinMode(STATE_2_PIN,INPUT_PULLUP);
  
  // switches
  pinMode(PUSHBUTTON1_PIN,INPUT_PULLUP);
  pinMode(PUSHBUTTON2_PIN,INPUT_PULLUP);
  pinMode(PUSHBUTTON3_PIN,INPUT_PULLUP);
  pinMode(PUSHBUTTON4_PIN,INPUT_PULLUP);
  
  digitalWrite(WS2812_1_PIN,0);
  //digitalWrite(WS2812_2_PIN,0);

  digitalWrite(VALVE_1_PIN,HIGH);
  digitalWrite(VALVE_2_PIN,HIGH);
  //digitalWrite(VALVE_3_PIN,HIGH);
  //digitalWrite(VALVE_4_PIN,HIGH);
  digitalWrite(XFORMER_PIN,HIGH);
  strip.updateType(NEO_GRB + NEO_KHZ800);
  strip.updateLength(32);
  strip.setPin(WS2812_1_PIN);

  CloseChargeValve(0);
  CloseChargeValve(1);
  CloseDischargeValve(0);
  CloseDischargeValve(1);
  SetVentStatus();
  randomSeed(analogRead(0));
  MonOut("Ventilator Started");
}

void loop() 
{
    if( push1state == false )
    {
      if( digitalRead(PUSHBUTTON1_PIN) == 0 )
      {
          delay(100);
          push1state = true;
          if( opmode[curventilator] == 0 )
          {
              CloseChargeValve(curventilator);
              CloseDischargeValve(curventilator);
              SetVentStatus();
              return;
          }
          if( trigger[curventilator] == 6 ) // running
          {
              CloseChargeValve(curventilator);
              OpenDischargeValve(curventilator);
              SetVentStatus();
              delay(2000);
              trigger[curventilator] = 0;
              trigger1millis[curventilator] = millis();
          }
          if( trigger[curventilator] == 0 ) // idle
          {
              trigger[curventilator] = 1;
              trigger1millis[curventilator] = millis();
          }
          else if( trigger[curventilator] == 1 ) // charging
          {
              trigger[curventilator] = 2;
              trigger2millis[curventilator] = millis(); 
          }
          else if( trigger[curventilator] == 2 ) // stopped
          {
              trigger[curventilator] = 3;
              trigger3millis[curventilator] = millis(); 
          }
          else if( trigger[curventilator] == 3 ) // discharging
          {
              trigger[curventilator] = 4;
              trigger4millis[curventilator] = millis(); 
          }
          else if( trigger[curventilator] == 4 ) // stopped again
          {
              trigger[curventilator] = 5;
              trigger5millis[curventilator] = millis(); 
          }
          SetVentStatus();
          MonOut("Push 1");
      }
    }
    else if( digitalRead(PUSHBUTTON1_PIN) == 1 )
    {
        delay(100);
        push1state = false;
    }
    
    if( push2state == false )  // pause
    {
      if( digitalRead(PUSHBUTTON2_PIN) == 0 )
      {
          delay(100);
          push2state = true;

          if( trigger[curventilator] == 6 )
          {
              if( curventilator == 1 )
              {
                  TransferSettings();
                  MonOut("Transfering settings");
              }
              else
              {
                  if( pausestate[curventilator] == false )
                  {
                      pausestate[curventilator] = true;
                      MonOut("Pausing");
                  }
                  else
                  {
                      pausestate[curventilator] = false;
                      MonOut("Continueing");
                  }
              }
          }
          MonOut("Push 2");
      }
    }
    else if( digitalRead(PUSHBUTTON2_PIN) == 1 )
    {
        delay(100);
        push2state = false;
    }
    
    
    if( push3state == false )  // opmode selection
    {
      if( digitalRead(PUSHBUTTON3_PIN) == 0 )
      {
          delay(100);
          push3state = true;
          opmode[curventilator]++;
          if( opmode[curventilator] > 8 )
          {
              loopcount[curventilator] = 0;
              opmode[curventilator] = 0;
          }
          MonOut("Push 3");
      }
    }
    else if( digitalRead(PUSHBUTTON3_PIN) == 1 )
    {
        delay(100);
        push3state = false;
    }
    
    if( push4state == false )
    {
      if( digitalRead(PUSHBUTTON4_PIN) == 0 )
      {
          delay(100);
          push4state = true;
          // not wired up yet
          curventilator++;
          if( curventilator >= 2 )
            curventilator = 0;
          MonOut("Push 4");
      }
    }
    else if( digitalRead(PUSHBUTTON4_PIN) == 1 )
    {
        delay(100);
        push4state = false;
    }
    
    DoVentilators();
}

int DoVentilators()
{
    DoVentilator(0);
    DoVentilator(1);
}

void TransferSettings()
{
    opmode[0] = opmode[1];
    trigger[0] = trigger[1];
    loopcount[0] = 0;
    lastmillis[0] = 0;
    trigger1millis[0] = trigger1millis[1];
    trigger2millis[0] = trigger2millis[1];
    trigger3millis[0] = trigger3millis[1];
    trigger4millis[0] = trigger4millis[1];
    trigger5millis[0] = trigger5millis[1];
    phase1millis[0] = phase1millis[1];
    phase2millis[0] = phase2millis[1];
    phase3millis[0] = phase3millis[1];
    phase4millis[0] = phase4millis[1];
    starttime[0] = 0;
    phase[0] = 0;
    pausestate[0] = false;
}
int DoVentilator( int venthdl )
{
    if( opmode[venthdl] == 0 )
    {
        SetVentStatus();
        lastmillis[venthdl] = millis();
    }
    if( opmode[venthdl] == 1 )    // basic user defined timing with pause
    {
        if( trigger[venthdl] == 1 )
        {
            OpenChargeValve(venthdl);
            CloseDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 2 )
        {
            CloseChargeValve(venthdl);
            CloseDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 3 )
        {
            CloseChargeValve(venthdl);
            OpenDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 4 )
        {
            CloseChargeValve(venthdl);
            CloseDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 5 )
        {
            // complete
            phase1millis[venthdl] = trigger2millis[venthdl] - trigger1millis[venthdl];
            phase2millis[venthdl] = trigger3millis[venthdl] - trigger1millis[venthdl];
            phase3millis[venthdl] = trigger4millis[venthdl] - trigger1millis[venthdl];
            phase4millis[venthdl] = trigger5millis[venthdl] - trigger1millis[venthdl];
            trigger[venthdl] = 6;
            phase[venthdl] = 0;
            // help1 - save to EEPROM
        }
    
        if( trigger[venthdl] != 6 )
        {
            SetVentStatus();
            lastmillis[venthdl] = millis();
            return FNCOK;
        }
    
        if( phase[venthdl] == 0 )
        {
            if( pausestate[venthdl] == true )
            {
                CloseChargeValve(venthdl);
                OpenDischargeValve(venthdl);
                SetVentStatus();
                lastmillis[venthdl] = millis();
                return;
            }
            phase[venthdl] = 1;
            starttime[venthdl] = millis();
            CloseDischargeValve(venthdl);
            OpenChargeValve(venthdl);
        }
        else if( phase[venthdl] == 1 )
        {
            if( (starttime[venthdl] + phase1millis[venthdl]) < millis() )
            {
                phase[venthdl] = 2;
                CloseChargeValve(venthdl);
            }
        }
        else if( phase[venthdl] == 2 )
        {
            if( (starttime[venthdl] + phase2millis[venthdl]) < millis() )
            {
                phase[venthdl] = 3;
                OpenDischargeValve(venthdl);
            }
        }
        else if( phase[venthdl] == 3 )
        {
            if( (starttime[venthdl] + phase3millis[venthdl]) < millis() )
            {
                phase[venthdl] = 4;
                CloseDischargeValve(venthdl);
            }
        }
        else if( phase[venthdl] == 4 )
        {
            if( (starttime[venthdl] + phase4millis[venthdl]) < millis() )
            {
                phase[venthdl] = 0;
                // start over
            }
        }
    }
    else if( opmode[venthdl] == 2 )   // overlapping charge and discharge - to clear tubes
    {
        if( trigger[venthdl] == 1 )
        {
            OpenChargeValve(venthdl);
            CloseDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 2 )
        {
            OpenChargeValve(venthdl);
            OpenDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 3 )
        {
            CloseChargeValve(venthdl);
            OpenDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 4 )
        {
            CloseChargeValve(venthdl);
            CloseDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 5 )
        {
            // complete
            phase1millis[venthdl] = trigger2millis[venthdl] - trigger1millis[venthdl];
            phase2millis[venthdl] = trigger3millis[venthdl] - trigger1millis[venthdl];
            phase3millis[venthdl] = trigger4millis[venthdl] - trigger1millis[venthdl];
            phase4millis[venthdl] = trigger5millis[venthdl] - trigger1millis[venthdl];
            trigger[venthdl] = 6;
            phase[venthdl] = 0;
            // help1 - save to EEPROM
        }
    
        if( trigger[venthdl] != 6 )
        {
            SetVentStatus();
            lastmillis[venthdl] = millis();
            return FNCOK;
        }
    
        if( phase[venthdl] == 0 )
        {
            if( pausestate[venthdl] == true )
            {
                CloseChargeValve(venthdl);
                OpenDischargeValve(venthdl);
                SetVentStatus();
                lastmillis[venthdl] = millis();
                return;
            }
            phase[venthdl] = 1;
            starttime[venthdl] = millis();
            CloseDischargeValve(venthdl);
            OpenChargeValve(venthdl);
        }
        else if( phase[venthdl] == 1 )
        {
            if( (starttime[venthdl] + phase1millis[venthdl]) < millis() )
            {
                phase[venthdl] = 2;
                OpenDischargeValve(venthdl);
            }
        }
        else if( phase[venthdl] == 2 )
        {
            if( (starttime[venthdl] + phase2millis[venthdl]) < millis() )
            {
                phase[venthdl] = 3;
                CloseChargeValve(venthdl);
            }
        }
        else if( phase[venthdl] == 3 )
        {
            if( (starttime[venthdl] + phase3millis[venthdl]) < millis() )
            {
                phase[venthdl] = 4;
                CloseDischargeValve(venthdl);
            }
        }
        else if( phase[venthdl] == 4 )
        {
            if( (starttime[venthdl] + phase4millis[venthdl]) < millis() )
            {
                phase[venthdl] = 0;
                // start over
            }
        }
    }
    else if( opmode[venthdl] == 3 )   // charge valve always on - discharge is cycled
    {
        if( trigger[venthdl] == 1 )
        {
            OpenChargeValve(venthdl);
            CloseDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 2 )
        {
            OpenChargeValve(venthdl);
            OpenDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 3 )
        {
            OpenChargeValve(venthdl);
            OpenDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 4 )
        {
            OpenChargeValve(venthdl);
            CloseDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 5 )
        {
            // complete
            phase1millis[venthdl] = trigger2millis[venthdl] - trigger1millis[venthdl];
            phase2millis[venthdl] = trigger3millis[venthdl] - trigger1millis[venthdl];
            phase3millis[venthdl] = trigger4millis[venthdl] - trigger1millis[venthdl];
            phase4millis[venthdl] = trigger5millis[venthdl] - trigger1millis[venthdl];
            trigger[venthdl] = 6;
            phase[venthdl] = 0;
            // help1 - save to EEPROM
        }
    
        if( trigger[venthdl] != 6 )
        {
            SetVentStatus();
            lastmillis[venthdl] = millis();
            return FNCOK;
        }
    
        if( phase[venthdl] == 0 )
        {
            if( pausestate[venthdl] == true )
            {
                OpenChargeValve(venthdl);
                OpenDischargeValve(venthdl);
                SetVentStatus();
                lastmillis[venthdl] = millis();
                return;
            }
            phase[venthdl] = 1;
            starttime[venthdl] = millis();
            CloseDischargeValve(venthdl);
            OpenChargeValve(venthdl);
        }
        else if( phase[venthdl] == 1 )
        {
            if( (starttime[venthdl] + phase1millis[venthdl]) < millis() )
            {
                phase[venthdl] = 2;
                OpenDischargeValve(venthdl);
            }
        }
        else if( phase[venthdl] == 2 )
        {
            if( (starttime[venthdl] + phase2millis[venthdl]) < millis() )
            {
                phase[venthdl] = 3;
                //OpenChargeValve(venthdl);
            }
        }
        else if( phase[venthdl] == 3 )
        {
            if( (starttime[venthdl] + phase3millis[venthdl]) < millis() )
            {
                phase[venthdl] = 4;
                CloseDischargeValve(venthdl);
            }
        }
        else if( phase[venthdl] == 4 )
        {
            if( (starttime[venthdl] + phase4millis[venthdl]) < millis() )
            {
                phase[venthdl] = 0;
                // start over
            }
        }
    }
    else if( opmode[venthdl] == 4 )   // manual charge valve on - discharge is cycled
    {
        if( trigger[venthdl] == 1 )
        {
            CloseChargeValve(venthdl);
            CloseDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 2 )
        {
            //OpenChargeValve(venthdl);
            OpenDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 3 )
        {
            //OpenChargeValve(venthdl);
            OpenDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 4 )
        {
            //OpenChargeValve(venthdl);
            CloseDischargeValve(venthdl);
        }
        else if( trigger[venthdl] == 5 )
        {
            // complete
            phase1millis[venthdl] = trigger2millis[venthdl] - trigger1millis[venthdl];
            phase2millis[venthdl] = trigger3millis[venthdl] - trigger1millis[venthdl];
            phase3millis[venthdl] = trigger4millis[venthdl] - trigger1millis[venthdl];
            phase4millis[venthdl] = trigger5millis[venthdl] - trigger1millis[venthdl];
            trigger[venthdl] = 6;
            phase[venthdl] = 0;
            // help1 - save to EEPROM
        }
    
        if( trigger[venthdl] != 6 )
        {
            SetVentStatus();
            lastmillis[venthdl] = millis();
            return FNCOK;
        }
    
        if( phase[venthdl] == 0 )
        {
            if( pausestate[venthdl] == true )
            {
                OpenChargeValve(venthdl);
                OpenDischargeValve(venthdl);
                SetVentStatus();
                lastmillis[venthdl] = millis();
                return;
            }
            phase[venthdl] = 1;
            starttime[venthdl] = millis();
            CloseDischargeValve(venthdl);
            CloseChargeValve(venthdl);
        }
        else if( phase[venthdl] == 1 )
        {
            if( (starttime[venthdl] + phase1millis[venthdl]) < millis() )
            {
                phase[venthdl] = 2;
                OpenDischargeValve(venthdl);
            }
        }
        else if( phase[venthdl] == 2 )
        {
            if( (starttime[venthdl] + phase2millis[venthdl]) < millis() )
            {
                phase[venthdl] = 3;
                //OpenChargeValve(venthdl);
            }
        }
        else if( phase[venthdl] == 3 )
        {
            if( (starttime[venthdl] + phase3millis[venthdl]) < millis() )
            {
                phase[venthdl] = 4;
                CloseDischargeValve(venthdl);
            }
        }
        else if( phase[venthdl] == 4 )
        {
            if( (starttime[venthdl] + phase4millis[venthdl]) < millis() )
            {
                phase[venthdl] = 0;
                // start over
            }
        }
    }
    SetVentStatus();
    lastmillis[venthdl] = millis();
}

void OpenChargeValve( int venthdl )
{
  //XformerOn();
  if( chargevalvestate[venthdl] == true )
    return;
  if( venthdl == 0 )
  {
      XformerOn();
      digitalWrite(VALVE_1_PIN,LOW);
  }
  //else
  //    digitalWrite(VALVE_3_PIN,LOW);
  chargevalvestate[venthdl] = true;
}

void OpenDischargeValve( int venthdl )
{
  //XformerOn();
  if( dischargevalvestate[venthdl] == true )
      return;
  if( venthdl == 0 )
  {
      XformerOn();
      digitalWrite(VALVE_2_PIN,LOW);
  }
  //else
  //    digitalWrite(VALVE_4_PIN,LOW);
  dischargevalvestate[venthdl] = true;
}

void CloseChargeValve( int venthdl )
{
  if( chargevalvestate[venthdl] == false )
    return;
  if( venthdl == 0 )
      digitalWrite(VALVE_1_PIN,HIGH);
  //else
  //    digitalWrite(VALVE_3_PIN,HIGH);
  chargevalvestate[venthdl] = false;
  if( (dischargevalvestate[0] == false) && (chargevalvestate[0] == false) )
      XformerOff();
}

void CloseDischargeValve( int venthdl )
{
  if( dischargevalvestate[venthdl] == false )
    return;
  if( venthdl == 0 )
      digitalWrite(VALVE_2_PIN,HIGH);
  //else
  //    digitalWrite(VALVE_4_PIN,HIGH);
  dischargevalvestate[venthdl] = false;
  if( (dischargevalvestate[0] == false) && (chargevalvestate[0] == false) )
      XformerOff();
}

void XformerOn()
{
    if( xformerstate == true )
        return;
    digitalWrite(XFORMER_PIN,LOW);
    xformerstate = true;
    delay(100);
}

void XformerOff()
{
    if( xformerstate == false )
        return;
    digitalWrite(XFORMER_PIN,HIGH);
    delay(100);
    xformerstate = false;
}

void SetVentStatus()
{
    int venthdl = 0;
    uint32_t blue = strip.Color(0,10,10);
    uint32_t red = strip.Color(220,0,0);
    uint32_t lightred = strip.Color(2,0,0);
    uint32_t green = strip.Color(0,220,0);
    uint32_t black = strip.Color(0,0,0);
    uint32_t white = strip.Color(100,100,100);
    uint32_t purple = strip.Color(80,0,80);
    uint32_t color1;
    uint32_t color2;

    color2 = blue;
    for(uint16_t i=0; i<strip.numPixels(); i++) 
           strip.setPixelColor(i,color2);
        
    if( (trigger[venthdl] > 0) && (opmode[venthdl] > 0) )
    {
      strip.setPixelColor(0,black);
      strip.setPixelColor(2,black);
      strip.setPixelColor(4,black);
      strip.setPixelColor(6,black);
      strip.setPixelColor(8,black);
   
      strip.setPixelColor(1,red);
      strip.setPixelColor(3,red);
      strip.setPixelColor(5,red);
      strip.setPixelColor(7,red);
  
      if( trigger[venthdl] == 6 )
          color1 = purple;
      else
          color1 = green;
      if( trigger[venthdl] >= 1 )
      {
          strip.setPixelColor(1,color1);
      }
      if( trigger[venthdl] >= 2 )
      {
          strip.setPixelColor(3,color1);
      }
      if( trigger[venthdl] >= 3 )
      {
          strip.setPixelColor(5,color1);
      }
      if( trigger[venthdl] >= 4 )
      {
          strip.setPixelColor(7,color1);
      }

      // show opmode
      if( opmode[venthdl] == 1 )
          strip.setPixelColor(2,lightred);
      else if( opmode[venthdl] == 2 )
          strip.setPixelColor(4,lightred);
      else if( opmode[venthdl] == 3 )
      {
          strip.setPixelColor(2,lightred);
          strip.setPixelColor(4,lightred);
      }
      else if( opmode[venthdl] == 4 )
          strip.setPixelColor(6,lightred);
      else if( opmode[venthdl] == 5 )
      {
          strip.setPixelColor(2,lightred);
          strip.setPixelColor(6,lightred);
      }
      else if( opmode[venthdl] == 6 )
      {
          strip.setPixelColor(4,lightred);
          strip.setPixelColor(6,lightred);
      }
      else if( opmode[venthdl] == 7 )
      {
          strip.setPixelColor(2,lightred);
          strip.setPixelColor(4,lightred);
          strip.setPixelColor(6,lightred);
      }
      else if( opmode[venthdl] == 8 )
         strip.setPixelColor(8,lightred);
          
      // show valves
      if( chargevalvestate[venthdl] == true )
          strip.setPixelColor(14,green);
      else
          strip.setPixelColor(14,red);
      if( dischargevalvestate[venthdl] == true )
          strip.setPixelColor(15,green);
      else
          strip.setPixelColor(15,red);
      
      if( trigger[venthdl] == 6 )
      {
          strip.setPixelColor(13,black);
          // show phase
          if( phase[venthdl] == 1 )
              strip.setPixelColor(9,green);
          else if( phase[venthdl] == 2 )
              strip.setPixelColor(10,green);
          else if( phase[venthdl] == 3 )
              strip.setPixelColor(11,green);
          else if( phase[venthdl] == 4 )
              strip.setPixelColor(12,green);
      }
    }

    // vent 2
    venthdl = 1;

    if( opmode[venthdl] == 0 )
    {
      if( curventilator == 0 )
          strip.setPixelColor(0,white);
      else
          strip.setPixelColor(16,white);
        strip.show();
        return;
    }
    if( trigger[venthdl] == 0 )
    {
        if( curventilator == 0 )
            strip.setPixelColor(0,white);
        else
            strip.setPixelColor(16,white);
        strip.show();
        return;
    }
    strip.setPixelColor(0 + 16,black);
    strip.setPixelColor(2 + 16,black);
    strip.setPixelColor(4 + 16,black);
    strip.setPixelColor(6 + 16,black);
    strip.setPixelColor(8 + 16,black);
 
    strip.setPixelColor(1 + 16,red);
    strip.setPixelColor(3 + 16,red);
    strip.setPixelColor(5 + 16,red);
    strip.setPixelColor(7 + 16,red);

    if( trigger[venthdl] == 6 )
        color1 = purple;
    else
        color1 = green;
    if( trigger[venthdl] >= 1 )
    {
        strip.setPixelColor(1 + 16,color1);
    }
    if( trigger[venthdl] >= 2 )
    {
        strip.setPixelColor(3 + 16,color1);
    }
    if( trigger[venthdl] >= 3 )
    {
        strip.setPixelColor(5 + 16,color1);
    }
    if( trigger[venthdl] >= 4 )
    {
        strip.setPixelColor(7 + 16,color1);
    }

    if( opmode[venthdl] == 1 )
        strip.setPixelColor(2 + 16,lightred);
    else if( opmode[venthdl] == 2 )
         strip.setPixelColor(4 + 16,lightred);
    else if( opmode[venthdl] == 3 )
    {
          strip.setPixelColor(2 + 16,lightred);
          strip.setPixelColor(4 + 16,lightred);
    }
    else if( opmode[venthdl] == 4 )
          strip.setPixelColor(6 + 16,lightred);
    else if( opmode[venthdl] == 5 )
    {
          strip.setPixelColor(2 + 16,lightred);
          strip.setPixelColor(6 + 16,lightred);
    }
    else if( opmode[venthdl] == 6 )
    {
          strip.setPixelColor(4 + 16,lightred);
          strip.setPixelColor(6 + 16,lightred);
    }
    else if( opmode[venthdl] == 7 )
    {
          strip.setPixelColor(2 + 16,lightred);
          strip.setPixelColor(4 + 16,lightred);
          strip.setPixelColor(6 + 16,lightred);
    }
    else if( opmode[venthdl] == 8 )
         strip.setPixelColor(8 + 16,lightred);
     
    // show valves
    if( chargevalvestate[venthdl] == true )
        strip.setPixelColor(14 + 16,green);
    else
        strip.setPixelColor(14 + 16,red);
    if( dischargevalvestate[venthdl] == true )
        strip.setPixelColor(15 + 16,green);
    else
        strip.setPixelColor(15 + 16,red);

    if( curventilator == 0 )
        strip.setPixelColor(0,white);
    else
        strip.setPixelColor(16,white);
        
    if( trigger[venthdl] != 6 )
    {
        strip.show();
        return;
    }
    strip.setPixelColor(13 + 16,black);
    // show phase
    if( phase[venthdl] == 1 )
        strip.setPixelColor(9 + 16,green);
    else if( phase[venthdl] == 2 )
        strip.setPixelColor(10 + 16,green);
    else if( phase[venthdl] == 3 )
        strip.setPixelColor(11 + 16,green);
    else if( phase[venthdl] == 4 )
        strip.setPixelColor(12 + 16,green);

    strip.show();
}


void MonOut( char *str )
{
  Serial.println(str);
}
