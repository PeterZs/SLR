//
//  TriangleMeshNode.cpp
//
//  Created by 渡部 心 on 2015/09/06.
//  Copyright (c) 2015年 渡部 心. All rights reserved.
//

#include "TriangleMeshNode.h"
#include "textures.hpp"
#include "surface_materials.hpp"
#include <libSLR/Core/Transform.h>
#include <libSLR/Core/SurfaceObject.h>
#include <libSLR/Surface/TriangleMesh.h>

namespace SLRSceneGraph {
    TriangleMeshNode::~TriangleMeshNode() {
        if (m_trianglesForRendering)
            delete[] m_trianglesForRendering;
    }

    uint64_t TriangleMeshNode::addVertex(const SLR::Vertex &v) {
        m_vertices.push_back(v);
        return m_vertices.size() - 1;
    }
    
    void TriangleMeshNode::addTriangles(const SurfaceMaterialRef &mat, const Normal3DTextureRef &normalMap, const FloatTextureRef &alphaMap,
                                        const std::vector<Triangle> &&triangles) {
        m_matGroups.emplace_back();
        MaterialGroup &matGroup = m_matGroups.back();
        matGroup.material = mat;
        matGroup.normalMap = normalMap;
        matGroup.alphaMap = alphaMap;
        matGroup.triangles = triangles;
        
        for (int i = 0; i < matGroup.triangles.size(); ++i) {
            const Triangle &tri = matGroup.triangles[i];
            const SLR::Vertex &vtx0 = m_vertices[tri.vIdx[0]];
            const SLR::Vertex &vtx1 = m_vertices[tri.vIdx[1]];
            const SLR::Vertex &vtx2 = m_vertices[tri.vIdx[2]];
            SLR::Vector3D gNormal = SLR::cross(vtx1.position - vtx0.position, vtx2.position - vtx0.position);
            bool faceAway = false;
            faceAway |= SLR::dot(vtx0.normal, gNormal) <= 0;
            faceAway |= SLR::dot(vtx1.normal, gNormal) <= 0;
            faceAway |= SLR::dot(vtx2.normal, gNormal) <= 0;
            if (faceAway)
                printf("Warning: triangle [%llu, %llu, %llu] has a shading normal too faced away from a geometric normal.\n", tri.vIdx[0], tri.vIdx[1], tri.vIdx[2]);
        }
    }
    
    void TriangleMeshNode::applyTransform(const SLR::StaticTransform &t) {
        for (int i = 0; i < m_vertices.size(); ++i) {
            Vertex &v = m_vertices[i];
            v.position = t * v.position;
            v.normal = normalize(t * v.normal);
            v.tangent = normalize(t * v.tangent);
        }
    }
    
    void TriangleMeshNode::createSurfaceObjects() {
        m_numRefinedObjs = 0;
        for (const MaterialGroup &matGroup : m_matGroups)
            m_numRefinedObjs += matGroup.triangles.size();

        m_refinedObjs = new SLR::SingleSurfaceObject*[m_numRefinedObjs];
        m_trianglesForRendering = new SLR::Triangle[m_numRefinedObjs];
        uint32_t triIdxBase = 0;
        for (const MaterialGroup &matGroup : m_matGroups) {
            for (int tIdx = 0; tIdx < matGroup.triangles.size(); ++tIdx) {
                const Triangle &tri = matGroup.triangles[tIdx];
                
                uint32_t trIdx = triIdxBase + tIdx;
                SLR::Triangle &triR = m_trianglesForRendering[trIdx];
                const SLR::FloatTexture* alphaMap = nullptr;
                if (matGroup.alphaMap)
                    alphaMap = matGroup.alphaMap->getRaw();
                new (&triR) SLR::Triangle(&m_vertices[tri.vIdx[0]], &m_vertices[tri.vIdx[1]], &m_vertices[tri.vIdx[2]], alphaMap);
                if (matGroup.normalMap)
                    m_refinedObjs[trIdx] = new SLR::BumpSingleSurfaceObject(&triR, matGroup.material->getRaw(), matGroup.normalMap->getRaw());
                else
                    m_refinedObjs[trIdx] = new SLR::SingleSurfaceObject(&triR, matGroup.material->getRaw());
            }
            triIdxBase += matGroup.triangles.size();
        }
    }
}
