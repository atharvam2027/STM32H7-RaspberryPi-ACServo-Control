# STM32H7-RaspberryPi-ACServo-Control

Embedded C firmware for the STM32H7A3ZI-Q Nucleo board and Python-based Raspberry Pi control over UART for dual-axis AC servo motor motion using pulse-and-direction signals.

## Overview

This project establishes UART communication between a Raspberry Pi 5 and an STM32H7A3ZI-Q Nucleo board. The Raspberry Pi sends motion commands through a Python script, while the STM32 processes the commands and generates pulse and direction signals for two AC servo motor drivers.

## Hardware

- Raspberry Pi 5
- STM32H7A3ZI-Q Nucleo-144
- Two AC servo motors with compatible servo drivers
- UART connection between the Raspberry Pi and STM32

## Software and Tools

- Embedded C
- STM32CubeIDE
- STM32CubeMX
- STM32 HAL
- Python 3
- PySerial
- UART communication
- Timer-based pulse generation
- GPIO direction control

## System Architecture

Raspberry Pi 5 → UART → STM32H7A3ZI-Q → Pulse-and-Direction Signals → Servo Drivers → AC Servo Motors

## UART Configuration

| Parameter | Configuration |
|---|---|
| Baud rate | 115200 |
| Data format | 8-N-1 |
| Raspberry Pi serial port | `/dev/ttyAMA0` |
| Communication interface | UART |

### Connections

| Raspberry Pi 5 | STM32H7A3ZI-Q |
|---|---|
| GPIO14 / TXD | PA3 / USART2_RX |
| GPIO15 / RXD | PA2 / USART2_TX |
| GND | GND |

## Supported Commands

| Command | Description |
|---|---|
| `M1,angle,rpm` | Move motor 1 |
| `M2,angle,rpm` | Move motor 2 |
| `BOTH,angle,rpm` | Command both motors using the same angle and RPM |
| `BOTH,angle1,rpm1,angle2,rpm2` | Command each motor with separate angle and RPM values |

Example:

```text
BOTH,90,10,180,15
```

The current Python script validates commands before sending them to the STM32.

## STM32 Responses

- `OK` — Command acknowledged.
- `ERR` — Command rejected or error reported.
- `DONE` — Movement completion reported by the firmware.

## Raspberry Pi Setup

Install Python 3 and PySerial:

```bash
python3 -m pip install pyserial
```

Run the control script:

```bash
python3 motor_control.py
```

Ensure that the configured UART port is available and that the Raspberry Pi and STM32 share a common ground.

## Project Structure

```text
STM32H7-RaspberryPi-ACServo-Control/
├── STM32_Firmware/
│   ├── Core/
│   ├── Drivers/
│   ├── .settings/
│   ├── .project
│   ├── .cproject
│   ├── .mxproject
│   └── STM_RASP.ioc
├── RaspberryPi/
│   └── motor_control.py
├── .gitignore
└── README.md
```

## Current Scope

The current implementation focuses on UART-based command handling and dual-axis servo control. Coordinated motion timing and full four-axis robotic arm integration are separate development steps.

## Future Work

- Improve coordinated multi-axis motion
- Integrate motion control with inverse kinematics
- Connect high-level pick and place planning
- Expand the controller to the complete robotic arm

## Author

Developed as part of a 4-DOF robotic arm project focused on embedded systems, stm32 timers, motion control, and robotics.
