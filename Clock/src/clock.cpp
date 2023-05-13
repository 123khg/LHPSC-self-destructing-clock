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
#define RIGHT_BUTTON 2
#define LEFT_BUTTON 3
#define UP_BUTTON 12
#define DOWN_BUTTON 5
#define CHANGE_BUTTON 1

#endif

#define TFT_BRIGHTNESS 200
TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_CLK, TFT_BRIGHTNESS);
ThreeWire myWire(7, 6, 4);
RtcDS1302<ThreeWire> Rtc(myWire);
unsigned long time, secondChange, minuteChange, hourChange, timeChange, timeChangePress, changing;
unsigned long realTimeX, realTimeY, alarmX, alarmY, monthX, monthY;
String second, minute, hour, lastMinute, MONTH[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
boolean inChange = false, check = false, DOWN = true, UP = true, LEFT = true, RIGHT = true, CHANGEE = true;
RtcDateTime now;

void UP_PRESS()
{
    if (inChange)
    {
        if (changing == 0)
            hourChange = min(24, hourChange + 1);
        else if (changing == 1)
            minuteChange = min(60, minuteChange + 1);
        else
            secondChange = min(60, secondChange + 1);
    }
}

void DOWN_PRESS()
{
    if (inChange)
    {
        if (changing == 0)
            hourChange = max(0, hourChange - 1);
        else if (changing == 1)
            minuteChange = max(0, minuteChange - 1);
        else
            secondChange = max(0, secondChange - 1);
    }
}

void LEFT_PRESS()
{
    changing = max(0, changing - 1);
    tft.clear();
}

void RIGHT_PRESS()
{
    changing = min(2, changing + 1);
    tft.clear();
}

void GetDate()
{
    second = String(now.Second());
    minute = String(now.Minute());
    hour = String(now.Hour());
    if (String(now.Hour()).length() == 1)
        hour = '0' + hour;
    if (String(now.Minute()).length() == 1)
        minute = '0' + minute;
    if (String(now.Second()).length() == 1)
        second = '0' + second;
}

String setTime(String a, int n)
{
    if (n == changing)
        return "--";
    if (a.length() == 1)
        return '0' + a;
    return a;
}

void check_button()
{
    if (digitalRead(CHANGE_BUTTON) == HIGH && CHANGEE)
        CHANGEE = false, inChange = !inChange;
    else
        CHANGEE = true;

    if (digitalRead(DOWN_BUTTON) == HIGH && DOWN)
        DOWN = false, DOWN_PRESS();
    else
        DOWN = true;

    if (digitalRead(UP_BUTTON) == HIGH && UP)
        UP = false, UP_PRESS();
    else
        UP = true;

    if (digitalRead(RIGHT_BUTTON) == HIGH && RIGHT)
        RIGHT = false, RIGHT_PRESS();
    else
        RIGHT = true;

    if (digitalRead(LEFT_BUTTON) == HIGH && LEFT)
        LEFT = false, LEFT_PRESS();
    else
        LEFT = true;
}

void screen_display()
{
    tft.setBackgroundColor(COLOR_BLACK);
    tft.drawLine(0, 88, 220, 88, COLOR_WHITE);
}

void write_text()
{
    if (inChange == false)
    {
        tft.setFont(Trebuchet_MS16x21);
        tft.drawText(realTimeX, realTimeY, hour + ":" + minute + ":" + second, COLOR_RED);
        tft.setFont(Terminal11x16);
        tft.drawText(monthX, monthY, MONTH[int(now.Month()) - 1]);
    }
    else
    {
        if (time - timeChange >= 1000)
        {
            tft.drawText(realTimeX, realTimeY, setTime(String(hourChange), 0) + ":" + setTime(String(minuteChange), 1) + ":" + setTime(String(secondChange), 2), COLOR_RED);
        }
        if (time - timeChange >= 1500)
            timeChange = time;
        else
            tft.drawText(realTimeX, realTimeY, setTime(String(hourChange), 5) + ":" + setTime(String(minuteChange), 5) + ":" + setTime(String(secondChange), 5), COLOR_RED);
    }
}

void update_time()
{
    time = millis();
    now = Rtc.GetDateTime();
    GetDate();
    if (minute != lastMinute && !inChange)
    {
        lastMinute = minute;
    }
}

void setup()
{
    tft.begin();
    tft.clear();
    tft.setOrientation(3);
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    Rtc.Begin();
    RtcDateTime currentTime = RtcDateTime(__DATE__, __TIME__);
    Rtc.SetDateTime(currentTime);
    GetDate();
    lastMinute = minute;
    secondChange = minuteChange = hourChange = 0;
    monthX = 160;
    monthY = 10;
    realTimeX = 10;
    realTimeY = 10;
    alarmX = 10;
    alarmY = 124;
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
    // screen_display();

    // Write text
    tft.clear();
    write_text();
}