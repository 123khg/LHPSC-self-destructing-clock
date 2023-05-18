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
ThreeWire myWire(7, 6, 4); // DAT, CLK, RST
RtcDS1302<ThreeWire> Rtc(myWire);
RtcDateTime now;

// pos var
String MONTH[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

// time var
int time, changing = 0, lastChanging = 0;
int countDown[6] = {0, 0, 0, 0, 0, 0}, alarmSet[6] = {0, 0, 0, 0, 0, 0}, alarm[6] = {0, 0, 0, 0, 0, 0};
int alarmPointerX[7] = {5, 22, 50, 67, 95, 112, 130};
int alarmPointerY[7] = {164, 164, 164, 164, 164, 164, 140};
long secondTillDeath;
String second, minute, hour, lastSecond;
long secondCD, minuteCD, hourCD;

// other var
boolean check = false;
boolean alarmOn = false;
unsigned long ButtonType = -1, timeAfterPressed[5];
long mode = 3; // 0 set countdown - 1 set time - 2 start countdown - 3 alarm off
String modeName[4] = {"Set BOMB!!!:", "Explode at:", "Detonating in:", "alarm is OFF!!"};

void UP_PRESS()
{
    Serial.println("UP BUTTON PRESSES");
    if (changing == 6)
    {
        alarmOn = !alarmOn;
        return;
    }
    if (mode == 1)
    {
        if (changing == 3 || changing == 5)
            alarm[changing] = (alarm[changing] + 1) % 10;
        else if (changing == 2 || changing == 4)
            alarm[changing] = (alarm[changing] + 1) % 6;
        else if (changing == 0)
        {
            alarm[changing] = (alarm[changing] + 1) % 3;
            if (alarm[changing] == 2)
                alarm[changing + 1] = min(4, alarm[changing + 1]);
        }
        else
        {
            if (alarm[0] == 2)
                alarm[changing] = (alarm[changing] + 1) % 5;
            else
                alarm[changing] = (alarm[changing] + 1) % 10;
        }
    }
    else if (changing != 6)
        alarm[changing] = min(9, alarm[changing] + 1);
}

void DOWN_PRESS()
{
    Serial.println("DOWN BUTTON PRESSES");
    if (changing == 6)
    {
        alarmOn = !alarmOn;
        return;
    }
    if (mode == 1)
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
    else
        alarm[changing] = max(0, alarm[changing] - 1);
}

void LEFT_PRESS()
{
    Serial.println("LEFT BUTTON PRESSES");
    changing = max(0, changing - 1);
}

void RIGHT_PRESS()
{
    Serial.println("RIGHT BUTTON PRESSES");
    changing = min(6, changing + 1);
}

void CHANGE_PRESS()
{
    Serial.println("CHANGE BUTTON PRESSES");
    tft.fillRectangle(0, 100, 176, 200, COLOR_BLACK);
    if (mode == 0 && mode == 1 && changing == 7)
    {
        // when user press set
        if (mode == 0)
        {
            for (int i = 0; i < 6; i++)
            {
                countDown[i] = alarm[i];
            }
            secondTillDeath = (countDown[0] * 10 + countDown[1]) * 3600 + (countDown[2] * 10 + countDown[3]) * 60 + (countDown[4] * 10 + countDown[5]);
        }
        else
        {
            for (int i = 0; i < 6; i++)
            {
                alarmSet[i] = alarm[i];
            }
            secondTillDeath = (alarmSet[0] * 10 + alarmSet[1]) * 3600 + (alarmSet[2] * 10 + alarmSet[3]) * 60 + (alarmSet[4] * 10 + alarmSet[5]);
            secondTillDeath = secondTillDeath - (hour.toInt() * 3600 + minute.toInt() * 60 + second.toInt());
            if (secondTillDeath < 0)
                secondTillDeath += 3600 * 24;
        }
        mode = 2;
    }
    else
    {
        mode = (mode + 1) % 3;
        if (mode == 0)
        {
            for (int i = 0; i < 6; i++)
            {
                alarm[i] = countDown[i];
            }
        }
        else if (mode == 1)
        {
            for (int i = 0; i < 6; i++)
            {
                alarm[i] = alarmSet[i];
            }
        }
    }
}

void drawTri(int x, int y, int size, int type)
{
    if (type)
    {
        tft.fillTriangle(x, y + size, x - size * 1.0 / 2, y - size * 1.0 / 2, x + size * 1.0 / 2, y - size * 1.0 / 2, COLOR_WHITE);
    }
    else
    {
        tft.fillTriangle(x + size, y, x - size * 1.0 / 2, y - size * 1.0 / 2, x - size * 1.0 / 2, y + size * 1.0 / 2, COLOR_WHITE);
    }
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

String setTime(String a)
{
    if (a.length() < 2)
        return '0' + a;
    return a;
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
    // Serial.println(output);
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
    // Real time
    tft.setBackgroundColor(COLOR_BLACK);
    tft.setFont(Trebuchet_MS16x21);
    tft.drawText(25, 5, hour, COLOR_WHITE);
    tft.drawText(60, 5, ":", COLOR_WHITE);
    tft.drawText(70, 5, minute, COLOR_WHITE);
    tft.drawText(105, 5, ":", COLOR_WHITE);
    tft.drawText(115, 5, second, COLOR_WHITE);
    // Serial.println(hour + ":" + minute + ":" + second);
    // Dates
    tft.setFont(Terminal11x16);
    tft.drawText(65, 50, MONTH[int(now.Month()) - 1]);
    tft.drawText(23, 50, String(now.Day()));
    tft.drawText(115, 50, String(now.Year()));

    // Alarm
    tft.setFont(Terminal11x16);
    tft.drawText(15, 100, modeName[mode], COLOR_WHITE);

    // Alarm time
    if (mode <= 1)
    {
        // Pointer
        if (lastChanging != changing)
        {
            tft.fillRectangle(0, 164, 140, 166, COLOR_BLACK);
            lastChanging = changing;
        }

        tft.fillRectangle(alarmPointerX[changing], alarmPointerY[changing], alarmPointerX[changing] + 16, alarmPointerY[changing] + 2, COLOR_DARKGREEN);

        // 2 settings
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

        tft.setBackgroundColor(COLOR_DARKCYAN);
        tft.drawText(140, 155, "SET", COLOR_WHITE);

        tft.setFont(Trebuchet_MS16x21);
        tft.drawText(5, 140, String(alarm[0]) + String(alarm[1]), COLOR_WHITE);
        tft.drawText(40, 140, ":", COLOR_WHITE);
        tft.drawText(50, 140, String(alarm[2]) + String(alarm[3]), COLOR_WHITE);
        tft.drawText(85, 140, ":", COLOR_WHITE);
        tft.drawText(95, 140, String(alarm[4]) + String(alarm[5]), COLOR_WHITE);
    }
    else if (mode == 3)
    {
        tft.setFont(Trebuchet_MS16x21);
        tft.drawText(5, 140, String(alarmSet[0]) + String(alarmSet[1]), COLOR_WHITE);
        tft.drawText(40, 140, ":", COLOR_WHITE);
        tft.drawText(85, 140, ":", COLOR_WHITE);
        tft.drawText(95, 140, setTime(String(secondCD)), COLOR_WHITE);
        tft.drawText(50, 140, setTime(String(minuteCD)), COLOR_WHITE);
        tft.drawText(5, 140, setTime(String(hourCD)), COLOR_WHITE);
    }
}

void update_time()
{
    time = millis();
    now = Rtc.GetDateTime();
    GetDate();
    if (second != lastSecond)
    {
        // Real time
        lastSecond = second;
        tft.fillRectangle(115, 5, 176, 25, COLOR_BLACK);
        if (second == "1")
        {
            tft.fillRectangle(70, 5, 104, 25, COLOR_BLACK);
            if (minute == "1")
                tft.fillRectangle(25, 5, 59, 25, COLOR_BLACK);
        }
        secondTillDeath--;
        if (mode == 3)
        {
            long long tmp = secondTillDeath;
            secondCD = tmp % 60;
            tmp -= tmp % 60;
            minuteCD = tmp % 3600 / 60;
            tmp = (tmp - tmp % 3600 / 60) / 3600;
            hourCD = tmp;

            tft.fillRectangle(95, 140, 139, 160, COLOR_BLACK);
            if (secondCD == 59)
            {
                tft.fillRectangle(50, 140, 80, 160, COLOR_BLACK);
                if (minuteCD == 59)
                    tft.fillRectangle(5, 140, 30, 160, COLOR_BLACK);
            }
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
    // Serial.println(String(changing) + " " + String(inChange) + " " + String(alarmPointer[changing]));
    // Serial.println(String(alarm[changing]));
    //  Button
    check_button();

    // Screen
    screen_display();

    // Write text
    write_text();
}