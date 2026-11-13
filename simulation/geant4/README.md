# Geant4 cosmic muon detector model

This Monte Carlo model simulates two vertically separated BC-408 scintillators coupled to photomultiplier tubes. It uses the QBBC physics list with Geant4 optical physics and models photon transport, wavelength-dependent photocathode quantum efficiency and PMT pulse formation.

[View the 50,000-event example run and figures](results/run-2026-11-13/README.md).

## Build and run

Use a Geant4 installation with its physics datasets available, a C++ compiler, CMake and Python 3 with NumPy, pandas and Matplotlib. Set `GEANT4_INSTALL` to your installation directory.

```bash
cd simulation/geant4
export GEANT4_INSTALL="$HOME/Geant4/geant4-install"
source "$GEANT4_INSTALL/bin/geant4.sh"
cmake -S . -B build -DCMAKE_PREFIX_PATH="$GEANT4_INSTALL"
cmake --build build --parallel
python3 collect_muon_data.py --binary ./build/exampleB1 --events 1000 --jobs 2
```

For the example run configuration, use `--events 50000 --jobs 12 --threshold 100 --waveforms 5 --seeds 12345 67890`. The script runs each Geant4 process in serial mode with its own seed pair and output directory, then combines the event records and generates plots. See the run's `workers.csv` for individual seed pairs and event ranges.

The script writes event-level CSV, a counting summary with Wilson intervals, a threshold scan, optical timing summaries and PNG figures under `runs/`. For an existing run, regenerate plots with `python3 collect_muon_data.py --analyze PATH_TO_RUN`.

## Model and source

The application is a detector-specific adaptation of [Geant4 example B1](https://github.com/Geant4/geant4/tree/v11.2.1/examples/basic/B1). See the [Geant4 Software License](LICENSE-Geant4.txt).

This product includes software developed by Members of the Geant4 Collaboration ( http://cern.ch/geant4 ).
