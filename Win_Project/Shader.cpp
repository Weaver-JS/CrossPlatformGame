#ifndef SHADER_CPP
#define SHADER_CPP

#include <iostream>

#include "GL/glew.h"
#include "GL/wglew.h"

namespace Shader {

	bool CompileShader(GLint &shaderId_, const char* shaderCode, GLenum shaderType)
	{
		shaderId_ = glCreateShader(shaderType);
		glShaderSource(shaderId_, 1, (const GLchar *const*)&shaderCode, nullptr);
		glCompileShader(shaderId_);

		GLint success = 0;
		glGetShaderiv(shaderId_, GL_COMPILE_STATUS, &success);

		{
			GLint maxLength = 0;
			glGetShaderiv(shaderId_, GL_INFO_LOG_LENGTH, &maxLength);

			if (maxLength > 0)
			{
				//The maxLength includes the NULL character
				GLchar *infoLog = new GLchar[maxLength];
				glGetShaderInfoLog(shaderId_, maxLength, &maxLength, &infoLog[0]);

				OutputDebugStringA(infoLog);
				OutputDebugStringA("\n");

				delete[] infoLog;
			}
		}
		return (success != GL_FALSE);
	}

	bool LinkShaders(GLint &programId_, GLint &vs, GLint &ps)
	{
		programId_ = glCreateProgram();

		glAttachShader(programId_, vs);

		glAttachShader(programId_, ps);

		glLinkProgram(programId_);

		GLint success = 0;
		glGetProgramiv(programId_, GL_LINK_STATUS, &success);
		{
			GLint maxLength = 0;
			glGetProgramiv(programId_, GL_INFO_LOG_LENGTH, &maxLength);

			if (maxLength > 0)
			{
				//The maxLength includes the NULL character
				GLchar *infoLog = new GLchar[maxLength];
				glGetProgramInfoLog(programId_, maxLength, &maxLength, &infoLog[0]);

				OutputDebugStringA(infoLog);
				OutputDebugStringA("\n");

				delete[] infoLog;
			}
		}
		return (success != GL_FALSE);
	}
	/**
		Compile and link the shaders
		Input: Data of the vertex and frame shaders
		Output: The program
	*/
	bool initShaders(GLint &programID_, const char* vsData, const char* fsData) {

		bool dataInitialized = false;
		GLint vs_ = 0, ps_ = 0;
		if (Shader::CompileShader(vs_, vsData, GL_VERTEX_SHADER) &&
			Shader::CompileShader(ps_, fsData, GL_FRAGMENT_SHADER) &&
			Shader::LinkShaders(programID_, vs_, ps_))
		{
			dataInitialized = true;
			std::cout << "Failed to Compile shaders in initShaders()" << std::endl;
		}

		if (vs_ > 0)
			glDeleteShader(vs_);
		if (ps_ > 0)
			glDeleteShader(ps_);

		return dataInitialized;
	}
}
#endif // !SHADER_CPP
