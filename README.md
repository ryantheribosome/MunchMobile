# Munch Mobile (ECEN 1400)

Munch Mobile is a tabletop robotic food delivery system designed to improve hygiene, accessibility, and efficiency in buffet-style dining. The robot uses an ESP32-C3 SuperMini, a TB6612FNG motor driver, a QTR reflectance sensor, and ESP-NOW communication. A separate Hopper ESP32 operates the gear mechanism that dispenses the bowl.

## Overview

1. The **Input ESP32** sends ESP-NOW commands to both the Robot ESP32 and the Hopper ESP32.
2. The **Robot ESP32-C3** receives the movement command, begins its driving cycle, checks its QTR sensor to avoid edges, and relays the last received command forward if needed.
3. The **Hopper ESP32** receives its own command from the Input ESP32 and rotates the hopper gears to dispense a bowl at the correct moment.

## Features

* Input ESP32 controlling two independent devices
* Hopper ESP32 rotates gears on command
* Autonomous tabletop robot motion
* Dual-motor control using TB6612FNG
* QTR reflectance sensor for edge detection
* ESP-NOW peer-to-peer communication
* Lightweight 3D-printed chassis

## Hardware Used

* ESP32-C3 SuperMini (Robot)
* ESP32-C3 SuperMini (Input Controller)
* ESP32 Dev Module or ESP32-C3 (Hopper)
* TB6612FNG motor driver (Robot)
* QTR-1A reflectance sensor (Robot)
* TT gear motors (Robot)
* Servo or DC motor (Hopper gears)
* 7.4V battery or USB power
* Custom 3D-printed frame

## Included Code

* Robot movement receiver firmware
* Hopper gear-control receiver firmware
* Input ESP32 command transmitter firmware
* Motor control functions
* QTR stopping logic
* Calibration tools
  * Wheel revolution timing
  * PWM tests
  * QTR threshold testing

## Pin Configuration

### Robot (ESP32-C3 → TB6612FNG)

* PWMA → 4
* AIN1 → 5
* AIN2 → 6
* PWMB → 7
* BIN1 → 8
* BIN2 → 9
* STBY → 10

### Robot QTR Sensor

* OUT → 3

### Hopper ESP32

* Gear motor output pin → (varies by wiring; commonly GPIO 2, 4, or 5)
* Uses simple rotate-on-command logic

### Input ESP32

* No motors or sensors
* Sends ESP-NOW packets to both MAC addresses

## Calibration Details

* Default PWM speed: 200
* One wheel rotation at PWM 50: approx. 2353 ms
* Wheel diameter: 6.5 cm
* QTR threshold: 400

