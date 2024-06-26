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

Class
    Foam::partitionedHydrostaticExnerFvPatchScalarField

Description
    Foam::partitionedHydrostaticExnerFvPatchScalarField

SourceFiles
    partitionedHydrostaticExnerFvPatchScalarField.C

\*---------------------------------------------------------------------------*/

#ifndef partitionedHydrostaticExnerFvPatchScalarFields_H
#define partitionedHydrostaticExnerFvPatchScalarFields_H

#include "fvPatchFields.H"
#include "fvsPatchFields.H"
#include "fixedGradientFvPatchFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                  Class partitionedHydrostaticExnerFvPatch Declaration
\*---------------------------------------------------------------------------*/

class partitionedHydrostaticExnerFvPatchScalarField
:
    public fixedGradientFvPatchScalarField
{

public:

    //- Runtime type information
    TypeName("partitionedHydrostaticExner");


    // Constructors

        //- Construct from patch and internal field
        partitionedHydrostaticExnerFvPatchScalarField
        (
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&
        );

        //- Construct from patch, internal field and dictionary
        partitionedHydrostaticExnerFvPatchScalarField
        (
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given
        //  partitionedHydrostaticExnerFvPatchScalarField onto a new patch
        partitionedHydrostaticExnerFvPatchScalarField
        (
            const partitionedHydrostaticExnerFvPatchScalarField&,
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy not allowed
        partitionedHydrostaticExnerFvPatchScalarField
        (
            const partitionedHydrostaticExnerFvPatchScalarField&
        ) = delete;

        //- Construct as copy setting internal field reference
        partitionedHydrostaticExnerFvPatchScalarField
        (
            const partitionedHydrostaticExnerFvPatchScalarField&,
            const DimensionedField<scalar, volMesh>&
        );

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatchScalarField> clone
        (
            const DimensionedField<scalar, volMesh>& iF
        ) const
        {
            return tmp<fvPatchScalarField>
            (
                new partitionedHydrostaticExnerFvPatchScalarField(*this, iF)
            );
        }


    // Member functions

        //- Update the coefficients associated with the patch field
        virtual void updateCoeffs();

        //- Write
        virtual void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
