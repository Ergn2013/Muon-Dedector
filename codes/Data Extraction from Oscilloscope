import pyvisa
import numpy as np
import time
import csv
import os

rm = pyvisa.ResourceManager()
print("Bulunan cihazlar:", rm.list_resources())

scope = rm.open_resource('USB0::0x0699::0x0368::C027903::INSTR')
scope.timeout = 10000

print("Cihaz:", scope.query("*IDN?"))

scope.write("DATA:WIDTH 1")
scope.write("DATA:ENC RPB")

print("CSV buraya kaydedilecek:", os.getcwd())

csv_file = open("live_two_channel_summary.csv", "w", newline="")
writer = csv.writer(csv_file)

writer.writerow([
    "event_no",
    "pc_time",
    "ch1_min_v",
    "ch1_max_v",
    "ch1_vpp",
    "ch2_min_v",
    "ch2_max_v",
    "ch2_vpp",
    "n_samples_ch1",
    "n_samples_ch2"
])

event_no = 0
start_time = time.time()

def read_channel(channel_name):
    scope.write(f"DATA:SOU {channel_name}")

    ymult = float(scope.query("WFMPRE:YMULT?"))
    yzero = float(scope.query("WFMPRE:YZERO?"))
    yoff  = float(scope.query("WFMPRE:YOFF?"))

    scope.write("CURVE?")
    data = scope.read_raw()

    n_digits = int(chr(data[1]))
    header_len = 2 + n_digits
    raw = data[header_len:-1]

    wave = np.frombuffer(raw, dtype=np.uint8)
    voltage = (wave - yoff) * ymult + yzero

    return voltage

try:
    while True:
        pc_time = time.time()

        v1 = read_channel("CH1")
        v2 = read_channel("CH2")

        ch1_min = np.min(v1)
        ch1_max = np.max(v1)
        ch2_min = np.min(v2)
        ch2_max = np.max(v2)

        writer.writerow([
            event_no,
            pc_time,
            ch1_min,
            ch1_max,
            ch1_max - ch1_min,
            ch2_min,
            ch2_max,
            ch2_max - ch2_min,
            len(v1),
            len(v2)
        ])

        csv_file.flush()

        if event_no % 10 == 0:
            print(
                f"Event {event_no} | "
                f"CH1 min={ch1_min:.4f} V | "
                f"CH2 min={ch2_min:.4f} V"
            )

        event_no += 1

except KeyboardInterrupt:
    duration = time.time() - start_time

    print("\nDurduruldu.")
    print("Toplam süre (s):", duration)
    print("Toplam süre (dk):", duration / 60)
    print("Toplam event:", event_no)

    if duration > 0:
        print("Event rate (event/s):", event_no / duration)

finally:
    csv_file.close()
    scope.close()
    print("CSV dosyası kapatıldı.")
