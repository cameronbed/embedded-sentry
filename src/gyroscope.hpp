#ifndef GYROSCOPE_HPP
#define GYROSCOPE_HPP

#include "mbed.h"

class Gyroscope
{
private:
    static const uint8_t CTRL_REG1 = 0x20; // ODR and bandwidth settings
    static const uint8_t CTRL_REG2 = 0x21; // High-pass filter settings
    static const uint8_t CTRL_REG4 = 0x23;
    static const uint8_t CTRL_REG5 = 0x24; // High-pass filter enable

    static const uint8_t CTRL_REG1_ODR = 0b0100; // ODR = 200 Hz, Cut-off = 12

    static const uint8_t CTRL_REG1_CONFIG = (CTRL_REG1_ODR << 4) | 0b1111;

    static const uint8_t CTRL_REG2_HPM1 = 0b00;   // High-pass filter normal mode
    static const uint8_t CTRL_REG2_HPCF = 0b0101; // High-pass filter cut-off. ODR = 200Hz, Cut-off = .5

    static const uint8_t CTRL_REG2_CONFIG = (00 | CTRL_REG2_HPM1 << 4) | CTRL_REG2_HPCF;

    static const uint8_t CTRL_REG4_FS = 0b00; // resolution // +/- 245 dps. Must change sensitivity
    static const uint8_t CTRL_REG4_CONFIG = (0 | CTRL_REG4_FS << 3) | 0b0000;

    static const uint8_t CTRL_REG5_CONFIG = 0b00010000; // Enable high-pass filter

    static constexpr float SENSITIVITY = 8.75f / 1000.0f;

    static const uint8_t OUT_X_L = 0x28;

    static const int SPI_FLAG = 1;

public:
    static float x_bias, y_bias, z_bias;

    bool calibrated = false;

    void spi_cb(int event);

    bool init();

    void set_register(SPI &spi, uint8_t reg, uint8_t value);

    void calibrate(); // Change return type to void

    bool is_calibrated();
    struct GyroData
    {
        float x_dps, y_dps, z_dps;
        int16_t x_raw, y_raw, z_raw;
    };

    GyroData read_gyro();
};

#endif // GYROSCOPE_H