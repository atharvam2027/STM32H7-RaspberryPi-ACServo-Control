# Dual-Axis AC Servo Motor Control using STM32H7A3ZI-Q Nucleo-144 Board

**Embedded C | STM32H7A3ZI-Q | STM32CubeIDE | STM32 HAL | TIMERS | UART | PWM | Raspberry Pi 5**

An embedded motion-control project integrating an **STM32H7A3ZI-Q Nucleo-144** with a **Raspberry Pi 5** to control two AC servo motor axes through UART commands and pulse and direction signals.

The STM32 firmware handles real-time pulse generation, direction control, UART command processing, and motion-completion reporting. A Python application running on the Raspberry Pi sends angle- and RPM-based motion commands to the STM32 over UART.

This project focuses on practical embedded firmware development, peripheral configuration, timer-based motor control, serial communication, and hardware–software integration for robotics.

## Key Features

- **Embedded C firmware** developed using STM32CubeIDE and STM32 HAL.
- **Dual-axis pulse generation** using TIM2 and TIM3 PWM channels.
- **UART communication** between Raspberry Pi 5 and STM32 at 115200 baud.
- **Angle and RPM based motion commands** through a custom UART protocol.
- **GPIO-based direction control** for both motor axes.
- **Interrupt-based pulse counting** to determine when the commanded pulse count has been reached.
- **STM32CubeMX configuration** maintained in an `.ioc` project file.
- **Python-based control interface** using PySerial on Raspberry Pi OS/Ubuntu Linux.
- **Modular project structure** separating STM32 firmware from the Raspberry Pi application.

## System Architecture

```text
          Raspberry Pi 5
        Python Control Script
                 |
                 | UART
                 | 115200 baud
                 v
       STM32H7A3ZI-Q Nucleo
          Embedded C / HAL
                 |
          +------+------+
          |             |
      TIM2 / GPIO   TIM3 / GPIO
          |             |
     Pulse + Dir    Pulse + Dir
          |             |
      Servo Driver  Servo Driver
          |             |
      AC Servo 1    AC Servo 2
```

The Raspberry Pi handles high-level command input, while the STM32 generates the pulse-and-direction signals used by the servo drivers.

## Hardware

- Raspberry Pi 5
- STM32H7A3ZI-Q Nucleo-144 development board
- Two AC servo motors with compatible servo drivers
- UART connection between the Raspberry Pi and STM32
- Appropriate power supplies and wiring for the controller and servo drivers

## Software and Tools

| Tool / Technology | Purpose |
|---|---|
| Embedded C | STM32 firmware development |
| STM32CubeIDE | Firmware development, build, and debugging |
| STM32CubeMX | Peripheral and clock configuration |
| STM32 HAL | Hardware abstraction and peripheral control |
| TIM2 and TIM3 | Timer-based pulse generation |
| PWM | Pulse signal generation |
| GPIO | Motor direction control |
| USART2 / UART | Raspberry Pi–STM32 communication |
| Python 3 | High-level motor command interface |
| PySerial | Serial communication from Raspberry Pi |
| Linux | Raspberry Pi development environment |
| Git and GitHub | Version control and project documentation |

## STM32 Firmware Implementation

The STM32H7A3ZI-Q Nucleo board is responsible for executing motor motion commands received from the Raspberry Pi.

### STM32CubeMX Configuration

The STM32 project is configured using STM32CubeMX, with the configuration saved in:

`STM32_Firmware/STM_RASP.ioc`

The firmware uses STM32 HAL APIs and is developed in STM32CubeIDE.

### Peripheral and Pin Configuration

| Peripheral | Pin | Configuration / Function |
|---|---|---|
| TIM2 Channel 1 | PA0 | Motor 1 pulse output |
| GPIO | PA1 | Motor 1 direction output |
| TIM3 Channel 1 | PA6 | Motor 2 pulse output |
| GPIO | PA7 | Motor 2 direction output |
| USART2 TX | PA2 | STM32 UART transmit |
| USART2 RX | PA3 | STM32 UART receive |

### Timer Configuration

The following are the initial timer settings configured during peripheral initialization.

| Parameter | TIM2 | TIM3 |
|---|---:|---:|
| Prescaler (PSC) | 0 | 3 |
| Initial auto-reload register (ARR) | 685 | 41999 |
| PWM channel | Channel 1 | Channel 1 |
| Output pin | PA0 | PA6 |
| Intended axis | Motor 1 | Motor 2 |

The firmware updates the timer auto-reload and compare registers according to the requested RPM. Therefore, the ARR values can change at runtime during motion.

The timer clock used in the pulse-frequency calculation must match the actual timer input clock configured for the STM32 clock tree.

### Motion-Control Workflow

1. The STM32 receives a command through USART2.
2. The firmware parses the command and validates its parameters.
3. The requested angle and RPM are used to calculate the target pulse count and pulse frequency.
4. The appropriate GPIO output sets the motor direction.
5. The corresponding timer generates PWM pulses for the servo driver.
6. Timer update interrupts count generated timer periods.
7. When the target pulse count is reached, the firmware stops pulse generation.
8. The firmware reports motion completion through UART.

**Position-control limitation:** Pulse counting tracks generated timer events. It does not measure the actual shaft position using encoder feedback. Missed pulses, mechanical slip, or a motor failing to follow its command are not independently detected by this pulse-counting mechanism.

## UART Communication

The Raspberry Pi communicates with the STM32 through USART2.

### UART Configuration

| Parameter | Configuration |
|---|---|
| Baud rate | 115200 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None |
| Data format | 8-N-1 |
| Raspberry Pi serial device | `/dev/ttyAMA0` |
| STM32 peripheral | USART2 |

### Hardware Connections

| Raspberry Pi 5 | STM32H7A3ZI-Q |
|---|---|
| GPIO14 / TXD, physical pin 8 | PA3 / USART2_RX |
| GPIO15 / RXD, physical pin 10 | PA2 / USART2_TX |
| GND | GND |

**Important:** UART transmit and receive lines are crossed: Raspberry Pi TX connects to STM32 RX, and Raspberry Pi RX connects to STM32 TX. Both boards must share a common ground. Verify the selected UART device and board pin mapping before powering the system.

## Supported UART Commands

The Python application sends comma-separated text commands to the STM32.

| Command | Description |
|---|---|
| `M1,angle,rpm` | Command motor 1 |
| `M2,angle,rpm` | Command motor 2 |
| `BOTH,angle,rpm` | Command both motors with the same angle and RPM |
| `BOTH,angle1,rpm1,angle2,rpm2` | Command each motor with separate angles and RPM values |

### Example Command

```text
BOTH,90,10,180,15
```

This command requests:

- Motor 1: 90° at 10 RPM
- Motor 2: 180° at 15 RPM

The command format is interpreted by the firmware's command parser. The Python application also validates commands before transmission.

### STM32 Responses

| Response | Meaning |
|---|---|
| `OK` | Command accepted |
| `ERR` | Command rejected or an error reported |
| `DONE` | Firmware reports completion of the commanded movement |

These responses indicate the firmware's reported command status. `DONE` does not independently verify the actual mechanical position of a motor.

## Raspberry Pi Setup

### Requirements

- Raspberry Pi 5
- Python 3
- PySerial
- UART access configured for the selected serial device
- Connection to the STM32 UART pins

### Install PySerial

```bash
python3 -m pip install pyserial
```

If your repository contains a `requirements.txt` file, you can alternatively install its dependencies with:

```bash
python3 -m pip install -r requirements.txt
```

### Run the Control Script

From the `RaspberryPi` directory, run:

```bash
python3 motor_control.py
```

Ensure that `/dev/ttyAMA0` is the correct serial device for your Raspberry Pi configuration. The script's configured port must match the UART interface physically connected to the STM32.

## Project Structure

```text
STM32H7-RaspberryPi-ACServo-Control/
├── STM32_Firmware/
│   ├── Core/
│   │   ├── Inc/
│   │   └── Src/
│   ├── Drivers/
│   ├── .settings/
│   ├── .project
│   ├── .cproject
│   ├── .mxproject
│   ├── STM_RASP.ioc
│   ├── STM32H7A3ZITXQ_FLASH.ld
│   └── STM32H7A3ZITXQ_RAM.ld
├── RaspberryPi/
│   ├── motor_control.py
│   └── requirements.txt
├── .gitignore
└── README.md
```

The STM32 firmware folder contains the embedded project, HAL-related source structure, linker scripts, and STM32CubeMX configuration. The Raspberry Pi folder contains the Python control application and its dependencies.

## Current Scope and Limitations

The current implementation focuses on UART-based command handling and pulse-based control of two AC servo motor axes.

- Motor motion is commanded using angle and RPM parameters.
- Pulse generation and completion tracking are implemented using STM32 timers and interrupts.
- Actual motor position is not verified through encoder feedback in the current pulse-counting workflow.
- The two motor axes are started sequentially by the current dual-motor implementation; synchronized start timing is not guaranteed.
- Full four-axis robotic arm integration, inverse kinematics, and coordinated trajectory execution are outside the current implementation scope.

These limitations define the current development stage and provide a clear path for future improvements.

## Future Work

- Implement coordinated multi-axis motion and synchronized start timing.
- Improve motion planning and trajectory generation.
- Integrate inverse kinematics for the four axis robotic arm.
- Connect high level pick and place planning to the motor control interface.
- Expand the firmware and communication protocol to support the complete robotic arm.
- Investigate encoder feedback and position verification for improved motion reliability.

## Application

This project forms part of a 4-DOF robotic arm development effort focused on embedded motion control and robotic pick and place applications. The current two axis controller provides a foundation for expanding the system to the complete arm.

## Author

**Atharva More**
