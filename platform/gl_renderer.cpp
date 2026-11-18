// gl_renderer.cpp - WebGL2 implementation of the immediate mode replacement.

#include "gl_renderer.h"

#if defined(MESHGEN_HEADLESS)

namespace render {
void Init() {}
void Shutdown() {}
void SetMatrices(const glm::mat4&, const glm::mat4&) {}
void SetFogEnabled(bool) {}
void SetFogRange(float, float) {}
void SetFogColor(const float*) {}
void SetTextureEnabled(bool) {}
void SetDepthMask(bool) {}
void Begin(Prim, float) {}
void Vertex(float, float, float, unsigned int, float, float) {}
void End() {}
}

#else

#include <GLES3/gl3.h>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <vector>

namespace render {

struct Vert
{
	float x, y, z;
	unsigned int color;
	float u, v;
};

static const char* s_vertexShader = R"(#version 300 es
precision highp float;
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_uv;
uniform mat4 u_proj;
uniform mat4 u_modelView;
uniform float u_pointSize;
out vec4 v_color;
out vec2 v_uv;
out float v_eyeDist;
void main()
{
	vec4 eye = u_modelView * vec4(a_pos, 1.0);
	v_eyeDist = length(eye.xyz);
	v_color = a_color;
	v_uv = a_uv;
	gl_PointSize = u_pointSize;
	gl_Position = u_proj * eye;
}
)";

static const char* s_fragmentShader = R"(#version 300 es
precision mediump float;
in vec4 v_color;
in vec2 v_uv;
in float v_eyeDist;
uniform sampler2D u_tex;
uniform int u_useTex;
uniform int u_fog;
uniform vec2 u_fogRange;
uniform vec4 u_fogColor;
out vec4 fragColor;
void main()
{
	vec4 c = v_color;
	if (u_useTex != 0)
		c *= texture(u_tex, v_uv);
	if (u_fog != 0)
	{
		float f = clamp((u_fogRange.y - v_eyeDist) / max(u_fogRange.y - u_fogRange.x, 0.0001), 0.0, 1.0);
		c.rgb = mix(u_fogColor.rgb, c.rgb, f);
	}
	fragColor = c;
}
)";

static GLuint s_program = 0;
static GLuint s_vao = 0;
static GLuint s_vbo = 0;
static GLuint s_checkerTex = 0;
static GLint u_proj = -1, u_modelView = -1, u_pointSize = -1, u_useTex = -1, u_fog = -1, u_fogRange = -1, u_fogColor = -1, u_tex = -1;

static glm::mat4 s_proj{ 1.0f };
static glm::mat4 s_modelView{ 1.0f };
static bool s_fogEnabled = false;
static float s_fogStart = 0.0f, s_fogEnd = 10.0f;
static float s_fogColor[4] = { 0.32f, 0.25f, 0.25f, 1.0f };
static bool s_texEnabled = false;

static Prim s_prim = Prim::Triangles;
static float s_size = 1.0f;
static std::vector<Vert> s_verts;
static std::vector<Vert> s_expanded;

static GLuint CompileShader(GLenum type, const char* src)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	GLint ok = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		char log[1024];
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		fprintf(stderr, "Shader compile failed: %s\n", log);
	}
	return shader;
}

static void CreateCheckerTexture()
{
	// Same pattern as the original GLCheckerTexture: grey border lines on white.
	const unsigned int col0 = 0xffd7d7d7; // duRGBA(215,215,215,255)
	const unsigned int col1 = 0xffffffff;
	static const int TSIZE = 64;
	static unsigned int data[TSIZE * TSIZE];

	glGenTextures(1, &s_checkerTex);
	glBindTexture(GL_TEXTURE_2D, s_checkerTex);

	int level = 0;
	int size = TSIZE;
	while (size > 0)
	{
		for (int y = 0; y < size; ++y)
			for (int x = 0; x < size; ++x)
				data[x + y * size] = (x == 0 || y == 0) ? col0 : col1;

		glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		size /= 2;
		level++;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Init()
{
	GLuint vs = CompileShader(GL_VERTEX_SHADER, s_vertexShader);
	GLuint fs = CompileShader(GL_FRAGMENT_SHADER, s_fragmentShader);
	s_program = glCreateProgram();
	glAttachShader(s_program, vs);
	glAttachShader(s_program, fs);
	glLinkProgram(s_program);
	GLint ok = 0;
	glGetProgramiv(s_program, GL_LINK_STATUS, &ok);
	if (!ok)
	{
		char log[1024];
		glGetProgramInfoLog(s_program, sizeof(log), nullptr, log);
		fprintf(stderr, "Program link failed: %s\n", log);
	}
	glDeleteShader(vs);
	glDeleteShader(fs);

	u_proj = glGetUniformLocation(s_program, "u_proj");
	u_modelView = glGetUniformLocation(s_program, "u_modelView");
	u_pointSize = glGetUniformLocation(s_program, "u_pointSize");
	u_useTex = glGetUniformLocation(s_program, "u_useTex");
	u_fog = glGetUniformLocation(s_program, "u_fog");
	u_fogRange = glGetUniformLocation(s_program, "u_fogRange");
	u_fogColor = glGetUniformLocation(s_program, "u_fogColor");
	u_tex = glGetUniformLocation(s_program, "u_tex");

	glGenVertexArrays(1, &s_vao);
	glGenBuffers(1, &s_vbo);
	glBindVertexArray(s_vao);
	glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vert), (void*)12);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)16);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	CreateCheckerTexture();
}

void Shutdown()
{
	if (s_checkerTex) glDeleteTextures(1, &s_checkerTex);
	if (s_vbo) glDeleteBuffers(1, &s_vbo);
	if (s_vao) glDeleteVertexArrays(1, &s_vao);
	if (s_program) glDeleteProgram(s_program);
	s_checkerTex = s_vbo = s_vao = s_program = 0;
}

void SetMatrices(const glm::mat4& projection, const glm::mat4& modelView)
{
	s_proj = projection;
	s_modelView = modelView;
}

void SetFogEnabled(bool enabled) { s_fogEnabled = enabled; }
void SetFogRange(float start, float end) { s_fogStart = start; s_fogEnd = end; }
void SetFogColor(const float color[4]) { for (int i = 0; i < 4; ++i) s_fogColor[i] = color[i]; }
void SetTextureEnabled(bool enabled) { s_texEnabled = enabled; }
void SetDepthMask(bool enabled) { glDepthMask(enabled ? GL_TRUE : GL_FALSE); }

void Begin(Prim prim, float size)
{
	s_prim = prim;
	s_size = size;
	s_verts.clear();
}

void Vertex(float x, float y, float z, unsigned int color, float u, float v)
{
	s_verts.push_back(Vert{ x, y, z, color, u, v });
}

void End()
{
	if (s_verts.empty() || !s_program)
		return;

	const std::vector<Vert>* verts = &s_verts;
	GLenum mode = GL_TRIANGLES;

	switch (s_prim)
	{
	case Prim::Points: mode = GL_POINTS; break;
	case Prim::Lines: mode = GL_LINES; break;
	case Prim::Triangles: mode = GL_TRIANGLES; break;
	case Prim::Quads:
		// WebGL has no quads: split each quad into two triangles.
		s_expanded.clear();
		s_expanded.reserve(s_verts.size() / 4 * 6);
		for (size_t i = 0; i + 3 < s_verts.size(); i += 4)
		{
			s_expanded.push_back(s_verts[i]);
			s_expanded.push_back(s_verts[i + 1]);
			s_expanded.push_back(s_verts[i + 2]);
			s_expanded.push_back(s_verts[i]);
			s_expanded.push_back(s_verts[i + 2]);
			s_expanded.push_back(s_verts[i + 3]);
		}
		verts = &s_expanded;
		mode = GL_TRIANGLES;
		break;
	}

	glUseProgram(s_program);
	glUniformMatrix4fv(u_proj, 1, GL_FALSE, glm::value_ptr(s_proj));
	glUniformMatrix4fv(u_modelView, 1, GL_FALSE, glm::value_ptr(s_modelView));
	glUniform1f(u_pointSize, s_size);
	glUniform1i(u_useTex, s_texEnabled ? 1 : 0);
	glUniform1i(u_fog, s_fogEnabled ? 1 : 0);
	glUniform2f(u_fogRange, s_fogStart, s_fogEnd);
	glUniform4fv(u_fogColor, 1, s_fogColor);
	glUniform1i(u_tex, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, s_texEnabled ? s_checkerTex : 0);

	glBindVertexArray(s_vao);
	glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
	glBufferData(GL_ARRAY_BUFFER, verts->size() * sizeof(Vert), verts->data(), GL_STREAM_DRAW);
	glDrawArrays(mode, 0, (GLsizei)verts->size());
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);

	s_verts.clear();
}

} // namespace render

#endif
