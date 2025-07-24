# BME280 SD Card Data Logger

A comprehensive environmental monitoring system using STM32 microcontroller that reads temperature, humidity, and pressure data from a BME280 sensor and logs it to an SD card in CSV format.

## Features

- Real-time environmental monitoring (Temperature, Humidity, Pressure)
- Data logging to SD card in CSV format with timestamps
- UART communication for system status and debugging
- Configurable measurement intervals
- Error handling and system status reporting
- Support for both 32-bit and 64-bit pressure calculations
- RTC support for real-time timestamps (optional)

## Hardware Requirements

- **Microcontroller**: STM32F4xx series (tested with STM32F401/F411)
- **Environmental Sensor**: BME280 (I2C interface)
- **Storage**: MicroSD card module (SPI interface)
- **Communication**: UART interface for debugging/monitoring
- **Power Supply**: 3.3V power supply
- **Optional**: RTC module for real-time timestamps

## Environment Setup

### Prerequisites

1. **STM32CubeIDE**: Version 1.8.0 or later
2. **STM32CubeMX**: For peripheral configuration (if modifications needed)
3. **ST-Link**: For programming and debugging
4. **USB to TTL Serial Converter**: For UART communication (optional)

### Software Setup

1. **Install STM32CubeIDE**
   - Download from [STMicroelectronics website](https://www.st.com/en/development-tools/stm32cubeide.html)
   - Install with default settings

2. **Clone/Download Project**
   ```bash
   git clone <your-repository-url>
   cd bme280-sd-logger
   ```

3. **Import Project into STM32CubeIDE**
   - Open STM32CubeIDE
   - File → Import → Existing Projects into Workspace
   - Select the project folder
   - Build the project (Ctrl+B)

### Required Libraries

The project uses the following libraries (included in the project):
- **HAL Driver**: STM32F4xx HAL library
- **FatFs**: For SD card file system operations
- **BME280 Driver**: Custom driver for BME280 sensor
- **Data Logging Module**: Custom logging functionality

## Hardware Connections

### Pin Configuration

#### BME280 Sensor (I2C1)
| BME280 Pin | STM32 Pin | Function |
|------------|-----------|----------|
| VCC        | 3.3V      | Power Supply |
| GND        | GND       | Ground |
| SCL        | PB8       | I2C1_SCL |
| SDA        | PB9       | I2C1_SDA |
| SDO        | GND       | I2C Address Select (0x76) |
| CSB        | 3.3V      | I2C Mode Select |

**Note**: BME280 address is configured as 0x76 (7-bit) / 0xEC (8-bit with SDO grounded)

#### SD Card Module (SPI1)
| SD Card Pin | STM32 Pin | Function |
|-------------|-----------|----------|
| VCC         | 3.3V      | Power Supply |
| GND         | GND       | Ground |
| CS          | PC4       | Chip Select |
| SCK         | PA5       | SPI1_SCK |
| MOSI        | PA7       | SPI1_MOSI |
| MISO        | PA6       | SPI1_MISO |

#### UART Communication (USART1)
| Function | STM32 Pin | External Connection |
|----------|-----------|-------------------|
| TX       | PA9       | RX of USB-TTL converter |
| RX       | PA10      | TX of USB-TTL converter |
| GND      | GND       | GND of USB-TTL converter |

**UART Settings**: 115200 baud, 8N1

#### Power Supply
| Pin | Connection |
|-----|------------|
| 3.3V | All module VCC pins |
| GND  | All module GND pins |
| 5V   | Not used (all modules are 3.3V) |

### Wiring Diagram

```
STM32F4xx Development Board
                                    
         3.3V ●─────┬─────────────┬──────── BME280 VCC
              │     │             │
              │     └──── SD Card VCC
              │     
         PA5  ●─────────────────────────── SD Card SCK
         PA6  ●─────────────────────────── SD Card MISO  
         PA7  ●─────────────────────────── SD Card MOSI
         PC4  ●─────────────────────────── SD Card CS
         
         PB8  ●─────────────────────────── BME280 SCL
         PB9  ●─────────────────────────── BME280 SDA
         
         PA9  ●─────────────────────────── UART TX (to external RX)
         PA10 ●─────────────────────────── UART RX (from external TX)
         
         GND  ●─────┬─────────────┬──────── BME280 GND
                    │             │
                    └──── SD Card GND
```

### Hardware Notes

1. **Pull-up Resistors**: I2C lines (SCL, SDA) require 4.7kΩ pull-up resistors to 3.3V (usually built into development boards)

2. **SD Card Compatibility**: Use high-quality SD cards (Class 10 recommended) formatted as FAT32

3. **Power Consumption**: Total system consumption approximately 50-100mA depending on measurement frequency

4. **Voltage Levels**: All connections are 3.3V compatible - do not connect 5V signals

## Configuration

### Measurement Settings

The BME280 sensor can be configured in `main.c`:

```c
BME280_Config(OSRS_2,      // Temperature oversampling
              OSRS_16,     // Pressure oversampling  
              OSRS_1,      // Humidity oversampling
              MODE_NORMAL, // Measurement mode
              T_SB_0p5,    // Standby time
              IIR_16);     // IIR filter coefficient
```

### Data Logging Interval

Modify the logging interval in `log_sensor_data.h`:

```c
#define LOG_INTERVAL_MS 5000  // 5 seconds between measurements
```

### File Output Format

Data is saved to `sensor_data.csv` with the following format:
```csv
Timestamp,Temperature(C),Humidity(%),Pressure(hPa)
1,23.45,65.23,1013.25
2,23.47,65.20,1013.30
```

## Usage

1. **Hardware Setup**: Connect all components according to the wiring diagram
2. **Power On**: Connect STM32 to power source
3. **Monitor via UART**: Connect UART to computer at 115200 baud
4. **Insert SD Card**: System will automatically create log file
5. **Data Collection**: System starts logging immediately after initialization

### UART Output Example
```
=== BME280 SD Card Data Logger ===
Initializing system...
BME280 sensor initialized
SD CARD mounted successfully...
SD CARD Total Size: 1906 KB
SD CARD Free Space: 1902 KB
New data file created with header
System ready. Starting data logging...
Data logged to SD card successfully
```

## Troubleshooting

### Common Issues

**SD Card Mount Failed**
- Check SPI connections
- Verify SD card format (FAT32)
- Try different SD card

**BME280 Not Detected**
- Verify I2C connections
- Check pull-up resistors on SCL/SDA
- Confirm sensor address (0x76/0x77)

**No UART Output**
- Check baud rate (115200)
- Verify TX/RX connections
- Ensure ground connection

**File Write Errors**
- Check SD card write protection
- Verify available space
- Try reformatting SD card

## License

This project is provided under the MIT License. See LICENSE file for details.

## Contributing

1. Fork the repository
2. Create feature branch
3. Commit changes
4. Push to branch
5. Create Pull Request

## Support

For questions and support, please create an issue in the project repository.