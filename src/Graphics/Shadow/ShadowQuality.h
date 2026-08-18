#pragma once

#include <glm/glm.hpp>
#include <Core/Types/Settings.h>

namespace Lindo {
namespace Graphics {

//==============================================================================
// Runtime-adjustable shadow quality. Higher tiers trade GPU cost for fidelity.
//==============================================================================

// Aggregated, CPU-side settings that drive both the shadow passes and the
// forward shader uniform block. Kept decoupled from any single light type.
struct ShadowQualitySettings {
    int   cascadeCount      = 4;      // number of directional cascades
    int   shadowMapSize     = 2048;   // per-map resolution (width == height)
    bool  pcssEnabled       = true;   // percentage-closer soft shadows
    int   pcfKernel         = 3;      // 3 => 3x3, 5 => 5x5 (used when !PCSS)
    float texelSpacing      = 1.0f;   // spacing multiplier for PCF taps
    float minDepthBias      = 0.005f; // constant component of depth bias
    float maxDepthBias      = 0.05f;  // slope-scaled component of depth bias
    float normalBias        = 0.02f;  // offset cast surface along normal
    float penumbraScale     = 1.0f;   // PCSS blocker-search scale
    float cascadeBlend      = 0.1f;   // blend band between cascades (view depth)
    int   maxPointShadows   = 4;      // simultaneous point-light shadow maps
    int   pointShadowSize   = 1024;   // cubemap face resolution
    int   spotShadowSize    = 1024;   // spot shadow map resolution
};

// Maps a quality enum to concrete numeric settings.
inline ShadowQualitySettings qualitySettingsOf(ShadowQuality q) {
    ShadowQualitySettings s;
    switch (q) {
        case ShadowQuality::Off:
            s.cascadeCount = 0;
            s.shadowMapSize = 0;
            s.pcssEnabled = false;
            s.pcfKernel = 0;
            break;
        case ShadowQuality::Low:
            s.cascadeCount = 1;
            s.shadowMapSize = 1024;
            s.pcssEnabled = false;
            s.pcfKernel = 3;
            s.texelSpacing = 1.0f;
            s.maxDepthBias = 0.06f;
            s.normalBias = 0.03f;
            s.pointShadowSize = 512;
            s.spotShadowSize = 512;
            break;
        case ShadowQuality::Medium:
            s.cascadeCount = 2;
            s.shadowMapSize = 2048;
            s.pcssEnabled = false;
            s.pcfKernel = 5;
            s.texelSpacing = 1.0f;
            s.maxDepthBias = 0.05f;
            s.normalBias = 0.025f;
            s.pointShadowSize = 1024;
            s.spotShadowSize = 1024;
            break;
        case ShadowQuality::High:
            s.cascadeCount = 3;
            s.shadowMapSize = 2048;
            s.pcssEnabled = true;
            s.penumbraScale = 1.0f;
            s.maxDepthBias = 0.04f;
            s.normalBias = 0.02f;
            s.pointShadowSize = 2048;
            s.spotShadowSize = 2048;
            break;
        case ShadowQuality::Ultra:
        default:
            s.cascadeCount = 4;
            s.shadowMapSize = 4096;
            s.pcssEnabled = true;
            s.penumbraScale = 1.0f;
            s.maxDepthBias = 0.03f;
            s.normalBias = 0.015f;
            s.pointShadowSize = 4096;
            s.spotShadowSize = 4096;
            break;
    }
    return s;
}

} // namespace Graphics
} // namespace Lindo
