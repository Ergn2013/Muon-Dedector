#include "EventAction.hh"

#include "G4AutoLock.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <vector>

namespace B1
{
    extern G4double pmtVoltageThresholdVolt;
    extern G4double pmtLoadResistanceOhm;
    extern G4double pmtAnodeRadiantSensitivityAW;
    extern G4double pmtCathodeRadiantSensitivityPeakAW;

    extern G4double pmtRiseTimeSeconds;
    extern G4double pmtFallTimeSeconds;
    extern G4double pmtWaveformTimeStepSeconds;
    extern G4double pmtVoltageCalibrationFactor;

    extern G4double pmtElectronTransitTimeSeconds;
    extern G4double pmtTransitTimeSpreadFwhmSeconds;

    extern G4int pmtWaveformSaveCount;
    extern G4int savedWaveformCount;

    extern G4int totalEdepCoincidence;
    extern G4int totalEdepHitDet1;
    extern G4int totalEdepHitDet2;

    extern G4double totalEdepMeVDet1;
    extern G4double totalEdepMeVDet2;
    extern G4double totalEdepSquaredMeV2Det1;
    extern G4double totalEdepSquaredMeV2Det2;

    extern G4int totalPMTHitDet1;
    extern G4int totalPMTHitDet2;
    extern G4int totalPMTVoltageCoincidence;

    extern G4int totalPMTPhotonEventsDet1;
    extern G4int totalPMTPhotonEventsDet2;
    extern G4int totalBothPMTPhotonEvents;

    extern G4int totalPhotoelectronEventsDet1;
    extern G4int totalPhotoelectronEventsDet2;
    extern G4int totalBothPhotoelectronEvents;

    extern G4long totalPMTPhotonsDet1;
    extern G4long totalPMTPhotonsDet2;
    extern G4long totalPhotoelectronsDet1;
    extern G4long totalPhotoelectronsDet2;

    extern G4double totalExpectedPhotoelectronsDet1;
    extern G4double totalExpectedPhotoelectronsDet2;

    extern G4double totalPMTEnergyMeVDet1;
    extern G4double totalPMTEnergyMeVDet2;

    extern G4double totalPeakVoltageDet1;
    extern G4double totalPeakVoltageDet2;

    extern G4long totalPMTPhotonsDet1InHitEvents;
    extern G4long totalPMTPhotonsDet2InHitEvents;

    extern G4double totalPeakVoltageDet1InHitEvents;
    extern G4double totalPeakVoltageDet2InHitEvents;

    extern G4long totalPMTPhotonsDet1InCoincidenceEvents;
    extern G4long totalPMTPhotonsDet2InCoincidenceEvents;

    extern G4double totalPeakVoltageDet1InCoincidenceEvents;
    extern G4double totalPeakVoltageDet2InCoincidenceEvents;

    extern G4long totalPMTTimingPhotonsDet1;
    extern G4long totalPMTTimingPhotonsDet2;

    extern G4double totalPMTTransportTimeNsDet1;
    extern G4double totalPMTTransportTimeNsDet2;

    extern G4double totalPMTPathLengthCmDet1;
    extern G4double totalPMTPathLengthCmDet2;

    extern G4double totalPMTSpeedCmPerNsDet1;
    extern G4double totalPMTSpeedCmPerNsDet2;

    extern G4double minPMTTransportTimeNsDet1;
    extern G4double minPMTTransportTimeNsDet2;

    extern G4double maxPMTTransportTimeNsDet1;
    extern G4double maxPMTTransportTimeNsDet2;

    extern G4double totalFirstTransitTimeNsDet1;
    extern G4double totalFirstTransitTimeNsDet2;
    extern G4double totalFirstTransitTimeSquaredNs2Det1;
    extern G4double totalFirstTransitTimeSquaredNs2Det2;

    extern G4double totalMeanTransitTimeNsDet1;
    extern G4double totalMeanTransitTimeNsDet2;
    extern G4double totalMeanTransitTimeSquaredNs2Det1;
    extern G4double totalMeanTransitTimeSquaredNs2Det2;

    extern G4int totalFirstTransitEntriesDet1;
    extern G4int totalFirstTransitEntriesDet2;
    extern G4int totalMeanTransitEntriesDet1;
    extern G4int totalMeanTransitEntriesDet2;

    extern G4double totalDeltaFirstTransitTimeNs;
    extern G4double totalDeltaFirstTransitTimeSquaredNs2;
    extern G4int totalDeltaFirstTransitEntries;
}

namespace
{
    G4Mutex eventActionMutex = G4MUTEX_INITIALIZER;

    constexpr G4double electronChargeCoulomb =
        1.602176634e-19;

    constexpr G4double hcEvNm =
        1239.8419843320026;

    constexpr G4double twoPi =
        6.2831853071795864769;

    struct PMTWaveform
    {
        std::vector<G4double> timeNs;
        std::vector<G4double> voltageVolt;
        G4double peakVoltageVolt = 0.;
    };

    struct PMTTimingStats
    {
        G4bool valid = false;

        G4double firstTransportTimeNs =
            std::numeric_limits<G4double>::quiet_NaN();

        G4double meanTransportTimeNs =
            std::numeric_limits<G4double>::quiet_NaN();
    };

    struct PMTPhotoelectronResult
    {
        std::vector<G4double> anodeArrivalTimeNs;

        G4double expectedPhotoelectrons = 0.;
        G4double meanQuantumEfficiency = 0.;
    };

    // Approximate wavelength-dependent QE curve for the Hamamatsu
    // R9880U-110 super-bialkali photocathode, used as the spectral
    // response model for the H10721-110 PMT module. Official values:
    // 8 mm active area, 230-700 nm response, 400 nm peak wavelength,
    // 110 mA/W cathode radiant sensitivity and about 34.1% QE at 400 nm.
    //
    // Hamamatsu publishes the QE response as a graph rather than a
    // numerical table. The points below are a digitised/approximated
    // typical curve; linear interpolation is used between points.
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

    G4double PhotonWavelengthNm(
        G4double photonEnergyMeV
    )
    {
        if (photonEnergyMeV <= 0.)
        {
            return 0.;
        }

        const G4double energyEv =
            photonEnergyMeV * 1.0e6;

        return hcEvNm / energyEv;
    }

    G4double QuantumEfficiencyFromWavelengthNm(
        G4double wavelengthNm
    )
    {
        if (wavelengthNm <= qeWavelengthNm[0] ||
            wavelengthNm >= qeWavelengthNm[qePointCount - 1])
        {
            return 0.;
        }

        for (G4int i = 0; i < qePointCount - 1; i++)
        {
            const G4double lambda0 =
                qeWavelengthNm[i];

            const G4double lambda1 =
                qeWavelengthNm[i + 1];

            if (wavelengthNm >= lambda0 &&
                wavelengthNm <= lambda1)
            {
                const G4double fraction =
                    (wavelengthNm - lambda0) /
                    (lambda1 - lambda0);

                return
                    qeFraction[i] +
                    fraction *
                    (qeFraction[i + 1] -
                     qeFraction[i]);
            }
        }

        return 0.;
    }

    G4double GaussianRandom(
        G4double sigma
    )
    {
        if (sigma <= 0.)
        {
            return 0.;
        }

        G4double u1 = G4UniformRand();

        if (u1 < 1.0e-15)
        {
            u1 = 1.0e-15;
        }

        const G4double u2 =
            G4UniformRand();

        return
            sigma *
            std::sqrt(-2.0 * std::log(u1)) *
            std::cos(twoPi * u2);
    }

    G4double PMTImpulseResponsePerSecond(
        G4double timeSeconds
    )
    {
        if (timeSeconds < 0.)
        {
            return 0.;
        }

        if (B1::pmtRiseTimeSeconds <= 0. ||
            B1::pmtFallTimeSeconds <= 0. ||
            B1::pmtFallTimeSeconds <=
            B1::pmtRiseTimeSeconds)
        {
            return 0.;
        }

        const G4double tauRise =
            B1::pmtRiseTimeSeconds;

        const G4double tauFall =
            B1::pmtFallTimeSeconds;

        return
            (std::exp(-timeSeconds / tauFall) -
             std::exp(-timeSeconds / tauRise)) /
            (tauFall - tauRise);
    }

    G4double PMTGain()
    {
        if (B1::pmtCathodeRadiantSensitivityPeakAW <= 0.)
        {
            return 0.;
        }

        return
            B1::pmtAnodeRadiantSensitivityAW /
            B1::pmtCathodeRadiantSensitivityPeakAW;
    }

    G4bool GetPhotonTimeLimits(
        const std::vector<PMTPhotonHit>& photons,
        G4double& minTimeNs,
        G4double& maxTimeNs
    )
    {
        if (photons.empty())
        {
            return false;
        }

        minTimeNs =
            std::numeric_limits<G4double>::max();

        maxTimeNs =
            -std::numeric_limits<G4double>::max();

        for (const auto& photon : photons)
        {
            if (photon.arrivalTimeNs < minTimeNs)
            {
                minTimeNs = photon.arrivalTimeNs;
            }

            if (photon.arrivalTimeNs > maxTimeNs)
            {
                maxTimeNs = photon.arrivalTimeNs;
            }
        }

        return true;
    }

    PMTTimingStats CalculateTimingStats(
        const std::vector<PMTPhotonHit>& photons
    )
    {
        PMTTimingStats stats;

        if (photons.empty())
        {
            return stats;
        }

        G4double firstTimeNs =
            std::numeric_limits<G4double>::max();

        G4double sumTimeNs = 0.;
        G4int validCount = 0;

        for (const auto& photon : photons)
        {
            if (photon.transportTimeNs < 0.)
            {
                continue;
            }

            if (photon.transportTimeNs < firstTimeNs)
            {
                firstTimeNs =
                    photon.transportTimeNs;
            }

            sumTimeNs +=
                photon.transportTimeNs;

            validCount++;
        }

        if (validCount <= 0)
        {
            return stats;
        }

        stats.valid = true;

        stats.firstTransportTimeNs =
            firstTimeNs;

        stats.meanTransportTimeNs =
            sumTimeNs /
            static_cast<G4double>(validCount);

        return stats;
    }

    PMTPhotoelectronResult GeneratePhotoelectrons(
        const std::vector<PMTPhotonHit>& photons
    )
    {
        PMTPhotoelectronResult result;

        result.anodeArrivalTimeNs.reserve(
            photons.size()
        );

        const G4double electronTransitTimeNs =
            B1::pmtElectronTransitTimeSeconds *
            1.0e9;

        // Datasheet T.T.S. is FWHM. Convert it to Gaussian sigma.
        const G4double transitTimeSigmaNs =
            (B1::pmtTransitTimeSpreadFwhmSeconds *
             1.0e9) / 2.354820045;

        G4double qeSum = 0.;

        for (const auto& photon : photons)
        {
            const G4double wavelengthNm =
                PhotonWavelengthNm(
                    photon.energyMeV
                );

            G4double qe =
                QuantumEfficiencyFromWavelengthNm(
                    wavelengthNm
                );

            if (qe < 0.)
            {
                qe = 0.;
            }
            else if (qe > 1.)
            {
                qe = 1.;
            }

            qeSum += qe;
            result.expectedPhotoelectrons += qe;

            if (G4UniformRand() < qe)
            {
                G4double anodeTimeNs =
                    photon.arrivalTimeNs +
                    electronTransitTimeNs +
                    GaussianRandom(
                        transitTimeSigmaNs
                    );

                result.anodeArrivalTimeNs.push_back(
                    anodeTimeNs
                );
            }
        }

        if (!photons.empty())
        {
            result.meanQuantumEfficiency =
                qeSum /
                static_cast<G4double>(
                    photons.size()
                );
        }

        return result;
    }

    G4bool GetTimeLimits(
        const std::vector<G4double>& timesNs,
        G4double& minTimeNs,
        G4double& maxTimeNs
    )
    {
        if (timesNs.empty())
        {
            return false;
        }

        minTimeNs =
            std::numeric_limits<G4double>::max();

        maxTimeNs =
            -std::numeric_limits<G4double>::max();

        for (const G4double timeNs : timesNs)
        {
            if (timeNs < minTimeNs)
            {
                minTimeNs = timeNs;
            }

            if (timeNs > maxTimeNs)
            {
                maxTimeNs = timeNs;
            }
        }

        return true;
    }

    PMTWaveform BuildPMTWaveform(
        const std::vector<G4double>& photoelectronTimesNs,
        G4double tStartNs,
        G4double tEndNs
    )
    {
        PMTWaveform waveform;

        if (photoelectronTimesNs.empty())
        {
            return waveform;
        }

        if (B1::pmtLoadResistanceOhm <= 0. ||
            B1::pmtRiseTimeSeconds <= 0. ||
            B1::pmtFallTimeSeconds <= 0. ||
            B1::pmtWaveformTimeStepSeconds <= 0. ||
            PMTGain() <= 0.)
        {
            return waveform;
        }

        if (tEndNs <= tStartNs)
        {
            tEndNs = tStartNs + 1.0;
        }

        const G4double dtSeconds =
            B1::pmtWaveformTimeStepSeconds;

        const G4double dtNs =
            dtSeconds * 1.0e9;

        const G4double responseTailNs =
            8.0 *
            B1::pmtFallTimeSeconds *
            1.0e9;

        const G4double peWindowNs =
            tEndNs - tStartNs;

        const G4int peBinCount =
            static_cast<G4int>(
                std::ceil(peWindowNs / dtNs)
            ) + 2;

        const G4int responseBinCount =
            static_cast<G4int>(
                std::ceil(responseTailNs / dtNs)
            ) + 2;

        const G4int waveformBinCount =
            peBinCount +
            responseBinCount +
            2;

        std::vector<G4int> photoelectronsByBin(
            peBinCount,
            0
        );

        for (const G4double timeNs :
             photoelectronTimesNs)
        {
            const G4int bin =
                static_cast<G4int>(
                    std::floor(
                        (timeNs - tStartNs) /
                        dtNs
                    )
                );

            if (bin >= 0 &&
                bin < peBinCount)
            {
                photoelectronsByBin[bin]++;
            }
        }

        waveform.timeNs.resize(
            waveformBinCount
        );

        waveform.voltageVolt.resize(
            waveformBinCount
        );

        const G4double chargePerPhotoelectronCoulomb =
            electronChargeCoulomb *
            PMTGain();

        for (G4int k = 0;
             k < waveformBinCount;
             k++)
        {
            G4double voltageVolt = 0.;

            const G4int maxResponseIndex =
                (k < responseBinCount)
                ? k
                : responseBinCount - 1;

            for (G4int r = 0;
                 r <= maxResponseIndex;
                 r++)
            {
                const G4int sourceBin =
                    k - r;

                if (sourceBin < 0 ||
                    sourceBin >= peBinCount)
                {
                    continue;
                }

                const G4int peCount =
                    photoelectronsByBin[
                        sourceBin
                    ];

                if (peCount <= 0)
                {
                    continue;
                }

                const G4double responseTimeSeconds =
                    static_cast<G4double>(r) *
                    dtSeconds;

                const G4double currentAmpere =
                    static_cast<G4double>(peCount) *
                    chargePerPhotoelectronCoulomb *
                    PMTImpulseResponsePerSecond(
                        responseTimeSeconds
                    );

                voltageVolt +=
                    currentAmpere *
                    B1::pmtLoadResistanceOhm *
                    B1::pmtVoltageCalibrationFactor;
            }

            waveform.timeNs[k] =
                tStartNs +
                static_cast<G4double>(k) *
                dtNs;

            waveform.voltageVolt[k] =
                voltageVolt;

            if (voltageVolt >
                waveform.peakVoltageVolt)
            {
                waveform.peakVoltageVolt =
                    voltageVolt;
            }
        }

        return waveform;
    }

    PMTWaveform BuildPMTWaveformAuto(
        const std::vector<G4double>& photoelectronTimesNs
    )
    {
        G4double minTimeNs = 0.;
        G4double maxTimeNs = 0.;

        if (!GetTimeLimits(
                photoelectronTimesNs,
                minTimeNs,
                maxTimeNs
            ))
        {
            return PMTWaveform();
        }

        return BuildPMTWaveform(
            photoelectronTimesNs,
            minTimeNs,
            maxTimeNs
        );
    }

    void SaveCoincidenceWaveformCSV(
        G4int eventID,
        const std::vector<G4double>& det1PhotoelectronTimesNs,
        const std::vector<G4double>& det2PhotoelectronTimesNs,
        G4double det1PeakVolt,
        G4double det2PeakVolt
    )
    {
        G4double min1 = 0.;
        G4double max1 = 0.;
        G4double min2 = 0.;
        G4double max2 = 0.;

        const G4bool has1 =
            GetTimeLimits(
                det1PhotoelectronTimesNs,
                min1,
                max1
            );

        const G4bool has2 =
            GetTimeLimits(
                det2PhotoelectronTimesNs,
                min2,
                max2
            );

        if (!has1 || !has2)
        {
            return;
        }

        const G4double globalStartNs =
            (min1 < min2) ? min1 : min2;

        const G4double globalEndNs =
            (max1 > max2) ? max1 : max2;

        PMTWaveform wf1 =
            BuildPMTWaveform(
                det1PhotoelectronTimesNs,
                globalStartNs,
                globalEndNs
            );

        PMTWaveform wf2 =
            BuildPMTWaveform(
                det2PhotoelectronTimesNs,
                globalStartNs,
                globalEndNs
            );

        const std::size_t n =
            (wf1.timeNs.size() <
             wf2.timeNs.size())
            ? wf1.timeNs.size()
            : wf2.timeNs.size();

        if (n == 0)
        {
            return;
        }

        std::ostringstream filename;

        filename
            << "pmt_waveform_event_"
            << B1::savedWaveformCount
            << "_geantEvent_"
            << eventID
            << ".csv";

        std::ofstream out(
            filename.str()
        );

        if (!out.is_open())
        {
            return;
        }

        out
            << "time_ns,"
            << "det1_voltage_mV,"
            << "det2_voltage_mV\n";

        const G4double t0 =
            wf1.timeNs.front();

        for (std::size_t i = 0;
             i < n;
             i++)
        {
            out
                << std::setprecision(10)
                << wf1.timeNs[i] - t0
                << ","
                << -1.0e3 *
                   wf1.voltageVolt[i]
                << ","
                << -1.0e3 *
                   wf2.voltageVolt[i]
                << "\n";
        }

        std::ofstream summary(
            "pmt_waveform_summary.csv",
            std::ios::app
        );

        if (summary.is_open())
        {
            summary
                << B1::savedWaveformCount
                << ","
                << eventID
                << ","
                << filename.str()
                << ","
                << det1PhotoelectronTimesNs.size()
                << ","
                << det2PhotoelectronTimesNs.size()
                << ","
                << 1.0e3 * det1PeakVolt
                << ","
                << 1.0e3 * det2PeakVolt
                << "\n";
        }

        B1::savedWaveformCount++;
    }

    void AppendEventAnalysisCSV(
        G4int eventID,
        G4double edepDet1MeV,
        G4double edepDet2MeV,
        G4bool edepHitDet1,
        G4bool edepHitDet2,
        G4bool edepCoincidence,
        G4long photonsDet1,
        G4long photonsDet2,
        const PMTPhotoelectronResult& peDet1,
        const PMTPhotoelectronResult& peDet2,
        const PMTTimingStats& timingDet1,
        const PMTTimingStats& timingDet2,
        G4double peakVoltageDet1,
        G4double peakVoltageDet2,
        G4bool voltageHitDet1,
        G4bool voltageHitDet2,
        G4bool voltageCoincidence
    )
    {
        const G4double nan =
            std::numeric_limits<G4double>::
            quiet_NaN();

        const G4double deltaFirstNs =
            (timingDet1.valid &&
             timingDet2.valid)
            ? timingDet2.firstTransportTimeNs -
              timingDet1.firstTransportTimeNs
            : nan;

        std::ofstream out(
            "pmt_event_analysis.csv",
            std::ios::app
        );

        if (!out.is_open())
        {
            return;
        }

        out
            << std::setprecision(12)
            << eventID << ","
            << edepDet1MeV << ","
            << edepDet2MeV << ","
            << static_cast<G4int>(
                   edepHitDet1
               ) << ","
            << static_cast<G4int>(
                   edepHitDet2
               ) << ","
            << static_cast<G4int>(
                   edepCoincidence
               ) << ","
            << photonsDet1 << ","
            << photonsDet2 << ","
            << peDet1.anodeArrivalTimeNs.size()
            << ","
            << peDet2.anodeArrivalTimeNs.size()
            << ","
            << peDet1.expectedPhotoelectrons
            << ","
            << peDet2.expectedPhotoelectrons
            << ","
            << peDet1.meanQuantumEfficiency
            << ","
            << peDet2.meanQuantumEfficiency
            << ","
            << (timingDet1.valid
                ? timingDet1.firstTransportTimeNs
                : nan) << ","
            << (timingDet2.valid
                ? timingDet2.firstTransportTimeNs
                : nan) << ","
            << (timingDet1.valid
                ? timingDet1.meanTransportTimeNs
                : nan) << ","
            << (timingDet2.valid
                ? timingDet2.meanTransportTimeNs
                : nan) << ","
            << deltaFirstNs << ","
            << 1.0e3 * peakVoltageDet1
            << ","
            << 1.0e3 * peakVoltageDet2
            << ","
            << static_cast<G4int>(
                   voltageHitDet1
               ) << ","
            << static_cast<G4int>(
                   voltageHitDet2
               ) << ","
            << static_cast<G4int>(
                   voltageCoincidence
               )
            << "\n";
    }
}

EventAction::EventAction()
: G4UserEventAction(),
  fEdepDet1(0.),
  fEdepDet2(0.),
  fPMTEnergyMeVDet1(0.),
  fPMTEnergyMeVDet2(0.),
  fPMTTimingPhotonsDet1(0),
  fPMTTimingPhotonsDet2(0),
  fPMTTransportTimeSumNsDet1(0.),
  fPMTTransportTimeSumNsDet2(0.),
  fPMTPathLengthSumCmDet1(0.),
  fPMTPathLengthSumCmDet2(0.),
  fPMTSpeedSumCmPerNsDet1(0.),
  fPMTSpeedSumCmPerNsDet2(0.),
  fPMTMinTransportTimeNsDet1(
      std::numeric_limits<G4double>::max()
  ),
  fPMTMinTransportTimeNsDet2(
      std::numeric_limits<G4double>::max()
  ),
  fPMTMaxTransportTimeNsDet1(0.),
  fPMTMaxTransportTimeNsDet2(0.)
{
    fPMTPhotonHitsDet1.reserve(2048);
    fPMTPhotonHitsDet2.reserve(2048);
}

EventAction::~EventAction()
{}

void EventAction::BeginOfEventAction(
    const G4Event*
)
{
    fEdepDet1 = 0.;
    fEdepDet2 = 0.;

    fPMTEnergyMeVDet1 = 0.;
    fPMTEnergyMeVDet2 = 0.;

    fPMTTimingPhotonsDet1 = 0;
    fPMTTimingPhotonsDet2 = 0;

    fPMTTransportTimeSumNsDet1 = 0.;
    fPMTTransportTimeSumNsDet2 = 0.;

    fPMTPathLengthSumCmDet1 = 0.;
    fPMTPathLengthSumCmDet2 = 0.;

    fPMTSpeedSumCmPerNsDet1 = 0.;
    fPMTSpeedSumCmPerNsDet2 = 0.;

    fPMTMinTransportTimeNsDet1 =
        std::numeric_limits<G4double>::max();

    fPMTMinTransportTimeNsDet2 =
        std::numeric_limits<G4double>::max();

    fPMTMaxTransportTimeNsDet1 = 0.;
    fPMTMaxTransportTimeNsDet2 = 0.;

    fPMTPhotonHitsDet1.clear();
    fPMTPhotonHitsDet2.clear();
}

void EventAction::AddEdep(
    G4int copyNo,
    G4double edep
)
{
    if (copyNo == 0)
    {
        fEdepDet1 += edep;
    }
    else if (copyNo == 1)
    {
        fEdepDet2 += edep;
    }
}

void EventAction::AddPMTPhoton(
    G4int copyNo,
    G4double photonEnergyMeV,
    G4double arrivalTimeNs,
    G4double transportTimeNs,
    G4double pathLengthCm
)
{
    PMTPhotonHit hit;

    hit.energyMeV =
        photonEnergyMeV;

    hit.arrivalTimeNs =
        arrivalTimeNs;

    hit.transportTimeNs =
        transportTimeNs;

    hit.pathLengthCm =
        pathLengthCm;

    if (copyNo == 0)
    {
        fPMTPhotonHitsDet1.push_back(
            hit
        );

        fPMTEnergyMeVDet1 +=
            photonEnergyMeV;

        if (transportTimeNs > 0. &&
            pathLengthCm >= 0.)
        {
            const G4double speedCmPerNs =
                pathLengthCm /
                transportTimeNs;

            fPMTTimingPhotonsDet1++;

            fPMTTransportTimeSumNsDet1 +=
                transportTimeNs;

            fPMTPathLengthSumCmDet1 +=
                pathLengthCm;

            fPMTSpeedSumCmPerNsDet1 +=
                speedCmPerNs;

            if (transportTimeNs <
                fPMTMinTransportTimeNsDet1)
            {
                fPMTMinTransportTimeNsDet1 =
                    transportTimeNs;
            }

            if (transportTimeNs >
                fPMTMaxTransportTimeNsDet1)
            {
                fPMTMaxTransportTimeNsDet1 =
                    transportTimeNs;
            }
        }
    }
    else if (copyNo == 1)
    {
        fPMTPhotonHitsDet2.push_back(
            hit
        );

        fPMTEnergyMeVDet2 +=
            photonEnergyMeV;

        if (transportTimeNs > 0. &&
            pathLengthCm >= 0.)
        {
            const G4double speedCmPerNs =
                pathLengthCm /
                transportTimeNs;

            fPMTTimingPhotonsDet2++;

            fPMTTransportTimeSumNsDet2 +=
                transportTimeNs;

            fPMTPathLengthSumCmDet2 +=
                pathLengthCm;

            fPMTSpeedSumCmPerNsDet2 +=
                speedCmPerNs;

            if (transportTimeNs <
                fPMTMinTransportTimeNsDet2)
            {
                fPMTMinTransportTimeNsDet2 =
                    transportTimeNs;
            }

            if (transportTimeNs >
                fPMTMaxTransportTimeNsDet2)
            {
                fPMTMaxTransportTimeNsDet2 =
                    transportTimeNs;
            }
        }
    }
}

void EventAction::EndOfEventAction(
    const G4Event* event
)
{
    const G4int eventID =
        event ? event->GetEventID() : -1;

    const G4double edepDet1MeV =
        fEdepDet1 / MeV;

    const G4double edepDet2MeV =
        fEdepDet2 / MeV;

    const G4bool edepHitDet1 =
        fEdepDet1 > 0.;

    const G4bool edepHitDet2 =
        fEdepDet2 > 0.;

    const G4bool edepCoincidence =
        edepHitDet1 &&
        edepHitDet2;

    const G4long photonsDet1 =
        static_cast<G4long>(
            fPMTPhotonHitsDet1.size()
        );

    const G4long photonsDet2 =
        static_cast<G4long>(
            fPMTPhotonHitsDet2.size()
        );

    const PMTPhotoelectronResult peDet1 =
        GeneratePhotoelectrons(
            fPMTPhotonHitsDet1
        );

    const PMTPhotoelectronResult peDet2 =
        GeneratePhotoelectrons(
            fPMTPhotonHitsDet2
        );

    PMTWaveform wfDet1 =
        BuildPMTWaveformAuto(
            peDet1.anodeArrivalTimeNs
        );

    PMTWaveform wfDet2 =
        BuildPMTWaveformAuto(
            peDet2.anodeArrivalTimeNs
        );

    const G4double peakVoltageDet1 =
        wfDet1.peakVoltageVolt;

    const G4double peakVoltageDet2 =
        wfDet2.peakVoltageVolt;

    const G4bool pmtHitDet1 =
        peakVoltageDet1 >=
        B1::pmtVoltageThresholdVolt;

    const G4bool pmtHitDet2 =
        peakVoltageDet2 >=
        B1::pmtVoltageThresholdVolt;

    const G4bool pmtCoincidence =
        pmtHitDet1 &&
        pmtHitDet2;

    const PMTTimingStats timingDet1 =
        CalculateTimingStats(
            fPMTPhotonHitsDet1
        );

    const PMTTimingStats timingDet2 =
        CalculateTimingStats(
            fPMTPhotonHitsDet2
        );

    G4AutoLock lock(
        &eventActionMutex
    );

    B1::totalEdepMeVDet1 +=
        edepDet1MeV;

    B1::totalEdepMeVDet2 +=
        edepDet2MeV;

    B1::totalEdepSquaredMeV2Det1 +=
        edepDet1MeV *
        edepDet1MeV;

    B1::totalEdepSquaredMeV2Det2 +=
        edepDet2MeV *
        edepDet2MeV;

    if (edepHitDet1)
    {
        B1::totalEdepHitDet1++;
    }

    if (edepHitDet2)
    {
        B1::totalEdepHitDet2++;
    }

    if (edepCoincidence)
    {
        B1::totalEdepCoincidence++;
    }

    if (photonsDet1 > 0)
    {
        B1::totalPMTPhotonEventsDet1++;
    }

    if (photonsDet2 > 0)
    {
        B1::totalPMTPhotonEventsDet2++;
    }

    if (photonsDet1 > 0 &&
        photonsDet2 > 0)
    {
        B1::totalBothPMTPhotonEvents++;
    }

    const G4long photoelectronsDet1 =
        static_cast<G4long>(
            peDet1.anodeArrivalTimeNs.size()
        );

    const G4long photoelectronsDet2 =
        static_cast<G4long>(
            peDet2.anodeArrivalTimeNs.size()
        );

    if (photoelectronsDet1 > 0)
    {
        B1::totalPhotoelectronEventsDet1++;
    }

    if (photoelectronsDet2 > 0)
    {
        B1::totalPhotoelectronEventsDet2++;
    }

    if (photoelectronsDet1 > 0 &&
        photoelectronsDet2 > 0)
    {
        B1::totalBothPhotoelectronEvents++;
    }

    B1::totalPMTPhotonsDet1 +=
        photonsDet1;

    B1::totalPMTPhotonsDet2 +=
        photonsDet2;

    B1::totalPhotoelectronsDet1 +=
        photoelectronsDet1;

    B1::totalPhotoelectronsDet2 +=
        photoelectronsDet2;

    B1::totalExpectedPhotoelectronsDet1 +=
        peDet1.expectedPhotoelectrons;

    B1::totalExpectedPhotoelectronsDet2 +=
        peDet2.expectedPhotoelectrons;

    B1::totalPMTEnergyMeVDet1 +=
        fPMTEnergyMeVDet1;

    B1::totalPMTEnergyMeVDet2 +=
        fPMTEnergyMeVDet2;

    B1::totalPeakVoltageDet1 +=
        peakVoltageDet1;

    B1::totalPeakVoltageDet2 +=
        peakVoltageDet2;

    if (timingDet1.valid)
    {
        B1::totalFirstTransitTimeNsDet1 +=
            timingDet1.firstTransportTimeNs;

        B1::totalFirstTransitTimeSquaredNs2Det1 +=
            timingDet1.firstTransportTimeNs *
            timingDet1.firstTransportTimeNs;

        B1::totalMeanTransitTimeNsDet1 +=
            timingDet1.meanTransportTimeNs;

        B1::totalMeanTransitTimeSquaredNs2Det1 +=
            timingDet1.meanTransportTimeNs *
            timingDet1.meanTransportTimeNs;

        B1::totalFirstTransitEntriesDet1++;
        B1::totalMeanTransitEntriesDet1++;
    }

    if (timingDet2.valid)
    {
        B1::totalFirstTransitTimeNsDet2 +=
            timingDet2.firstTransportTimeNs;

        B1::totalFirstTransitTimeSquaredNs2Det2 +=
            timingDet2.firstTransportTimeNs *
            timingDet2.firstTransportTimeNs;

        B1::totalMeanTransitTimeNsDet2 +=
            timingDet2.meanTransportTimeNs;

        B1::totalMeanTransitTimeSquaredNs2Det2 +=
            timingDet2.meanTransportTimeNs *
            timingDet2.meanTransportTimeNs;

        B1::totalFirstTransitEntriesDet2++;
        B1::totalMeanTransitEntriesDet2++;
    }

    if (timingDet1.valid &&
        timingDet2.valid)
    {
        const G4double deltaFirstNs =
            timingDet2.firstTransportTimeNs -
            timingDet1.firstTransportTimeNs;

        B1::totalDeltaFirstTransitTimeNs +=
            deltaFirstNs;

        B1::totalDeltaFirstTransitTimeSquaredNs2 +=
            deltaFirstNs *
            deltaFirstNs;

        B1::totalDeltaFirstTransitEntries++;
    }

    B1::totalPMTTimingPhotonsDet1 +=
        fPMTTimingPhotonsDet1;

    B1::totalPMTTimingPhotonsDet2 +=
        fPMTTimingPhotonsDet2;

    B1::totalPMTTransportTimeNsDet1 +=
        fPMTTransportTimeSumNsDet1;

    B1::totalPMTTransportTimeNsDet2 +=
        fPMTTransportTimeSumNsDet2;

    B1::totalPMTPathLengthCmDet1 +=
        fPMTPathLengthSumCmDet1;

    B1::totalPMTPathLengthCmDet2 +=
        fPMTPathLengthSumCmDet2;

    B1::totalPMTSpeedCmPerNsDet1 +=
        fPMTSpeedSumCmPerNsDet1;

    B1::totalPMTSpeedCmPerNsDet2 +=
        fPMTSpeedSumCmPerNsDet2;

    if (fPMTTimingPhotonsDet1 > 0)
    {
        if (fPMTMinTransportTimeNsDet1 <
            B1::minPMTTransportTimeNsDet1)
        {
            B1::minPMTTransportTimeNsDet1 =
                fPMTMinTransportTimeNsDet1;
        }

        if (fPMTMaxTransportTimeNsDet1 >
            B1::maxPMTTransportTimeNsDet1)
        {
            B1::maxPMTTransportTimeNsDet1 =
                fPMTMaxTransportTimeNsDet1;
        }
    }

    if (fPMTTimingPhotonsDet2 > 0)
    {
        if (fPMTMinTransportTimeNsDet2 <
            B1::minPMTTransportTimeNsDet2)
        {
            B1::minPMTTransportTimeNsDet2 =
                fPMTMinTransportTimeNsDet2;
        }

        if (fPMTMaxTransportTimeNsDet2 >
            B1::maxPMTTransportTimeNsDet2)
        {
            B1::maxPMTTransportTimeNsDet2 =
                fPMTMaxTransportTimeNsDet2;
        }
    }

    if (pmtHitDet1)
    {
        B1::totalPMTHitDet1++;

        B1::totalPMTPhotonsDet1InHitEvents +=
            photonsDet1;

        B1::totalPeakVoltageDet1InHitEvents +=
            peakVoltageDet1;
    }

    if (pmtHitDet2)
    {
        B1::totalPMTHitDet2++;

        B1::totalPMTPhotonsDet2InHitEvents +=
            photonsDet2;

        B1::totalPeakVoltageDet2InHitEvents +=
            peakVoltageDet2;
    }

    if (pmtCoincidence)
    {
        B1::totalPMTVoltageCoincidence++;

        B1::totalPMTPhotonsDet1InCoincidenceEvents +=
            photonsDet1;

        B1::totalPMTPhotonsDet2InCoincidenceEvents +=
            photonsDet2;

        B1::totalPeakVoltageDet1InCoincidenceEvents +=
            peakVoltageDet1;

        B1::totalPeakVoltageDet2InCoincidenceEvents +=
            peakVoltageDet2;
    }

    AppendEventAnalysisCSV(
        eventID,
        edepDet1MeV,
        edepDet2MeV,
        edepHitDet1,
        edepHitDet2,
        edepCoincidence,
        photonsDet1,
        photonsDet2,
        peDet1,
        peDet2,
        timingDet1,
        timingDet2,
        peakVoltageDet1,
        peakVoltageDet2,
        pmtHitDet1,
        pmtHitDet2,
        pmtCoincidence
    );

    if (pmtCoincidence &&
        B1::savedWaveformCount <
        B1::pmtWaveformSaveCount)
    {
        SaveCoincidenceWaveformCSV(
            eventID,
            peDet1.anodeArrivalTimeNs,
            peDet2.anodeArrivalTimeNs,
            peakVoltageDet1,
            peakVoltageDet2
        );
    }
}
