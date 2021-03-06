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
    setIsothermalBalance

Description
    Find discretely balanced theta and Exner profiles given a uniform
    temperature profile

\*---------------------------------------------------------------------------*/

#include "HodgeOps.H"
#include "fvCFD.H"
#include "ExnerTheta.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"
    #include "readEnvironmentalProperties.H"
    #include "readThermoProperties.H"
    HodgeOps H(mesh);
    const surfaceScalarField gd("gd", g & H.delta());
    #include "createFields.H"
      
    const dictionary& itsDict = mesh.solutionDict().subDict("initialisation");
    const int maxIters = itsDict.lookupOrDefault<int>("maxIters", 100);
    const int BCiters  = itsDict.lookupOrDefault<int>("BCiters", 10);
    const scalar BCtol = itsDict.lookupOrDefault<scalar>("BCtol", SMALL);
    const scalar boundaryRelaxation
         = itsDict.lookupOrDefault<scalar>("boundaryRelaxation", 0.5);
   
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    bool innerConverged = false;
    bool outerConverged = false;
    scalar topBCval = Exner.boundaryField()[topBC][0];
    for(label iter = 0; iter < maxIters && !(innerConverged && outerConverged); iter++)
    {
        Info << "Outer iteration " << iter << endl;
        
        // Inner iterations with fixed top boundary
        innerConverged = false;
        for(label BCiter = 0; BCiter < BCiters && !innerConverged; BCiter++)
        {
            theta == T0/Exner;
            thetaf = fvc::interpolate(theta);
            U = H.ddirToFlux(gd)
              - H.ddirToFluxOffDiag(Cp*thetaf*H.magd()*fvc::snGrad(Exner));

            fvScalarMatrix ExnerEqn
            (
                fvc::div(U)
              - fvm::laplacian
                (
                    H.Hdiag()*Cp*thetaf*H.magd()/mesh.magSf(), Exner
                )
            );
            innerConverged = 
                ExnerEqn.solve(Exner.name()).nIterations() == 0;
        }
        outerConverged = (mag(1-Exner.boundaryField()[groundBC][0])< BCtol);

        scalar maxGroundExner = max(Exner.boundaryField()[groundBC]);
        outerConverged = (mag(1-maxGroundExner)< BCtol);
        
        // modify the top boundary condition
        Info << "Exner ground value = " << maxGroundExner
             << "  ground value minus one = "
             << maxGroundExner-1
             << " Exner top BC going from  = " << topBCval << " to ";

        topBCval = (1-boundaryRelaxation)*topBCval
                 + boundaryRelaxation*Exner.boundaryField()[topBC][0]
                       /maxGroundExner;
        topBCval = min(max(topBCval, scalar(0)), scalar(1));
        Info << topBCval << endl;
        Exner.boundaryFieldRef()[topBC] == topBCval;
    }

    // Change the top boundary type to be fixedFluxBuoyantExner
    Info << "Correcting top boundary conditions for Exner" << endl;
    wordList ExnerBCs = Exner.boundaryField().types();
    ExnerBCs[topBC] = "fixedFluxBuoyantExner";
    volScalarField ExnerNew
    (
        IOobject("Exner", runTime.timeName(), mesh, IOobject::NO_READ),
        Exner,
        ExnerBCs
    );
    ExnerNew.correctBoundaryConditions();
    ExnerNew.write();

    Info << "Calculating and writing out the Brutvaissala frequency" << endl;
    volScalarField BruntFreq
    (
        IOobject("BruntFreq", runTime.timeName(), mesh),
        Foam::sqrt(-(g & fvc::grad(theta))/theta)
    );
    BruntFreq.write();

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //

