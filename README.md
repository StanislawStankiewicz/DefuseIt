## DefuseIt

Arduino-based multi-module bomb-defusal game. A single master controller discovers every connected module over I²C, sets a per-round "version" seed, and tracks whether each module is solved or failed before the main timer expires.

### Repository Layout

| Path | Description |
| --- | --- |
| `Master/` | Main controller that discovers modules, displays timer/version, drives buzzer/LEDs, and orchestrates start/end commands. |
| `ModuleComms/` | Shared library that wraps I²C Master/Slave communication (command opcodes, status codes, and helper classes). |
| `Keypad/` | Binary/Math keypad module using an LCD and 4×4 keypad. Implements `Slave` callbacks for puzzles. |
| `KeypadTester/` | Stand-alone sketch to validate keypad wiring. Serial-only logging; helpful during perfboard bring-up. |
| `Symbols/`, `SimonSays/`, `Cables/`, etc. | Additional module sketches (some still prototypes / TBD). |
| `Symbols/mux_test/` | SoftwareWire-based tester for the TCA9548A multiplexer + multiple OLEDs. |

### Module Bus (ModuleComms)

- Transport: I²C (SDA/SCL) plus power (5 V or 3.3 V) and optional INT line for per-module LEDs.
- Discovery: The Master performs a sweep (1–126) sending `CMD_IDENTIFY`. Modules with `ModuleComms::Slave` respond with `CMD_IDENTIFY` and are stored in the Master's address table.
- Runtime commands:
	- `CMD_START_GAME` / `CMD_END_GAME`
	- `CMD_SET_VERSION` / `CMD_GET_VERSION`
	- `CMD_SET_STATUS` / `CMD_GET_STATUS`
- Status codes: `STATUS_UNSOLVED`, `STATUS_PASSED`, `STATUS_FAILED`.
- Every module instantiates `Slave(address, gameLoopFn, resetStateFn, solvedLedPin)` and calls `module.begin()` in `setup()` then `module.slaveLoop()` inside `loop()`.

### Power & Wiring Requirements

Each module exports the same edge connector pinout:

- `VCC` (5 V for ATmega328p-based boards)
- `GND`
- `SDA`
- `SCL`
- `INT` (optional, typically tied to a solved indicator LED)

Keep bus cables short; add 4.7 kΩ pull-ups to SDA/SCL if the master backplane does not provide them.

### Building & Flashing

1. Install the Arduino IDE or `arduino-cli`, plus any board cores you use (e.g., MiniCore for bare ATmega328p).
2. Install libraries: `LiquidCrystal`, `ModuleComms` (this repo’s `ModuleComms/` folder can be added to your Arduino `libraries` folder), `TM1637Display`, etc.
3. Open the desired sketch (`Master/Master.ino`, `Keypad/Keypad.ino`, …) and select the correct board/port.
4. Upload to each MCU.

#### ATmega328p Fuse (8 MHz internal clock)

```powershell
avrdude -c usbasp -p m328p -B 10 -U lfuse:w:0xE2:m
```

### Current Modules

- **Master** – Handles timer, version display, buzzer, and dispatches start/end/status commands to every discovered module. Implements safety lockouts and a strike system.
- **Keypad** – Features Binary/Math puzzles selected per-round via Master version. Uses an LCD + keypad, handles mistakes, drives the module solved LED, and now exposes a `resetState()` hook so the Master can clear the UI when the game ends.
- **Symbols, SimonSays, Cables, etc.** – Skeletons or in-progress modules. See each folder for the current sketch.

### Utilities & Diagnostics

- **KeypadTester** (`KeypadTester/KeypadTester.ino`) – Serial-based keypad reader that prints the character, matrix row/col, and pin numbers. Use this when wiring a new keypad board.
- **Mux/OLED tester** (`Symbols/mux_test/mux_test.ino`) – Uses `SoftwareWire` to talk to a TCA9548A multiplexer at `0x70`, scan channels, and confirm SSD1306 displays at `0x3C` respond.
- **ModuleComms library** – Can be copied into `~/Documents/Arduino/libraries/ModuleComms` for reuse across sketches.

### Startup Sequence (Master)

1. Initialize I²C and digital IO.
2. Discover modules by scanning 1–126 for `CMD_IDENTIFY`.
3. Generate and send a round “version” (seed) to each module.
4. Issue `CMD_START_GAME` to kick off module gameplay.
5. Poll statuses; when every module reports `STATUS_PASSED`, declare victory, otherwise handle strikes/timeouts.

### Development TODO

- Improve ModuleComms serial logging / diagnostics output.
- Add better button debouncing and post-fail lockout inside Simon Says.
- Expand documentation for remaining modules (Cables, Symbols puzzles, etc.).