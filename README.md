# STM32 Clap Pattern Detection System

A real-time clap-pattern recognition project built using the **STM32F407 Discovery Board** and its onboard MEMS microphone.

The system captures audio through the microphone, converts PDM data into PCM samples, detects claps, and recognizes a custom clap sequence to control the onboard green LED.

## Features

- Real-time audio acquisition using the onboard MEMS microphone (MP45DT02).
- I2S2 master receiver configuration.
- DMA-based continuous audio acquisition.
- PDM-to-PCM conversion using the STM32 PDM Filter Library.
- USB CDC audio streaming for PC-side monitoring and testing.
- Clap detection using peak amplitude and crest factor.
- Custom three-clap pattern recognition.
- LED control based on successful pattern recognition.

## Clap Recognition Pattern

The system recognizes the following sequence:

**👏 Clap → Pause → 👏 Clap → 👏 Clap**

Timing requirements:

| Event | Timing |
|---|---|
| First to second clap | 400–800 ms |
| Second to third clap | Maximum 400 ms |
| Minimum interval between detections | 100 ms |

When the complete pattern is recognized, the onboard green LED toggles.

Incorrect sequences are rejected.

## Hardware Requirements

- STM32F407G-DISC1 / STM32F4 Discovery Board (MB997 series)
- USB connection
- Onboard MP45DT02 MEMS microphone
- Onboard green LED

No external microphone or additional hardware is required.

## Software and Tools

- STM32CubeIDE
- STM32 HAL Drivers
- STM32 PDM Filter Library
- STM32 USB Device CDC Middleware
- Python (for audio recording and signal analysis)
- NumPy
- SoundFile / WAV audio tools

## System Architecture

```text
       Onboard MEMS Microphone
                 |
                 v
          PDM Audio Output
                 |
                 v
             I2S2 RX
                 |
                 v
          DMA Acquisition
                 |
                 v
           PDM-to-PCM
                 |
                 v
          PCM Audio Samples
                 |
                 v
           Clap Detection
                 |
                 v
       Pattern Recognition
                 |
                 v
           Green LED Toggle
```

## Audio Processing Configuration

| Parameter | Configuration |
|---|---|
| Microphone | MP45DT02 |
| I2S Peripheral | I2S2 |
| I2S Mode | Master Receive |
| I2S Data Format | 16-bit |
| PDM Decimation | 64 |
| PCM Sample Rate | 8 kHz |
| DMA Mode | Circular |
| Audio Transfer | USB CDC |

## Clap Detection

The detector analyzes PCM audio using:

- Peak amplitude threshold.
- RMS-based energy calculation.
- Crest factor to distinguish impulsive sounds from ordinary speech.
- Overlapping 64-sample analysis windows.
- 32-sample window hop.
- 100 ms refractory interval to reduce duplicate detections.

The clap-pattern recognition logic uses timing constraints to identify the intended sequence.

## Project Status

**Working prototype — successfully tested on STM32F407 Discovery hardware.**

The onboard microphone, audio processing pipeline, clap detection, and custom pattern recognition have been implemented and tested.

## Future Improvements

- Adjustable clap sensitivity.
- Recognition of multiple clap patterns.
- Noise-resistant detection in different environments.
- Configurable LED and GPIO actions.
- Improved audio visualization and diagnostics.

## Author

Developed as an embedded systems and digital signal processing project using STM32.
