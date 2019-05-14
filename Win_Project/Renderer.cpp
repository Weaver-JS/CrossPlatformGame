#ifndef RENDERER_CPP
#define RENDERER_CPP

#include "Framework.h"

namespace Renderer {

	RendererData* InitRendererData() {
		return new RendererData();
	}

	void FinalizeRendererData(RendererData* rendererData) {
		delete rendererData;
	}

	void GenerateBuffer(RendererData* rendererData) {
		glGenBuffers(1, &rendererData->instanceDataBuffer); // creates a buffer. The buffer id is written to the variable "instanceDataBuffer"
		glBindBuffer(GL_UNIFORM_BUFFER, rendererData->instanceDataBuffer); // sets "instanceDataBuffer" as the current buffer
		glBufferData(  // sets the buffer content
			GL_UNIFORM_BUFFER,		// type of buffer
			sizeof(InstanceData),	// size of the buffer content
			nullptr,				// content of the buffer
			GL_DYNAMIC_DRAW);		// usage of the buffer. DYNAMIC -> will change frequently. DRAW -> from CPU to GPU

									// we initialize the buffer without content so we can later call "glBufferSubData". Here we are only reserving the size.

		glBindBuffer(GL_UNIFORM_BUFFER, 0); // set no buffer as the current buffer
	}

	void BindBuffer(RendererData* rendererData, InstanceData instanceData) {
		glBindBuffer(GL_UNIFORM_BUFFER, rendererData->instanceDataBuffer);
		glBufferSubData(GL_UNIFORM_BUFFER, // change the contents of the buffer
			0, // offset from the beginning of the buffer
			sizeof(InstanceData), // size of the changes
			(GLvoid*)&instanceData); // new content
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, rendererData->instanceDataBuffer); // sets "instanceDataBuffer" to the slot "0"
	}

	VAO CreateVertexArrayObject(const VertexTN* vertexs, int numVertex, const uint16_t* indexs, int numIndexs) {
		VAO vao = {};
		vao.numIndex = numIndexs;
		glGenVertexArrays(1, &vao.vao);
		glGenBuffers(1, &vao.elementArrayBufferObject);
		glGenBuffers(1, &vao.vertexBufferObject);
		glBindVertexArray(vao.vao);
		// Activate VAO to save the buffer information
		// element information
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao.elementArrayBufferObject);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t)* numIndexs, indexs, GL_STATIC_DRAW);
		// array buffer
		glBindBuffer(GL_ARRAY_BUFFER, vao.vertexBufferObject);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexTN)* numVertex, vertexs, GL_STATIC_DRAW);

		glVertexAttribPointer(0, // Vertex Attrib Index
			3, GL_FLOAT, // 3 floats
			GL_FALSE, // no normalization
			sizeof(VertexTN), // offset from a vertex to the next
			(GLvoid*)offsetof(VertexTN, p) // offset from the start of the buffer to the first vertex
		); // positions
		glVertexAttribPointer(1, // Vertex Attrib Index
			4, GL_FLOAT, // 4 floats
			GL_FALSE, // no normalization
			sizeof(VertexTN), // offset from a vertex to the next
			(GLvoid*)offsetof(VertexTN, c) // offset from the start of the buffer to the first vertex
		); // colors
		glVertexAttribPointer(2, // Vertex Attrib Index
			2, GL_FLOAT, // 2 floats
			GL_FALSE, // no normalization
			sizeof(VertexTN), // offset from a vertex to the next
			(GLvoid*)offsetof(VertexTN, t) // offset from the start of the buffer to the first vertex
		); // texture

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// reset default opengl state
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		return vao;
	}
	
	void DrawElements(RendererData* rendererData) {
		glBindVertexArray(rendererData->quadVAO.vao);
		glDrawElements(GL_TRIANGLES, rendererData->quadVAO.numIndex, GL_UNSIGNED_SHORT, 0);
	}
};
#endif