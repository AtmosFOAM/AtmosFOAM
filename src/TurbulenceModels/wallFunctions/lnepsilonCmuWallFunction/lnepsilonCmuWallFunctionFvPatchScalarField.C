/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2022 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "lnepsilonCmuWallFunctionFvPatchScalarField.H"
#include "nutkAtmRoughCmuWallFunctionFvPatchScalarField.H"
#include "momentumTransportModel.H"
#include "fvMatrix.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

Foam::scalar Foam::lnepsilonCmuWallFunctionFvPatchScalarField::tolerance_ = 1e-5;

// * * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * //

void Foam::lnepsilonCmuWallFunctionFvPatchScalarField::setMaster()
{
    if (master_ != -1)
    {
        return;
    }

    const volScalarField& lnepsilon =
        static_cast<const volScalarField&>(this->internalField());

    const volScalarField::Boundary& bf = lnepsilon.boundaryField();

    label master = -1;
    forAll(bf, patchi)
    {
        if (isA<lnepsilonCmuWallFunctionFvPatchScalarField>(bf[patchi]))
        {
            lnepsilonCmuWallFunctionFvPatchScalarField& epf = lnepsilonPatch(patchi);

            if (master == -1)
            {
                master = patchi;
            }

            epf.master() = master;
        }
    }
}


void Foam::lnepsilonCmuWallFunctionFvPatchScalarField::createAveragingWeights()
{
    const volScalarField& lnepsilon =
        static_cast<const volScalarField&>(this->internalField());

    const volScalarField::Boundary& bf = lnepsilon.boundaryField();

    const fvMesh& mesh = lnepsilon.mesh();

    if (initialised_ && !mesh.changing())
    {
        return;
    }

    volScalarField weights
    (
        IOobject
        (
            "weights",
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            false // do not register
        ),
        mesh,
        dimensionedScalar(dimless, 0)
    );

    DynamicList<label> lnepsilonPatches(bf.size());
    forAll(bf, patchi)
    {
        if (isA<lnepsilonCmuWallFunctionFvPatchScalarField>(bf[patchi]))
        {
            lnepsilonPatches.append(patchi);

            const labelUList& faceCells = bf[patchi].patch().faceCells();
            forAll(faceCells, i)
            {
                weights[faceCells[i]]++;
            }
        }
    }

    cornerWeights_.setSize(bf.size());
    forAll(lnepsilonPatches, i)
    {
        label patchi = lnepsilonPatches[i];
        const fvPatchScalarField& wf = weights.boundaryField()[patchi];
        cornerWeights_[patchi] = 1.0/wf.patchInternalField();
    }

    G_.setSize(internalField().size(), 0.0);
    lnepsilon_.setSize(internalField().size(), 0.0);

    initialised_ = true;
}


Foam::lnepsilonCmuWallFunctionFvPatchScalarField&
Foam::lnepsilonCmuWallFunctionFvPatchScalarField::lnepsilonPatch(const label patchi)
{
    const volScalarField& lnepsilon =
        static_cast<const volScalarField&>(this->internalField());

    const volScalarField::Boundary& bf = lnepsilon.boundaryField();

    const lnepsilonCmuWallFunctionFvPatchScalarField& epf =
        refCast<const lnepsilonCmuWallFunctionFvPatchScalarField>(bf[patchi]);

    return const_cast<lnepsilonCmuWallFunctionFvPatchScalarField&>(epf);
}


void Foam::lnepsilonCmuWallFunctionFvPatchScalarField::calculateTurbulenceFields
(
    const momentumTransportModel& turbModel,
    scalarField& G0,
    scalarField& lnepsilon0
)
{
    // Ask for the wall distance in advance. Some processes may not have any
    // corner weights, so we cannot allow functions inside the if statements
    // below to trigger the wall distance calculation.
    turbModel.y();

    // Accumulate all of the G and lnepsilon contributions
    forAll(cornerWeights_, patchi)
    {
        if (!cornerWeights_[patchi].empty())
        {
            lnepsilonCmuWallFunctionFvPatchScalarField& epf = lnepsilonPatch(patchi);

            const List<scalar>& w = cornerWeights_[patchi];

            epf.calculate(turbModel, w, epf.patch(), G0, lnepsilon0);
        }
    }

    // Apply zero-gradient condition for lnepsilon
    forAll(cornerWeights_, patchi)
    {
        if (!cornerWeights_[patchi].empty())
        {
            lnepsilonCmuWallFunctionFvPatchScalarField& epf = lnepsilonPatch(patchi);

            epf == scalarField(lnepsilon0, epf.patch().faceCells());
        }
    }
}


void Foam::lnepsilonCmuWallFunctionFvPatchScalarField::calculate
(
    const momentumTransportModel& turbModel,
    const List<scalar>& cornerWeights,
    const fvPatch& patch,
    scalarField& G0,
    scalarField& lnepsilon0
)
{
    const label patchi = patch.index();

    const nutkAtmRoughCmuWallFunctionFvPatchScalarField& nutw =
        nutkAtmRoughCmuWallFunctionFvPatchScalarField::nutw(turbModel, patchi);

    const scalarField& y = turbModel.y()[patchi];

    const tmp<scalarField> tnuw = turbModel.nu(patchi);
    const scalarField& nuw = tnuw();

    const tmp<volScalarField> tk = turbModel.k();
    const volScalarField& k = tk();

    const fvPatchVectorField& Uw = turbModel.U().boundaryField()[patchi];

    const scalarField magGradUw(mag(Uw.snGrad()));

    // Set lnepsilon and G
    forAll(nutw, facei)
    {
        const label celli = patch.faceCells()[facei];

        const scalar Cmu = nutw.Cmu()[celli];
        const scalar Cmu25 = pow025(Cmu);
        const scalar Cmu75 = pow(Cmu, 0.75);

        const scalar yPlus = Cmu25*y[facei]*sqrt(k[celli])/nuw[facei];

        const scalar w = cornerWeights[facei];

        if (yPlus > nutw.yPlusLam())
        {
            lnepsilon0[celli] = log
            (
               w*Cmu75*pow(k[celli], 1.5)/(nutw.kappa()*y[facei])
            );

            G0[celli] +=
                w
               *(nutw[facei] + nuw[facei])
               *magGradUw[facei]
               *Cmu25*sqrt(k[celli])
              /(nutw.kappa()*y[facei]);
        }
        else
        {
            lnepsilon0[celli] = log
            (
                w*2.0*k[celli]*nuw[facei]/sqr(y[facei])
            );
        }
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::lnepsilonCmuWallFunctionFvPatchScalarField::
lnepsilonCmuWallFunctionFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedValueFvPatchField<scalar>(p, iF),
    G_(),
    lnepsilon_(),
    initialised_(false),
    master_(-1),
    cornerWeights_()
{}


Foam::lnepsilonCmuWallFunctionFvPatchScalarField::
lnepsilonCmuWallFunctionFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchField<scalar>(p, iF, dict),
    G_(),
    lnepsilon_(),
    initialised_(false),
    master_(-1),
    cornerWeights_()
{
    // Apply zero-gradient condition on start-up
    this->operator==(patchInternalField());
}


Foam::lnepsilonCmuWallFunctionFvPatchScalarField::
lnepsilonCmuWallFunctionFvPatchScalarField
(
    const lnepsilonCmuWallFunctionFvPatchScalarField& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedValueFvPatchField<scalar>(ptf, p, iF, mapper),
    G_(),
    lnepsilon_(),
    initialised_(false),
    master_(-1),
    cornerWeights_()
{}


Foam::lnepsilonCmuWallFunctionFvPatchScalarField::
lnepsilonCmuWallFunctionFvPatchScalarField
(
    const lnepsilonCmuWallFunctionFvPatchScalarField& ewfpsf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedValueFvPatchField<scalar>(ewfpsf, iF),
    G_(),
    lnepsilon_(),
    initialised_(false),
    master_(-1),
    cornerWeights_()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::scalarField& Foam::lnepsilonCmuWallFunctionFvPatchScalarField::G(bool init)
{
    if (patch().index() == master_)
    {
        if (init)
        {
            G_ = 0.0;
        }

        return G_;
    }

    return lnepsilonPatch(master_).G();
}


Foam::scalarField& Foam::lnepsilonCmuWallFunctionFvPatchScalarField::lnepsilon
(
    bool init
)
{
    if (patch().index() == master_)
    {
        if (init)
        {
            lnepsilon_ = 0.0;
        }

        return lnepsilon_;
    }

    return lnepsilonPatch(master_).lnepsilon(init);
}


void Foam::lnepsilonCmuWallFunctionFvPatchScalarField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    const momentumTransportModel& turbModel =
        db().lookupObject<momentumTransportModel>
        (
            IOobject::groupName
            (
                momentumTransportModel::typeName,
                internalField().group()
            )
        );

    setMaster();

    if (patch().index() == master_)
    {
        createAveragingWeights();
        calculateTurbulenceFields(turbModel, G(true), lnepsilon(true));
    }

    const scalarField& G0 = this->G();
    const scalarField& lnepsilon0 = this->lnepsilon();

    typedef DimensionedField<scalar, volMesh> FieldType;
    FieldType& G =
        const_cast<FieldType&>
        (
            db().lookupObject<FieldType>(turbModel.GName())
        );
    FieldType& lnepsilon =
        const_cast<FieldType&>
        (
            internalField()
        );

    scalarField weights(patch().magSf()/patch().patch().magFaceAreas());
    forAll(weights, facei)
    {
        scalar& w = weights[facei];
        w = w <= tolerance_ ? 0 : (w - tolerance_)/(1 - tolerance_);
    }

    forAll(weights, facei)
    {
        const scalar w = weights[facei];
        const label celli = patch().faceCells()[facei];

        G[celli] = (1 - w)*G[celli] + w*G0[celli];
        lnepsilon[celli] = (1 - w)*lnepsilon[celli] + w*lnepsilon0[celli];
    }

    this->operator==(scalarField(lnepsilon, patch().faceCells()));

    fvPatchField<scalar>::updateCoeffs();
}


void Foam::lnepsilonCmuWallFunctionFvPatchScalarField::manipulateMatrix
(
    fvMatrix<scalar>& matrix
)
{
    if (manipulatedMatrix())
    {
        return;
    }

    const scalarField& lnepsilon0 = this->lnepsilon();

    scalarField weights(patch().magSf()/patch().patch().magFaceAreas());
    forAll(weights, facei)
    {
        scalar& w = weights[facei];
        w = w <= tolerance_ ? 0 : (w - tolerance_)/(1 - tolerance_);
    }

    matrix.setValues
    (
        patch().faceCells(),
        UIndirectList<scalar>(lnepsilon0, patch().faceCells()),
        weights
    );

    fvPatchField<scalar>::manipulateMatrix(matrix);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchScalarField,
        lnepsilonCmuWallFunctionFvPatchScalarField
    );
}


// ************************************************************************* //
