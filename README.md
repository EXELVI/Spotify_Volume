# 🎧 Spotify Volume Control

[![Arduino](https://img.shields.io/badge/Arduino-IDE-00979D?style=for-the-badge&logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![Repository](https://img.shields.io/github/languages/code-size/EXELVI/Spotify_Volume?style=for-the-badge)](https://github.com/EXELVI/Spotify_Volume)
[![Last Commit](https://img.shields.io/github/last-commit/EXELVI/Spotify_Volume?style=for-the-badge)](https://github.com/EXELVI/Spotify_Volume)

## Overview

This project is an Arduino-based Spotify volume controller with a sleek LCD display and LED matrix. It allows you to adjust the volume of your Spotify playback using a rotary encoder, and displays the current volume level in real-time on both the LCD and the LED matrix. 

## Features

- 🌐 **WiFi Connectivity**: Connects to the Spotify API via WiFi.
- 🔄 **Rotary Encoder**: Adjusts the volume with tactile feedback.
- 📊 **LCD Display**: Shows the volume percentage and device name.
- 💡 **LED Matrix**: Visual representation of volume adjustments.

## Hardware Requirements

- Arduino UNO R4 WIFI
- Rotary encoder
- 20x4 I2C LCD Display

## Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software)
- Arduino Libraries:
  - `ArduinoGraphics`
  - `Arduino_LED_Matrix`
  - `LiquidCrystal_I2C`
  - `WiFiS3`
  - `WiFiUdp`
  - `Arduino_JSON`

## Getting Started

1. Clone this repository:
    ```bash
    git clone https://github.com/EXELVI/Spotify_Volume.git
    ```
2. Open the `Spotify_Volume.ino` file in the Arduino IDE.
3. Install the required libraries via the Arduino Library Manager.
4. Enter your WiFi credentials and Spotify API credentials in the `arduino_secrets.h` file.
5. Upload the sketch to your Arduino board.

## Usage

- After uploading the sketch, the device will connect to your WiFi network.
- Access the provided URL on the LCD display to authorize your device with Spotify.
- Once authorized, use the rotary encoder to adjust the volume. The changes will reflect on the LED matrix and LCD display.

## How It Works

- **WiFi Connection**: The device connects to the Spotify API using stored credentials.
- **Volume Control**: The rotary encoder's position corresponds to the Spotify volume level (0-100%).
- **Display Feedback**: Both the LED matrix and LCD display provide visual feedback of the current volume level.

## Contributing

Feel free to fork this repository, make your changes, and submit a pull request. Contributions are welcome! 
