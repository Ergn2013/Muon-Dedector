# Cosmic Muon Detector

An undergraduate experimental physics project at Middle East Technical University. The prototype uses two plastic scintillators coupled to photomultiplier tubes (PMTs). The team uses oscilloscope measurements to characterize PMT pulses and is developing coincidence electronics for event counting.

Two PMT channels feed TLV3601 discriminators and SN74LVC2G17 Schmitt-trigger buffers. The buffered signals feed an SN74LVC1G08 AND gate. Pulse-stretching stages provide two singles outputs and one coincidence output for the planned STM32 readout.

| Section | Contents |
| --- | --- |
| [Mechanical design](mechanical/README.md) | SolidWorks assembly, revised parts and 3D-print exports |
| [Coincidence electronics](electronics/ltspice/README.md) | Current TLV3601 schematic, LTspice test cases and setup instructions |
| [Oscilloscope and analysis scripts](codes/) | Earlier data acquisition and pulse analysis scripts |
| [Geant4](simulation/geant4/README.md) | Source import checklist |

The detector is a prototype. The LTspice sweeps use synthetic test signals and noise. Hardware validation and muon-lifetime measurements are ongoing project goals.
