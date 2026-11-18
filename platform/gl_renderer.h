// gl_renderer.h - small replacement for the OpenGL 1.x immediate mode drawing
// that MeshGenerator used. Works on WebGL2 / OpenGL ES 3.
#pragma once

#include <glm/glm.hpp>

namespace render {

enum class Prim { Points, Lines, Triangles, Quads };

void Init();
void Shutdown();

// Camera matrices for the 3D scene.
void SetMatrices(const glm::mat4& projection, const glm::mat4& modelView);

void SetFogEnabled(bool enabled);
void SetFogRange(float start, float end);
void SetFogColor(const float color[4]);

void SetTextureEnabled(bool enabled);   // checker texture used by Recast debug draw
void SetDepthMask(bool enabled);

void Begin(Prim prim, float size = 1.0f);
void Vertex(float x, float y, float z, unsigned int color, float u = 0.0f, float v = 0.0f);
void End();

} // namespace render
