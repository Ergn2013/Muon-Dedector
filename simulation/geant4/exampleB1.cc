#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4VModularPhysicsList.hh"

#include "QBBC.hh"
#include "G4OpticalPhysics.hh"

#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4ios.hh"
#include "Randomize.hh"

#include <cstdlib>

using namespace B1;

namespace B1
{
    extern G4double pmtVoltageThresholdVolt;
    extern G4double pmtLoadResistanceOhm;
    extern G4double pmtAnodeRadiantSensitivityAW;

    extern G4double pmtRiseTimeSeconds;
    extern G4double pmtFallTimeSeconds;
    extern G4double pmtWaveformTimeStepSeconds;
    extern G4double pmtVoltageCalibrationFactor;

    extern G4int pmtWaveformSaveCount;
}

int main(int argc, char** argv)
{
    G4UIExecutive* ui = nullptr;

    if (argc == 1)
    {
        ui = new G4UIExecutive(argc, argv);
    }

    // Usage:
    //
    // Visual:
    // ./exampleB1
    //
    // Batch default:
    // ./exampleB1 ../run_novis.mac
    //
    // Waveform model:
    // ./exampleB1 ../run_novis.mac 100 50 2.6e5 0.57 6.0 0.1 1.0 5
    //
    // Args:
    // argv[2] = threshold in mV
    // argv[3] = load resistance in ohm
    // argv[4] = anode radiant sensitivity in A/W
    // argv[5] = rise time in ns
    // argv[6] = fall time in ns
    // argv[7] = waveform time step in ns
    // argv[8] = voltage calibration factor
    // argv[9] = number of coincidence waveforms to save

    if (argc >= 3)
    {
        B1::pmtVoltageThresholdVolt =
            std::atof(argv[2]) * 1.0e-3;

        if (B1::pmtVoltageThresholdVolt < 0.)
        {
            B1::pmtVoltageThresholdVolt = 0.;
        }
    }

    if (argc >= 4)
    {
        B1::pmtLoadResistanceOhm =
            std::atof(argv[3]);

        if (B1::pmtLoadResistanceOhm <= 0.)
        {
            B1::pmtLoadResistanceOhm = 50.;
        }
    }

    if (argc >= 5)
    {
        B1::pmtAnodeRadiantSensitivityAW =
            std::atof(argv[4]);

        if (B1::pmtAnodeRadiantSensitivityAW <= 0.)
        {
            B1::pmtAnodeRadiantSensitivityAW = 2.6e5;
        }
    }

    if (argc >= 6)
    {
        B1::pmtRiseTimeSeconds =
            std::atof(argv[5]) * 1.0e-9;

        if (B1::pmtRiseTimeSeconds <= 0.)
        {
            B1::pmtRiseTimeSeconds = 0.57e-9;
        }
    }

    if (argc >= 7)
    {
        B1::pmtFallTimeSeconds =
            std::atof(argv[6]) * 1.0e-9;

        if (B1::pmtFallTimeSeconds <= B1::pmtRiseTimeSeconds)
        {
            B1::pmtFallTimeSeconds = 6.0e-9;
        }
    }

    if (argc >= 8)
    {
        B1::pmtWaveformTimeStepSeconds =
            std::atof(argv[7]) * 1.0e-9;

        if (B1::pmtWaveformTimeStepSeconds <= 0.)
        {
            B1::pmtWaveformTimeStepSeconds = 0.1e-9;
        }
    }

    if (argc >= 9)
    {
        B1::pmtVoltageCalibrationFactor =
            std::atof(argv[8]);

        if (B1::pmtVoltageCalibrationFactor <= 0.)
        {
            B1::pmtVoltageCalibrationFactor = 1.0;
        }
    }

    if (argc >= 10)
    {
        B1::pmtWaveformSaveCount =
            std::atoi(argv[9]);

        if (B1::pmtWaveformSaveCount < 0)
        {
            B1::pmtWaveformSaveCount = 0;
        }
    }

    G4cout << "\nPMT waveform electronics model\n"
           << "----------------------------------------------\n"
           << "Voltage threshold          : "
           << B1::pmtVoltageThresholdVolt * 1.0e3 << " mV\n"
           << "Load resistance            : "
           << B1::pmtLoadResistanceOhm << " ohm\n"
           << "Anode radiant sensitivity  : "
           << B1::pmtAnodeRadiantSensitivityAW << " A/W\n"
           << "Rise time                  : "
           << B1::pmtRiseTimeSeconds * 1.0e9 << " ns\n"
           << "Fall time                  : "
           << B1::pmtFallTimeSeconds * 1.0e9 << " ns\n"
           << "Waveform time step         : "
           << B1::pmtWaveformTimeStepSeconds * 1.0e9 << " ns\n"
           << "Voltage calibration factor : "
           << B1::pmtVoltageCalibrationFactor << "\n"
           << "Waveforms to save          : "
           << B1::pmtWaveformSaveCount << "\n"
           << G4endl;

    auto* runManager =
        G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

    runManager->SetUserInitialization(new DetectorConstruction());

    auto* physicsList = new QBBC;
    physicsList->SetVerboseLevel(0);

    auto* opticalPhysics = new G4OpticalPhysics();
    opticalPhysics->SetVerboseLevel(0);

    physicsList->RegisterPhysics(opticalPhysics);

    runManager->SetUserInitialization(physicsList);

    runManager->SetUserInitialization(new ActionInitialization());

    auto* visManager = new G4VisExecutive;
    visManager->Initialize();

    G4UImanager* UImanager =
        G4UImanager::GetUIpointer();

    if (!ui)
    {
        G4String command = "/control/execute ";
        G4String fileName = argv[1];
        UImanager->ApplyCommand(command + fileName);
    }
    else
    {
        UImanager->ApplyCommand("/control/execute vis.mac");
        ui->SessionStart();
        delete ui;
    }

    delete visManager;
    delete runManager;

    return 0;
}
