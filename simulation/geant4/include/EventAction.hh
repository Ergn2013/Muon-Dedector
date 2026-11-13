#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

#include <vector>

class G4Event;

struct PMTPhotonHit
{
    G4double energyMeV;
    G4double arrivalTimeNs;
    G4double transportTimeNs;
    G4double pathLengthCm;
};

class EventAction : public G4UserEventAction
{
public:
    EventAction();
    ~EventAction() override;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    void AddEdep(G4int copyNo, G4double edep);

    void AddPMTPhoton(
        G4int copyNo,
        G4double photonEnergyMeV,
        G4double arrivalTimeNs,
        G4double transportTimeNs,
        G4double pathLengthCm
    );

private:
    G4double fEdepDet1;
    G4double fEdepDet2;

    G4double fPMTEnergyMeVDet1;
    G4double fPMTEnergyMeVDet2;

    G4int fPMTTimingPhotonsDet1;
    G4int fPMTTimingPhotonsDet2;

    G4double fPMTTransportTimeSumNsDet1;
    G4double fPMTTransportTimeSumNsDet2;

    G4double fPMTPathLengthSumCmDet1;
    G4double fPMTPathLengthSumCmDet2;

    G4double fPMTSpeedSumCmPerNsDet1;
    G4double fPMTSpeedSumCmPerNsDet2;

    G4double fPMTMinTransportTimeNsDet1;
    G4double fPMTMinTransportTimeNsDet2;

    G4double fPMTMaxTransportTimeNsDet1;
    G4double fPMTMaxTransportTimeNsDet2;

    std::vector<PMTPhotonHit> fPMTPhotonHitsDet1;
    std::vector<PMTPhotonHit> fPMTPhotonHitsDet2;
};

#endif
