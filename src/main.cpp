#include "mbed.h"
#include "TS_DISCO_F429ZI.h"
#include "LCD_DISCO_F429ZI.h"

#include "gyroscope.hpp"
// --- LCD and Touchscreen Initialization ---
LCD_DISCO_F429ZI lcd;
TS_DISCO_F429ZI ts;

// ----- Arrays to store movement sequences -----
static int reference_array[3][50] = {0}; // Array to store a movement sequence for reference
static int recorded_array[3][50] = {0};  // Array to store a recorded movement sequences

// ----- Function Prototypes -----
static void setup();
static void setup_screen();
static void display_xyz(int16_t x, int16_t y, int16_t z);
static void draw_buttons();

// --- SPI Transfer Callback Function ---
// Called automatically when an SPI transfer completes
int main()
{
    Gyroscope gyro;
    setup();
    setup_screen();
    draw_buttons();
    // --- Continuous Gyroscope Data Reading ---
    if (!gyro.init())
    {
        printf("Gyroscope initialization failed\n");
        return -1;
    }

    while (1)
    {
        Gyroscope::GyroData data = gyro.read_gyro();

        printf("X(dps): %d, Y(dps): %d, Z(dps): %d\n", data.x_dps, data.y_dps, data.z_dps);
        printf("X(raw): %d, Y(raw): %d, Z(raw): %d\n", data.x_raw, data.y_raw, data.z_raw);

        display_xyz(data.x_raw, data.y_raw, data.z_raw);

        thread_sleep_for(300);
    }
}

void setup()
{
}

void setup_screen()
{
    TS_StateTypeDef TS_State;
    uint16_t x, y;
    uint8_t text[30];
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

    // while (1)
    // {

    //     ts.GetState(&TS_State);
    //     if (TS_State.TouchDetected)
    //     {
    //         x = TS_State.X;
    //         y = TS_State.Y;
    //         sprintf((char *)text, "x=%d y=%d    ", x, y);
    //         lcd.DisplayStringAt(0, LINE(0), (uint8_t *)&text, LEFT_MODE);
    //     }
    // }
}

void display_xyz(int16_t x, int16_t y, int16_t z)
{
    char x_text[6];
    char y_text[30];
    char z_text[30];
    sprintf(x_text, "X: %d", x);
    sprintf(y_text, "Y: %d", y);
    sprintf(z_text, "Z: %d", z);

    lcd.ClearStringLine(LINE(1));
    lcd.ClearStringLine(LINE(2));
    lcd.ClearStringLine(LINE(3));

    lcd.DisplayStringAt(0, LINE(1), (uint8_t *)x_text, LEFT_MODE);
    lcd.DisplayStringAt(0, LINE(2), (uint8_t *)y_text, LEFT_MODE);
    lcd.DisplayStringAt(0, LINE(3), (uint8_t *)z_text, LEFT_MODE);
}

void draw_buttons()
{
    int KeysTopY = 110;
    lcd.SetTextColor(LCD_COLOR_WHITE);
    for (int y = KeysTopY; y <= KeysTopY + 180; y = y + 60)
        for (int x = 30; x <= 210; x = x + 60)
            lcd.FillCircle(x, y, 25);
}
