# Coincidence electronics

Each PMT channel feeds a TLV3601 discriminator and an SN74LVC2G17 Schmitt-trigger buffer. The buffered signals (`CH1_PS`, `CH2_PS`) feed an SN74LVC1G08 AND gate. Pulse stretching provides two singles outputs and one coincidence output for the planned STM32 readout.

[View the schematic](schematic.svg)

## Run in LTspice

1. Keep the [testbench folder](testbench/) together. It contains the schematics, local symbols, include files and synthetic PWL noise inputs.
2. Place the three TI model files below in `testbench/`, next to the schematics. You can also use the files from the original Rev AG package.

   | Local filename | TI download |
   | --- | --- |
   | `tlv3601.lib` | [TLV3601 TINA-TI Model, SNOM713A](https://www.ti.com/product/TLV3601#design-tools-simulation) |
   | `SN74LVC2G17.cir` | [SN74LVC2G17 Behavioral SPICE Model, SCEM615](https://www.ti.com/product/SN74LVC2G17#design-tools-simulation) |
   | `SN74LVC1G08.cir` | [SN74LVC1G08 Behavioral SPICE Model, SCEM642](https://www.ti.com/product/SN74LVC1G08#design-tools-simulation) |

   For the TLV3601 TINA download, open the schematic in TINA, choose **Enter Macro** in the comparator's properties and save the SPICE netlist as `tlv3601.lib`.

3. Open a test below and select **Run**. For the signal path, inspect `CH1_PS`, `CH2_PS`, `COINC_FAST` and `STM_COINC`.

| Test | Input |
| --- | --- |
| [Quick start](testbench/00_quick_start.asc) | One coincident event with a 30 mV nominal negative pulse per channel, 20 ns FWHM and nominal 10 mV RMS noise on signal and comparator reference |
| [Amplitude and width](testbench/01_amplitude_width_sweep.asc) | 30 mV to 1 V and 10 to 30 ns FWHM |
| [Channel delay](testbench/03_channel_delay_sweep.asc) | Relative pulse delay between the two PMTs |
| [Singles and noise-only](testbench/05_single_channel_and_noise_only.asc) | One channel pulsed at a time and a noise-only case |

The tests use synthetic inputs and model the STM32 connections as electrical loads. To count threshold crossings in the generated LTspice output, run `python TOOLS/analyze_outputs.py` from `testbench/`. The script writes `edge_counts.csv` and `edge_counts.json`.

[Full development test set in Git history](https://github.com/Ergn2013/Muon-Dedector/tree/ba4f7d2715aff6d7d7e44ef1ddcb9fbe6d40304d/electronics/ltspice)
