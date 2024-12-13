#ifndef GYROSCOPE_HPP
#define GYROSCOPE_HPP

#include "mbed.h"

class Gyroscope
{
private:
    static const uint8_t CTRL_REG1 = 0x20;
    static const uint8_t CTRL_REG1_CONFIG = 0b01101111;
    static const uint8_t CTRL_REG4 = 0x23;
    static const uint8_t CTRL_REG4_CONFIG = 0b00000000;

    static constexpr float SENSITIVITY = 8.75f / 1000.0f;

    static const uint8_t OUT_X_L = 0x28;

    static const int SPI_FLAG = 1;

public:
    void spi_cb(int event);

    bool init();

    struct GyroData
    {
        float x_dps, y_dps, z_dps;
        int16_t x_raw, y_raw, z_raw;
    };

    GyroData read_gyro();
};

#endif // GYROSCOPE_H