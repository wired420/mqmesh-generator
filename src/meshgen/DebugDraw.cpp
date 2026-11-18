//
// DebugDraw.cpp
//
// Recast debug draw implementation. The original used OpenGL 1.x immediate
// mode; this version forwards to a small buffered renderer so it runs on WebGL2.

#include "meshgen/DebugDraw.h"
#include "platform/gl_renderer.h"

//----------------------------------------------------------------------------

void DebugDrawGL::depthMask(bool state)
{
	render::SetDepthMask(state);
}

void DebugDrawGL::texture(bool state)
{
	render::SetTextureEnabled(state);
}

void DebugDrawGL::begin(duDebugDrawPrimitives prim, float size)
{
	switch (prim)
	{
	case DU_DRAW_POINTS: render::Begin(render::Prim::Points, size); break;
	case DU_DRAW_LINES: render::Begin(render::Prim::Lines, size); break;
	case DU_DRAW_TRIS: render::Begin(render::Prim::Triangles, size); break;
	case DU_DRAW_QUADS: render::Begin(render::Prim::Quads, size); break;
	};
}

void DebugDrawGL::vertex(const float* pos, unsigned int color)
{
	render::Vertex(pos[0], pos[1], pos[2], color);
}

void DebugDrawGL::vertex(const float x, const float y, const float z, unsigned int color)
{
	render::Vertex(x, y, z, color);
}

void DebugDrawGL::vertex(const float* pos, unsigned int color, const float* uv)
{
	render::Vertex(pos[0], pos[1], pos[2], color, uv[0], uv[1]);
}

void DebugDrawGL::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
	render::Vertex(x, y, z, color, u, v);
}

void DebugDrawGL::end()
{
	render::End();
}
