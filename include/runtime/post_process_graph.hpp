#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Balmung::Runtime {

enum class PostPassType : std::uint8_t {
    Bloom,
    ToneMap,
    ColorGrading,
    Vignette,
    Sharpen,
    StylizedPixel,
};

struct PostPassDesc {
    PostPassType type = PostPassType::ToneMap;
    bool enabled = true;
    float intensity = 1.0f;
};

class PostProcessGraph {
public:
    void clear();
    void set_default_tier_a_2d_stack();
    void set_default_tier_b_unified_stack();
    void add_pass(PostPassDesc pass);

    [[nodiscard]] const std::vector<PostPassDesc>& passes() const noexcept { return _passes; }
    [[nodiscard]] std::string debug_summary() const;

private:
    std::vector<PostPassDesc> _passes;
};

} // namespace Balmung::Runtime



