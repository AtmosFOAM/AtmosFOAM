/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 1991-2009 OpenCFD Ltd.
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Application
    boussinesqFoam

Description
    Transient solver for dry, single-fluid Boussinesq equations. Made to be a 
    single-fluid version of the multiFluidBoussinesqFoam solver.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "fvcCurlf.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    Foam::argList::addBoolOption("isoThermal", "do not solve buoyancy equation");
    Foam::argList::addBoolOption("linear", "do not include non-linear advection");

    // Allow running solver with -postProcess option
    #include "postProcess.H"

    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"
    #include "zeros.H"

    #include "createFields.H"

    const dictionary& itsDict = mesh.solution().subDict("iterations");
    const int nOuterCorr = itsDict.lookupOrDefault<int>("nOuterCorrectors", 2);
    const int nCorr = itsDict.lookupOrDefault<int>("nCorrectors", 1);
    const int nNonOrthCorr =
        itsDict.lookupOrDefault<int>("nNonOrthogonalCorrectors", 0);
    const scalar offCentre = readScalar(mesh.schemes().lookup("offCentre"));
    label pRefCell = mesh.solution().lookupOrDefault<label>("pRefCell", 0);

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nStarting time loop\n" << endl;

    while (runTime.loop())
    {
        Info<< "Time = " << runTime.timeName() << endl;

        #include "courantNo.H"

        for (int ucorr=0; ucorr < nOuterCorr; ucorr++)
        {
            if (!args.options().found("isoThermal"))
            {
                #include "bEqn.H"
            }
            // Pressure and velocity updates
            for (int corr=0; corr<nCorr; corr++)
            {
                #include "PEqn.H"
                u = fvc::reconstruct(volFlux);
            }
        }

        runTime.write();

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
