//Team Members: Cameron Bedard, Neha Das, Eric Piskarev

#include "mbed.h"
#include "TS_DISCO_F429ZI.h"
#include "LCD_DISCO_F429ZI.h"

#include "gyroscope.hpp"
// --- LCD and Touchscreen Initialization ---
LCD_DISCO_F429ZI lcd; //object to handle LCD display functionalities
TS_DISCO_F429ZI ts; //object to handle touchscreen functionalities

// ----- Arrays to store movement sequences -----
constexpr int SAMPLES = 30; //number of samples to collect for each gesture
double reference_array[SAMPLES][3] = {0}; // Array to store a movement sequence for reference
double recorded_array[SAMPLES][3] = {0};  // Array to store a recorded movement sequences

// ----- Function Prototypes -----
static void setup(); //sets up application environment
static void setup_screen(); //initializes the screen
static void display_xyz(float x_dps, float y_dps, float z_dps, int16_t x_raw, int16_t y_raw, int16_t z_raw); //displays gyroscope data on screen
static void draw_screen(); //ui elements on the LCD
static void display_touch(uint16_t x, uint16_t y); //displays touchscreen coordinates on the screen
pair<uint16_t, uint16_t> read_touchscreen(TS_StateTypeDef &TS_State); //reads touchscreen input and returns coordinates
static void calibrate_gyro(Gyroscope &gyro); //calibrates gyroscope
static void display_success_screen(); //displays "success" if unlock attempt is successful
static void display_wrong_gesture_screen(); //displays an error message if unlock attempt is unsuccessful
static double normalize(int16_t value); //normalizes gyroscope data to [-1,1]
static double calculate_similarity(double gesture1[SAMPLES][3], double gesture2[SAMPLES][3]); //calculates the similarity between two gestures
static void display_count(uint8_t count); //displays the current sample count during recording/unlocking
static void clearButtons(); //resets the button on the screen
static void update_status(const char *status); //updates status messages on the screen

// ----- Global Variables -----
bool is_recording = false; //boolean for if system is in recording mode
bool is_unlocking = false; //boolean for if system is in unlocking mode
uint8_t sampleCount = 0; // number of collected samples
bool alreadyRecorded = false; //boolean for if recording is already made
bool alreadyUnlocked = false; //tracks if unlock has been attempted
bool unlock_success = false; //tracks if unlock attempt was successful
bool unlock_fail = false; //tracks if unlock attempt was unsuccessful

int main()
{
    Gyroscope gyro; //gyroscope object
    if (!gyro.init()) //initialize gyro
    {
        printf("Gyroscope initialization failed\n");
        return -1;
    }
    calibrate_gyro(gyro);

    setup(); //initialize screen and interface
    if (gyro.is_calibrated()) //check if gyro is calibrated
    {
        lcd.SetFont(&Font8);
        lcd.DisplayStringAtLine(40, (uint8_t *)"GYROSCOPE CALIBRATED");
        printf("Gyroscope calibrated (xyz): %f, %f, %f\n", gyro.x_bias, gyro.y_bias, gyro.z_bias);
        lcd.SetFont(&Font20);
    }

    TS_StateTypeDef TS_State; //object to store touchscreen data

    while (1)
    {
        Gyroscope::GyroData data = gyro.read_gyro(); //read gyro data

        // Print data to Teleplot
        // printf(">x_axis(raw): %d|g\n", data.x_raw);
        // printf(">y_axis(raw): %d|g\n", data.y_raw);
        // printf(">z_axis(raw): %d|g\n", data.z_raw);

        // printf(">x_axis(dps): %.5f|dps\n", data.x_dps);
        // printf(">y_axis(dps): %.5f|dps\n", data.y_dps);
        // printf(">z_axis(dps): %.5f|dps\n", data.z_dps);

        // Display data on LCD
        display_xyz(data.x_dps, data.y_dps, data.z_dps, data.x_raw, data.y_raw, data.z_raw);

        // Collect data if recording or unlocking
        if (is_recording)
        {
            if (sampleCount < SAMPLES)
            {
                display_count(sampleCount); //show sample count

                recorded_array[sampleCount][0] = normalize(data.x_raw); //show normalized data in the recorded array
                recorded_array[sampleCount][1] = normalize(data.y_raw);
                recorded_array[sampleCount][2] = normalize(data.z_raw);
                sampleCount++;
            }
            else
            {
                is_recording = false; //stop recording
                sampleCount = 0;

                // clear sample count
                lcd.ClearStringLine(10);

                // display recording stored
                update_status("Recording stored");

                // reset button color
                clearButtons();
            }
        }
        if (is_unlocking) //handle unlocking mode
        {
            if (sampleCount < SAMPLES)
            {
                display_count(sampleCount); //show sample count
                reference_array[sampleCount][0] = normalize(data.x_raw); //show normalized data in the recorded array
                reference_array[sampleCount][1] = normalize(data.y_raw);
                reference_array[sampleCount][2] = normalize(data.z_raw);
                sampleCount++;
            }
            else
            {
                is_unlocking = false; //stop unlocking
                sampleCount = 0;

                // clear sample count
                lcd.ClearStringLine(10);

                // reset button color
                clearButtons();

                double similarity = calculate_similarity(recorded_array, reference_array); //compare gestures and decide if unlock attempt is successful or not
                printf("Similarity (Correlation): %.6f\n", similarity);

                if (similarity > 0.8)
                {
                    // Gestures match
                    printf("Successfully unlocked\n");
                    display_success_screen();
                    update_status("Successfully unlocked");
                }
                else
                {
                    // Gestures do not match
                    printf("Failed to unlock\n");
                    display_wrong_gesture_screen();
                    update_status("Failed to unlock");
                }
            }
        }
        // Handle touch input
        pair<uint16_t, uint16_t> touch = read_touchscreen(TS_State);
        if (touch.first != 0 && touch.second != 0)
        {
            display_touch(touch.first, touch.second); //show touchscreen coordinates
            if (touch.first > 130 && touch.first < 220 && touch.second > 10 && touch.second < 50)
            {
                is_recording = true; //start recording
                printf("Recording...\n");
                // change button color
                lcd.SetTextColor(LCD_COLOR_RED);
                lcd.FillRect(132, 252, 98, 48);
            }
            else if (touch.first > 20 && touch.first < 110 && touch.second > 10 && touch.second < 50)
            {
                is_unlocking = true;
                printf("Unlocking...\n");
                // change button color
                lcd.SetTextColor(LCD_COLOR_GREEN);
                lcd.FillRect(12, 252, 98, 48);
            }
        }
        thread_sleep_for(25); //delay
    }
}

void setup()
{
    // screen
    setup_screen();
    draw_screen();
}

void setup_screen()
{
    uint8_t status;

    BSP_LCD_SetFont(&Font20); //set font for display text

    lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"TOUCHSCREEN DEMO", CENTER_MODE); //display initialization message
    thread_sleep_for(1);

    status = ts.Init(lcd.GetXSize(), lcd.GetYSize()); //initialize touchscreen

    if (status != TS_OK) //check initialization status
    {
        lcd.Clear(LCD_COLOR_RED); //red for failure
        lcd.SetBackColor(LCD_COLOR_RED);
        lcd.SetTextColor(LCD_COLOR_WHITE);
        lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"TOUCHSCREEN INIT FAIL", CENTER_MODE);
    }
    else
    {
        lcd.Clear(LCD_COLOR_GREEN); //green for success
        lcd.SetBackColor(LCD_COLOR_GREEN);
        lcd.SetTextColor(LCD_COLOR_WHITE);
        lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"TOUCHSCREEN INIT OK", CENTER_MODE);
    }

    thread_sleep_for(1);
    lcd.Clear(LCD_COLOR_BLUE); //default blue background
    lcd.SetBackColor(LCD_COLOR_BLUE);
    lcd.SetTextColor(LCD_COLOR_WHITE);
}

pair<uint16_t, uint16_t> read_touchscreen(TS_StateTypeDef &TS_State)
{
    uint16_t x = 0;
    uint16_t y = 0;
    ts.GetState(&TS_State); //read touchscreen state
    if (TS_State.TouchDetected) //get touchscreen coordinates
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
    lcd.SetFont(&Font20);

    char x_dps_text[30];
    char y_dps_text[30];
    char z_dps_text[30];
    char x_raw_text[30];
    char y_raw_text[30];
    char z_raw_text[30];
//format gyro data as strings
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

    thread_sleep_for(100);
}

void draw_screen()
{
    // 1200x600
    // Draw Box for labels
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

void calibrate_gyro(Gyroscope &gyro)
{
    lcd.SetBackColor(LCD_COLOR_GREEN);
    lcd.DisplayStringAtLine(5, (uint8_t *)"GYROSCOPE CALIBRATING...");
    gyro.calibrate();
    lcd.ClearStringLine(LINE(5));
}

void display_success_screen()
{
    for (int i = 0; i < 25; i++) // Strobe 10 times
    {
        lcd.Clear(LCD_COLOR_GREEN); // Set background to green
        lcd.SetBackColor(LCD_COLOR_GREEN);
        lcd.SetTextColor(LCD_COLOR_WHITE);
        lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"SUCCESS", CENTER_MODE); // Display "SUCCESS"
        thread_sleep_for(80);                                               // Wait for 100ms

        lcd.Clear(LCD_COLOR_BLACK); // Set background to black
        lcd.SetBackColor(LCD_COLOR_BLACK);
        lcd.SetTextColor(LCD_COLOR_WHITE);
        lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"SUCCESS", CENTER_MODE); // Keep "SUCCESS"
        thread_sleep_for(100);                                              // Wait for 100ms
    }

    // Final display as static green success screen
    lcd.Clear(LCD_COLOR_GREEN);
    lcd.SetBackColor(LCD_COLOR_GREEN);
    lcd.SetTextColor(LCD_COLOR_WHITE);
    lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"SUCCESS", CENTER_MODE);

    setup_screen();
    draw_screen();
}

// Add the wrong gesture display
void display_wrong_gesture_screen()
{
    lcd.Clear(LCD_COLOR_RED); // Clear screen and make it red
    lcd.SetBackColor(LCD_COLOR_RED);
    lcd.SetTextColor(LCD_COLOR_WHITE);                                        // Text color for visibility
    lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"WRONG GESTURE", CENTER_MODE); // Display "WRONG GESTURE"

    thread_sleep_for(1000);
    setup_screen();
    draw_screen();
}

double normalize(int16_t value)
{
    return static_cast<double>(value) / 32768.0; // Normalized to range [-1, 1]
}

//This function determines similarity between recorded and unlocking gesture
double calculate_similarity(double gesture1[SAMPLES][3], double gesture2[SAMPLES][3])
{
    const double ENERGY_THRESHOLD = 10.0; // adjusted based on testing
    double energy1 = 0.0, energy2 = 0.0;

    double sum_gesture1[3] = {0.0}, sum_gesture2[3] = {0.0};

    double mse = 0.0;

    // Compute cumulative sums and energy for each gesture
    for (int i = 0; i < SAMPLES; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            // calculating energy of both signals
            energy1 += fabs(gesture1[i][j]); 
            energy2 += fabs(gesture2[i][j]);
            // calculate sum
            sum_gesture1[j] += gesture1[i][j];
            sum_gesture2[j] += gesture2[i][j];
            // Calculate MSE
            double diff = gesture1[i][j] - gesture2[i][j];
            mse += diff * diff; // sum of squared differences
        }
    }

    mse /= (SAMPLES * 3); //This holds the total mean squared error; Smaller error means more similarity

    printf("Energy1: %.6f, Energy2: %.6f\n", energy1, energy2);
    printf("Sum gesture1: %.6f, %.6f, %.6f\n", sum_gesture1[0], sum_gesture1[1], sum_gesture1[2]);
    printf("Sum gesture2: %.6f, %.6f, %.6f\n", sum_gesture2[0], sum_gesture2[1], sum_gesture2[2]);
    printf("MSE: %.6f\n", mse);

    // Energy Diff
    double energy_diff = fabs(energy1 - energy2);
    // Sign Diff
    int sign_diff = 0;
    for (int k = 0; k < 3; ++k)
    {   //This comparison ensures that sign is taken into account in determining similarity. 
        if ((sum_gesture1[k] >= 0) != (sum_gesture2[k] >= 0))
        {
            sign_diff++; //increment if signs don't match. Reduces overall similarity
        }
    }

    // weights given to each comparison mechanism in determining final similarity
    // MSE is weighed as 'most important' as it tracks how close the 2 signals are
    // Energy of the signal is considered in addition to MSE
    // Sign is considered minimally to ensure that mirrored gestured don't register as the same. 
    
    const double weight_mse = 0.6;
    const double weight_energy = 0.3;
    const double weight_sign = 0.1; 

    // normalizd Metrics
    double normalized_mse = 1.0 / (1.0 + mse); 
    double normalized_energy = 1.0 / (1.0 + energy_diff);
    double normalized_sign = (3 - sign_diff) / 3.0;

    // Compute Final Similarity
    double final_similarity = (weight_mse * normalized_mse) +
                              (weight_energy * normalized_energy) +
                              (weight_sign * normalized_sign);

    printf("normalized_mse: %.6f\n with weight: %.6f\n", normalized_mse, weight_mse);
    printf("normalized_energy: %.6f\n with weight: %.6f\n", normalized_energy, weight_energy);
    printf("normalized_sign: %.6f\n with weight: %.6f\n", normalized_sign, weight_sign);
    printf("Final Similarity: %.6f\n", final_similarity);

    return final_similarity;
}

void clearButtons()
{
    lcd.SetBackColor(LCD_COLOR_BLUE);
    lcd.SetTextColor(LCD_COLOR_BLUE);
    lcd.FillRect(132, 252, 98, 48);
    lcd.FillRect(12, 252, 98, 48);
    lcd.SetTextColor(LCD_COLOR_WHITE);
}

void display_count(uint8_t count)
{
    lcd.SetBackColor(LCD_COLOR_BLUE);
    lcd.SetTextColor(LCD_COLOR_WHITE);
    char countStr[10];
    sprintf(countStr, "%d", count);
    lcd.SetFont(&Font20);
    lcd.ClearStringLine(10);
    lcd.DisplayStringAtLine(10, (uint8_t *)countStr);
    lcd.SetFont(&Font16);
}

void update_status(const char *status)
{
    lcd.SetFont(&Font16);
    // char messageStr[10];
    // sprintf(messageStr, "%d", status);
    lcd.SetBackColor(LCD_COLOR_BLUE);
    lcd.SetTextColor(LCD_COLOR_WHITE);
    lcd.ClearStringLine(11);
    lcd.DisplayStringAtLine(11, (uint8_t *)status);
}
// #include "mbed.h"
