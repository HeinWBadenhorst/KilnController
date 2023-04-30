#include "max6675.h"
#include <PID_v1.h>
  
#include "U8glib.h"
U8GLIB_SH1106_128X64 u8g(4, 5, 6, 7);  // SW SPI Com: SCK = 4, MOSI = 5, CS = 6, A0 = 7 (new blue HalTec OLED)
 
// analog pin connected to keypad
#define KEYPAD_PIN 0
 
// milliseconds to wait, to make sure key is pressed
#define TIME_TO_WAIT 50

int ktcSO = 8;
int ktcCS = 9;
int ktcCLK = 10;//Temperature Cycle Settings

uint8_t state = 0;
uint16_t max_temp = 130; //in degrees C
uint8_t soak_time = 10; //in minutes
uint8_t ramp_rate = 100; //in degrees C/min
uint8_t cool_down = 10; //in degrees C/min

//PID parameters
double init_temp, celciusTemp;
double Input, Output, Setpoint;
uint8_t init_read = 1; //flag to prevent 2 temp reads on first PID pass
uint32_t PID_interval = 5000; //time in ms to run PID interval
 
//Object Instantiation
MAX6675 ktc(ktcCLK, ktcCS, ktcSO);
PID myPID(&Input, &Output, &Setpoint, 2, 5, 1, DIRECT);

  
void setup() {
  Serial.begin(9600);
  // give the MAX a little time to settle
  pinMode(13, OUTPUT);           
  digitalWrite(13, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  celciusTemp = 25;  
}

void u8g_prepare(void) {
  u8g.setFont(u8g_font_6x10);
  u8g.setFontRefHeightExtendedText();
  u8g.setDefaultForegroundColor();
  u8g.setFontPosTop();
}

void doubleToCharArray(double inDouble, char * arrayTempIn) {
 int fwidth;

if (inDouble < 10.0)
{
   fwidth = 4;
}
else if (inDouble < 100.0)
{
    fwidth = 5;
}
else if (inDouble < 1000.0)
{    fwidth = 5;
}
else
{
   fwidth = 6;
}

dtostrf(inDouble, fwidth, 2, arrayTempIn);
 
}

void relayOn() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
}

void relayOff() {
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off (LOW is the voltage level)
}

String getKey() 
{
    String retChar = "x";
    int r1 = analogRead(KEYPAD_PIN);
    //  Serial.print("AnalogRead="); 
    //  Serial.println(r1);
    // waiting
    delay(TIME_TO_WAIT);
    // reading 2nd time - to make sure key is pressed for at least TIME_TO_WAIT milliseconds
    int r2 = analogRead(KEYPAD_PIN);
    int r3 = abs(r1 - r2);
    if  ( (r3 < 5) &&  (r2 > 4)) {
        if ((r2 >= 96) && (r2 <= 100)) {
          retChar = "1";
          return retChar;
        }
        if ((r2 >= 121) && (r2 <= 127)) {
          retChar = "2";
          return retChar;
        }
        if ((r2 >= 155) && (r2 <= 162)) {
          retChar = "3";
          return retChar;
        }
        if ((r2 >= 267) && (r2 <= 275)) {
          retChar = "4";
          return retChar;
        }
        if ((r2 >= 322) && (r2 <= 331)) {
          retChar = "5";
          return retChar;
        }
        if ((r2 >= 390) && (r2 <= 400)) {
          retChar = "6";
          return retChar;
        }
        if ((r2 >= 497) && (r2 <= 507)) {
          retChar = "7";
          return retChar;
        }
        if ((r2 >= 565) && (r2 <= 575)) {
          retChar = "8";
          return retChar;
        }
        if ((r2 >= 631) && (r2 <= 647)) {
          retChar = "9";
          return retChar;
        }
        if ((r2 >= 735) && (r2 <= 751)) {
          retChar = "*";
          return retChar;
        }
        if ((r2 >= 793) && (r2 <= 800)) {
          retChar = "0";
          return retChar;
        }
        if ((r2 >= 829) && (r2 <= 847)) {
          retChar = "#";
          return retChar;
        }
        if ((r2 >= 190) && (r2 <= 206)) {
          retChar = "A";
          return retChar;
        }
        if ((r2 >= 446) && (r2 <= 456)) {
          retChar = "B";
          return retChar;
        }
        if ((r2 >= 685) && (r2 <= 695)) {
          retChar = "C";
          return retChar;
        }
        if ((r2 >= 860) && (r2 <= 878)) {
          retChar = "D";
          return retChar;
        }
    }
    return retChar;
  
}



void run_PID(double Kp, double Ki, double Kd, uint16_t WindowSize, uint32_t time_interval)
{
   double ratio;
   uint32_t windowStartTime;
   uint8_t buttons;
 
   //Specify the links and initial tuning parameters
   myPID.SetOutputLimits(0, WindowSize);
   myPID.SetTunings(Kp, Ki, Kd);
   myPID.SetMode(AUTOMATIC);
 
   //This prevents the system from initially hanging up
   if(init_read){init_read = 0;Setpoint = Setpoint + 1;}
   else{}
 
   windowStartTime = millis();
 
   Input = ktc.readCelsius();
   myPID.Compute();
 
   ratio = Output / WindowSize;
   //Serial.print("Output"); 
   //Serial.println(Output);
   //Serial.print("WindowSize"); 
   //Serial.println(WindowSize);
 
   relayOn();
   while(millis() - windowStartTime < time_interval * ratio) {
      String returnC = getKey();
      if (returnC != "x") {
        if (returnC == "A") {
          max_temp = max_temp + 5;
        } else
        if (returnC == "B") {
          max_temp = max_temp - 5;
          if (max_temp < 0) max_temp = 0;
        }
        // Serial.println("Return Character = " + returnC);
          
        }
      updateDisplay();
   }
   relayOff();
   while(millis() - windowStartTime < time_interval) {
      String returnC = getKey();
      if (returnC != "x") {
        if (returnC == "A") {
          max_temp = max_temp + 5;
        } else
        if (returnC == "B") {
          max_temp = max_temp - 5;
          if (max_temp < 0) max_temp = 0;
        }
        // Serial.println("Return Character = " + returnC);
          
        }
      updateDisplay();
   }
}
 
void run_cycle_time(void)
{
   uint32_t initial_time = millis();
   uint32_t elapsed_time;
   double diff_time_min;
   double cycle_time;
 
   //This set of statements calculates time of cycle phase
   if(state == 0) //rising ramp, increasing temperature
   {
      //Calculate time remaining in rise phase based on temperature and rate
      init_temp = ktc.readCelsius();
      cycle_time = float(max_temp - init_temp)/ramp_rate;
   }
   else if(state == 1) //soak time
   {
      //Soak time is already determined
      cycle_time = soak_time;
   }
   else if(state == 2) //falling ramp, decreasing temperature
   {
      //Calculate time remaining in fall phase based on temperature and rate each cycle
      cycle_time = (ktc.readCelsius() - init_temp)/cool_down;
   }
 
   //Determine time left in current phase
   elapsed_time = millis();
   diff_time_min = float(elapsed_time - initial_time) / 60000;

   while(diff_time_min < cycle_time)
   {
      if(state == 0) //rising ramp, increasing temperature
      {
          //While increasing, Setpoint increases based on elapsed time
          Setpoint = (diff_time_min * ramp_rate) + init_temp;
      }
      else if(state == 1) //soak time
      {
          Setpoint = max_temp;
      }
      else if(state == 2) //falling ramp, decreasing temperature
      {
          //While decreasing, Setpoint increases based on elapsed time
          Setpoint = max_temp - (diff_time_min * cool_down);
      }
 
      //Determine PID response based on current temp
      run_PID(2, 5, 1, 500, PID_interval);
      //Determine time left in current phase
      celciusTemp = ktc.readCelsius();
      elapsed_time = millis();
      diff_time_min = float(elapsed_time - initial_time) / 60000;
      Serial.print("diff_time_min="); 
      Serial.println(diff_time_min);
      Serial.print("Setpoint="); 
      Serial.println(Setpoint);
      Serial.print("Deg C = "); 
      Serial.println(ktc.readCelsius());
      Serial.print("State="); 
      Serial.println(state);
      
      Serial.print("cycle_time="); 
      Serial.println(cycle_time);
      Serial.print("elapsed_time="); 
      Serial.println(elapsed_time);
   }
}
 
double CtoF(double temp_C)
{
   double temp_F = (temp_C * 1.8) + 32;
   return temp_F;
}

void updateDisplay()
{
   u8g.firstPage();  
   do {
     u8g_prepare();
     u8g.drawStr( 0, 0*10+10, "  Ramp-Up:");
     char arrayRampUp[6] = " ";
     String rampupString = String(ramp_rate);
     rampupString.toCharArray(arrayRampUp,sizeof(arrayRampUp));
     u8g.drawStr(60, 0*10+10, arrayRampUp);
     u8g.drawStr(0, 1*10+10,"Soak-Time:");
     char arraySoakTime[6] = " ";
     String soaktimeString = String(soak_time);
     soaktimeString.toCharArray(arraySoakTime,sizeof(arraySoakTime));
     u8g.drawStr(60, 1*10+10, arraySoakTime);
     u8g.drawStr(0, 2*10+10, "Soak Temp:");
     char arrayMaxTemp[6] = " ";
     String tempString = String(max_temp);
     tempString.toCharArray(arrayMaxTemp,sizeof(arrayMaxTemp));
     u8g.drawStr(60, 2*10+10, arrayMaxTemp);
     u8g.drawStr(0, 3*10+10, "Cool-Down:");
     char arrayCoolDown[6] = " ";
     String cooldownString = String(cool_down);
     cooldownString.toCharArray(arrayCoolDown,sizeof(arrayCoolDown));
     u8g.drawStr(60, 3*10+10, arrayCoolDown);
     u8g.drawStr(0, 4*10+10, "  Temp-Celcius:");
     char arrayTemp[6] = " ";
     doubleToCharArray(celciusTemp,arrayTemp);
     u8g.drawStr(90, 4*10+10, arrayTemp);
  } while( u8g.nextPage() );
  
}

void loop() {
   Serial.print("Deg C = "); 
   Serial.println(ktc.readCelsius());
   //Serial.print("\t Deg F = ");
   //Serial.println(ktc.readFahrenheit());
      switch (state) {
      case 0: //Ramp Up
         run_cycle_time();
         state = state + 1;
         break;
      case 1: //Soak Time
         run_cycle_time();
         state = state + 1;
         break;
      case 2: //Cool Down
         run_cycle_time();
         state = state + 1;
         break;
      case 3: //Finished
         while(1);
    }

}
 
