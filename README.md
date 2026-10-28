# 🍅 STM32F407 Pomodoro Timer with ILI9341 LCD

A **Pomodoro Timer** application built for the **STM32F407** microcontroller with a **2.4" ILI9341 TFT LCD** display and **LVGL** graphics library. This project demonstrates embedded GUI development, SPI communication, and real-time timer functionality.

<div align="center">
  <img src="https://github.com/dchithinh/stm32f407xx_pomodoro/blob/master/Doc/pomo_demo.png" alt="Project Demo" width="400">
</div>

---

## 📋 Table of Contents

- [🍅 STM32F407 Pomodoro Timer with ILI9341 LCD](#-stm32f407-pomodoro-timer-with-ili9341-lcd)
  - [📋 Table of Contents](#-table-of-contents)
  - [✨ Features](#-features)
  - [🎯 Project Overview](#-project-overview)
  - [🖥️ Try Before You Build - Windows Simulator](#️-try-before-you-build---windows-simulator)
  - [🛠️ Hardware Requirements](#️-hardware-requirements)
  - [🔌 Hardware Connections](#-hardware-connections)
    - [LCD Display Connections (SPI2)](#lcd-display-connections-spi2)
    - [Touch controller (XPT2046, SPI3, HAT)](#touch-controller-xpt2046-spi3-hat)
    - [Debug UART (USART2)](#debug-uart-usart2)
  - [⚙️ Software Requirements](#️-software-requirements)
  - [🚀 Getting Started](#-getting-started)
    - [Prerequisites](#prerequisites)
    - [Installation](#installation)
    - [Building the Project](#building-the-project)
    - [Flashing to MCU](#flashing-to-mcu)
  - [📖 Usage](#-usage)
    - [Debug Output](#debug-output)
  - [🏗️ Project Structure](#️-project-structure)
  - [🔧 Configuration](#-configuration)
    - [Clock Configuration](#clock-configuration)
    - [Display Configuration](#display-configuration)
    - [Debug Configuration](#debug-configuration)
  - [📚 Technical Notes](#-technical-notes)
    - [SPI Communication](#spi-communication)
    - [LVGL Integration](#lvgl-integration)
    - [Known Issues](#known-issues)
  - [🤝 Contributing](#-contributing)
    - [💡 Fun Fact](#-fun-fact)
    - [Contribution Guidelines](#contribution-guidelines)
  - [🙏 Acknowledgments](#-acknowledgments)
  - [📬 Contact](#-contact)

---

## ✨ Features

- 🍅 **Full Pomodoro Timer** functionality (25min work, 5min break cycles (Configurable) )
- 📱 **Touch-enabled GUI** with LVGL graphics library
- 🖥️ **2.4" ILI9341 LCD** display (240x320 resolution)
- 🔧 **Modular code structure** with separated concerns
- 🐛 **UART debugging** support with formatted logging
- ⚡ **Multiple clock configurations** (16MHz HSI, 84MHz, 168MHz)
- 🎨 **Beautiful UI** with progress indicators and animations
- 🔄 **Touch cursor debugging** for input validation
- 📊 **Statistics tracking** for completed sessions

---

## 🎯 Project Overview

This project was built for **learning purposes**, focusing on:
- **Embedded GUI development** with LVGL
- **SPI communication** with LCD displays
- **Real-time timer applications**
- **STM32 HAL library** usage and best practices
- **Modular embedded software architecture**

The Pomodoro Technique is a time management method that uses 25-minute work intervals followed by short breaks, helping improve focus and productivity.

---

## 🖥️ Try Before You Build - Windows Simulator

**Want to experience the Pomodoro app before building the hardware?**

Check out **[MicroPomo](https://github.com/dchithinh/MicroPomo)** - a Windows simulator that runs the exact same Pomodoro application with identical UI and functionality. This allows you to:

- 🎮 **Test the complete user interface** and timer functionality
- 🔍 **Explore all features** without any hardware setup
- 📱 **Experience the touch interactions** using mouse input
- ⚡ **Quick evaluation** before investing in STM32 hardware
- 🎯 **Perfect for demonstrations** and proof-of-concept

The simulator uses the same LVGL code base, providing an authentic preview of what you'll get on the actual STM32 hardware.

**[👉 Try MicroPomo Simulator Now](https://github.com/dchithinh/MicroPomo)**

---

## 🛠️ Hardware Requirements

| Component | Specification | Notes |
|-----------|--------------|--------|
| **Microcontroller** | STM32F407VGT6 | ARM Cortex-M4, 168MHz |
| **Display** | 2.4" ILI9341 LCD | 240x320, SPI interface |
| **Touch Controller** | XPT2046 | SPI touch controller |
| **Development Board** | STM32F407 Discovery | With **stm32f407_tft_2.4inch_hat** (2.4" ILI9341 + XPT2046) |
| **Power Supply** | 5V | For LCD backlight |
| **Debugger** | ST-Link/V2 | For programming and debugging |

---

## 🔌 Hardware Connections

### LCD Display Connections (SPI2)
| LCD Pin | STM32F407 Pin | Function | Notes |
|---------|---------------|----------|-------|
| **VCC** | 5V | Power Supply | External 5V required |
| **GND** | GND | Ground | Common ground |
| **CS** | PB9 | Chip Select | Active low |
| **RESX** | PD10 | Reset | Active low |
| **DCX** | PD9 | Data/Command | Data=1, Command=0 |
| **SDI/MOSI** | PB15 | SPI MOSI | SPI2 data out |
| **SDO/MISO** | PC2 | SPI MISO | Optional for reading |
| **SCK** | PB13 | SPI Clock | SPI2 clock |
| **LED** | 5V | Backlight | Always on |

Pin mapping matches the **stm32f407_tft_2.4inch_hat** schematic (`LCD_*` nets on SPI2). Discovery **P2** pads: SCK **38** (PB13), MOSI **40** (PB15), MISO **9** (PC2), RESET **41** (PD10), DC **42** (PD9); **P1** pad **19** (PB9) = `LCD_CS`.

### Touch controller (XPT2046, SPI3, HAT)

Resistive touch uses **SPI3** on the same bus as the optional microSD slot (`LCD_SD_*` nets on the HAT). Chip select and IRQ are separate GPIOs. Firmware defines these pins in `bsp/lcd/hw_def.h`.

| Touch signal | Module pin | STM32F407 | SPI / GPIO | HAT net | Discovery (P1) pad |
|--------------|------------|-----------|------------|---------|---------------------|
| **T_CLK** | T_CLK | **PC10** | SPI3_SCK | `LCD_SD_CLK` | 38 |
| **T_DO** | T_MISO | **PC11** | SPI3_MISO | `LCD_SD_MISO` | 37 |
| **T_DIN** | T_MOSI | **PC12** | SPI3_MOSI | `LCD_SD_MOSI` | 36 |
| **T_CS** | T_CS | **PE3** | GPIO (CS, active low) | `LCD_T_CS` | 15 |
| **T_IRQ** | T_IRQ | **PE5** | GPIO input (active low, pull-up) | `LCD_T_IRQ` | 12 |

**Not used for touch on this HAT:** Discovery SPI1 (PA5 / PA6 / PA7), PA8, or PA15 as in older wiring notes.

Touch calibration (min/max, invert) is in `bsp/lvgl/lv_drv_conf.h` (`XPT2046_*`).

### Debug UART (USART2)
| UART Pin | STM32F407 Pin | Function | Notes |
|----------|---------------|----------|-------|
| **TX** | PA2 | Debug output | USART2_TX (AF7) |
| **RX** | PA3 | Debug input | USART2_RX (AF7) |

---

## ⚙️ Software Requirements

- **STM32CubeIDE** Version: 1.17.0
- **STM32CubeMX** (for configuration)
- **STM32 HAL Library** (included)
- **LVGL v9.4.0** (included as submodule)
- **Git** (for cloning repository)

---

## 🚀 Getting Started

### Prerequisites

1. Install **STM32CubeIDE** from [STMicroelectronics website](https://www.st.com/en/development-tools/stm32cubeide.html)
2. Install **Git** for version control
3. Have your **STM32F407** board and **ILI9341 LCD** ready

### Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/dchithinh/stm32f407xx_pomodoro.git
   cd stm32f407xx_pomodoro
   ```

2. **Initialize LVGL submodule:**
   ```bash
   git submodule update --init --recursive
   ```

3. **Open in STM32CubeIDE:**
   - File → Import → Existing Projects into Workspace
   - Select the project folder

### Building the Project

1. **Configure build:**
   - Right-click project → Properties → C/C++ Build
   - Select your preferred configuration (Debug/Release)

2. **Build the project:**
   - Press `Ctrl+B` or use Project → Build Project

### Flashing to MCU

1. **Connect ST-Link debugger** to your STM32F407 board
2. **Flash the firmware:**
   - Right-click project → Run As → STM32 C/C++ Application
   - Or use the debug configuration for development

---

## 📖 Usage

1. **Power on** the system
2. **Touch the screen** to interact with the Pomodoro timer
3. **Start a session** by tapping the start button
4. **Monitor progress** through the circular progress indicator
5. **Take breaks** when prompted between work sessions
6. **View statistics** to track your productivity

### Debug Output

Connect a **USB-to-TTL adapter** to UART2 pins (PA2/PA3) to see debug output:
- **Baud rate:** 115200
- **Data bits:** 8
- **Stop bits:** 1
- **Parity:** None

---

## 🏗️ Project Structure

```
stm32f407xx_pomodoro/
├── 📁 Core/
│   ├── 📁 Inc/           # Header files
│   │   ├── main.h
│   │   ├── clock_config.h
│   │   └── debug_utils.h
│   └── 📁 Src/           # Source files
│       ├── main.c
│       ├── clock_config.c
│       └── debug_utils.c
├── 📁 bsp/              # Board Support Package
│   ├── 📁 lcd/          # LCD driver
│   └── 📁 lvgl/         # LVGL port
├── 📁 lvgl/             # LVGL library (submodule)
├── 📁 Drivers/          # STM32 HAL drivers
├── 📁 Doc/              # Documentation and images
└── 📄 README.md         # This file
```

---

## 🔧 Configuration

### Clock Configuration

The project supports multiple clock configurations in `clock_config.h`:

```c
// Uncomment one of the following:
// #define USE_HSI_16MHZ    1    // 16MHz HSI
// #define USE_HSI_84MHZ    1    // 84MHz PLL
// Default: 168MHz HSE PLL
```

### Display Configuration

Display settings are configured in `bsp/lcd/config.h`:
- **Resolution:** 240x320
- **Color depth:** 16-bit RGB565
- **Orientation:** Portrait/Landscape

### Debug Configuration

Debug features can be enabled/disabled in `debug_utils.h`:
- **UART logging:** Enable/disable debug output
- **Touch cursor:** Visual touch point indicator

---

## 📚 Technical Notes

### SPI Communication

**LCD (SPI2)** — settings in `bsp/lcd/lcd.c`:

- **Mode:** Full-duplex master
- **Clock polarity:** Low (CPOL=0)
- **Clock phase:** 1st edge (CPHA=0)
- **Data size:** 8-bit
- **MSB first**

**Touch (SPI3, XPT2046)** — settings in `bsp/lcd/tsc/XPT2046.c`:

- **Pins:** PC10 / PC11 / PC12 (AF6), CS **PE3**, IRQ **PE5** (see `bsp/lcd/hw_def.h`)
- **Mode:** Master, CPOL=0, CPHA=0, 8-bit, software CS on PE3
- **Shared bus:** Same SPI3 lines as the HAT microSD socket; only touch CS (`LCD_T_CS`) is driven for the Pomodoro UI

### LVGL Integration

- **Version:** LVGL v9.4.0
- **Color depth:** 16-bit (RGB565)
- **Memory:** Static allocation
- **Input:** Touch controller integration

### Known Issues

1. **SPI Read Operations:** Unable to read LCD ID/status registers reliably
2. **16-bit HAL SPI:** Issues with 16-bit transfers using HAL API
3. **Touch Input Lag:** On first touch at a new position, the system may respond to the previous touch location. A second touch is required to register the correct position.
4. **System Stability at 168MHz:** When using 168MHz system clock configuration, the system occasionally hangs or generates hard faults. Consider using 84MHz configuration for more stable operation.

---

## 🤝 Contributing

### 💡 Fun Fact

I'm an **embedded developer, not a UI/UX designer** - so the UI in this app may not be that great! 😅 Feel free to improve the user interface and make it more beautiful. The original purpose of this project was to learn **LVGL**, **STM32 HAL**, and **SPI communication** - and I'm always excited to see how the community can enhance it further!

---

Contributions are welcome! Please follow these steps:

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

### Contribution Guidelines

- Follow the existing code style
- Add comments for complex functions
- Test your changes thoroughly
- Update documentation as needed

---

## 🙏 Acknowledgments

- **[niekiran/EmbeddedGraphicsLVGL-MCU3](https://github.com/niekiran/EmbeddedGraphicsLVGL-MCU3)** - Original inspiration
- **[LVGL Team](https://lvgl.io/)** - Amazing graphics library
- **[STMicroelectronics](https://www.st.com/)** - STM32 ecosystem and HAL library
- **[MicroPomo](https://github.com/dchithinh/MicroPomo)** - Windows simulator for testing the Pomodoro app
- **Embedded systems community** - For knowledge sharing and support

---

## 📬 Contact

- **GitHub:** [@dchithinh](https://github.com/dchithinh)
- **Project Link:** [https://github.com/dchithinh/stm32f407xx_pomodoro](https://github.com/dchithinh/stm32f407xx_pomodoro)

---

⭐ **Star this repository** if you found it helpful!




