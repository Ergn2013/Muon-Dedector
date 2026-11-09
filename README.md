# Cosmic Muon Detector

A two-channel cosmic-ray muon detector prototype developed at Middle East Technical University. Plastic scintillators and photomultiplier tubes (PMTs) provide the signals. The project combines 3D-printed detector mounts, oscilloscope measurements and coincidence electronics for a planned STM32 readout.

| Section | Start here |
| --- | --- |
| [Electronics](electronics/ltspice/README.md) | [Schematic preview](electronics/ltspice/schematic.svg), LTspice setup and four focused test benches |
| [Mechanics](mechanical/README.md) | SolidWorks assembly, revised parts and 3D-print files |
| [Acquisition and analysis](software/README.md) | Early oscilloscope capture and pulse analysis scripts |
| [Geant4 simulation](simulation/geant4/README.md) | Source import checklist |

PMT pulses are measured on an oscilloscope. The LTspice tests use synthetic signals to study the coincidence circuit. Hardware validation is ongoing. Measuring the muon lifetime is a later goal.
