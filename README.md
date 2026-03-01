# Digital Safe using Finite State Machine (Dual Arduino Architecture)

## Project Overview

This project is a **Digital Safe System** implemented using a **Finite State Machine (FSM)** architecture.  
The system supports **three different authentication methods** and uses **two Arduino boards** to separate logic control from hardware output control.

The goal of this project is to demonstrate:

- Finite State Machine implementation
- Multi-input authentication system
- Inter-board communication
- Modular embedded system design

---

# System Architecture

## Dual Arduino Structure

| Board | Role |
|-------|------|
| **Board 1 (State Controller)** | Handles FSM logic and processes all inputs |
| **Board 2 (Output Controller)** | Controls LEDs, buzzer, and servo lock |

Board 1 communicates with Board 2 via Serial communication.

### Architecture Flow

```
User Input → Board 1 (FSM) → Serial Communication → Board 2 → Hardware Output
```

---

# Finite State Machine (FSM)

The safe operates using a structured state machine.

## States

- `IDLE`
- `SELECT_INPUT_MODE`
- `RECEIVE_INPUT`
- `VERIFY_PASSWORD`
- `ACCESS_GRANTED`
- `ACCESS_DENIED`

## State Flow

```
IDLE
  ↓
SELECT_INPUT_MODE
  ↓
RECEIVE_INPUT
  ↓
VERIFY_PASSWORD
  ↓
[Correct] → ACCESS_GRANTED
[Wrong]   → ACCESS_DENIED
```

FSM ensures:
- Clean logic structure
- Predictable transitions
- Easy debugging
- Scalable system design

---

# Authentication Methods

## 1️.Serial Monitor Password Input

- User types password in Serial Monitor.
- Input is read as string.
- Compared with stored password.

### Example Concept

```cpp
if (inputPassword == correctPassword) {
    currentState = ACCESS_GRANTED;
}
```

---

## 2️.Potentiometer Password Input

- Uses analog input (`analogRead()`).
- Maps analog value to digits (0–9).
- Button press confirms each digit.

### Concept

```cpp
int value = analogRead(A0);
int digit = map(value, 0, 1023, 0, 9);
```

This simulates a rotary combination lock.

---

## 3️.Morse Code Password (Binary Input)

- One push button is used.
- Short press = `0`
- Long press = `1`
- Press duration measured using `millis()`

### Concept

```cpp
if (pressDuration < threshold) {
    input += "0";
} else {
    input += "1";
}
```

Binary sequence is then validated as password.

---

# Inter-Board Communication

Board 1 sends command strings to Board 2:

- `"OPEN"` → Unlock safe
- `"ERROR"` → Wrong password
- `"LOCK"` → Lock state

### Example

```cpp
Serial.println("OPEN");
```

Board 2 listens:

```cpp
if (Serial.available()) {
    command = Serial.readString();
}
```

---

# Hardware Components

- 2x Arduino Boards
- Potentiometer
- Push Button
- Servo Motor
- LEDs (Red & Green)
- Buzzer
- Resistors
- Breadboard & Jumper Wires

---

# Output Behavior

## Access Granted
- Green LED ON
- Servo rotates to unlock
- Success buzzer tone

## Access Denied
- Red LED blink
- Error buzzer sound

## Locked State
- Servo returns to lock position
- LEDs reset

---

# Key Programming Concepts

- Finite State Machine (FSM)
- Serial Communication
- Analog-to-Digital Conversion
- Time-based Input Detection
- Modular System Design
- Embedded System Architecture

---

# How to Use

## 1️.Upload Code

- Upload `Digital_Safe_Board_State.ino` to Board 1
- Upload `Digital_Safe_Board_Output.ino` to Board 2

## 2️.Connect Boards

- TX ↔ RX
- Common GND

## 3️.Power On

Select input mode and enter password.

---

# Testing Checklist

- Test each authentication method independently
- Verify correct state transitions
- Test wrong password attempts
- Validate Morse timing threshold
- Confirm inter-board communication

---

# Known Issues

- Analog value may fluctuate (requires stable resistor)
- Morse timing threshold may need calibration
- Serial delay may cause minor latency
- Input decision logic may occasionally miscount timing

---

# Future Improvements

- Add LCD display
- Store password in EEPROM
- Add attempt counter with auto-lock
- Add keypad module
- Improve debounce logic
- Add encryption between boards

---

# Learning Outcomes

- Understanding Finite State Machine in embedded systems
- Designing modular hardware-software architecture
- Handling multiple input methods
- Implementing time-based input detection
- Managing dual-microcontroller communication

---

# Educational Purpose

This project is designed for embedded systems practice and FSM implementation study.

---

## Video of Robot Car Testing
https://youtu.be/FyMOOXe-VQQ

---

