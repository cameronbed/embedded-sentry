#include "gyroscope.hpp"

EventFlags flags;

SPI spi(PF_9, PF_8, PF_7, PC_1, use_gpio_ssel);

bool Gyroscope::init()
{
    static uint8_t write_buf[32];
    static uint8_t read_buf[32];

    // SPI:
    // - 8-bit data size
    // - Mode 3 (CPOL = 1, CPHA = 1): idle clock high, data sampled on falling edge
    spi.format(8, 3);

    // Set SPI communication frequency to 1 MHz
    spi.frequency(1'000'000);

    write_buf[0] = CTRL_REG1;
    write_buf[1] = CTRL_REG1_CONFIG;
    spi.transfer(write_buf, 2, read_buf, 2, callback(this, &Gyroscope::spi_cb)); // Initiate SPI transfer
    flags.wait_all(SPI_FLAG);                                                    // Wait until the transfer completes

    // Configure Control Register 4 (CTRL_REG4)
    // - write_buf[0]: address of the register to write (CTRL_REG4)
    // - write_buf[1]: configuration value to set sensitivity and high-resolution mode
    write_buf[0] = CTRL_REG4;
    write_buf[1] = CTRL_REG4_CONFIG;
    spi.transfer(write_buf, 2, read_buf, 2, callback(this, &Gyroscope::spi_cb)); // Initiate SPI transfer
    flags.wait_all(SPI_FLAG);

    return true;
}

void Gyroscope::spi_cb(int event)
{
    flags.set(SPI_FLAG); // Set the SPI_FLAG to signal that transfer is complete
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

    data.x_dps = (float)data.x_raw * SENSITIVITY;
    data.y_dps = (float)data.y_raw * SENSITIVITY;
    data.z_dps = (float)data.z_raw * SENSITIVITY;

    printf("X(dps): %.5f, Y(dps): %.5f, Z(dps): %.5f\n", data.x_dps, data.y_dps, data.z_dps);

    return data;
}