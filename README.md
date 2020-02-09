# Helical milling

This project allows a fixed horizontal milling machine to COPY helical teeths from sample to target.

An encoder is attached to the X axis to read its trajectory, and according to a previous configuration
![ratio](https://latex.codecogs.com/svg.latex?%5Cfrac%7BAnglesToRotate%7D%7BStepsInX%7D)
it rotates the piece to follow the desired helical angle.

## Requirements
- Electronic device using a PIC16F877A (Schematic Design provided in the folder `/PCB`)
- Encoder to read X axis movements
- Mandrel moved by a motor
- Compatible motor driver
- 5-10V DC Supply


## Videos
- https://www.youtube.com/watch?v=wu8dKf8xgoI
