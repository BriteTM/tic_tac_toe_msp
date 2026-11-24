# MSP430 Tic-Tac-Toe

![Language](https://img.shields.io/badge/Language-C-blue?style=flat-square)
![MCU](https://img.shields.io/badge/MCU-MSP430FR6989-red?style=flat-square)
![IDE](https://img.shields.io/badge/IDE-Code%20Composer%20Studio-lightgrey?style=flat-square)

A bare-metal Tic-Tac-Toe game written in C for the **TI MSP430FR6989 LaunchPad** and **Educational BoosterPack MKII**. Supports Player vs. Player (PvP), Player vs. Computer (PvE) with a heuristic AI, and two independent games running simultaneously with mid-game board switching.

## Demo

[![MSP430 Tic-Tac-Toe Demo](https://img.youtube.com/vi/BpVjIdnURR4/hqdefault.jpg)](https://youtube.com/shorts/BpVjIdnURR4)

## Features

* **PvP & PvE Modes:** Per-board mode selection menu remembered independently across board switches.
* **Dual Simultaneous Games:** Run two separate matches at once and swap between them mid-game without losing state or cursor position.
* **Heuristic AI:** Evaluates moves in priority order—immediate win, block opponent's win, center square, corners, then remaining edges.
* **Visual & Audio Feedback:**
  * Blinking tile cursor that automatically skips occupied squares.
  * Onboard LEDs indicate the active board (**Red** = Board 1, **Green** = Board 2).
  * Passive buzzer driven via 2 kHz, 50% duty-cycle PWM (`Timer_B`) for win, invalid-move, and reset feedback.
  * Winning strikethrough line and dismissible win/draw banner so players can inspect the finished board before resetting.

## Controls

| Input | Action |
| :--- | :--- |
| **Joystick** | Navigate menu options / move cursor between open squares |
| **S1 (P1.1)** | Select menu mode / place `X` or `O` / dismiss banner & reset finished board |
| **S2 (P1.2) — Short Press** | Switch between Board 1 and Board 2 |
| **S2 (P1.2) — Hold (~0.8s)** | Reset the active board and return to the mode selection menu |

## Hardware & Peripherals

* **Microcontroller:** TI MSP430FR6989 (DCO configured to 16 MHz `MCLK` / `SMCLK`)
* **Display:** Crystalfontz 128×128 Pixel LCD (TI GrLib)

| Peripheral / Feature | Pin | Configuration |
| :--- | :--- | :--- |
| **Joystick X-Axis** | `P9.2` (`A10`) | 12-bit `ADC12` sequence-of-channels |
| **Joystick Y-Axis** | `P8.7` (`A4`) | 12-bit `ADC12` sequence-of-channels |
| **Passive Buzzer** | `P2.7` | `TB0.6` PWM output (2 kHz, 50% duty cycle) |
| **Cursor Blink / Hold Timer** | Internal | `TA0` in Up Mode on ACLK (`CCR0 = 32767`) |
| **Board 1 / Board 2 LEDs** | `P1.0` / `P9.7` | Digital GPIO output |
| **Buttons S1 / S2** | `P1.1` / `P1.2` | Digital GPIO input with internal pull-ups |

## Code Architecture

Game state is encapsulated in two nested structs (`square` and `board`). Each `board` tracks its 9-tile array, active turn, saved cursor position, selected gamemode, and win/banner state. All gameplay, rendering, and AI routines operate through a single `board *activeBoard` pointer—switching games simply saves the current cursor, reassigns the pointer to `&Board1` or `&Board2`, and calls `redraw_board()`.

## Project Structure

```text
.
├── main.c              # Core game logic, peripheral init, AI, and UI rendering
├── Grlib/              # TI Graphics Library
├── LcdDriver/          # Crystalfontz 128x128 display driver
└── targetConfigs/      # MSP430FR6989 CCS target configuration
