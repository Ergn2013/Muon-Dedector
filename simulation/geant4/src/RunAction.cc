#include "RunAction.hh"

#include "G4Run.hh"
#include "G4ios.hh"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace B1
{

G4double pmtVoltageThresholdVolt = 100.0e-3;
G4double pmtLoadResistanceOhm = 50.;

// Official H10721-110 typical peak sensitivities at Vcont = +1.0 V.
G4double pmtAnodeRadiantSensitivityAW = 2.2e5;
G4double pmtCathodeRadiantSensitivityPeakAW = 0.110;

G4double pmtRiseTimeSeconds = 0.57e-9;
G4double pmtFallTimeSeconds = 6.0e-9;
G4double pmtWaveformTimeStepSeconds = 0.1e-9;
G4double pmtVoltageCalibrationFactor = 1.0;

// R9880U-110 typical timing values, used as the H10721-110
// internal-PMT timing approximation.
G4double pmtElectronTransitTimeSeconds = 2.7e-9;
G4double pmtTransitTimeSpreadFwhmSeconds = 0.2e-9;

G4int pmtWaveformSaveCount = 5;
G4int savedWaveformCount = 0;

G4int totalEdepCoincidence = 0;
G4int totalEdepHitDet1 = 0;
G4int totalEdepHitDet2 = 0;

G4double totalEdepMeVDet1 = 0.;
G4double totalEdepMeVDet2 = 0.;
G4double totalEdepSquaredMeV2Det1 = 0.;
G4double totalEdepSquaredMeV2Det2 = 0.;

G4int totalPMTHitDet1 = 0;
G4int totalPMTHitDet2 = 0;
G4int totalPMTVoltageCoincidence = 0;

G4int totalPMTPhotonEventsDet1 = 0;
G4int totalPMTPhotonEventsDet2 = 0;
G4int totalBothPMTPhotonEvents = 0;

G4int totalPhotoelectronEventsDet1 = 0;
G4int totalPhotoelectronEventsDet2 = 0;
G4int totalBothPhotoelectronEvents = 0;

G4long totalPMTPhotonsDet1 = 0;
G4long totalPMTPhotonsDet2 = 0;
G4long totalPhotoelectronsDet1 = 0;
G4long totalPhotoelectronsDet2 = 0;

G4double totalExpectedPhotoelectronsDet1 = 0.;
G4double totalExpectedPhotoelectronsDet2 = 0.;

G4double totalPMTEnergyMeVDet1 = 0.;
G4double totalPMTEnergyMeVDet2 = 0.;

G4double totalPeakVoltageDet1 = 0.;
G4double totalPeakVoltageDet2 = 0.;

G4long totalPMTPhotonsDet1InHitEvents = 0;
G4long totalPMTPhotonsDet2InHitEvents = 0;

G4double totalPeakVoltageDet1InHitEvents = 0.;
G4double totalPeakVoltageDet2InHitEvents = 0.;

G4long totalPMTPhotonsDet1InCoincidenceEvents = 0;
G4long totalPMTPhotonsDet2InCoincidenceEvents = 0;

G4double totalPeakVoltageDet1InCoincidenceEvents = 0.;
G4double totalPeakVoltageDet2InCoincidenceEvents = 0.;

G4long totalPMTTimingPhotonsDet1 = 0;
G4long totalPMTTimingPhotonsDet2 = 0;

G4double totalPMTTransportTimeNsDet1 = 0.;
G4double totalPMTTransportTimeNsDet2 = 0.;

G4double totalPMTPathLengthCmDet1 = 0.;
G4double totalPMTPathLengthCmDet2 = 0.;

G4double totalPMTSpeedCmPerNsDet1 = 0.;
G4double totalPMTSpeedCmPerNsDet2 = 0.;

G4double minPMTTransportTimeNsDet1 =
    std::numeric_limits<G4double>::max();

G4double minPMTTransportTimeNsDet2 =
    std::numeric_limits<G4double>::max();

G4double maxPMTTransportTimeNsDet1 = 0.;
G4double maxPMTTransportTimeNsDet2 = 0.;

G4double totalFirstTransitTimeNsDet1 = 0.;
G4double totalFirstTransitTimeNsDet2 = 0.;
G4double totalFirstTransitTimeSquaredNs2Det1 = 0.;
G4double totalFirstTransitTimeSquaredNs2Det2 = 0.;

G4double totalMeanTransitTimeNsDet1 = 0.;
G4double totalMeanTransitTimeNsDet2 = 0.;
G4double totalMeanTransitTimeSquaredNs2Det1 = 0.;
G4double totalMeanTransitTimeSquaredNs2Det2 = 0.;

G4int totalFirstTransitEntriesDet1 = 0;
G4int totalFirstTransitEntriesDet2 = 0;
G4int totalMeanTransitEntriesDet1 = 0;
G4int totalMeanTransitEntriesDet2 = 0;

G4double totalDeltaFirstTransitTimeNs = 0.;
G4double totalDeltaFirstTransitTimeSquaredNs2 = 0.;
G4int totalDeltaFirstTransitEntries = 0;

namespace
{
    constexpr G4double electronChargeCoulomb =
        1.602176634e-19;

    constexpr G4int qePointCount = 19;

    const G4double qeWavelengthNm[qePointCount] =
    {
        230., 250., 275., 300., 325.,
        350., 375., 400., 425., 450.,
        475., 500., 525., 550., 575.,
        600., 625., 650., 700.
    };

    const G4double qeFraction[qePointCount] =
    {
        0.000, 0.040, 0.150, 0.270, 0.330,
        0.350, 0.350, 0.341, 0.326, 0.305,
        0.275, 0.240, 0.200, 0.155, 0.112,
        0.075, 0.050, 0.025, 0.000
    };

    G4double PMTGain()
    {
        if (pmtCathodeRadiantSensitivityPeakAW <= 0.)
        {
            return 0.;
        }

        return
            pmtAnodeRadiantSensitivityAW /
            pmtCathodeRadiantSensitivityPeakAW;
    }

    G4double PMTImpulseResponsePeakPerSecond()
    {
        if (pmtRiseTimeSeconds <= 0. ||
            pmtFallTimeSeconds <= 0. ||
            pmtFallTimeSeconds <= pmtRiseTimeSeconds)
        {
            return 0.;
        }

        const G4double tauRise =
            pmtRiseTimeSeconds;

        const G4double tauFall =
            pmtFallTimeSeconds;

        const G4double tPeak =
            std::log(tauFall / tauRise) /
            ((1.0 / tauRise) -
             (1.0 / tauFall));

        return
            (std::exp(-tPeak / tauFall) -
             std::exp(-tPeak / tauRise)) /
            (tauFall - tauRise);
    }

    G4double SafeMean(
        G4double sum,
        G4long count
    )
    {
        return
            (count > 0)
            ? sum /
              static_cast<G4double>(count)
            : 0.;
    }

    G4double SafeRMS(
        G4double sum,
        G4double sumSquares,
        G4long count
    )
    {
        if (count <= 0)
        {
            return 0.;
        }

        const G4double mean =
            sum /
            static_cast<G4double>(count);

        G4double variance =
            sumSquares /
            static_cast<G4double>(count) -
            mean * mean;

        if (variance < 0. &&
            variance > -1.0e-12)
        {
            variance = 0.;
        }

        return
            (variance > 0.)
            ? std::sqrt(variance)
            : 0.;
    }

    void ResetRunCounters()
    {
        savedWaveformCount = 0;

        totalEdepCoincidence = 0;
        totalEdepHitDet1 = 0;
        totalEdepHitDet2 = 0;

        totalEdepMeVDet1 = 0.;
        totalEdepMeVDet2 = 0.;
        totalEdepSquaredMeV2Det1 = 0.;
        totalEdepSquaredMeV2Det2 = 0.;

        totalPMTHitDet1 = 0;
        totalPMTHitDet2 = 0;
        totalPMTVoltageCoincidence = 0;

        totalPMTPhotonEventsDet1 = 0;
        totalPMTPhotonEventsDet2 = 0;
        totalBothPMTPhotonEvents = 0;

        totalPhotoelectronEventsDet1 = 0;
        totalPhotoelectronEventsDet2 = 0;
        totalBothPhotoelectronEvents = 0;

        totalPMTPhotonsDet1 = 0;
        totalPMTPhotonsDet2 = 0;
        totalPhotoelectronsDet1 = 0;
        totalPhotoelectronsDet2 = 0;

        totalExpectedPhotoelectronsDet1 = 0.;
        totalExpectedPhotoelectronsDet2 = 0.;

        totalPMTEnergyMeVDet1 = 0.;
        totalPMTEnergyMeVDet2 = 0.;

        totalPeakVoltageDet1 = 0.;
        totalPeakVoltageDet2 = 0.;

        totalPMTPhotonsDet1InHitEvents = 0;
        totalPMTPhotonsDet2InHitEvents = 0;

        totalPeakVoltageDet1InHitEvents = 0.;
        totalPeakVoltageDet2InHitEvents = 0.;

        totalPMTPhotonsDet1InCoincidenceEvents = 0;
        totalPMTPhotonsDet2InCoincidenceEvents = 0;

        totalPeakVoltageDet1InCoincidenceEvents = 0.;
        totalPeakVoltageDet2InCoincidenceEvents = 0.;

        totalPMTTimingPhotonsDet1 = 0;
        totalPMTTimingPhotonsDet2 = 0;

        totalPMTTransportTimeNsDet1 = 0.;
        totalPMTTransportTimeNsDet2 = 0.;

        totalPMTPathLengthCmDet1 = 0.;
        totalPMTPathLengthCmDet2 = 0.;

        totalPMTSpeedCmPerNsDet1 = 0.;
        totalPMTSpeedCmPerNsDet2 = 0.;

        minPMTTransportTimeNsDet1 =
            std::numeric_limits<G4double>::max();

        minPMTTransportTimeNsDet2 =
            std::numeric_limits<G4double>::max();

        maxPMTTransportTimeNsDet1 = 0.;
        maxPMTTransportTimeNsDet2 = 0.;

        totalFirstTransitTimeNsDet1 = 0.;
        totalFirstTransitTimeNsDet2 = 0.;
        totalFirstTransitTimeSquaredNs2Det1 = 0.;
        totalFirstTransitTimeSquaredNs2Det2 = 0.;

        totalMeanTransitTimeNsDet1 = 0.;
        totalMeanTransitTimeNsDet2 = 0.;
        totalMeanTransitTimeSquaredNs2Det1 = 0.;
        totalMeanTransitTimeSquaredNs2Det2 = 0.;

        totalFirstTransitEntriesDet1 = 0;
        totalFirstTransitEntriesDet2 = 0;
        totalMeanTransitEntriesDet1 = 0;
        totalMeanTransitEntriesDet2 = 0;

        totalDeltaFirstTransitTimeNs = 0.;
        totalDeltaFirstTransitTimeSquaredNs2 = 0.;
        totalDeltaFirstTransitEntries = 0;
    }

    void WriteQECurveCSV()
    {
        std::ofstream out(
            "pmt_qe_curve.csv"
        );

        if (!out.is_open())
        {
            return;
        }

        out
            << "wavelength_nm,"
            << "quantum_efficiency_fraction,"
            << "quantum_efficiency_percent\n";

        for (G4int i = 0;
             i < qePointCount;
             i++)
        {
            out
                << qeWavelengthNm[i]
                << ","
                << qeFraction[i]
                << ","
                << 100. * qeFraction[i]
                << "\n";
        }
    }
}

RunAction::RunAction()
: G4UserRunAction()
{}

RunAction::~RunAction()
{}

void RunAction::BeginOfRunAction(
    const G4Run*
)
{
    if (!IsMaster())
    {
        return;
    }

    ResetRunCounters();
    WriteQECurveCSV();

    {
        std::ofstream out(
            "pmt_event_analysis.csv"
        );

        if (out.is_open())
        {
            out
                << "event_id,"
                << "edep_det1_MeV,"
                << "edep_det2_MeV,"
                << "edep_hit_det1,"
                << "edep_hit_det2,"
                << "edep_coincidence,"
                << "photons_pmt1,"
                << "photons_pmt2,"
                << "photoelectrons_pmt1,"
                << "photoelectrons_pmt2,"
                << "expected_photoelectrons_pmt1,"
                << "expected_photoelectrons_pmt2,"
                << "mean_qe_pmt1,"
                << "mean_qe_pmt2,"
                << "first_transport_pmt1_ns,"
                << "first_transport_pmt2_ns,"
                << "mean_transport_pmt1_ns,"
                << "mean_transport_pmt2_ns,"
                << "delta_first_pmt2_minus_pmt1_ns,"
                << "peak_pmt1_mV,"
                << "peak_pmt2_mV,"
                << "voltage_hit_pmt1,"
                << "voltage_hit_pmt2,"
                << "voltage_coincidence\n";
        }
    }

    if (pmtWaveformSaveCount > 0)
    {
        std::ofstream summary(
            "pmt_waveform_summary.csv"
        );

        if (summary.is_open())
        {
            summary
                << "saved_index,"
                << "geant_event_id,"
                << "filename,"
                << "det1_photoelectrons,"
                << "det2_photoelectrons,"
                << "det1_peak_mV,"
                << "det2_peak_mV\n";
        }
    }
}

void RunAction::EndOfRunAction(
    const G4Run* run
)
{
    if (!IsMaster())
    {
        return;
    }

    const G4int nEvents =
        run->GetNumberOfEvent();

    const G4double pmtGain =
        PMTGain();

    const G4double singlePhotoelectronPeakVolt =
        electronChargeCoulomb *
        pmtGain *
        PMTImpulseResponsePeakPerSecond() *
        pmtLoadResistanceOhm *
        pmtVoltageCalibrationFactor;

    const G4double qeAt425Nm =
        0.326;

    const G4double meanPhotonPeak425Volt =
        qeAt425Nm *
        singlePhotoelectronPeakVolt;

    const G4double edepCoincidenceEfficiency =
        (nEvents > 0)
        ? static_cast<G4double>(
              totalEdepCoincidence
          ) / nEvents
        : 0.;

    const G4double pmtCoincidenceEfficiency =
        (nEvents > 0)
        ? static_cast<G4double>(
              totalPMTVoltageCoincidence
          ) / nEvents
        : 0.;

    const G4double pmtOverEdepEfficiency =
        (totalEdepCoincidence > 0)
        ? static_cast<G4double>(
              totalPMTVoltageCoincidence
          ) / totalEdepCoincidence
        : 0.;

    const G4double expectedBC408SpeedCmPerNs =
        29.9792458 / 1.58;

    const G4double firstMeanDet1Ns =
        SafeMean(
            totalFirstTransitTimeNsDet1,
            totalFirstTransitEntriesDet1
        );

    const G4double firstMeanDet2Ns =
        SafeMean(
            totalFirstTransitTimeNsDet2,
            totalFirstTransitEntriesDet2
        );

    const G4double firstRMSDet1Ns =
        SafeRMS(
            totalFirstTransitTimeNsDet1,
            totalFirstTransitTimeSquaredNs2Det1,
            totalFirstTransitEntriesDet1
        );

    const G4double firstRMSDet2Ns =
        SafeRMS(
            totalFirstTransitTimeNsDet2,
            totalFirstTransitTimeSquaredNs2Det2,
            totalFirstTransitEntriesDet2
        );

    const G4double meanTransitDet1Ns =
        SafeMean(
            totalMeanTransitTimeNsDet1,
            totalMeanTransitEntriesDet1
        );

    const G4double meanTransitDet2Ns =
        SafeMean(
            totalMeanTransitTimeNsDet2,
            totalMeanTransitEntriesDet2
        );

    const G4double meanTransitRMSDet1Ns =
        SafeRMS(
            totalMeanTransitTimeNsDet1,
            totalMeanTransitTimeSquaredNs2Det1,
            totalMeanTransitEntriesDet1
        );

    const G4double meanTransitRMSDet2Ns =
        SafeRMS(
            totalMeanTransitTimeNsDet2,
            totalMeanTransitTimeSquaredNs2Det2,
            totalMeanTransitEntriesDet2
        );

    const G4double deltaFirstMeanNs =
        SafeMean(
            totalDeltaFirstTransitTimeNs,
            totalDeltaFirstTransitEntries
        );

    const G4double deltaFirstRMSNs =
        SafeRMS(
            totalDeltaFirstTransitTimeNs,
            totalDeltaFirstTransitTimeSquaredNs2,
            totalDeltaFirstTransitEntries
        );

    std::ostringstream report;

    report
        << std::setprecision(8)
        << "\n"
        << "============================================================\n"
        << "        SIMULASYON TAMAMLANDI - GERCEKCI QE MODELI\n"
        << "============================================================\n"
        << "Number of events                       : "
        << nEvents << "\n"
        << "------------------------------------------------------------\n"
        << "PMT QE / GAIN MODEL\n"
        << "Photocathode model                     : H10721-110 / R9880U-110 SBA approximation\n"
        << "QE model                               : wavelength dependent\n"
        << "QE at 400 nm                           : 34.1 %\n"
        << "QE at 425 nm                           : 32.6 % approx.\n"
        << "Cathode radiant sensitivity at peak    : "
        << pmtCathodeRadiantSensitivityPeakAW
        << " A/W\n"
        << "Anode radiant sensitivity at peak      : "
        << pmtAnodeRadiantSensitivityAW
        << " A/W\n"
        << "Derived PMT gain                       : "
        << pmtGain << "\n"
        << "Electron transit time                  : "
        << 1.0e9 * pmtElectronTransitTimeSeconds
        << " ns\n"
        << "Transit time spread FWHM               : "
        << 1.0e9 * pmtTransitTimeSpreadFwhmSeconds
        << " ns\n"
        << "Single-photoelectron waveform peak     : "
        << 1.0e3 * singlePhotoelectronPeakVolt
        << " mV\n"
        << "Mean 425 nm photon peak contribution   : "
        << 1.0e3 * meanPhotonPeak425Volt
        << " mV\n"
        << "------------------------------------------------------------\n"
        << "ENERGY DEPOSIT\n"
        << "Det1 total deposited energy            : "
        << totalEdepMeVDet1 << " MeV\n"
        << "Det2 total deposited energy            : "
        << totalEdepMeVDet2 << " MeV\n"
        << "Det1 energy-deposit hit count          : "
        << totalEdepHitDet1 << "\n"
        << "Det2 energy-deposit hit count          : "
        << totalEdepHitDet2 << "\n"
        << "Energy-deposit coincidence count       : "
        << totalEdepCoincidence << "\n"
        << "Energy-deposit coincidence fraction    : "
        << edepCoincidenceEfficiency << "\n"
        << "------------------------------------------------------------\n"
        << "PHOTONS AND PHOTOELECTRONS\n"
        << "Events with PMT1 photons               : "
        << totalPMTPhotonEventsDet1 << "\n"
        << "Events with PMT2 photons               : "
        << totalPMTPhotonEventsDet2 << "\n"
        << "Events with photons at both PMTs       : "
        << totalBothPMTPhotonEvents << "\n"
        << "Events with PMT1 photoelectrons        : "
        << totalPhotoelectronEventsDet1 << "\n"
        << "Events with PMT2 photoelectrons        : "
        << totalPhotoelectronEventsDet2 << "\n"
        << "Events with PE at both PMTs            : "
        << totalBothPhotoelectronEvents << "\n"
        << "Photons reaching PMT1                  : "
        << totalPMTPhotonsDet1 << "\n"
        << "Photons reaching PMT2                  : "
        << totalPMTPhotonsDet2 << "\n"
        << "Expected photoelectrons PMT1           : "
        << totalExpectedPhotoelectronsDet1 << "\n"
        << "Expected photoelectrons PMT2           : "
        << totalExpectedPhotoelectronsDet2 << "\n"
        << "Generated photoelectrons PMT1          : "
        << totalPhotoelectronsDet1 << "\n"
        << "Generated photoelectrons PMT2          : "
        << totalPhotoelectronsDet2 << "\n"
        << "Observed PE/photon ratio PMT1          : "
        << ((totalPMTPhotonsDet1 > 0)
            ? static_cast<G4double>(
                  totalPhotoelectronsDet1
              ) / totalPMTPhotonsDet1
            : 0.) << "\n"
        << "Observed PE/photon ratio PMT2          : "
        << ((totalPMTPhotonsDet2 > 0)
            ? static_cast<G4double>(
                  totalPhotoelectronsDet2
              ) / totalPMTPhotonsDet2
            : 0.) << "\n"
        << "------------------------------------------------------------\n"
        << "EVENT-LEVEL OPTICAL TIMING\n"
        << "PMT1 first-photon mean / RMS           : "
        << firstMeanDet1Ns << " / "
        << firstRMSDet1Ns << " ns\n"
        << "PMT2 first-photon mean / RMS           : "
        << firstMeanDet2Ns << " / "
        << firstRMSDet2Ns << " ns\n"
        << "PMT1 event-mean transit mean / RMS     : "
        << meanTransitDet1Ns << " / "
        << meanTransitRMSDet1Ns << " ns\n"
        << "PMT2 event-mean transit mean / RMS     : "
        << meanTransitDet2Ns << " / "
        << meanTransitRMSDet2Ns << " ns\n"
        << "Delta t = t2 - t1 mean / RMS           : "
        << deltaFirstMeanNs << " / "
        << deltaFirstRMSNs << " ns\n"
        << "------------------------------------------------------------\n"
        << "PMT WAVEFORM ELECTRONICS\n"
        << "Voltage threshold                      : "
        << 1.0e3 * pmtVoltageThresholdVolt
        << " mV\n"
        << "Load resistance                        : "
        << pmtLoadResistanceOhm
        << " ohm\n"
        << "Rise time                              : "
        << 1.0e9 * pmtRiseTimeSeconds
        << " ns\n"
        << "Fall time                              : "
        << 1.0e9 * pmtFallTimeSeconds
        << " ns\n"
        << "Waveform time step                     : "
        << 1.0e9 * pmtWaveformTimeStepSeconds
        << " ns\n"
        << "Voltage calibration factor             : "
        << pmtVoltageCalibrationFactor << "\n"
        << "Det1 PMT voltage hit events            : "
        << totalPMTHitDet1 << "\n"
        << "Det2 PMT voltage hit events            : "
        << totalPMTHitDet2 << "\n"
        << "PMT voltage coincidence                : "
        << totalPMTVoltageCoincidence << "\n"
        << "PMT coincidence fraction / fired muon  : "
        << pmtCoincidenceEfficiency << "\n"
        << "PMT coincidence / edep coincidence     : "
        << pmtOverEdepEfficiency << "\n"
        << "Saved waveform count                   : "
        << savedWaveformCount << "\n"
        << "------------------------------------------------------------\n"
        << "OPTICAL TRANSPORT\n"
        << "Expected BC-408 photon speed c/n       : "
        << expectedBC408SpeedCmPerNs
        << " cm/ns\n";

    if (totalPMTTimingPhotonsDet1 > 0)
    {
        report
            << "PMT1 timing photon count               : "
            << totalPMTTimingPhotonsDet1 << "\n"
            << "PMT1 avg photon transport time         : "
            << totalPMTTransportTimeNsDet1 /
               totalPMTTimingPhotonsDet1
            << " ns\n"
            << "PMT1 min/max transport time            : "
            << minPMTTransportTimeNsDet1
            << " / "
            << maxPMTTransportTimeNsDet1
            << " ns\n"
            << "PMT1 avg optical path length           : "
            << totalPMTPathLengthCmDet1 /
               totalPMTTimingPhotonsDet1
            << " cm\n"
            << "PMT1 avg effective photon speed        : "
            << totalPMTSpeedCmPerNsDet1 /
               totalPMTTimingPhotonsDet1
            << " cm/ns\n";
    }

    if (totalPMTTimingPhotonsDet2 > 0)
    {
        report
            << "PMT2 timing photon count               : "
            << totalPMTTimingPhotonsDet2 << "\n"
            << "PMT2 avg photon transport time         : "
            << totalPMTTransportTimeNsDet2 /
               totalPMTTimingPhotonsDet2
            << " ns\n"
            << "PMT2 min/max transport time            : "
            << minPMTTransportTimeNsDet2
            << " / "
            << maxPMTTransportTimeNsDet2
            << " ns\n"
            << "PMT2 avg optical path length           : "
            << totalPMTPathLengthCmDet2 /
               totalPMTTimingPhotonsDet2
            << " cm\n"
            << "PMT2 avg effective photon speed        : "
            << totalPMTSpeedCmPerNsDet2 /
               totalPMTTimingPhotonsDet2
            << " cm/ns\n";
    }

    report
        << "------------------------------------------------------------\n"
        << "Generated files:\n"
        << "  pmt_qe_curve.csv\n"
        << "  pmt_event_analysis.csv\n"
        << "  pmt_analysis_summary.txt\n"
        << "  pmt_waveform_summary.csv\n"
        << "============================================================\n";

    G4cout
        << report.str()
        << G4endl;

    std::ofstream summaryOut(
        "pmt_analysis_summary.txt"
    );

    if (summaryOut.is_open())
    {
        summaryOut <<
            report.str();
    }
}

}
