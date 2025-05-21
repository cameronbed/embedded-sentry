# Embedded Sentry

**Embedded Sentry** is a gesture-based authentication system developed for the STM32F429ZI Discovery board. It enables users to record a unique hand gesture using the onboard gyroscope and touchscreen interface, which can later be used to unlock the system. This project showcases real-time sensor data processing, user interface design, and embedded system programming.

## Installation

To set up the project:

1. Clone the repository,
2. Import the project into PlatformIO on VSCode.
3. Connect the STM32F429ZI Discovery board to your computer via USB.
4. Build and flash the project to the board.

## Features

* **Gesture Recording**: Capture a sequence of 30 gyroscope samples to define a unique unlock gesture.
* **Gesture Authentication**: Compare a new gesture against the recorded one using a similarity algorithm.
* **Touchscreen Interface**: Intuitive UI with "Record" and "Unlock" buttons for user interaction.
* **Visual Feedback**: Real-time display of gyroscope data and authentication results on the LCD.
* **Data Normalization**: Gyroscope data is normalized to ensure consistency across different sessions.

## Technologies Used

* **Hardware**: STM32F429ZI Discovery board
* **Programming Language**: C++
* **Libraries**:

  * `mbed.h` for hardware abstraction
  * `TS_DISCO_F429ZI.h` and `LCD_DISCO_F429ZI.h` for touchscreen and LCD functionalities
* **Development Environment**: PlatformIO on VSCode

## 📄 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
