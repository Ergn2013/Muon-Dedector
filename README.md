# Cosmic Muon Detector

A two-channel cosmic-ray muon detector prototype developed at Middle East Technical University. Plastic scintillators and photomultiplier tubes (PMTs) provide the signals. The project combines 3D-printed detector mounts, oscilloscope measurements and coincidence electronics for a planned STM32 readout.

**Project team** · [Ergün Özakyıldız](https://www.linkedin.com/in/erg%C3%BCn-%C3%B6zaky%C4%B1ld%C4%B1z-822a24188), [Burhan Karaduman](https://www.linkedin.com/in/burhan-karaduman-57b348362), [Nazlıcan Yıldırım](https://www.linkedin.com/in/nazlicanyildirim255) and Emre Yağız Öztürk

| Section | Start here |
| --- | --- |
| [Electronics](electronics/ltspice/README.md) | [Schematic preview](electronics/ltspice/schematic.svg), LTspice setup and four focused test benches |
| [Mechanics](mechanical/README.md) | SolidWorks assembly, revised parts and 3D-print files |
| [Oscilloscope measurements](measurements/oscilloscope/README.md) | Four paired waveform exports and selected bench captures |
| [Acquisition and analysis](software/README.md) | Early oscilloscope capture and pulse analysis scripts |
| [Geant4 simulation](simulation/geant4/README.md) | Build and run the optical detector model, or browse the [50,000-event simulation](simulation/geant4/results/run-2026-11-13/README.md) |

PMT pulses are measured on an oscilloscope. The LTspice tests use synthetic signals to study the coincidence circuit. Hardware validation is ongoing. Measuring the muon lifetime is a later goal.
