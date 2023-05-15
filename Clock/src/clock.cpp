#include <Arduino.h>
#include "SPI.h"
#include "TFT_22_ILI9225.h"
#include <../fonts/FreeSans12pt7b.h>
#include <../fonts/FreeSans24pt7b.h>
// #include <ThreeWire.h>
#include <RtcDS1302.h>
#if defined(ARDUINO_ARCH_STM32F1)
#define TFT_RST PA1
#define TFT_RS PA2
#define TFT_CS PA0  // SS
#define TFT_SDI PA7 // MOSI
#define TFT_CLK PA5 // SCK
#define TFT_LED 0   // 0 if wired to +5V directly
#elif defined(ESP8266)
#define TFT_RST 4  // D2
#define TFT_CLK 14 // D5 SCK
// #define TFT_SDO 12  // D6 MISO
#define TFT_SDI 13 // D7 MOSI
#define TFT_CS 15  // D8 SS
#elif defined(ESP32)
#define TFT_RST 26 // IO 26
#define TFT_RS 25  // IO 25
#define TFT_CLK 14 // HSPI-SCK
// #define TFT_SDO 12  // HSPI-MISO
#define TFT_SDI 13 // HSPI-MOSI
#define TFT_CS 15  // HSPI-SS0
#define TFT_LED 0  // 0 if wired to +5V directly
#else
#define TFT_RST 8
#define TFT_RS 9
#define TFT_CS 10
#define TFT_SDI 11
#define TFT_CLK 13
#define RIGHT_BUTTON 17
#define LEFT_BUTTON 16
#define UP_BUTTON 14
#define DOWN_BUTTON 15
#define CHANGE_BUTTON 18
#define SPEAKER 3
#endif

#define TFT_BRIGHTNESS 100

// hardware var
TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_CLK, TFT_BRIGHTNESS);
ThreeWire myWire(7, 6, 4);
RtcDS1302<ThreeWire> Rtc(myWire);
RtcDateTime now;

// pos var
String MONTH[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

// time var
unsigned long time, alarm[6] = {0, 0, 0, 0, 0, 0}, changing;
unsigned long alarmSet[6] = {0, 0, 0, 0, 0, 0};
unsigned long minuteChanging, secondChanging, hourChanging;
unsigned long DownButtonReady, UpButtonReady, LeftButtonReady, RightButtonReady, ChangeButtonReady;
String second, minute, hour, lastSecond;

// other var
boolean inChange = false, check = false;
boolean alarmOn = false;
unsigned long ButtonType = -1, timeAfterPressed[5];

void UP_PRESS()
{
    Serial.println("UP BUTTON PRESSES");
    if (inChange)
    {
        if (changing == 3 || changing == 5)
            alarm[changing] = (alarm[changing] + 1) % 10;
        else if (changing == 2 || changing == 4)
            alarm[changing] = (alarm[changing] + 1) % 6;
        else if (changing == 0)
            alarm[changing] = (alarm[changing] + 1) % 3;
        else
        {
            if (alarm[0] == 2)
                alarm[changing] = (alarm[changing] + 1) % 5;
            else
                alarm[changing] = (alarm[changing] + 1) % 10;
        }
    }
}

void DOWN_PRESS()
{
    Serial.println("DOWN BUTTON PRESSES");
    if (inChange)
    {
        if (changing == 3 || changing == 5)
            (alarm[changing] - 1 >= 0) ? alarm[changing]-- : alarm[changing] = 9;
        else if (changing == 2 || changing == 4)
            (alarm[changing] - 1 >= 0) ? alarm[changing]-- : alarm[changing] = 5;
        else if (changing == 0)
            (alarm[changing] - 1 >= 0) ? alarm[changing]-- : alarm[changing] = 2;
        else
        {
            if (alarm[0] == 2)
                (alarm[changing] - 1 >= 0) ? alarm[changing]-- : alarm[changing] = 4;
            else
                (alarm[changing] - 1 >= 0) ? alarm[changing]-- : alarm[changing] = 9;
        }
    }
}

void LEFT_PRESS()
{
    Serial.println("LEFT BUTTON PRESSES");
    changing = max(0, changing - 1);
    // tft.clear();
}

void RIGHT_PRESS()
{
    Serial.println("RIGHT BUTTON PRESSES");
    changing = min(5, changing + 1);
    // tft.clear();
}

void CHANGE_PRESS()
{
    Serial.println("CHANGE BUTTON PRESSES");
    inChange = !inChange;
}

void GetDate()
{
    second = String(now.Second());
    minute = String(now.Minute());
    hour = String(now.Hour());
    if (hour.length() == 1)
        hour = '0' + hour;
    if (minute.length() == 1)
        minute = '0' + minute;
    if (second.length() == 1)
        second = '0' + second;
}

void check_button()
{
    unsigned long readButton[5];
    readButton[0] = analogRead(CHANGE_BUTTON);
    readButton[1] = analogRead(DOWN_BUTTON);
    readButton[2] = analogRead(UP_BUTTON);
    readButton[3] = analogRead(LEFT_BUTTON);
    readButton[4] = analogRead(RIGHT_BUTTON);
    String output = String(timeAfterPressed[0]) + ", " + String(timeAfterPressed[1]) + ", " + String(timeAfterPressed[2]) + ", " + String(timeAfterPressed[3]) + ", " + String(timeAfterPressed[4]);
    //Serial.println(output);
    for (int i = 0; i < 5; i++)
    {
        if (readButton[i] >= 1016 && readButton[i] <= 1019)
        {
            timeAfterPressed[i]++;
        }
        else
        {
            if (timeAfterPressed[i] >= 2)
            {
                if (i == 0)
                    CHANGE_PRESS();
                else if (i == 1)
                    DOWN_PRESS();
                else if (i == 2)
                    UP_PRESS();
                else if (i == 3)
                    LEFT_PRESS();
                else if (i == 4)
                    RIGHT_PRESS();
                timeAfterPressed[i] = 0;
            }
            else
                timeAfterPressed[i] = 0;
        }
    }
}

void screen_display()
{
    tft.drawLine(0, 90, 176, 90, COLOR_WHITE);
}

void write_text()
{
    tft.setBackgroundColor(COLOR_BLACK);
    tft.setFont(Trebuchet_MS16x21);
    tft.drawText(25, 5, hour, COLOR_WHITE);
    tft.drawText(60, 5, ":", COLOR_WHITE);
    tft.drawText(70, 5, minute, COLOR_WHITE);
    tft.drawText(105, 5, ":", COLOR_WHITE);
    tft.drawText(115, 5, second, COLOR_WHITE);
    // Serial.println(hour + ":" + minute + ":" + second);
    tft.setFont(Terminal11x16);
    tft.drawText(65, 50, MONTH[int(now.Month()) - 1]);
    tft.drawText(23, 50, String(now.Day()));
    tft.drawText(115, 50, String(now.Year()));

    tft.setFont(Terminal11x16);
    tft.drawText(10, 100, "Detontating in: ", COLOR_WHITE);
    if (!inChange)
    {
        tft.setFont(Trebuchet_MS16x21);
        tft.drawText(15, 140, String(alarm[0]) + String(alarm[1]), COLOR_WHITE);
        tft.drawText(50, 140, ":", COLOR_WHITE);
        tft.drawText(60, 140, String(alarm[2]) + String(alarm[3]), COLOR_WHITE);
        tft.drawText(95, 140, ":", COLOR_WHITE);
        tft.drawText(105, 140, String(alarm[4]) + String(alarm[5]), COLOR_WHITE);
        // Serial.println(setTime(hourChange) + ":" + setTime(minuteChange) + ":" + setTime(secondChange));
        tft.setFont(Terminal11x16);
        if (alarmOn)
        {
            // tft.fillRectangle(130, 135, 160, 160, COLOR_GREEN);
            tft.setBackgroundColor(COLOR_GREEN);
            tft.drawText(140, 130, "ON", COLOR_WHITE);
        }
        else
        {
            // tft.fillRectangle(130, 135, 160, 160, COLOR_RED);
            tft.setBackgroundColor(COLOR_RED);
            tft.drawText(140, 130, "OFF", COLOR_WHITE);
        }
        tft.setBackgroundColor(COLOR_CYAN);
        tft.drawText(140, 150, "SET", COLOR_BLACK);
    }
}

void update_time()
{
    time = millis();
    now = Rtc.GetDateTime();
    GetDate();
    if (second != lastSecond)
    {
        lastSecond = second;
        tft.fillRectangle(115, 5, 176, 25, COLOR_BLACK);
        if (second == "1")
        {
            tft.fillRectangle(70, 5, 104, 25, COLOR_BLACK);
            if (minute == "1")
                tft.fillRectangle(25, 5, 59, 25, COLOR_BLACK);
        }
    }
}

void setup()
{
    tft.begin();
    tft.clear();
    tft.setOrientation(0);
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    Rtc.Begin();
    RtcDateTime currentTime = RtcDateTime(__DATE__, __TIME__);
    Rtc.SetDateTime(currentTime);
    GetDate();
    lastSecond = second;
    inChange = 0;
    pinMode(UP_BUTTON, INPUT);
    pinMode(DOWN_BUTTON, INPUT);
    pinMode(LEFT_BUTTON, INPUT);
    pinMode(RIGHT_BUTTON, INPUT);
    pinMode(CHANGE_BUTTON, INPUT);
}

void loop()
{
    // Time & Date
    update_time();
    // Button
    check_button();

    // Screen
    screen_display();

    // Write text
    write_text();
}

/*if (analogRead(CHANGE_BUTTON) <= 1019 && analogRead(CHANGE_BUTTON) >= 1017)
    {
        LeftButtonReady = RightButtonReady = UpButtonReady = DownButtonReady = time;
        ButtonType = "CHANGE";
    }
    else if (analogRead(DOWN_BUTTON) <= 2 && analogRead(DOWN_BUTTON) != 0)
    {
        LeftButtonReady = RightButtonReady = ChangeButtonReady = UpButtonReady = time;
        ButtonType = "DOWN";
    }
    else if (analogRead(UP_BUTTON) <= 2 && analogRead(UP_BUTTON) != 0)
    {
        LeftButtonReady = RightButtonReady = ChangeButtonReady = DownButtonReady = time;
        ButtonType = "UP";
    }
    else if (analogRead(RIGHT_BUTTON) <= 2 && analogRead(RIGHT_BUTTON) != 0)
    {
        LeftButtonReady = ChangeButtonReady = UpButtonReady = DownButtonReady = time;
        ButtonType = "RIGHT";
    }
    else if (analogRead(LEFT_BUTTON) <= 2 && analogRead(LEFT_BUTTON) != 0)
    {
        RightButtonReady = ChangeButtonReady = UpButtonReady = DownButtonReady = time;
        ButtonType = "LEFT";
    }
    else
    {
        if (time - ChangeButtonReady >= 100 && ButtonType == "CHANGE")
        {
            inChange = !inChange;
            Serial.println("Change Button Pressed");
            ChangeButtonReady = time;
            ButtonType = "NONE";
        }
        else if (time - LeftButtonReady >= 100 && ButtonType == "LEFT")
        {
            LEFT_PRESS();
            LeftButtonReady = time;
            ButtonType = "NONE";
        }
        else if (time - RightButtonReady >= 100 && ButtonType == "RIGHT")
        {
            RIGHT_PRESS();
            RightButtonReady = time;
            ButtonType = "NONE";
        }
        else if (time - DownButtonReady >= 100 && ButtonType == "DOWN")
        {
            DOWN_PRESS();
            DownButtonReady = time;
            ButtonType = "NONE";
        }
        else if (time - UpButtonReady >= 100 && ButtonType == "UP")
        {
            UP_PRESS();
            UpButtonReady = time;
            ButtonType = "NONE";
        }
        else
        {
            LeftButtonReady = RightButtonReady = ChangeButtonReady = UpButtonReady = DownButtonReady = time;
        }
    } */