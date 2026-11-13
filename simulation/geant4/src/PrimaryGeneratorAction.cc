#include "PrimaryGeneratorAction.hh"

#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "Randomize.hh"

#include <cmath>

namespace
{
    // ------------------------------------------------------------
    // Cosmic muon energy sampler
    //
    // Approximation:
    // dN/dE ∝ E^(-gamma)
    //
    // This is not a full atmospheric muon model, but it is much
    // more realistic than a uniform 1-5 GeV distribution.
    // ------------------------------------------------------------
    G4double SamplePowerLawEnergy(
        G4double minEnergy,
        G4double maxEnergy,
        G4double gamma
    )
    {
        const G4double u = G4UniformRand();

        const G4double eMinGeV = minEnergy / GeV;
        const G4double eMaxGeV = maxEnergy / GeV;

        const G4double oneMinusGamma = 1.0 - gamma;

        const G4double a = std::pow(eMinGeV, oneMinusGamma);
        const G4double b = std::pow(eMaxGeV, oneMinusGamma);

        const G4double eGeV =
            std::pow(
                a + u * (b - a),
                1.0 / oneMinusGamma
            );

        return eGeV * GeV;
    }

    // ------------------------------------------------------------
    // Cosmic muon angular sampler
    //
    // Atmospheric muon intensity:
    // I(theta) ∝ cos^2(theta)
    //
    // For a horizontal source plane, the number crossing the plane
    // has one extra cos(theta) projection factor:
    //
    // pdf(theta) ∝ sin(theta) * cos^3(theta)
    //
    // CDF(theta) = [1 - cos^4(theta)] / [1 - cos^4(thetaMax)]
    // ------------------------------------------------------------
    G4ThreeVector SampleCosmicMuonDirection(G4double thetaMax)
    {
        const G4double u = G4UniformRand();

        const G4double cosThetaMax = std::cos(thetaMax);
        const G4double cosThetaMax4 =
            cosThetaMax * cosThetaMax * cosThetaMax * cosThetaMax;

        const G4double cosTheta =
            std::pow(
                1.0 - u * (1.0 - cosThetaMax4),
                0.25
            );

        const G4double sinTheta =
            std::sqrt(1.0 - cosTheta * cosTheta);

        const G4double phi =
            G4UniformRand() * 360.0 * deg;

        const G4double px =
            sinTheta * std::cos(phi);

        const G4double py =
            sinTheta * std::sin(phi);

        const G4double pz =
            -cosTheta;

        return G4ThreeVector(px, py, pz);
    }
}

PrimaryGeneratorAction::PrimaryGeneratorAction()
: G4VUserPrimaryGeneratorAction(),
  fParticleGun(nullptr)
{
    G4int nParticle = 1;
    fParticleGun = new G4ParticleGun(nParticle);

    G4ParticleTable* particleTable =
        G4ParticleTable::GetParticleTable();

    G4ParticleDefinition* particle =
        particleTable->FindParticle("mu-");

    fParticleGun->SetParticleDefinition(particle);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
    // ------------------------------------------------------------
    // 1. Particle type
    //
    // Surface cosmic muons contain both mu+ and mu-.
    // As a simple approximation, use mostly mu+.
    //
    // mu+ probability ≈ 56%
    // mu- probability ≈ 44%
    // ------------------------------------------------------------

    G4ParticleTable* particleTable =
        G4ParticleTable::GetParticleTable();

    const G4double muPlusProbability = 0.56;

    G4ParticleDefinition* particle = nullptr;

    if (G4UniformRand() < muPlusProbability)
    {
        particle = particleTable->FindParticle("mu+");
    }
    else
    {
        particle = particleTable->FindParticle("mu-");
    }

    fParticleGun->SetParticleDefinition(particle);

    // ------------------------------------------------------------
    // 2. Source plane
    //
    // Plane is above the detector.
    // Larger than before, because inclined cosmic muons can enter
    // the detector from off-axis positions.
    // ------------------------------------------------------------

    const G4double sourcePlaneSizeXY = 60.0 * cm;
    const G4double sourceZ = 30.0 * cm;

    const G4double x0 =
        sourcePlaneSizeXY * (G4UniformRand() - 0.5);

    const G4double y0 =
        sourcePlaneSizeXY * (G4UniformRand() - 0.5);

    const G4double z0 =
        sourceZ;

    fParticleGun->SetParticlePosition(
        G4ThreeVector(x0, y0, z0)
    );

    // ------------------------------------------------------------
    // 3. Cosmic muon direction
    //
    // Use cos^2(theta)-like angular distribution.
    // thetaMax = 45 deg is enough for this two-scintillator
    // telescope geometry. Larger angles mostly miss both plates.
    // ------------------------------------------------------------

    const G4double thetaMax = 45.0 * deg;

    fParticleGun->SetParticleMomentumDirection(
        SampleCosmicMuonDirection(thetaMax)
    );

    // ------------------------------------------------------------
    // 4. Cosmic muon kinetic energy
    //
    // Approximate ground-level energy distribution:
    // dN/dE ∝ E^(-2.7)
    //
    // Range:
    // 1 GeV to 100 GeV
    //
    // Most generated muons will still be in the few-GeV region.
    // ------------------------------------------------------------

    const G4double minEnergy = 1.0 * GeV;
    const G4double maxEnergy = 100.0 * GeV;
    const G4double spectralIndex = 2.7;

    const G4double energy =
        SamplePowerLawEnergy(
            minEnergy,
            maxEnergy,
            spectralIndex
        );

    fParticleGun->SetParticleEnergy(energy);

    fParticleGun->GeneratePrimaryVertex(anEvent);
}
