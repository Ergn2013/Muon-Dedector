#include "DetectorConstruction.hh"

#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4Element.hh"

#include "G4Box.hh"
#include "G4ExtrudedSolid.hh"
#include "G4Tubs.hh"
#include "G4TwoVector.hh"

#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4RotationMatrix.hh"

#include "G4SystemOfUnits.hh"
#include "G4MaterialPropertiesTable.hh"

#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4OpticalSurface.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include <vector>


DetectorConstruction::DetectorConstruction()
: G4VUserDetectorConstruction(),
  fScoringVolume(nullptr),
  fPMTVolume(nullptr)
{}


DetectorConstruction::~DetectorConstruction()
{}


G4VPhysicalVolume* DetectorConstruction::Construct()
{
    G4NistManager* nist =
        G4NistManager::Instance();

    const G4bool checkOverlaps = true;

    // ============================================================
    // MATERIALS
    // ============================================================

    G4Material* air =
        nist->FindOrBuildMaterial("G4_AIR");

    G4Material* teflonMat =
        nist->FindOrBuildMaterial("G4_TEFLON");

    G4Material* tapeMat =
        nist->FindOrBuildMaterial("G4_POLYVINYL_CHLORIDE");

    G4Material* siliconMat =
        nist->FindOrBuildMaterial("G4_Si");

    G4Element* elH =
        nist->FindOrBuildElement("H");

    G4Element* elC =
        nist->FindOrBuildElement("C");

    // BC-408 approximation: PVT-like C9H10
    G4Material* scintMat =
        new G4Material(
            "BC408",
            1.023 * g / cm3,
            2
        );

    scintMat->AddElement(elC, 9);
    scintMat->AddElement(elH, 10);


    // ============================================================
    // OPTICAL PHOTON ENERGY GRID
    // ============================================================

    const G4int nEntries = 5;

    G4double photonEnergy[nEntries] =
    {
        2.58 * eV,   // approximately 480 nm
        2.76 * eV,   // approximately 450 nm
        2.92 * eV,   // approximately 425 nm
        3.10 * eV,   // approximately 400 nm
        3.26 * eV    // approximately 380 nm
    };


    // ============================================================
    // BC-408 OPTICAL PROPERTIES
    // ============================================================

    G4double rIndexBC408[nEntries] =
    {
        1.58,
        1.58,
        1.58,
        1.58,
        1.58
    };

    G4double scintSpectrum[nEntries] =
    {
        0.10,
        0.45,
        1.00,
        0.55,
        0.15
    };

    G4double absLengthBC408[nEntries] =
    {
        380. * cm,
        380. * cm,
        380. * cm,
        380. * cm,
        380. * cm
    };

    auto mptBC408 =
        new G4MaterialPropertiesTable();

    mptBC408->AddProperty(
        "RINDEX",
        photonEnergy,
        rIndexBC408,
        nEntries
    );

    mptBC408->AddProperty(
        "ABSLENGTH",
        photonEnergy,
        absLengthBC408,
        nEntries
    );

    mptBC408->AddProperty(
        "SCINTILLATIONCOMPONENT1",
        photonEnergy,
        scintSpectrum,
        nEntries
    );

    mptBC408->AddConstProperty(
        "SCINTILLATIONYIELD",
        10000. / MeV
    );

    mptBC408->AddConstProperty(
        "RESOLUTIONSCALE",
        1.0
    );

    mptBC408->AddConstProperty(
        "SCINTILLATIONTIMECONSTANT1",
        2.1 * ns
    );

    mptBC408->AddConstProperty(
        "SCINTILLATIONYIELD1",
        1.0
    );

    scintMat->SetMaterialPropertiesTable(mptBC408);


    // ============================================================
    // AIR OPTICAL PROPERTIES
    // ============================================================

    G4double rIndexAir[nEntries] =
    {
        1.0003,
        1.0003,
        1.0003,
        1.0003,
        1.0003
    };

    auto mptAir =
        new G4MaterialPropertiesTable();

    mptAir->AddProperty(
        "RINDEX",
        photonEnergy,
        rIndexAir,
        nEntries
    );

    air->SetMaterialPropertiesTable(mptAir);


    // ============================================================
    // TEFLON OPTICAL PROPERTIES
    // ============================================================

    G4double rIndexTeflon[nEntries] =
    {
        1.35,
        1.35,
        1.35,
        1.35,
        1.35
    };

    auto mptTeflon =
        new G4MaterialPropertiesTable();

    mptTeflon->AddProperty(
        "RINDEX",
        photonEnergy,
        rIndexTeflon,
        nEntries
    );

    teflonMat->SetMaterialPropertiesTable(mptTeflon);


    // ============================================================
    // BLACK TAPE OPTICAL PROPERTIES
    // ============================================================

    G4double rIndexTape[nEntries] =
    {
        1.54,
        1.54,
        1.54,
        1.54,
        1.54
    };

    auto mptTape =
        new G4MaterialPropertiesTable();

    mptTape->AddProperty(
        "RINDEX",
        photonEnergy,
        rIndexTape,
        nEntries
    );

    tapeMat->SetMaterialPropertiesTable(mptTape);


    // ============================================================
    // PMT ACTIVE MATERIAL
    // ============================================================

    G4double rIndexPMT[nEntries] =
    {
        3.50,
        3.50,
        3.50,
        3.50,
        3.50
    };

    G4double absLengthPMT[nEntries] =
    {
        1. * um,
        1. * um,
        1. * um,
        1. * um,
        1. * um
    };

    auto mptPMT =
        new G4MaterialPropertiesTable();

    mptPMT->AddProperty(
        "RINDEX",
        photonEnergy,
        rIndexPMT,
        nEntries
    );

    mptPMT->AddProperty(
        "ABSLENGTH",
        photonEnergy,
        absLengthPMT,
        nEntries
    );

    siliconMat->SetMaterialPropertiesTable(mptPMT);


    // ============================================================
    // WORLD
    // ============================================================

    G4Box* sWorld =
        new G4Box(
            "World",
            1.0 * m,
            1.0 * m,
            1.0 * m
        );

    G4LogicalVolume* lWorld =
        new G4LogicalVolume(
            sWorld,
            air,
            "World"
        );

    G4VPhysicalVolume* pWorld =
        new G4PVPlacement(
            nullptr,
            G4ThreeVector(),
            lWorld,
            "World",
            nullptr,
            false,
            0,
            checkOverlaps
        );


    // ============================================================
    // REAL SCINTILLATOR DIMENSIONS
    // ============================================================
    //
    // Main rectangular body:
    //
    //     length    = 13.7 cm
    //     width     = 10.0 cm
    //     thickness = 1.4 cm
    //
    // Tapered section:
    //
    //     length         = 4.9 cm
    //     starting width = 10.0 cm
    //     end width      = 4.0 cm
    //
    // Total length:
    //
    //     13.7 + 4.9 = 18.6 cm
    //
    // Coordinate system:
    //
    //     X = detector length
    //     Y = detector width
    //     Z = detector thickness
    //
    // PMT is placed on the +X narrow end.
    // ============================================================

    const G4double scintHalfThickness =
        0.7 * cm;

    const G4double teflonThickness =
        0.02 * cm;

    const G4double tapeThickness =
        0.02 * cm;

    const G4double teflonHalfThickness =
        scintHalfThickness + teflonThickness;

    const G4double tapeHalfThickness =
        teflonHalfThickness + tapeThickness;


    // ============================================================
    // SCINTILLATOR POLYGON
    // ============================================================
    //
    // Clockwise vertex order.
    //
    // X coordinates:
    //
    //     back edge       = -9.3 cm
    //     taper beginning = +4.4 cm
    //     narrow end      = +9.3 cm
    //
    // Checks:
    //
    //     4.4 - (-9.3) = 13.7 cm
    //     9.3 - 4.4    = 4.9 cm
    //     9.3 - (-9.3) = 18.6 cm
    //
    // Y coordinates:
    //
    //     main body = +/- 5.0 cm  -> 10 cm width
    //     narrow end = +/- 2.0 cm -> 4 cm width
    // ============================================================

    std::vector<G4TwoVector> scintPolygon =
    {
        G4TwoVector(-9.3 * cm,  5.0 * cm),
        G4TwoVector( 4.4 * cm,  5.0 * cm),
        G4TwoVector( 9.3 * cm,  2.0 * cm),
        G4TwoVector( 9.3 * cm, -2.0 * cm),
        G4TwoVector( 4.4 * cm, -5.0 * cm),
        G4TwoVector(-9.3 * cm, -5.0 * cm)
    };

    std::vector<G4ExtrudedSolid::ZSection> scintSections;

    scintSections.emplace_back(
        -scintHalfThickness,
        G4TwoVector(0., 0.),
        1.0
    );

    scintSections.emplace_back(
        scintHalfThickness,
        G4TwoVector(0., 0.),
        1.0
    );

    G4ExtrudedSolid* sScint =
        new G4ExtrudedSolid(
            "Scint",
            scintPolygon,
            scintSections
        );

    G4LogicalVolume* lScint =
        new G4LogicalVolume(
            sScint,
            scintMat,
            "Scint"
        );


    // ============================================================
    // TEFLON OUTER POLYGON
    // ============================================================
    //
    // The vertices are an approximately uniform 0.2 mm outward
    // offset of the scintillator polygon.
    // ============================================================

    std::vector<G4TwoVector> teflonPolygon =
    {
        G4TwoVector(-9.320000 * cm,  5.020000 * cm),
        G4TwoVector( 4.405636 * cm,  5.020000 * cm),
        G4TwoVector( 9.320000 * cm,  2.011206 * cm),
        G4TwoVector( 9.320000 * cm, -2.011206 * cm),
        G4TwoVector( 4.405636 * cm, -5.020000 * cm),
        G4TwoVector(-9.320000 * cm, -5.020000 * cm)
    };

    std::vector<G4ExtrudedSolid::ZSection> teflonSections;

    teflonSections.emplace_back(
        -teflonHalfThickness,
        G4TwoVector(0., 0.),
        1.0
    );

    teflonSections.emplace_back(
        teflonHalfThickness,
        G4TwoVector(0., 0.),
        1.0
    );

    G4ExtrudedSolid* sTeflon =
        new G4ExtrudedSolid(
            "Teflon",
            teflonPolygon,
            teflonSections
        );

    G4LogicalVolume* lTeflon =
        new G4LogicalVolume(
            sTeflon,
            teflonMat,
            "Teflon"
        );


    // Scintillator placed inside teflon.
    G4VPhysicalVolume* pScint =
        new G4PVPlacement(
            nullptr,
            G4ThreeVector(),
            lScint,
            "Scint",
            lTeflon,
            false,
            0,
            checkOverlaps
        );


    // ============================================================
    // PMT ACTIVE AREA
    // ============================================================
    //
    // PMT active area:
    //
    //     diameter = 8 mm
    //     radius   = 4 mm = 0.4 cm
    //
    // The disk is placed at the centre of the 4 cm wide tip.
    //
    // G4Tubs default cylinder axis is local Z.
    // rotateY(90 deg) makes the cylinder axis parallel to global X.
    //
    // PMT front face touches the scintillator at x = +9.3 cm.
    // ============================================================

    const G4double pmtRadius =
        0.4 * cm;

    const G4double pmtHalfThickness =
        0.005 * cm;

    const G4double scintTipX =
        9.3 * cm;

    G4Tubs* sPMT =
        new G4Tubs(
            "PMT",
            0.,
            pmtRadius,
            pmtHalfThickness,
            0.,
            360. * deg
        );

    G4LogicalVolume* lPMT =
        new G4LogicalVolume(
            sPMT,
            siliconMat,
            "PMT"
        );

    auto pmtRotation =
        new G4RotationMatrix();

    pmtRotation->rotateY(90. * deg);

    G4VPhysicalVolume* pPMT =
        new G4PVPlacement(
            pmtRotation,
            G4ThreeVector(
                scintTipX + pmtHalfThickness,
                0.,
                0.
            ),
            lPMT,
            "PMT",
            lTeflon,
            false,
            0,
            checkOverlaps
        );


    // ============================================================
    // BLACK TAPE OUTER POLYGON
    // ============================================================
    //
    // This is the teflon polygon expanded by another 0.2 mm.
    // Total outward thickness relative to scintillator is about
    // 0.4 mm.
    // ============================================================

    std::vector<G4TwoVector> tapePolygon =
    {
        G4TwoVector(-9.340000 * cm,  5.040000 * cm),
        G4TwoVector( 4.411272 * cm,  5.040000 * cm),
        G4TwoVector( 9.340000 * cm,  2.022412 * cm),
        G4TwoVector( 9.340000 * cm, -2.022412 * cm),
        G4TwoVector( 4.411272 * cm, -5.040000 * cm),
        G4TwoVector(-9.340000 * cm, -5.040000 * cm)
    };

    std::vector<G4ExtrudedSolid::ZSection> tapeSections;

    tapeSections.emplace_back(
        -tapeHalfThickness,
        G4TwoVector(0., 0.),
        1.0
    );

    tapeSections.emplace_back(
        tapeHalfThickness,
        G4TwoVector(0., 0.),
        1.0
    );

    G4ExtrudedSolid* sTape =
        new G4ExtrudedSolid(
            "Tape",
            tapePolygon,
            tapeSections
        );

    G4LogicalVolume* lTape =
        new G4LogicalVolume(
            sTape,
            tapeMat,
            "Tape"
        );


    // Teflon placed inside black tape.
    G4VPhysicalVolume* pTeflon =
        new G4PVPlacement(
            nullptr,
            G4ThreeVector(),
            lTeflon,
            "Teflon",
            lTape,
            false,
            0,
            checkOverlaps
        );


    // ============================================================
    // TWO DETECTOR MODULES
    // ============================================================
    //
    // Both modules have the same orientation.
    // Their PMT tips point toward +X.
    // ============================================================

    new G4PVPlacement(
        nullptr,
        G4ThreeVector(0., 0., 15. * cm),
        lTape,
        "Det1",
        lWorld,
        false,
        0,
        checkOverlaps
    );

    new G4PVPlacement(
        nullptr,
        G4ThreeVector(0., 0., -15. * cm),
        lTape,
        "Det2",
        lWorld,
        false,
        1,
        checkOverlaps
    );


    // ============================================================
    // TEFLON REFLECTIVE SURFACE
    // ============================================================

    G4double zeroEfficiency[nEntries] =
    {
        0.0,
        0.0,
        0.0,
        0.0,
        0.0
    };

    auto teflonSurface =
        new G4OpticalSurface(
            "TeflonReflectorSurface"
        );

    teflonSurface->SetType(dielectric_metal);
    teflonSurface->SetFinish(groundfrontpainted);
    teflonSurface->SetModel(unified);
    teflonSurface->SetSigmaAlpha(0.20);

    G4double teflonReflectivity[nEntries] =
    {
        0.97,
        0.98,
        0.98,
        0.97,
        0.95
    };

    auto teflonSurfaceMPT =
        new G4MaterialPropertiesTable();

    teflonSurfaceMPT->AddProperty(
        "REFLECTIVITY",
        photonEnergy,
        teflonReflectivity,
        nEntries
    );

    teflonSurfaceMPT->AddProperty(
        "EFFICIENCY",
        photonEnergy,
        zeroEfficiency,
        nEntries
    );

    teflonSurface->SetMaterialPropertiesTable(
        teflonSurfaceMPT
    );

    new G4LogicalBorderSurface(
        "ScintToTeflonSurface",
        pScint,
        pTeflon,
        teflonSurface
    );

    new G4LogicalBorderSurface(
        "TeflonToScintSurface",
        pTeflon,
        pScint,
        teflonSurface
    );


    // ============================================================
    // BLACK TAPE ABSORBING SURFACE
    // ============================================================

    auto blackTapeSurface =
        new G4OpticalSurface(
            "BlackTapeAbsorberSurface"
        );

    blackTapeSurface->SetType(dielectric_metal);
    blackTapeSurface->SetFinish(groundfrontpainted);
    blackTapeSurface->SetModel(unified);
    blackTapeSurface->SetSigmaAlpha(0.30);

    G4double blackTapeReflectivity[nEntries] =
    {
        0.040,
        0.035,
        0.030,
        0.025,
        0.020
    };

    auto blackTapeSurfaceMPT =
        new G4MaterialPropertiesTable();

    blackTapeSurfaceMPT->AddProperty(
        "REFLECTIVITY",
        photonEnergy,
        blackTapeReflectivity,
        nEntries
    );

    blackTapeSurfaceMPT->AddProperty(
        "EFFICIENCY",
        photonEnergy,
        zeroEfficiency,
        nEntries
    );

    blackTapeSurface->SetMaterialPropertiesTable(
        blackTapeSurfaceMPT
    );

    new G4LogicalSkinSurface(
        "BlackTapeSkinSurface",
        lTape,
        blackTapeSurface
    );


    // ============================================================
    // VISUALISATION
    // ============================================================

    lWorld->SetVisAttributes(
        G4VisAttributes::GetInvisible()
    );

    auto scintVis =
        new G4VisAttributes(
            G4Colour(0.0, 1.0, 0.0, 0.65)
        );

    scintVis->SetVisibility(true);
    scintVis->SetForceSolid(true);

    lScint->SetVisAttributes(scintVis);


    auto teflonVis =
        new G4VisAttributes(
            G4Colour(1.0, 1.0, 1.0, 1.0)
        );

    teflonVis->SetVisibility(true);
    teflonVis->SetForceWireframe(true);

    lTeflon->SetVisAttributes(teflonVis);


    auto tapeVis =
        new G4VisAttributes(
            G4Colour(0.15, 0.15, 0.15, 1.0)
        );

    tapeVis->SetVisibility(true);
    tapeVis->SetForceWireframe(true);

    lTape->SetVisAttributes(tapeVis);


    auto pmtVis =
        new G4VisAttributes(
            G4Colour(1.0, 1.0, 0.0, 1.0)
        );

    pmtVis->SetVisibility(true);
    pmtVis->SetForceSolid(true);

    lPMT->SetVisAttributes(pmtVis);


    // ============================================================
    // SCORING VOLUMES
    // ============================================================

    fScoringVolume = lScint;
    fPMTVolume = lPMT;

    return pWorld;
}
