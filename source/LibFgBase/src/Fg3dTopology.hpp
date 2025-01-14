//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
// Data structures for topological analysis of a 3D triangulated surface
//

#ifndef FG3TOPOLOGY_HPP
#define FG3TOPOLOGY_HPP

#include "FgStdLibs.hpp"

#include "FgTypes.hpp"
#include "FgMatrixC.hpp"
#include "FgMatrixV.hpp"
#include "FgOpt.hpp"
#include "Fg3dNormals.hpp"

namespace Fg {

struct Fg3dTopology
{
    struct      Tri
    {
        Vec3UI           vertInds;
        Vec3UI           edgeInds;   // In same order as verts (0-1,1-2,3-0)

        Vec2UI
        edge(uint relIdx) const;        // Return ordered vert inds of 0,1,2 edge of tri
    };
    struct      Edge
    {
        Vec2UI           vertInds;   // Lower index first
        Svec<uint>        triInds;

        uint
        otherVertIdx(uint vertIdx) const;
    };
    struct      Vert
    {
        Svec<uint>        edgeInds;   // Can be zero if vert is unused
        Svec<uint>        triInds;    // "
    };
    Svec<Tri>             m_tris;
    Svec<Edge>            m_edges;
    Svec<Vert>            m_verts;

    Fg3dTopology(
        const Vec3Fs &            verts,
        const Svec<Vec3UI> &  tris);

    Vec2UI
    edgeFacingVertInds(uint edgeIdx) const;

    bool
    vertOnBoundary(uint vertIdx) const;

    // Returns a list of vertex indices which are separated by a boundary edge. This list
    // is always non-empty, and will always be of size 2 for a manifold surface:
    Svec<uint>
    vertBoundaryNeighbours(uint vertIdx) const;

    Svec<uint>
    vertNeighbours(uint vertIdx) const;

    // Sets of vertex indices corresponding to each connected seam.
    // A seam vertex has > 0 (always an even number for valid topologies) single-valence edges connected.
    Svec<std::set<uint> >
    seams() const;

    // Returns the connected seam containing vertIdx, unless vertIdx is not on a seam in which
    // case the empty set is returned:
    std::set<uint>
    seamContaining(uint vertIdx) const;

    // Trace a fold consisting only of edges whose facet normals differ by at least 60 degrees.
    std::set<uint>
    traceFold(
        const Normals & norms,  // Must be created from a single (unified) tri-only surface
        Svec<FgBool> &    done,
        uint                vertIdx) const;
    
    // Returns number of boundary edges, intersection edges and reversed edges respectively.
    // If all are zero the mesh is watertight. If the last 2 are zero the mesh is manifold.
    Vec3UI
    isManifold() const;

    size_t
    unusedVerts() const;

    // Returns the minimum edge distance to the given vertex for each vertex.
    // Unconnected verts will have value float_max.
    Svec<float>
    edgeDistanceMap(const Vec3Fs & verts,size_t vertIdx) const;

    // As above where 'init' has at least 1 distance defined, the rest set to float_max:
    void
    edgeDistanceMap(const Vec3Fs & verts,Svec<float> & init) const;

private:

    Svec<uint>
    findSeam(Svec<FgBool> & done) const;

    uint
    oppositeVert(uint triIdx,uint edgeIdx) const;
};

}

#endif

// */
