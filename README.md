## Modules

- tic tac toe with simple ai
- enclosed button
- repeat pattern
- defuse wires
- flip switches
- binary/math
- enter valid regex
- wordle

## I2C Codes

## Startup sequence

- BFS looking for modules
- each module responds with own unique address
- master saves addresses

## Technical requirements

Each module needs 4/5 connections on each side:

- 5/3.3 V
- GND
- SDA
- SCL
- INT (optional)

## Setup

### ATTiny85U
`avrdude -c usbasp -p t85 -B 10 -U lfuse:w:0xE2:m` - use to enable full 8MHz clock for USBasp
