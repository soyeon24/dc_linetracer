# dc_linetracer — STM32H533 line tracer

Firmware for a 16-channel line-following robot, built at **ZETIN**, the robotics club of the
University of Seoul. Bare-metal C on STM32 LL drivers, no RTOS.

This is the middle of three line tracer boards I built, and the first one to use **brushed DC
motors with quadrature encoders** after the stepper-driven STM32F411 version
([`step_linetracer_f411`](https://github.com/soyeon24/step_linetracer_f411)). Closed-loop motor
control, the windowed position estimator and the marker state machine were all worked out here and
carried over to the final board, [`dc-tracer-743`](https://github.com/soyeon24/dc-tracer-743)
(STM32H743), which added the recorded second run and cross-marker recovery.

**Status: archived.** Developed January–May 2025, stopped at a working first run. This board never
raced; see [Known limitations](#known-limitations) for what was left unfinished.

---

## Hardware

| | |
|---|---|
| MCU | STM32H533RET6 — Cortex-M33 @ 250 MHz |
| Line sensor | 16 IR channels, two banks of 8 behind 8:1 analog multiplexers |
| Motors | 2 × brushed DC, quadrature encoders at 2048 counts / motor revolution |
| Drivetrain | 63:17 gearbox, 40.5 mm wheels |
| Motor model | R = 0.68 Ω, Kₑ = 0.0146 V·s/rad (used for the voltage command, see below) |
| Driver | dual H-bridge, PWM pair + enable lines per side |
| Display | SSD1331 96×64 OLED over SPI2 |
| Input | two push buttons (short / long press decoded in firmware) |
| Battery sense | 21:1 divider into ADC2 |

## Timer map

Everything is interrupt-driven; `main()` only runs the menu.

| Timer | Rate | Job |
|---|---|---|
| TIM1 | 1 MHz | µs / ms busy-wait delay |
| TIM6 | 31.25 kHz | sensor ISR — one mux step per interrupt, so a full 16-channel frame lands at **≈3.9 kHz** |
| TIM2 / TIM5 | — | 32-bit quadrature encoder counters (×2 decoding on TI1), left / right |
| TIM3 / TIM4 | 50 kHz | motor PWM **and** the per-wheel control ISR (20 µs), left / right |
| TIM7 | 2 kHz | velocity profile and steering mix (500 µs) |

## How it works

### 1. Sensing — `Sensor_TIM6_IRQ()`

Each interrupt drives one 3-bit mux address on PC0–PC2, strobes PC3, throws away one ADC
conversion so the phototransistor can settle, then reads **both banks from a single 3-rank ADC1
sequence**: rank 2 is the left bank on PA7, rank 3 the right bank on PC5. 12-bit results are
shifted down to 8-bit. Eight interrupts cover all 16 channels.

Each channel is normalized against its own white/black calibration maxima, then thresholded
(default 100) into a 16-bit `sensorState` — bit 15 is the leftmost sensor, bit 0 the rightmost.
Battery voltage is sampled on ADC2 in the same ISR.

### 2. Position — same ISR

Sensors sit at fixed lateral coordinates from −30000 to +30000 in steps of 4000. Position is the
intensity-weighted mean, but taken **only over a ±3-sensor window centred on the previous
estimate**. That window is the central idea of the whole firmware: anything lit outside it is, by
construction, not the line — which is what makes marker detection possible without a separate
marker sensor.

### 3. Steering and velocity profile — `Drive_TIM7_IRQ()`

A trapezoidal profile ramps the current velocity toward the target (defaults: accel 4.5, decel
8.0, target 1.0 m/s). Centre speed is then cut by a curvature proxy, and the two wheels are split
around it:

```
v_centre = v · K / (K + |pos|)          K = curve_decel, default 19000
v_right  = v_centre · (1 − r · pos)     r = curve_rate,  default 6.8e-5
v_left   = v_centre · (1 + r · pos)
```

So one gain sets steering strength and one sets how hard the robot slows into a curve — the two
knobs that actually get tuned at a competition, both reachable from the on-board settings menu.

### 4. Motor control — `Motor_L_TIM3_IRQ()` / `Motor_R_TIM4_IRQ()`

Per wheel, at 50 kHz, PD on the encoder's integrated distance *and* angular velocity against the
commanded ones:

```
Cur  = −(Kp · ΔD + Kd · ΔV)             Kp = 1024, Kd = 0
Volt =  Cur · R + ω · Kₑ                 ← back-EMF feed-forward
duty =  ARR · |Volt| / V_batt            ← live battery compensation
```

The current command is converted to a voltage through the motor's electrical model rather than
being fed to the H-bridge directly, and the duty is divided by the *measured* pack voltage, so the
same command produces the same torque on a full and a half-empty battery. The sign of `Volt` picks
the H-bridge direction.

### 5. Marker state machine — `State_Machine()` in `drive.c`

A three-state FSM (IDLE → MARK → DECISION) accumulates marker frames. Each frame's `sensorState`
is **shifted by the current position index before being OR-ed** into a 32-bit accumulator, so a
marker read while the robot is drifting sideways still lands on consistent bits. When the marker
clears, the accumulator is classified:

| Accumulator pattern | Verdict |
|---|---|
| bits 8–22 all set | cross |
| far-left **and** far-right set | end marker |
| far-left only | left marker |
| far-right only | right marker |

`Drive_First()` counts markers and records the sequence until the second end marker, then pit-in:
deceleration is recomputed as `v² / (2 · 0.2 m)` so the robot stops within 20 cm of the line end.
The marker counts are shown on the OLED afterwards, with the option to keep or discard the run.

### 6. Calibration persistence

The 32 bytes of per-channel white/black maxima are zero-padded to 128 and written as quad-words to
flash at `0x08040000` (bank 2), then reloaded from the menu, so the robot survives a power cycle
without re-calibration. This was the last feature to land (Feb 2025).

## Menu

Left button = previous, right = next, both = select. The same convention holds inside every
submenu, where "both" exits.

| Group | Entries |
|---|---|
| Run | `first drive` |
| Calibration | `calibration`, `Cali Flash save`, `load Saved cali` |
| Quick presets | `tv 0.5`, `tv 1.0`, `tv setting` |
| Debug | `velocity test`, `v change`, `motor test`, `sensor Raw`, `window test`, `mark check`, `state debug`, `sensor state test`, `position test`, `check delay`, `Flash save`, `Flash load` |
| `settings` | threshold, pit-in, accel, decel, target velocity, curve decel, curve rate |

## Layout

```
Main/              application code — sensor.c, motor.c, drive.c, init.c (menu)
External Library/  club board support — OLED, switches, delay, GPIO, flash, filesystem
Core/              CubeMX-generated init, clock config, interrupt vectors
Drivers/           ST HAL / LL drivers
533.ioc            CubeMX project file
```

## Build

Import as an existing project in **STM32CubeIDE** (target `STM32H533RETx`) and build the `Debug`
configuration; `533 Debug.launch` is set up for ST-LINK.

## Known limitations

Kept here deliberately — this board was superseded rather than finished.

- **No second run.** Only the recording pass (`Drive_First`) exists. `mark_length[]` is declared
  but never filled, and `STATE_CROSS` / `STATE_CROSS_DECISION` are defined but never entered, so
  cross-marker recovery is not implemented on this board. Both landed on the H743 version.
- **Division by zero in the sensor ISR.** `position_value = weighted_sum / normalized_value` runs
  unguarded at 31.25 kHz; if every channel reads black (robot fully off the line) the divisor is 0.
  `position_test()` has the guard, the ISR does not.
- **Flash saving is heavy-handed.** `Cali_Flash_save()` mass-erases all of bank 2 to store 32
  bytes, calls `HAL_FLASH_Unlock()` without a matching `Lock()`, and its status printout reads
  `blackMax[16]` — one past the end of a 16-element array.
- **Dead board-support code.** `custom_flash.c` and `custom_filesystem.c` are compiled and linked
  but never called; calibration persistence is hand-rolled in `init.c` instead.
- **Debug-menu mislabel.** `state_debug()` prints "end" for `mark == 3`, but `MARK_END` is 4, so
  end markers are never labelled in that view.
- `calibration` appears twice in the main menu, and `Motor_Test_Velocity()` is an empty stub.

## Credits

`External Library/` is ZETIN board-support code written by club seniors — **Joonho Gwon**
(delay, GPIO, switches, flash, filesystem, exception), **Seongho Lee** (SSD1331 OLED driver), with
modifications by **Pierre de Starlit**. `Drivers/` is ST's HAL/LL under ST's license. Everything
under `Main/` is mine.
