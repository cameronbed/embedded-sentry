#include "mbed.h"
#include "TS_DISCO_F429ZI.h"
#include "LCD_DISCO_F429ZI.h"

#include "gyroscope.hpp"
// --- LCD and Touchscreen Initialization ---
LCD_DISCO_F429ZI lcd;
TS_DISCO_F429ZI ts;

InterruptIn button(BUTTON1);

DigitalOut led(LED1);

Timer debounce_timer;

// ----- Arrays to store movement sequences -----
static int reference_array[3][50] = {0}; // Array to store a movement sequence for reference
static int recorded_array[3][50] = {0};  // Array to store a recorded movement sequences

// ----- Function Prototypes -----
static void setup();
static void setup_screen();
static void display_xyz(float x_dps, float y_dps, float z_dps, int16_t x_raw, int16_t y_raw, int16_t z_raw);
static void draw_buttons();
static void display_touch(uint16_t x, uint16_t y);
pair<uint16_t, uint16_t> read_touchscreen(TS_StateTypeDef &TS_State);
static void reset_screen();
static void record();
static void unlock();
static void toggle_led();

// --- SPI Transfer Callback Function ---
// Called automatically when an SPI transfer completes
int main()
{
    Gyroscope gyro;
    // setup();
    setup_screen();
    draw_buttons();
    TS_StateTypeDef TS_State;

    // Start the debounce timer
    debounce_timer.start();

    // Attach the toggle function to the button's rising edge
    button.rise(&toggle_led);
    // --- Continuous Gyroscope Data Reading ---
    if (!gyro.init())
    {
        printf("Gyroscope initialization failed\n");
        return -1;
    }

    while (1)
    {
        Gyroscope::GyroData data = gyro.read_gyro();

        printf("X(dps): %.5f, Y(dps): %.5f, Z(dps): %.5f\n", data.x_dps, data.y_dps, data.z_dps);

        printf("X(raw): %d, Y(raw): %d, Z(raw): %d\n", data.x_raw, data.y_raw, data.z_raw);

        display_xyz(data.x_dps, data.y_dps, data.z_dps, data.x_raw, data.y_raw, data.z_raw);

        pair<uint16_t, uint16_t> touch = read_touchscreen(TS_State);

        if (touch.first != 0 && touch.second != 0)
        {
            lcd.ClearStringLine(LINE(10));
            display_touch(touch.first, touch.second);
            if (touch.first > 10 && touch.first < 110 && touch.second > 250 && touch.second < 300) // Record button
            {
                record();
            }
            else if (touch.first > 130 && touch.first < 230 && touch.second > 250 && touch.second < 300) // Unlock button
            {
                unlock();
            }
            thread_sleep_for(300);
            lcd.ClearStringLine(LINE(5));
        }
        thread_sleep_for(300);
        printf(">x_axis(raw): %d|g\n", data.x_raw);
        printf(">y_axis(raw): %d|g\n", data.y_raw);
        printf(">z_axis(raw): %d|g\n", data.z_raw);
    }
}

void setup()
{
}

void setup_screen()
{
    uint8_t status;

    BSP_LCD_SetFont(&Font20);

    lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"TOUCHSCREEN", CENTER_MODE);
    lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"DEMO", CENTER_MODE);
    ThisThread::sleep_for(1s);

    status = ts.Init(lcd.GetXSize(), lcd.GetYSize());

    if (status != TS_OK)
    {
        lcd.Clear(LCD_COLOR_RED);
        lcd.SetBackColor(LCD_COLOR_RED);
        lcd.SetTextColor(LCD_COLOR_WHITE);
        lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"TOUCHSCREEN", CENTER_MODE);
        lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"INIT FAIL", CENTER_MODE);
    }
    else
    {
        lcd.Clear(LCD_COLOR_GREEN);
        lcd.SetBackColor(LCD_COLOR_GREEN);
        lcd.SetTextColor(LCD_COLOR_WHITE);
        lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"TOUCHSCREEN", CENTER_MODE);
        lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"INIT OK", CENTER_MODE);
    }

    ThisThread::sleep_for(1s);
    lcd.Clear(LCD_COLOR_BLUE);
    lcd.SetBackColor(LCD_COLOR_BLUE);
    lcd.SetTextColor(LCD_COLOR_WHITE);
}

pair<uint16_t, uint16_t> read_touchscreen(TS_StateTypeDef &TS_State)
{
    uint16_t x = 0;
    uint16_t y = 0;
    ts.GetState(&TS_State);
    if (TS_State.TouchDetected)
    {
        x = TS_State.X;
        y = TS_State.Y;
    }

    return pair<uint16_t, uint16_t>(x, y);
}

void display_touch(uint16_t x, uint16_t y)
{
    uint8_t text[30];
    sprintf((char *)text, "x=%d y=%d    ", x, y);
    lcd.ClearStringLine(LINE(9));
    lcd.DisplayStringAt(0, LINE(5), (uint8_t *)&text, CENTER_MODE);
}

void display_xyz(float x_dps, float y_dps, float z_dps, int16_t x_raw, int16_t y_raw, int16_t z_raw)
{
    char x_dps_text[30];
    char y_dps_text[30];
    char z_dps_text[30];
    char x_raw_text[30];
    char y_raw_text[30];
    char z_raw_text[30];

    sprintf(x_dps_text, "X(dps): %.2f", x_dps);
    sprintf(y_dps_text, "Y(dps): %.2f", y_dps);
    sprintf(z_dps_text, "Z(dps): %.2f", z_dps);

    sprintf(x_raw_text, "X(raw): %d", x_raw);
    sprintf(y_raw_text, "Y(raw): %d", y_raw);
    sprintf(z_raw_text, "Z(raw): %d", z_raw);

    lcd.SetTextColor(LCD_COLOR_BLUE);
    lcd.FillRect(0, 0, 180, 160);
    thread_sleep_for(10);
    lcd.SetTextColor(LCD_COLOR_WHITE);

    lcd.DisplayStringAt(0, 10, (uint8_t *)x_dps_text, LEFT_MODE);
    lcd.DisplayStringAt(0, 30, (uint8_t *)y_dps_text, LEFT_MODE);
    lcd.DisplayStringAt(0, 50, (uint8_t *)z_dps_text, LEFT_MODE);

    lcd.DisplayStringAt(0, 70, (uint8_t *)x_raw_text, LEFT_MODE);
    lcd.DisplayStringAt(0, 90, (uint8_t *)y_raw_text, LEFT_MODE);
    lcd.DisplayStringAt(0, 110, (uint8_t *)z_raw_text, LEFT_MODE);
}

void draw_buttons()
{
    // 1200x600
    // Draw Box for values
    lcd.DrawRect(2, 2, 180, 160);

    // Draw Buttons
    lcd.DrawRect(10, 250, 100, 50);
    lcd.DisplayStringAt(60, 220, (uint8_t *)"Record", CENTER_MODE);
    lcd.DrawRect(130, 250, 100, 50);
    lcd.DisplayStringAt(180, 220, (uint8_t *)"Unlock", CENTER_MODE);
}

void reset_screen()
{
}

void record()
{
    lcd.SetBackColor(LCD_COLOR_GREEN);
    lcd.DisplayStringAtLine(5, (uint8_t *)"Recording...");
}

void unlock()
{
    lcd.SetBackColor(LCD_COLOR_RED);
    lcd.DisplayStringAtLine(5, (uint8_t *)"Unlocking...");
}

void toggle_led()
{
    if (debounce_timer.read_ms() > 200) // Check if 200ms have passed since the last interrupt
    {
        led = !led;             // Toggle the LED
        debounce_timer.reset(); // Reset the debounce timer
    }
}