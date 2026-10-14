#pragma once

#include "glory/engine/gl_api.hpp"

namespace glory::engine {

class GroundPlane {
public:
    explicit GroundPlane(GlApi& gl);
    ~GroundPlane();
    GroundPlane(const GroundPlane&) = delete;
    GroundPlane& operator=(const GroundPlane&) = delete;

    void draw() const noexcept;

private:
    GlApi& gl_;
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
};

} // namespace glory::engine
