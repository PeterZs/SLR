//
//  BidirectionalPathTracingRenderer.h
//
//  Created by 渡部 心 on 2016/01/31.
//  Copyright (c) 2016年 渡部 心. All rights reserved.
//

#ifndef BidirectionalPathTracingRenderer_h
#define BidirectionalPathTracingRenderer_h

#include "../defines.h"
#include "../references.h"
#include "../Core/Renderer.h"

#include "../Core/geometry.h"
#include "../Core/directional_distribution_functions.h"

namespace SLR {
    class SLR_API BidirectionalPathTracingRenderer : public Renderer {
        struct SLR_API DDFQuery {
            Vector3D dir_sn;
            Normal3D gNormal_sn;
            int16_t wlHint;
            bool adjoint;
        };
        
        struct DDFProxy {
            virtual const void* getDDF() const = 0;
            virtual SampledSpectrum evaluate(const DDFQuery &query, const Vector3D &dir_sn) const = 0;
            virtual float evaluatePDF(const DDFQuery &query, const Vector3D &dir_sn) const = 0;
        };
        struct EDFProxy : public DDFProxy {
            const EDF* edf;
            EDFProxy(const EDF* _edf) : edf(_edf) {}
            const void* getDDF() const override { return edf; }
            SampledSpectrum evaluate(const DDFQuery &query, const Vector3D &dir_sn) const override {
                EDFQuery edfQuery;
                return edf->evaluate(edfQuery, dir_sn);
            }
            float evaluatePDF(const DDFQuery &query, const Vector3D &dir_sn) const override {
                EDFQuery edfQuery;
                return edf->evaluatePDF(edfQuery, dir_sn);
            }
        };
        struct BSDFProxy : public DDFProxy {
            const BSDF* bsdf;
            BSDFProxy(const BSDF* _bsdf) : bsdf(_bsdf) {}
            const void* getDDF() const override { return bsdf; }
            SampledSpectrum evaluate(const DDFQuery &query, const Vector3D &dir_sn) const override {
                BSDFQuery bsdfQuery(query.dir_sn, query.gNormal_sn, query.wlHint, DirectionType::All, query.adjoint);
                return bsdf->evaluate(bsdfQuery, dir_sn);
            }
            float evaluatePDF(const DDFQuery &query, const Vector3D &dir_sn) const override {
                BSDFQuery bsdfQuery(query.dir_sn, query.gNormal_sn, query.wlHint, DirectionType::All, query.adjoint);
                return bsdf->evaluatePDF(bsdfQuery, dir_sn);
            }
        };
        struct IDFProxy : public DDFProxy {
            const IDF* idf;
            IDFProxy(const IDF* _idf) : idf(_idf) {}
            const void* getDDF() const override { return idf; }
            SampledSpectrum evaluate(const DDFQuery &query, const Vector3D &dir_sn) const override {
                return idf->evaluate(dir_sn);
            }
            float evaluatePDF(const DDFQuery &query, const Vector3D &dir_sn) const override {
                return idf->evaluatePDF(dir_sn);
            }
        };
        
        struct BPTVertex {
            SurfacePoint surfPt;
            Vector3D dirIn_sn;
            Normal3D gNormal_sn;
            const DDFProxy* ddf;
            SampledSpectrum alpha;
            float areaPDF;
            float RRProb;
            float revAreaPDF;
            float revRRProb;
            int16_t wlFlags;
            BPTVertex(const SurfacePoint &surfacePoint, const DDFProxy* proxy, const SampledSpectrum &alp, float _areaPDF, float _RRProb, int16_t _wlFlags) :
            surfPt(surfacePoint), ddf(proxy), alpha(alp), areaPDF(_areaPDF), RRProb(_RRProb), wlFlags(_wlFlags) {}
        };
        
        struct Job {
            const Scene* scene;
            
            ArenaAllocator* mems;
            RandomNumberGenerator** rngs;
            
            const Camera* camera;
            float timeStart;
            float timeEnd;
            
            ImageSensor* sensor;
            uint32_t imageWidth;
            uint32_t imageHeight;
            uint32_t numPixelX;
            uint32_t numPixelY;
            uint32_t basePixelX;
            uint32_t basePixelY;
            
            // working area
            int16_t wlHint;
            std::vector<BPTVertex> lightVertices;
            std::vector<BPTVertex> eyeVertices;
            
            void kernel(uint32_t threadID);
            void generateSubPath(const WavelengthSamples &initWLs, const SampledSpectrum &initAlpha, const SLR::Ray &initRay, float dirPDF, float cosLast,
                                 bool adjoint, RandomNumberGenerator &rng, SLR::ArenaAllocator &mem);
            float calculateMISWeight(float lExtendAreaPDF, float lExtendRRProb, float eExtendAreaPDF, float eExtendRRProb,
                                     uint32_t numLVtx, uint32_t numEVtx) const;
        };
        
        uint32_t m_samplesPerPixel;
    public:
        BidirectionalPathTracingRenderer(uint32_t spp);
        void render(const Scene &scene, const RenderSettings &settings) const override;
    };
}

#endif /* BidirectionalPathTracingRenderer_hpp */