#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include <Windows.h>

#include "GL/glew.h"
#include "GL/wglew.h"

#include "../Gameplay/glm/glm.hpp"
#include "../Gameplay/glm/gtc/matrix_transform.hpp"

#include "../Gameplay/imgui/imgui.h"

#include <cassert>

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "winmm.lib")

struct VertexTN {
	glm::vec3 p;
	glm::vec4 c;
	glm::vec2 t;
};

struct VAO {
	GLuint vao;
	GLuint elementArrayBufferObject, vertexBufferObject;
	int numIndex;
};

struct InstanceData
{
	glm::mat4 modelMatrix;
	glm::vec4 colorModifier;
};

struct FullFile
{
	uint8_t* data;
	size_t size;
	uint64_t lastWriteTime;
};

struct RendererData
{
	// pack all render-related data into this struct
	GLint programID;
	VAO quadVAO;
	GLuint instanceDataBuffer;
	// map of all textures available
	GLuint textures[2];
	GLuint uniform;
};

#endif // !FRAMEWORK_H
