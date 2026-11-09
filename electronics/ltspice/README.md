# TLV3601 coincidence electronics

This LTspice package uses two TLV3601 comparators, SN74LVC2G17 Schmitt-trigger buffers, and an SN74LVC1G08 AND gate. The two AND inputs are the buffered channel signals `CH1_PS` and `CH2_PS`. The post-gate and singles paths stretch pulses for the intended STM32 inputs.

[View the schematic preview](SEMA_ONIZLEME.svg) · [Original test notes in Turkish](README_ILK_OKU.md)

## Run the first test

1. Keep this folder intact, including the local symbols, include files and `NOISE/` folder.
2. Obtain the following Texas Instruments model files and place them next to the schematic. The model filenames are used by `MODELS.inc`.

   | Required file | TI download |
   | --- | --- |
   | `tlv3601.lib` | [TLV3601 TINA-TI Model, SNOM713A](https://www.ti.com/product/TLV3601#design-tools-simulation) |
   | `SN74LVC2G17.cir` | [SN74LVC2G17 Behavioral SPICE Model, SCEM615](https://www.ti.com/product/SN74LVC2G17#design-tools-simulation) |
   | `SN74LVC1G08.cir` | [SN74LVC1G08 Behavioral SPICE Model, SCEM642](https://www.ti.com/product/SN74LVC1G08#design-tools-simulation) |

   The original Rev AG files can also be copied into this folder. TI's TLV3601 download contains a TINA schematic and a `.TSM` macro. To obtain the SPICE text, open the schematic in TINA, open the TLV3601 macro's properties, select **Enter Macro**, and save the macro netlist as `tlv3601.lib`. [MODEL_KONTROLU.csv](MODEL_KONTROLU.csv) records the hashes of the model files used to assemble this test package.

3. Open [00_ONCE_BUNU_AC.asc](00_ONCE_BUNU_AC.asc) in LTspice and select **Run**. This case uses a 30 mV nominal negative pulse with 20 ns FWHM and nominal 10 mV RMS noise on the signal and comparator reference. Examine `CH1_PS`, `CH2_PS`, `COINC_FAST` and `STM_COINC` to trace how the pulse moves through the circuit.
4. For the main amplitude and width sweep, open [01_GENLIK_GENISLIK_10mV.asc](01_GENLIK_GENISLIK_10mV.asc). Each schematic has a corresponding `.cir` entry point for a netlist-based run. [TEST_LISTESI.csv](TEST_LISTESI.csv) indexes the cases.

The additional cases explore channel delay, asymmetric amplitudes, singles, noise sources, comparator offsets and output loading. The PWL records are synthetic simulation inputs. To summarize simulated edge counts after generating the LTspice outputs, run `python TOOLS/analyze_outputs.py` from this directory. The count threshold is an analysis setting. The simulated STM32 loads represent electrical inputs.
