#!/usr/bin/env python3
"""Run the Geant4 MuonProject and plot the simulated detector response."""

import argparse
from datetime import datetime
import os
from pathlib import Path
import random
import re
import shutil
import subprocess
import tempfile
import time

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


CHANNEL_COLORS = ("#18658c", "#c5602e")
DEFAULT_THRESHOLD_MV = 100.0
COUNT_COLUMNS = {
    "voltage_hit_pmt1": "Det1 PMT voltage hit events",
    "voltage_hit_pmt2": "Det2 PMT voltage hit events",
    "voltage_coincidence": "PMT voltage coincidence",
    "edep_coincidence": "Energy-deposit coincidence count",
}


def save_plot(figure, plots, filename):
    figure.savefig(plots / filename, dpi=180, bbox_inches="tight")
    plt.close(figure)


def histogram(axis, columns, xlabel):
    samples = [column[np.isfinite(column)] for column in columns]
    nonempty = [sample for sample in samples if sample.size]
    if nonempty:
        bins = np.histogram_bin_edges(np.concatenate(nonempty), bins=50)
        for index, sample in enumerate(samples):
            if sample.size:
                label = f"PMT {index + 1} (n={sample.size:,})" if len(samples) > 1 else None
                axis.hist(sample, bins=bins, histtype="step", lw=1.7,
                          color=CHANNEL_COLORS[index], label=label)
        if len(samples) > 1:
            axis.legend(frameon=False)
    else:
        axis.text(0.5, 0.5, "No entries", ha="center", va="center",
                  transform=axis.transAxes)
    axis.set(xlabel=xlabel, ylabel="Events")
    axis.grid(axis="y", alpha=0.2)


def plot_waveforms(run, plots, threshold):
    index = run / "pmt_waveform_summary.csv"
    if not index.is_file():
        return
    for row in pd.read_csv(index).itertuples(index=False):
        waveform = run / Path(row.filename).name
        trace = pd.read_csv(waveform)
        fig, ax = plt.subplots(figsize=(7, 3.5), layout="constrained")
        for channel, color in enumerate(CHANNEL_COLORS, start=1):
            ax.plot(trace["time_ns"], trace[f"det{channel}_voltage_mV"],
                    color=color, label=f"PMT {channel}")
        ax.axhline(-threshold, color="0.35", ls="--", label="Run threshold")
        ax.set(title=f"Simulated pulses · event {row.geant_event_id}",
               xlabel="Time from start of trace [ns]", ylabel="Voltage [mV]")
        ax.legend(frameon=False)
        save_plot(fig, plots, f"waveform_event_{row.geant_event_id}.png")


def read_events(run, expected_events=None):
    report = (run / "pmt_analysis_summary.txt").read_text(encoding="utf-8")
    threshold_match = re.search(r"Voltage threshold\s*:\s*([\d.eE+-]+) mV", report)
    events_match = re.search(r"Number of events\s*:\s*(\d+)", report)
    if not threshold_match or not events_match:
        raise ValueError(f"Missing event count or voltage threshold in the Geant4 summary in {run}.")
    threshold = float(threshold_match[1])
    reported_events = int(events_match[1])
    data = pd.read_csv(run / "pmt_event_analysis.csv").sort_values("event_id")
    events = len(data)
    if events == 0 or events != reported_events or (
        expected_events is not None and events != expected_events
    ) or not np.array_equal(data["event_id"], np.arange(events)):
        raise ValueError(f"Incomplete or duplicate events in {run} ({events} CSV rows).")
    for column, label in COUNT_COLUMNS.items():
        if not data[column].isin((0, 1)).all():
            raise ValueError(f"Invalid event flags in {column} in {run}.")
        match = re.search(rf"^{re.escape(label)}\s*:\s*(\d+)", report, re.MULTILINE)
        if not match or int(match[1]) != data[column].sum():
            raise ValueError(f"Geant4 summary and event CSV disagree on {label} in {run}.")
    if not (data["voltage_coincidence"] ==
            (data["voltage_hit_pmt1"] & data["voltage_hit_pmt2"])).all():
        raise ValueError(f"Voltage coincidence flags disagree with the PMT flags in {run}.")
    return data, threshold


def analyze(run, expected_events=None):
    data, threshold = read_events(run, expected_events)
    events = len(data)

    energy = [data[f"edep_det{i}_MeV"].to_numpy() for i in (1, 2)]
    peaks = [data[f"peak_pmt{i}_mV"].to_numpy() for i in (1, 2)]
    photons = [data[f"photons_pmt{i}"].to_numpy() for i in (1, 2)]
    photoelectrons = [data[f"photoelectrons_pmt{i}"].to_numpy() for i in (1, 2)]
    delta = data["delta_first_pmt2_minus_pmt1_ns"].to_numpy()
    delta = delta[np.isfinite(delta)]
    if any(not np.isfinite(peak).all() or (peak < 0).any() for peak in peaks):
        raise ValueError("Invalid peak voltages in the event CSV.")

    labels = ["PMT 1 above threshold", "PMT 2 above threshold",
              "Both PMTs above threshold", "Energy in both scintillators"]
    counts = data[list(COUNT_COLUMNS)].sum().to_numpy()
    fractions = counts / events
    z = 1.95996398454
    denominator = 1 + z**2 / events
    center = (fractions + z**2 / (2 * events)) / denominator
    half = z * np.sqrt(fractions * (1 - fractions) / events + z**2 / (4 * events**2))
    half /= denominator
    summary = pd.DataFrame({
        "metric": labels,
        "events": events,
        "count": counts,
        "fraction": fractions,
        "wilson_95_low": np.clip(center - half, 0, 1),
        "wilson_95_high": np.clip(center + half, 0, 1),
    })
    summary.to_csv(run / "counting_summary.csv", index=False)

    plots = run / "plots"
    plots.mkdir(exist_ok=True)
    qe = pd.read_csv(run / "pmt_qe_curve.csv")
    fig, ax = plt.subplots(figsize=(7, 4), layout="constrained")
    ax.plot(qe["wavelength_nm"], qe["quantum_efficiency_percent"],
            color=CHANNEL_COLORS[0], marker="o", markersize=3)
    ax.set(title="PMT quantum efficiency used in the simulation",
           xlabel="Wavelength [nm]", ylabel="Quantum efficiency [%]", ylim=(0, None))
    ax.grid(alpha=0.2)
    save_plot(fig, plots, "quantum_efficiency.png")

    fig, axes = plt.subplots(1, 2, figsize=(10, 4), layout="constrained")
    histogram(axes[0], [column[column > 0] for column in energy],
              "Positive energy deposit [MeV]")
    histogram(axes[1], [column[column > 0] for column in peaks],
              "Nonzero PMT pulse magnitude [mV]")
    axes[1].axvline(threshold, color="0.35", ls="--", label="Run threshold")
    axes[1].legend(frameon=False)
    fig.suptitle("Energy deposit and simulated PMT pulse height")
    save_plot(fig, plots, "energy_and_pulse_height.png")

    fig, axes = plt.subplots(1, 2, figsize=(10, 4), layout="constrained")
    for i, ax in enumerate(axes):
        reached = photons[i] > 0
        if reached.any():
            cells = ax.hexbin(photons[i][reached], photoelectrons[i][reached],
                              gridsize=35, mincnt=1, bins="log", cmap="viridis")
            fig.colorbar(cells, ax=ax, label="Events per bin")
        else:
            ax.text(0.5, 0.5, "No photons reached this PMT", ha="center", va="center",
                    transform=ax.transAxes)
        ax.set(title=f"PMT {i + 1} · n={reached.sum():,}",
               xlabel="Photons reaching PMT", ylabel="Generated photoelectrons")
    save_plot(fig, plots, "photons_and_photoelectrons.png")

    first_photon = [data[f"first_transport_pmt{i}_ns"].to_numpy() for i in (1, 2)]
    fig, axes = plt.subplots(1, 2, figsize=(10, 4), layout="constrained")
    histogram(axes[0], first_photon, "First-photon transport time [ns]")
    histogram(axes[1], [delta], "PMT 2 − PMT 1 transport time [ns]")
    axes[1].set_title(f"Paired events · n={delta.size:,}")
    fig.suptitle("First-photon optical transport")
    save_plot(fig, plots, "optical_timing.png")
    pd.DataFrame([{
        "paired_events": delta.size,
        "mean_ns": delta.mean() if delta.size else None,
        "sample_std_ns": delta.std(ddof=1) if delta.size > 1 else None,
    }]).to_csv(run / "optical_timing_summary.csv", index=False)

    sweep = np.unique(np.r_[np.arange(10, 501, 5), threshold])
    sweep = sweep[sweep > 0]
    amplitudes = [*peaks, np.minimum(*peaks)]
    sweep_data = {"threshold_mV": sweep}
    fig, ax = plt.subplots(figsize=(7, 4), layout="constrained")
    for key, label, values, color in zip(
        ("pmt1", "pmt2", "both"), labels[:3], amplitudes, (*CHANNEL_COLORS, "#41846a")
    ):
        passed = events - np.searchsorted(np.sort(values), sweep, side="left")
        sweep_data[f"{key}_count"] = passed
        sweep_data[f"{key}_fraction"] = passed / events
        ax.plot(sweep, 100 * passed / events, label=label, color=color)
    ax.axvline(threshold, color="0.35", ls="--", label="Run threshold")
    ax.set(title="Threshold scan from stored pulse peaks",
           xlabel="Common threshold [mV]", ylabel="Fraction of generated events [%]")
    ax.legend(frameon=False)
    save_plot(fig, plots, "threshold_scan.png")
    pd.DataFrame(sweep_data).to_csv(run / "threshold_scan.csv", index=False)
    plot_waveforms(run, plots, threshold)

    print(summary[["metric", "count", "fraction"]].to_string(index=False))
    print(f"Data: {run}\nPlots: {plots}")


def available_cpus():
    if hasattr(os, "sched_getaffinity"):
        return len(os.sched_getaffinity(0))
    return os.cpu_count() or 1


def start_worker(worker, events, seeds, command, env):
    worker.mkdir()
    seed_line = " ".join(map(str, seeds))
    (worker / "collect.mac").write_text(
        f"/random/setSeeds {seed_line}\n"
        "/run/initialize\n"
        "/run/verbose 0\n"
        "/tracking/verbose 0\n"
        "/tracking/storeTrajectory 0\n"
        "/vis/disable\n"
        f"/run/printProgress {max(1, events // 50)}\n"
        f"/run/beamOn {events}\n",
        encoding="utf-8",
    )
    with (worker / "geant4.log").open("w", encoding="utf-8") as log:
        log.write(f"Geant4 command: {command!r}\nSeeds: {seed_line}\n")
        log.flush()
        return subprocess.Popen(command, cwd=worker, env=env, stdout=log,
                                stderr=subprocess.STDOUT)


def merge_workers(run, workers, max_waveforms):
    frames = []
    manifest = []
    waveforms = []
    offset = 0
    reference_qe = (workers[0][0] / "pmt_qe_curve.csv").read_bytes()
    reference_threshold = None
    for worker, events, seeds in workers:
        data, threshold = read_events(worker, events)
        if reference_threshold is None:
            reference_threshold = threshold
        if threshold != reference_threshold or (
            (worker / "pmt_qe_curve.csv").read_bytes() != reference_qe
        ):
            raise ValueError(f"PMT settings differ between workers in {run}.")
        data["worker_id"] = worker.name
        data["worker_event_id"] = data["event_id"]
        data["event_id"] += offset
        frames.append(data)
        manifest.append({
            "worker": str(worker.relative_to(run)), "events": events,
            "seed_1": seeds[0], "seed_2": seeds[1], "first_event_id": offset,
        })

        index = worker / "pmt_waveform_summary.csv"
        if index.is_file() and len(waveforms) < max_waveforms:
            candidates = pd.read_csv(index).head(max_waveforms - len(waveforms))
            for row in candidates.to_dict(orient="records"):
                local_id = int(row["geant_event_id"])
                if not 0 <= local_id < events:
                    raise ValueError(f"Invalid waveform event ID in {index}.")
                event_id = offset + local_id
                filename = f"pmt_waveform_event_{event_id}.csv"
                shutil.copyfile(worker / Path(row["filename"]).name, run / filename)
                row.update(saved_index=len(waveforms), geant_event_id=event_id,
                           filename=filename)
                waveforms.append(row)
        offset += events

    combined = pd.concat(frames, ignore_index=True)
    combined.to_csv(run / "pmt_event_analysis.csv", index=False)
    (run / "pmt_qe_curve.csv").write_bytes(reference_qe)
    pd.DataFrame(manifest).to_csv(run / "workers.csv", index=False)
    if waveforms:
        pd.DataFrame(waveforms).to_csv(run / "pmt_waveform_summary.csv", index=False)
    totals = combined[list(COUNT_COLUMNS)].sum()
    report = [
        "Combined Geant4 run",
        f"Worker processes: {len(workers)}",
        f"Number of events: {len(combined)}",
        f"Voltage threshold: {reference_threshold:.12g} mV",
    ]
    report.extend(f"{label}: {totals[column]}" for column, label in COUNT_COLUMNS.items())
    (run / "pmt_analysis_summary.txt").write_text("\n".join(report) + "\n", encoding="utf-8")


def collect(args):
    binary = args.binary
    if binary is None:
        default = Path("build/exampleB1")
        binary = default if default.is_file() else Path("build-clean/exampleB1")
    binary = binary.resolve()
    if not binary.is_file() or not os.access(binary, os.X_OK):
        raise ValueError(f"Executable not found or not executable: {binary}")
    args.output.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    run = Path(tempfile.mkdtemp(prefix=f"run-{stamp}-", dir=args.output.resolve()))
    job_count = min(args.jobs, args.events)
    quotient, remainder = divmod(args.events, job_count)
    seed_generator = random.Random(f"{args.seeds[0]}:{args.seeds[1]}")
    first_seeds = tuple(args.seeds)
    used_first_seeds = {first_seeds[0]}
    workers = []
    for index in range(job_count):
        seeds = first_seeds
        if index:
            seed = seed_generator.randrange(1, 2_000_000_000)
            while seed in used_first_seeds:
                seed = seed_generator.randrange(1, 2_000_000_000)
            used_first_seeds.add(seed)
            seeds = (seed, seed_generator.randrange(1, 2_000_000_000))
        worker = run / "workers" / f"worker-{index + 1:03d}"
        workers.append((worker, quotient + int(index < remainder), seeds))
    (run / "workers").mkdir()

    # The current Geant4 application shares its run counters and CSV files.
    # Separate processes isolate those counters and files.
    env = os.environ.copy()
    env["G4RUN_MANAGER_TYPE"] = "Serial"
    env["G4FORCE_RUN_MANAGER_TYPE"] = "Serial"
    command = [str(binary), "collect.mac", str(args.threshold), "50", "220000",
               "0.57", "6.0", "0.1", "1.0", str(args.waveforms)]
    print(f"Running {args.events:,} total events on {job_count} Geant4 processes in {run}",
          flush=True)
    running = {}
    finished_events = 0
    try:
        for worker, events, seeds in workers:
            running[worker] = (start_worker(worker, events, seeds, command, env), events)
        while running:
            for worker, (process, events) in list(running.items()):
                status = process.poll()
                if status is None:
                    continue
                del running[worker]
                if status != 0:
                    raise ValueError(f"Geant4 exited with status {status}. See {worker / 'geant4.log'}.")
                finished_events += events
                print(f"{worker.name} finished · {finished_events:,}/{args.events:,} events",
                      flush=True)
            if running:
                time.sleep(0.5)
    finally:
        for process, _ in running.values():
            if process.poll() is None:
                process.terminate()
        for process, _ in running.values():
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

    try:
        merge_workers(run, workers, args.waveforms)
        analyze(run, args.events)
    except (OSError, ValueError, KeyError, TypeError, pd.errors.ParserError) as exc:
        raise ValueError(f"{exc} Geant4 logs: {run / 'workers'}.") from exc


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path,
                        help="Geant4 executable (default: build/exampleB1, then build-clean/exampleB1).")
    parser.add_argument("--events", type=int, default=50000)
    parser.add_argument("--jobs", type=int, default=available_cpus(),
                        help="Parallel Geant4 processes (default: available logical CPUs).")
    parser.add_argument("--threshold", type=float, default=DEFAULT_THRESHOLD_MV,
                        help="Positive pulse magnitude threshold in mV.")
    parser.add_argument("--waveforms", type=int, default=5,
                        help="Maximum number of simulated coincidence traces to save.")
    parser.add_argument("--seeds", type=int, nargs=2, default=(12345, 67890))
    parser.add_argument("--output", type=Path, default=Path("runs"))
    parser.add_argument("--analyze", type=Path,
                        help="Regenerate plots from an existing Geant4 run directory.")
    args = parser.parse_args()
    if not 1 <= args.events <= 2**31 - 1 or not 0 <= args.waveforms <= 2**31 - 1:
        parser.error("Events must be positive and waveforms nonnegative, within the Geant4 integer range.")
    if args.jobs < 1:
        parser.error("Jobs must be positive.")
    if not np.isfinite(args.threshold) or args.threshold <= 0:
        parser.error("Threshold must be finite and positive.")
    if not all(0 < seed < 2**31 - 1 for seed in args.seeds):
        parser.error("Seeds must be positive 32-bit signed integers below 2147483647.")
    try:
        if args.analyze is not None:
            analyze(args.analyze.resolve())
        else:
            collect(args)
    except (OSError, ValueError, KeyError, TypeError, pd.errors.ParserError) as exc:
        parser.exit(1, f"Error: {exc}\n")
    except KeyboardInterrupt:
        parser.exit(130, "Interrupted. Partial worker files remain in the run directory.\n")


if __name__ == "__main__":
    main()
