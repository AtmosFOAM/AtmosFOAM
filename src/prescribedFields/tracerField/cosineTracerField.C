#include "cosineTracerField.H"
#include "addToRunTimeSelectionTable.H"

namespace Foam
{
defineTypeNameAndDebug(cosineTracerField, 0);
addToRunTimeSelectionTable(tracerField, cosineTracerField, dict);

cosineTracerField::cosineTracerField
(
    const dictionary& dict,
    const advectable& velocityField
)
:
    tracerField(velocityField),
    width_(readScalar(dict.lookup("width"))),
    centre_(dict.lookup("centre")),
    maxTracer_(readScalar(dict.lookup("maxTracer"))),
    backgroundTracer_(dict.lookupOrDefault<scalar>("backgroundTracer", 0))
{}

scalar cosineTracerField::tracerAt(const point& p, const Time& t) const
{
    scalar r = mag(p - centre_);
    if (r < width_)
    {
        return backgroundTracer_
             + maxTracer_*0.5*(1+Foam::cos(M_PI*r/width_));
    }
    else
    {
        return backgroundTracer_;
    }
}

}
