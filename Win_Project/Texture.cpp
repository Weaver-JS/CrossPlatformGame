#ifndef TEXTURE_CPP
#define TEXTURE_CPP

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "../Gameplay/stb/stb_image.h"

#include "GL/glew.h"

#include "Framework.h"
#include "GL/wglew.h"

namespace Texture {
	/** Load and generate the texture,**/
	GLuint LoadTexture(const char* path) {
		GLuint result;
		int x, y, comp;
		unsigned char *data = stbi_load(path, &x, &y, &comp, 4);

		glGenTextures(1, &result);
		glBindTexture(GL_TEXTURE_2D, result);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		glBindTexture(GL_TEXTURE_2D, 0);
		return result;
	}

	GLuint GenerateFontImgui() {
		GLuint imgui_texture = 0;
		
		glGenTextures(1, &imgui_texture);
		// TODO reload imgui
		uint8_t* pixels;
		int width, height;
		ImGui::GetIO().Fonts[0].GetTexDataAsRGBA32(&pixels, &width, &height, nullptr);
		
		glBindTexture(GL_TEXTURE_2D, imgui_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		GLenum formats[4] = { GL_RED, GL_RG, GL_RGB, GL_RGBA };

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		glBindTexture(GL_TEXTURE_2D, 0);
		
		return imgui_texture;
	}
}

#endif // !TEXTURE_CPP
