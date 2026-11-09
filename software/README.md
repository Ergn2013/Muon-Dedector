# Oscilloscope capture and pulse analysis

| Script | Purpose |
| --- | --- |
| [oscilloscope_capture.py](oscilloscope_capture.py) | Early two-channel oscilloscope capture using PyVISA and NumPy. Set the VISA resource address for your instrument before running. |
| [pulse_analysis.m](pulse_analysis.m) | MATLAB script for thresholding and plotting an earlier event waveform CSV. It expects `live_muon_data3.csv` with `event_no`, `time_s` and `voltage_V` columns. |

These scripts use different CSV layouts. The MATLAB script was written for waveform samples. The Python script saves a per-event channel summary.
