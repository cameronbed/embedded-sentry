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
constexpr int SAMPLES = 50;
static int reference_array[3][SAMPLES] = {0}; // Array to store a movement sequence for reference
static int recorded_array[3][SAMPLES] = {0};  // Array to store a recorded movement sequences

// ----- Function Prototypes -----
static void setup();
static void setup_screen();
static void display_xyz(float x_dps, float y_dps, float z_dps, int16_t x_raw, int16_t y_raw, int16_t z_raw);
static void draw_screen();
static void display_touch(uint16_t x, uint16_t y);
pair<uint16_t, uint16_t> read_touchscreen(TS_StateTypeDef &TS_State);
static void record();
static void unlock();
static void toggle_led();
static void calibrate_gyro(Gyroscope &gyro);
void compare_gesture();

// --- SPI Transfer Callback Function ---
// Called automatically when an SPI transfer completes

// Add global flags and index
bool is_recording = false;
bool is_unlocking = false;
int sample_index = 0;

int main()
{
    Gyroscope gyro;
    if (!gyro.init())
    {
        printf("Gyroscope initialization failed\n");
        return -1;
    }
    calibrate_gyro(gyro);

    setup();
    if (gyro.is_calibrated())
    {
        lcd.SetFont(&Font8);
        lcd.DisplayStringAtLine(40, (uint8_t *)"GYROSCOPE CALIBRATED");
        lcd.SetFont(&Font20);
    }

    TS_StateTypeDef TS_State;

    while (1)
    {
        Gyroscope::GyroData data = gyro.read_gyro();

        // Print data to Teleplot
        printf(">x_axis(raw): %d|g\n", data.x_raw);
        printf(">y_axis(raw): %d|g\n", data.y_raw);
        printf(">z_axis(raw): %d|g\n", data.z_raw);
        printf(">x_axis(dps): %.5f|dps\n", data.x_dps);
        printf(">y_axis(dps): %.5f|dps\n", data.y_dps);
        printf(">z_axis(dps): %.5f|dps\n", data.z_dps);

        // Display data on LCD
        display_xyz(data.x_dps, data.y_dps, data.z_dps, data.x_raw, data.y_raw, data.z_raw);

        if (is_recording || is_unlocking)
        {
            // Collect data if recording
            if (is_recording)
            {
                recorded_array[0][sample_index] = data.x_raw;
                recorded_array[1][sample_index] = data.y_raw;
                recorded_array[2][sample_index] = data.z_raw;
                sample_index++;
                if (sample_index >= SAMPLES)
                {
                    is_recording = false;
                    sample_index = 0;
                    lcd.SetFont(&Font20);
                    lcd.SetTextColor(LCD_COLOR_BLUE);
                    lcd.FillRect(132, 252, 98, 48);
                    lcd.SetTextColor(LCD_COLOR_WHITE);
                    lcd.SetBackColor(LCD_COLOR_BLUE);
                }
            }

            // Collect data if unlocking
            if (is_unlocking)
            {
                reference_array[0][sample_index] = data.x_raw;
                reference_array[1][sample_index] = data.y_raw;
                reference_array[2][sample_index] = data.z_raw;
                sample_index++;
                if (sample_index >= SAMPLES)
                {
                    is_unlocking = false;
                    sample_index = 0;
                    compare_gesture();
                    lcd.SetFont(&Font20);
                    lcd.SetTextColor(LCD_COLOR_BLUE);
                    lcd.FillRect(12, 252, 98, 48);
                    lcd.SetTextColor(LCD_COLOR_WHITE);
                    lcd.SetBackColor(LCD_COLOR_BLUE);
                }
            }
        }
        else
        {
            // Handle touch input
            pair<uint16_t, uint16_t> touch = read_touchscreen(TS_State);
            if (touch.first != 0 && touch.second != 0)
            {
                display_touch(touch.first, touch.second);
                if (touch.first > 130 && touch.first < 220 && touch.second > 10 && touch.second < 50)
                {
                    printf("Recording...\n");
                    record();
                }
                else if (touch.first > 20 && touch.first < 110 && touch.second > 10 && touch.second < 50)
                {
                    printf("Unlocking...\n");
                    unlock();
                }
            }
        }
        thread_sleep_for(100);
    }
}

void setup()
{
    // button interrupt
    button.rise(&toggle_led);
    debounce_timer.start();
    // screen
    setup_screen();
    draw_screen();
}

void record()
{
    lcd.SetFont(&Font16);
    lcd.SetTextColor(LCD_COLOR_RED);
    lcd.FillRect(132, 252, 98, 48);
    lcd.SetBackColor(LCD_COLOR_RED);

    is_recording = true;
    sample_index = 0;
}

void unlock()
{
    lcd.SetFont(&Font16);
    lcd.SetTextColor(LCD_COLOR_GREEN);
    lcd.FillRect(12, 252, 98, 48);
    lcd.SetBackColor(LCD_COLOR_GREEN);

    is_unlocking = true;
    sample_index = 0;
}

void compare_gesture()
{
    lcd.SetTextColor(LCD_COLOR_WHITE);
    lcd.SetBackColor(LCD_COLOR_BLUE);
    if (reference_array[0][0] == 0 || reference_array[1][0] == 0 || reference_array[2][0] == 0)
    {
        printf("No gesture recorded. Unlock failed.\n");
        lcd.DisplayStringAtLine(12, (uint8_t *)"No Recording. Unlock Failed");
        thread_sleep_for(2000);
        lcd.ClearStringLine(12);
        return;
    }
    else if (recorded_array[0][0] == 0 || recorded_array[1][0] == 0 || recorded_array[2][0] == 0)
    {
        printf("No gesture recorded. Unlock failed.\n");
        lcd.DisplayStringAtLine(12, (uint8_t *)"No Recording. Unlock Failed");
        thread_sleep_for(2000);
        lcd.ClearStringLine(12);
        return;
    }
    const float threshold = 20000.0f; // Adjust this value based on testing
    float total_difference = 0.0f;

    for (int i = 0; i < SAMPLES; ++i)
    {
        // Calculate differences for each axis
        float x_diff = recorded_array[0][i] - reference_array[0][i];
        float y_diff = recorded_array[1][i] - reference_array[1][i];
        float z_diff = recorded_array[2][i] - reference_array[2][i];

        // Accumulate sum of squared differences
        total_difference += x_diff * x_diff + y_diff * y_diff + z_diff * z_diff;
    }

    lcd.SetBackColor(LCD_COLOR_BLUE);
    lcd.SetTextColor(LCD_COLOR_WHITE);
    if (total_difference < threshold)
    {
        printf("Gesture matched! Unlock successful.\n");
        lcd.DisplayStringAtLine(12, (uint8_t *)"Unlocked!");
    }
    else
    {
        printf("Gesture did not match. Unlock failed.\n");
        lcd.DisplayStringAtLine(12, (uint8_t *)"Unlock Failed");
    }
    thread_sleep_for(2000);
    lcd.ClearStringLine(12);
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
    lcd.SetFont(&Font16);
    uint8_t text[30];
    sprintf((char *)text, "x=%d y=%d    ", x, y);
    lcd.ClearStringLine(9);
    lcd.DisplayStringAtLine(9, (uint8_t *)&text);
    lcd.SetFont(&Font20);
}

void display_xyz(float x_dps, float y_dps, float z_dps, int16_t x_raw, int16_t y_raw, int16_t z_raw)
{
    char x_dps_text[30];
    char y_dps_text[30];
    char z_dps_text[30];
    char x_raw_text[30];
    char y_raw_text[30];
    char z_raw_text[30];

    sprintf(x_dps_text, "%.2f", x_dps);
    sprintf(y_dps_text, "%.2f", y_dps);
    sprintf(z_dps_text, "%.2f", z_dps);

    sprintf(x_raw_text, "%d", x_raw);
    sprintf(y_raw_text, "%d", y_raw);
    sprintf(z_raw_text, "%d", z_raw);

    lcd.SetTextColor(LCD_COLOR_BLUE);
    lcd.FillRect(97, 4, 135, 133);
    thread_sleep_for(10);
    lcd.SetTextColor(LCD_COLOR_WHITE);

    lcd.DisplayStringAt(235, 10, (uint8_t *)x_dps_text, RIGHT_MODE);
    lcd.DisplayStringAt(235, 30, (uint8_t *)y_dps_text, RIGHT_MODE);
    lcd.DisplayStringAt(235, 50, (uint8_t *)z_dps_text, RIGHT_MODE);

    lcd.DisplayStringAt(230, 70, (uint8_t *)x_raw_text, RIGHT_MODE);
    lcd.DisplayStringAt(230, 90, (uint8_t *)y_raw_text, RIGHT_MODE);
    lcd.DisplayStringAt(230, 110, (uint8_t *)z_raw_text, RIGHT_MODE);
}

void draw_screen()
{
    // 1200x600
    // Draw Box for labels
    //          x, y, height, width
    lcd.DrawRect(2, 2, 90, 135);
    // Draw box for values
    lcd.DrawRect(95, 2, 140, 135);

    // Draw Buttons
    lcd.DrawRect(10, 250, 100, 50);
    lcd.DisplayStringAt(60, 220, (uint8_t *)"Record", CENTER_MODE);
    lcd.DrawRect(130, 250, 100, 50);
    lcd.DisplayStringAt(180, 220, (uint8_t *)"Unlock", CENTER_MODE);

    // Draw Text
    lcd.DisplayStringAt(5, 10, (uint8_t *)"X.dps:", LEFT_MODE);
    lcd.DisplayStringAt(5, 30, (uint8_t *)"Y.dps:", LEFT_MODE);
    lcd.DisplayStringAt(5, 50, (uint8_t *)"Z.dps:", LEFT_MODE);

    lcd.DisplayStringAt(5, 70, (uint8_t *)"X.raw:", LEFT_MODE);
    lcd.DisplayStringAt(5, 90, (uint8_t *)"Y.raw:", LEFT_MODE);
    lcd.DisplayStringAt(5, 110, (uint8_t *)"Z.raw:", LEFT_MODE);
}

void toggle_led()
{
    if (debounce_timer.read_ms() > 200) // Check if 200ms have passed since the last interrupt
    {
        led = !led;             // Toggle the LED
        debounce_timer.reset(); // Reset the debounce timer
    }
}

void calibrate_gyro(Gyroscope &gyro)
{
    lcd.SetBackColor(LCD_COLOR_GREEN);
    lcd.DisplayStringAtLine(5, (uint8_t *)"GYROSCOPE CALIBRATING...");
    gyro.calibrate();
    lcd.ClearStringLine(LINE(5));
}

// #include "mbed.h"