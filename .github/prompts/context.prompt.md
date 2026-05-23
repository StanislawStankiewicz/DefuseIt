---
description: "Use built-in DefuseIt context to modify code and thesis text without re-scanning the repo each time"
name: "context"
argument-hint: "task=<code|thesis|both> goal=<what to change>"
agent: "agent"
---
You are working on DefuseIt.

Project context (preloaded)
- Engineering thesis project: modular bomb-defusal/logic-task embedded system for ATmega328P, implemented in C++.
- Core architecture: distributed modules + central `Master`, connected over I2C using custom `ModuleComms` (`Master`/`Slave` abstraction).
- Protocol constants include:
  - commands: `CMD_START_GAME`, `CMD_END_GAME`, `CMD_GET_STATUS`, `CMD_SET_STATUS`, `CMD_GET_VERSION`, `CMD_SET_VERSION`, `CMD_IDENTIFY`, `CMD_SET_REMAINING_SECONDS`, `CMD_SET_MISTAKE_COUNT`
  - statuses: `STATUS_UNSOLVED`, `STATUS_PASSED`, `STATUS_FAILED`
- Master responsibilities: discovery, start/end orchestration, version/seed propagation, timer control, status polling, and mistake propagation.
- Implemented module families in repo: `SimonSays`, `Keypad`, `Switch`, `Labyrinth`, `Radio`, `Symbols`.
- Current README address map baseline:
  - `Symbols` `0x10`
  - `SimonSays` `0x11`
  - `Keypad` `0x12`
  - `Switch` `0x13`
  - `Labyrinth` `0x14`
  - `Radio` `0x15`
- Thesis baseline topics: non-blocking multitasking (`millis()`), SRAM/Flash limits on ATmega328P, use of `PROGMEM`, and modular scalability.
- Known implementation example: `Radio` stores Morse-related data in Flash (`PROGMEM`) and uses OLED + encoder interaction.
- Test notes exist in `tests.md` (cold boot repeatability, fail handling, timeout behavior, etc.).

Primary files to edit (use as first choice, no broad search by default)
- Thesis text: [thesis.tex](../../thesis/thesis.tex)
- Protocol and integration: [ModuleComms.h](../../ModuleComms/ModuleComms.h), [ModuleComms.cpp](../../ModuleComms/ModuleComms.cpp)
- Master logic: [Master.ino](../../Master/Master.ino)
- Modules: [SimonSays.ino](../../SimonSays/SimonSays.ino), [Keypad.ino](../../Keypad/Keypad.ino), [Switch.ino](../../Switch/Switch.ino), [Labyrinth.ino](../../Labyrinth/Labyrinth.ino), [Radio.ino](../../Radio/Radio.ino), [Symbols.ino](../../Symbols/Symbols.ino)

Code-edit rules (project-specific)
- Prefer non-blocking timing over `delay()` in gameplay-critical loops.
- Respect ATmega328P memory constraints; prefer `PROGMEM` for large static data.
- Preserve existing module addresses and protocol opcodes unless user explicitly requests protocol migration.
- Keep `Master`/`Slave` interaction semantics stable.

Thesis-edit rules (project-specific)
- Default language: Polish technical-academic style.
- Preserve LaTeX structure, labels, references, and chapter flow.
- Do not claim features/tests not present in code or `tests.md`.
- When editing architecture or module descriptions, ensure terminology matches code (`Master`, `Slave`, `ModuleComms`, I2C, ATmega328P).

Expected response format
- Briefly state what was changed.
- List modified files.
- If relevant, include "Consistency Notes" with exact mismatch and proposed alignment.

Optional arguments
- `task`: `code` | `thesis` | `both`
- `goal`: plain-language change request
