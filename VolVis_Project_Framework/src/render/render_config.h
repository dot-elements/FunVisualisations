#pragma once
#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <cstring> // memcmp  // macOS change TH

namespace render {

enum class RenderMode {
    RenderSlicer,
    RenderMIP,
    RenderIso,
    RenderComposite,
    RenderTF2D,
    RenderIsoCartoon
};

struct RenderConfig {
    RenderMode renderMode { RenderMode::RenderIsoCartoon };
    glm::ivec2 renderResolution;

    bool volumeShading { true };
    float isoValue { 95.0f };

    // 1D transfer function.
    std::array<glm::vec4, 256> tfColorMap;
    // Used to convert from a value to an index in the color map.
    // index = (value - start) / range * tfColorMap.size();
    float tfColorMapIndexStart;
    float tfColorMapIndexRange;

    // 2D transfer function.
    float TF2DIntensity;
    float TF2DRadius;
    glm::vec4 TF2DColor;
    glm::vec3 modelColor { 1.f, 0.f, 0.f };
    glm::vec3 coolColor { 0.f, 0.f, .4f };
    glm::vec3 warmColor { .4f, .4f, 0.f };
    float alphaColor {.2f};
    float betaColor {.6f};

};

// NOTE(Mathijs): should be replaced by C++20 three-way operator (aka spaceship operator) if we require C++ 20 support from Linux users (GCC10 / Clang10).
inline bool operator==(const RenderConfig& lhs, const RenderConfig& rhs)
{
    return std::memcmp(&lhs, &rhs, sizeof(RenderConfig)) == 0;
}
inline bool operator!=(const RenderConfig& lhs, const RenderConfig& rhs)
{
    return !(lhs == rhs);
}

}