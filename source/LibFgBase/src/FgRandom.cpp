//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//

#include "stdafx.h"

#include "FgRandom.hpp"
#include "FgDiagnostics.hpp"
#include "FgImage.hpp"
#include "FgImgDisplay.hpp"
#include "FgMath.hpp"
#include "FgAffine1.hpp"
#include "FgMain.hpp"

using namespace std;

namespace Fg {

namespace {

struct      RNG
{
    random_device       rd;
    mt19937_64          gen;

    // This will initialize using unique bits from the device, including time:
    RNG() : gen(rd()) {}
};

static RNG rng;

}

uint32
randUint()
{return uint32(rng.gen()); }

uint
randUint(uint size)
{
    FGASSERT(size>1);
    // Lightweight class, not a performance issue to construct each time:
    uniform_int_distribution<uint> d(0,size-1);     // Convert from inclusive to exclusive upper bound
    return d(rng.gen);
}

uint64
randUint64()
{return rng.gen(); }

double
randUniform()
{return double(randUint64()) / double(numeric_limits<uint64>::max()); }

void
randSeedRepeatable(uint64 seed)
{rng.gen.seed(seed); }

double
randUniform(double lo,double hi) 
{return (randUniform() * (hi-lo) + lo); }

double
randNormal()
{
    // Polar (Box-Muller) method; See Knuth v2, 3rd ed, p122.
    double  x, y, r2;
    do {
        x = -1 + 2 * randUniform();
        y = -1 + 2 * randUniform();
        // see if it is in the unit circle:
        r2 = x * x + y * y;
    }
    while (r2 > 1.0 || r2 == 0);
    // Box-Muller transform:
    return y * sqrt (-2.0 * log (r2) / r2);
}

Doubles
randNormals(size_t num,double mean,double stdev)
{
    Doubles      ret(num);
    for (size_t ii=0; ii<num; ++ii)
        ret[ii] = mean + stdev * randNormal();
    return ret;
}

Floats
randNormalFs(size_t num,float mean,float stdev)
{
    Floats      ret(num);
    for (size_t ii=0; ii<num; ++ii)
        ret[ii] = mean + stdev * randNormalF();
    return ret;
}

static
char
randChar()
{
    uint    val = randUint(26+26+10);
    if (val < 10)
        return char(48+val);
    val -= 10;
    if (val < 26)
        return char(65+val);
    val -= 26;
    FGASSERT(val < 26);
    return char(97+val);
}

string
randString(uint numChars)
{
    string  ret;
    for (uint ii=0; ii<numChars; ++ii)
        ret = ret + randChar();
    return ret;
}

void
fgRandomTest(const CLArgs &)
{
    fgout << fgnl << "sizeof(RNG) = " << sizeof(RNG);
    // Create a histogram of normal samples:
    randSeedRepeatable();
    size_t          numStdevs = 6,
                    binsPerStdev = 50,
                    numSamples = 1000000,
                    sz = numStdevs * 2 * binsPerStdev;
    vector<size_t>  histogram(sz,0);
    Affine1D      randToHist(VecD2(-double(numStdevs),numStdevs),VecD2(0,sz));
    for (size_t ii=0; ii<numSamples; ++ii) {
        int         rnd = round<int>(randToHist * randNormal());
        if ((rnd >= 0) && (rnd < int(sz)))
            ++histogram[rnd]; }

    // Make a bar graph of it:
    // numSamples = binScale * stdNormIntegral * binsPerStdev
    double          binScale = double(numSamples) / (sqrt2Pi() * binsPerStdev),
                    hgtRatio = 0.9;
    ImgC4UC     img(sz,sz,RgbaUC(0));
    for (size_t xx=0; xx<sz; ++xx) {
        size_t      hgt = round<int>(histogram[xx] * sz * hgtRatio / binScale);
        for (size_t yy=0; yy<hgt; ++yy)
            img.xy(xx,yy) = RgbaUC(255); }

    // Superimpose a similarly scaled Gaussian:
    Affine1D      histToRand = randToHist.inverse();
    for (size_t xx=0; xx<sz; ++xx) {
        double      val = std::exp(-0.5 * sqr(histToRand * (xx + 0.5)));
        size_t      hgt = round<int>(val * sz * hgtRatio);
        img.xy(xx,hgt) = RgbaUC(255,0,0,255); }

    // Display:
    fgImgFlipVertical(img);
    imgDisplay(img);
}

bool
randBool()
{return (rng.gen() > numeric_limits<uint32>::max()/2); }

double
randNearUnit()
{
    double      unit = randBool() ? 1.0 : -1.0;
    return unit + randNormal()*0.125;
}

std::vector<double>
randNearUnits(size_t num)
{return fgGenerate<double>(randNearUnit,num); }

}

// */
