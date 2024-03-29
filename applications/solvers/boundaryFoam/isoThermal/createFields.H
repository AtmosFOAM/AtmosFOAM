Info<< "Reading field U\n" << endl;
volVectorField U
(
    IOobject
    (
        "U",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);


Info<< "Creating face flux\n" << endl;
surfaceScalarField phi
(
    IOobject
    (
        "phi",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar(mesh.Sf().dimensions()*U.dimensions(), 0)
);


autoPtr<viscosityModel> viscosity(viscosityModel::New(mesh));

autoPtr<incompressible::momentumTransportModel> turbulence
(
    incompressible::momentumTransportModel::New(U, phi, viscosity)
);

dimensionedVector Ubar("Ubar", dimVelocity, viscosity);

dimensionedVector gradP
(
    "gradP",
    dimensionSet(0, 1, -2, 0, 0),
    viscosity
);

const bool fixUbar = magSqr(Ubar.value()) != 0;
if (fixUbar && magSqr(gradP.value()) != 0)
{
    FatalErrorIn("boundaryFoamHS")
        << " cannot fix both Ubar != 0 and gradP != 0. One must be (0 0 0)"
        << exit(FatalError);
}

vector flowDirection = fixUbar ? (Ubar/mag(Ubar)).value() 
                               :(gradP/mag(gradP)).value();
tensor flowMask = sqr(flowDirection);

#include "createFvModels.H"
#include "createFvConstraints.H"
