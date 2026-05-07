# Quick Hardware Test Plan

These are low-friction tests that can be performed without reflashing firmware many times.

## Test 1: Cold Boot Repeatability
- Runs: 10
- Procedure: Power-cycle the full system.
- Expected result: Module discovery succeeds every time, and the game reaches the ready state.
- Record: Pass/fail per boot, discovered module count.

note: i will visually verify if all modules lit up upon starting
results:
10/10 success

## Test 2: Start/End Command Sanity
- Runs: 10
- Procedure: Start a game, then end it using the normal reset/end path.
- Expected result: All modules return to idle cleanly.
- Record: Any module that stays locked or needs extra reset.

results:
the labyrinth oled still shows the labyrinth and radio still has the led lit up, verify if current code fixed that, currently the game has old code flashed

## Test 3: Single-Fail Handling
- Runs: 10
- Procedure: Intentionally fail one module during play.
- Expected result: Master increments the mistake count and the failed module recovers correctly.
- Record: Mistake count, recovery behavior, any freeze.

results
note 1 i managed to press a button on simon says while another was lit, the another button did not go off and remain lit until another refresh happened,
note 2 masters turn on switch doesnt work after failing (i turn it up, nothing happens, turn it down - then it starts, also it turning it on should interupt showing that game failed)
10/10 success

## Test 4: Win Path Consistency
- Runs: 5
- Procedure: Solve all modules normally.
- Expected result: Victory triggers only when every module reports passed.
- Record: False wins or false blocks.

results
5/5 success

## Test 5: Timeout Path
- Runs: 3
- Procedure: Start a game and let the timer expire.
- Expected result: Timeout behavior is consistent, with end-of-game signaling and lockout.
- Record: Whether any module remains active after timeout.

results
note 1 i feel like the second change and the beep at the end (the last 10 seconds) are not synchronized
test 1 - success, time varied by 2s
test 2 - success, again ~2 s (verified by eye, the acceptable deviation would be probably so high it doesnt matter), both runs were 2s early
test 3 - succes, ~2s
test 4 - success, + ~5s
test 5 - success, + ~6s

## Test 6: I2C Robustness
- Runs: 3
- Procedure: While idle or in-game, briefly disconnect and reconnect one module or gently disturb a bus connection.
- Expected result: The rest of the system should not freeze completely, and the Master should recover or continue cleanly.
- Record: Recovery behavior and whether the bus locks up.

disconnecting was not implemented

## Test 7: Debounce Spot Check
- Runs: 20 fast presses per button-heavy module
- Procedure: Rapidly press buttons or flip switches repeatedly.
- Expected result: No extra triggers beyond what the firmware should logically register.
- Record: Double reads or missed presses.



## Test 8: Short Soak Test
- Runs: 1
- Duration: 30 minutes
- Procedure: Leave the system running and interact with it periodically.
- Expected result: No lockups, display corruption, or missed polls.
- Record: Any instability over time.

## Suggested Reporting Table

| Test ID | Runs | Pass | Fail | Notes |
| --- | ---: | ---: | ---: | --- |
| T01 | 10 |  |  | Cold boot repeatability |
| T02 | 10 |  |  | Start/end command sanity |
| T03 | 10 |  |  | Single-fail handling |
| T04 | 5 |  |  | Win path consistency |
| T05 | 3 |  |  | Timeout path |
| T06 | 3 |  |  | I2C robustness |
| T07 | 20 |  |  | Debounce spot check |
| T08 | 1 |  |  | Short soak test |

## Notes for Thesis Use
- Keep the tests practical and repeatable.
- Emphasize that they validate reliability, communication stability, and state-machine correctness without requiring repeated firmware changes.
- If you collect simple pass/fail counts and a few timing observations, this is already enough to support a solid test section.
