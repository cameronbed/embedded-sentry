#include "gyroscope.hpp"

EventFlags flags;

SPI spi(PF_9, PF_8, PF_7, PC_1, use_gpio_ssel);

float Gyroscope::x_bias = 0.0f;
float Gyroscope::y_bias = 0.0f;
float Gyroscope::z_bias = 0.0f;

bool Gyroscope::init()
{
    // SPI:
    // - 8-bit data size
    // - Mode 3 (CPOL = 1, CPHA = 1): idle clock high, data sampled on falling edge
    spi.format(8, 3);

    // Set SPI communication frequency to 1 MHz
    spi.frequency(1'000'000);

    set_register(spi, CTRL_REG1, CTRL_REG1_CONFIG);
    set_register(spi, CTRL_REG2, CTRL_REG2_CONFIG);
    set_register(spi, CTRL_REG4, CTRL_REG4_CONFIG);
    set_register(spi, CTRL_REG5, CTRL_REG5_CONFIG);

    return true;
}

void Gyroscope::spi_cb(int event)
{
    flags.set(SPI_FLAG); // Set the SPI_FLAG to signal that transfer is complete
}

void Gyroscope::set_register(SPI &spi, uint8_t reg, uint8_t value)
{
    static uint8_t write_buf[32];
    static uint8_t read_buf[32];

    write_buf[0] = reg;
    write_buf[1] = value;
    spi.transfer(write_buf, 2, read_buf, 2, callback(this, &Gyroscope::spi_cb)); // Initiate SPI transfer
    flags.wait_all(SPI_FLAG);                                                    // Wait until the transfer completes
}

Gyroscope::GyroData Gyroscope::read_gyro()
{
    static uint8_t write_buf[32];
    static uint8_t read_buf[32];
    GyroData data;

    // Prepare to read gyroscope output starting at OUT_X_L
    // - write_buf[0]: register address with read (0x80) and auto-increment (0x40) bits set
    write_buf[0] = OUT_X_L | 0x80 | 0x40; // Read mode + auto-increment

    // Perform SPI transfer to read 6 bytes (X, Y, Z axis data)
    // - write_buf[1:6] contains dummy data for clocking
    // - read_buf[1:6] will store received data
    spi.transfer(write_buf, 7, read_buf, 7, callback(this, &Gyroscope::spi_cb));
    flags.wait_all(SPI_FLAG); // Wait until the transfer completes

    // --- Extract and Convert Raw Data ---
    // Combine high and low bytes for X-axis
    data.x_raw = (int16_t)(((read_buf[2]) << 8) | read_buf[1]);

    // Combine high and low bytes for Y-axis
    data.y_raw = (int16_t)(((read_buf[4]) << 8) | read_buf[3]);

    // Combine high and low bytes for Z-axis
    data.z_raw = (int16_t)(((read_buf[6]) << 8) | read_buf[5]);

    if (calibrated)
    {
        data.x_raw -= x_bias;
        data.y_raw -= y_bias;
        data.z_raw -= z_bias;
    }

    data.x_dps = (float)data.x_raw * SENSITIVITY;
    data.y_dps = (float)data.y_raw * SENSITIVITY;
    data.z_dps = (float)data.z_raw * SENSITIVITY;

    printf("X(dps): %.5f, Y(dps): %.5f, Z(dps): %.5f\n", data.x_dps, data.y_dps, data.z_dps);

    return data;
}

void Gyroscope::calibrate()
{
    const int numSamples = 200;
    int16_t xReadings[numSamples], yReadings[numSamples], zReadings[numSamples];
    float xSum = 0.0, ySum = 0.0, zSum = 0.0;

    // Collect samples
    for (int i = 0; i < numSamples; ++i)
    {
        xReadings[i] = read_gyro().x_raw;
        yReadings[i] = read_gyro().y_raw;
        zReadings[i] = read_gyro().z_raw;

        xSum += xReadings[i];
        ySum += yReadings[i];
        zSum += zReadings[i];
        thread_sleep_for(1);
    }

    Gyroscope::x_bias = xSum / numSamples;
    Gyroscope::y_bias = ySum / numSamples;
    Gyroscope::z_bias = zSum / numSamples;

    calibrated = true;
}

bool Gyroscope::is_calibrated()
{
    return calibrated;
}