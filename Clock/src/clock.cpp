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
unsigned long time, secondChange, minuteChange, hourChange, timeChange, changing;
unsigned long minuteChanging, secondChanging, hourChanging;
unsigned long DownButtonReady, UpButtonReady, LeftButtonReady, RightButtonReady, ChangeButtonReady;
String second, minute, hour, lastSecond;

// other var
boolean inChange = false, check = false;
boolean alarmOn = false;

void UP_PRESS()
{
    Serial.println("UP BUTTON PRESSES");
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
    Serial.println("DOWN BUTTON PRESSES");
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
    Serial.println("LEFT BUTTON PRESSES");
    changing = max(0, changing - 1);
    // tft.clear();
}

void RIGHT_PRESS()
{
    Serial.println("RIGHT BUTTON PRESSES");
    changing = min(2, changing + 1);
    // tft.clear();
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

String setTime(int a)
{
    String tmp = String(a);
    if (tmp.length() == 1)
        return '0' + tmp;
    return tmp;
}

void check_button()
{
    if (analogRead(CHANGE_BUTTON) <= 2 && analogRead(CHANGE_BUTTON) != 0)
    {
        if (analogRead(CHANGE_BUTTON) <= 2 && analogRead(CHANGE_BUTTON) != 0 && time - ChangeButtonReady >= 20)
            ChangeButtonReady = false, inChange = !inChange, Serial.println("CHANGE BUTTON PRESSES"), ChangeButtonReady = time;
    }
    else if (analogRead(DOWN_BUTTON) <= 2 && analogRead(DOWN_BUTTON) != 0)
    {
        if (analogRead(DOWN_BUTTON) <= 2 && analogRead(DOWN_BUTTON) != 0 && time - DownButtonReady >= 20)
            DownButtonReady = false, DOWN_PRESS(), DownButtonReady = time;
    }
    else if (analogRead(UP_BUTTON) <= 2 && analogRead(UP_BUTTON) != 0)
    {
        if (analogRead(UP_BUTTON) <= 2 && analogRead(UP_BUTTON) != 0 && UpButtonReady)
            UpButtonReady = false, UP_PRESS(), UpButtonReady = time;
    }
    else if (analogRead(RIGHT_BUTTON) <= 2 && analogRead(RIGHT_BUTTON) != 0)
    {
        if (analogRead(RIGHT_BUTTON) <= 2 && analogRead(RIGHT_BUTTON) != 0 && time - RightButtonReady >= 20)
            RightButtonReady = false, RIGHT_PRESS(), RightButtonReady = time;
    }
    else if (analogRead(LEFT_BUTTON) <= 2 && analogRead(LEFT_BUTTON) != 0)
    {
        if (analogRead(LEFT_BUTTON) <= 2 && analogRead(LEFT_BUTTON) != 0 && time - LeftButtonReady >= 20)
            LeftButtonReady = false, LEFT_PRESS(), LeftButtonReady = time;
    }
    else
        LeftButtonReady = RightButtonReady = ChangeButtonReady = UpButtonReady = DownButtonReady = time;
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
    tft.setFont(Terminal11x16);
    tft.drawText(65, 50, MONTH[int(now.Month()) - 1]);
    tft.drawText(23, 50, String(now.Day()));
    tft.drawText(115, 50, String(now.Year()));

    tft.setFont(Terminal11x16);
    tft.drawText(10, 100, "Detontating in: ", COLOR_WHITE);
    if (!inChange)
    {
        tft.setFont(Trebuchet_MS16x21);
        tft.drawText(15, 140, setTime(hourChange), COLOR_WHITE);
        tft.drawText(50, 140, ":", COLOR_WHITE);
        tft.drawText(60, 140, setTime(minuteChange), COLOR_WHITE);
        tft.drawText(95, 140, ":", COLOR_WHITE);
        tft.drawText(105, 140, setTime(secondChange), COLOR_WHITE);
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
        tft.drawText(140, 150, "SET", COLOR_WHITE);
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
    inChange = secondChange = minuteChange = hourChange = 0;
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