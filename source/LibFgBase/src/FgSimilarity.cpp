//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//

#include "stdafx.h"

#include "FgSimilarity.hpp"
#include "FgMatrixSolver.hpp"
#include "FgBounds.hpp"
#include "FgApproxEqual.hpp"
#include "FgMain.hpp"

using namespace std;

namespace Fg {

SimilarityD
similarityRand()
{
    return
        SimilarityD(
            std::exp(randNormal()),
            QuaternionD::rand(),
            Vec3D::randNormal());
}

// Uses approach originally from [Horn '87 "Closed-Form Solution of Absolute Orientation..."
// (taken from [Jain '95 "machine vision" 12.3]) to find an approximate similarity transform
// between two sets of corresponding points, FROM the domain points TO the range points:
SimilarityD
similarityApprox(
    const vector<Vec3D> &    domainPts,
    const vector<Vec3D> &    rangePts)
{
    SimilarityD            ret;
    FGASSERT(domainPts.size() > 2);     // Not solvable with only 2 points.
    uint                    numPts = uint(domainPts.size());
    FGASSERT(numPts == rangePts.size());
    // Compute the sufficient statistics for scale & rotation
    Vec3D        domMean = cMean(domainPts),
                    ranMean = cMean(rangePts);
    double          domRayMag = 0.0,
                    ranRayMag = 0.0;
    Mat33D     S(0.0);
    for (uint ii=0; ii<numPts; ii++) {
        Vec3D    domRay = domainPts[ii] - domMean,
                    ranRay = rangePts[ii] - ranMean;
        domRayMag += domRay.mag();
        ranRayMag += ranRay.mag();
        S += domRay * ranRay.transpose();
    }
    double          scale = sqrt(ranRayMag / domRayMag );
    Mat44D        N(0.0);
    double  Sxx = S.cr(0,0),   Sxy = S.cr(1,0),   Sxz = S.cr(2,0),
            Syx = S.cr(0,1),   Syy = S.cr(1,1),   Syz = S.cr(2,1),
            Szx = S.cr(0,2),   Szy = S.cr(1,2),   Szz = S.cr(2,2);
    // Set the upper triangular elements of N not including the diagonal:
    N.cr(1,0)=Syz-Szy;         N.cr(2,0)=Szx-Sxz;      N.cr(3,0)=Sxy-Syx;
                                N.cr(2,1)=Sxy+Syx;      N.cr(3,1)=Szx+Sxz;
                                                         N.cr(3,2)=Syz+Szy;
    // Since it's symmetric, set the lower triangular (not including diagonal) by:
    N += N.transpose();
    // And set the diagonal elements:
    N.cr(0,0) = Sxx+Syy+Szz;
    N.cr(1,1) = Sxx-Syy-Szz;
    N.cr(2,2) = Syy-Sxx-Szz;
    N.cr(3,3) = Szz-Sxx-Syy;
    // Calculate rotation from N per [Jain '95]. 'fgEigsRsm' Leaves largest eigVal in last index:
    QuaternionD       pose(fgEigsRsm(N).vecs.colVec(3));
    // Calculate the 'trans' term: The transform is given by:
    // X = SR(d-dm)+rm = SR(d)-SR(dm)+rm
    Vec3D            trans = -scale * (pose.asMatrix() * domMean) + ranMean;
    ret = SimilarityD(scale,pose,trans);
    // Measure residual:
    double  resid = cRms(rangePts-mapXft(domainPts,ret.asAffine())) / fgMaxElem(cDims(rangePts));
    fgout << fgnl << "SimilarityApprox() relative RMS residual: " << resid;
    return ret;
}

Affine3D
TransSim::asAffine() const
{
    Affine3D      ret;
    ret.linear = rot.asMatrix() * scale;
    ret.translation = ret.linear * trans;
    return ret;
}

void
fgSimilarityTest(const CLArgs &)
{
    SimilarityD    sim(expSafe(randNormal()),QuaternionD(Vec4D::randNormal()),Vec3D::randNormal());
    SimilarityD    id = sim * sim.inverse();
    Mat33D        diff = id.asAffine() * Mat33D::identity() - Mat33D::identity();
    FGASSERT(cMax(mapAbs(diff.m)) < numeric_limits<double>::epsilon()*8);
}

void
fgSimilarityApproxTest(const CLArgs &)
{
    randSeedRepeatable();
    for (uint nn=0; nn<10; nn++) {
        uint                numPts = 3;
        Vec3Ds              domain(numPts),
                            range(numPts);
        for (uint ii=0; ii<numPts; ii++)
            domain[ii] = Vec3D::randNormal();
        for (uint mm=0; mm<10; mm++) {
            SimilarityD     simRef(expSafe(3.0 * randNormal()),QuaternionD::rand(),Vec3D::randNormal());
            mapXf_(domain,simRef.asAffine(),range);
            SimilarityD     sim = similarityApprox(domain,range);
            mapXf_(domain,sim.asAffine());
            FGASSERT(fgApproxEqual(domain,range));
        }
        numPts *= 2;
    }
}

}

// */
