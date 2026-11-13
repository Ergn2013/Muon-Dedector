#include "SteppingAction.hh"

#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4TouchableHandle.hh"
#include "G4Track.hh"
#include "G4OpticalPhoton.hh"
#include "G4SystemOfUnits.hh"

SteppingAction::SteppingAction(EventAction* eventAction)
: G4UserSteppingAction(),
  fEventAction(eventAction)
{}

SteppingAction::~SteppingAction()
{}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    if (!step) return;
    if (!fEventAction) return;

    const auto detectorConstruction =
        static_cast<const DetectorConstruction*>(
            G4RunManager::GetRunManager()->GetUserDetectorConstruction()
        );

    if (!detectorConstruction) return;

    G4Track* track = step->GetTrack();
    if (!track) return;

    if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition())
    {
        G4LogicalVolume* pmtVolume = detectorConstruction->GetPMTVolume();
        if (!pmtVolume) return;

        G4StepPoint* postStepPoint = step->GetPostStepPoint();
        if (!postStepPoint) return;

        G4VPhysicalVolume* postVolume = postStepPoint->GetPhysicalVolume();
        if (!postVolume) return;

        G4LogicalVolume* postLogicalVolume =
            postVolume->GetLogicalVolume();

        if (postLogicalVolume == pmtVolume)
        {
            auto touchable = postStepPoint->GetTouchableHandle();
            if (!touchable) return;

            // PMT hierarchy:
            // depth 0 -> PMT
            // depth 1 -> Teflon
            // depth 2 -> Det1 / Det2
            G4int detCopyNo =
                touchable->GetCopyNumber(2);

            const G4double photonEnergyMeV =
                track->GetTotalEnergy() / MeV;

            // Global arrival time includes scintillation emission time.
            const G4double arrivalTimeNs =
                track->GetGlobalTime() / ns;

            // Local time is transport time from photon creation to PMT.
            const G4double transportTimeNs =
                track->GetLocalTime() / ns;

            const G4double pathLengthCm =
                track->GetTrackLength() / cm;

            fEventAction->AddPMTPhoton(
                detCopyNo,
                photonEnergyMeV,
                arrivalTimeNs,
                transportTimeNs,
                pathLengthCm
            );

            track->SetTrackStatus(fStopAndKill);
        }

        return;
    }

    G4LogicalVolume* scoringVolume =
        detectorConstruction->GetScoringVolume();

    if (!scoringVolume) return;

    G4StepPoint* preStepPoint =
        step->GetPreStepPoint();

    if (!preStepPoint) return;

    auto touchable =
        preStepPoint->GetTouchableHandle();

    if (!touchable) return;

    G4VPhysicalVolume* physicalVolume =
        touchable->GetVolume();

    if (!physicalVolume) return;

    G4LogicalVolume* logicalVolume =
        physicalVolume->GetLogicalVolume();

    if (logicalVolume != scoringVolume) return;

    G4double edep =
        step->GetTotalEnergyDeposit();

    if (edep <= 0.) return;

    // Scintillator hierarchy:
    // depth 0 -> Scint
    // depth 1 -> Teflon
    // depth 2 -> Det1 / Det2
    G4int detCopyNo =
        touchable->GetCopyNumber(2);

    fEventAction->AddEdep(detCopyNo, edep);
}
