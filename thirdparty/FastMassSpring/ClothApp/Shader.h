#pragma once
#include <fstream>
#include <GL\glew.h>
#include <glm/glm.hpp>

class NonCopyable {
private:
	NonCopyable(const NonCopyable& other) = delete;
	NonCopyable& operator=(const NonCopyable& other) = delete;

public:
	NonCopyable() {}
};

class GLShader : public NonCopyable {
private:
	GLuint handle; // Shader handle

public:
	GLShader(GLenum shaderType);
	/*GLShader(GLenum shaderType, const char* source);
	GLShader(GLenum shaderType, std::ifstream& source);*/
	void compile(const char* source);
	void compile(std::ifstream& source);
	operator GLuint() const; // cast to GLuint
	~GLShader();
};

class GLProgram : public NonCopyable {
protected:
	GLuint handle;
	GLuint uModelViewMatrix, uProjectionMatrix;
	void setUniformMat4(GLuint unif, glm::mat4 m);

public:
	GLProgram();
	virtual void link(const GLShader& vshader, const GLShader& fshader);
	virtual void postLink();
	operator GLuint() const; // cast to GLuint
	void setModelView(glm::mat4 m);
	void setProjection(glm::mat4 m);

	~GLProgram();
};

class ProgramInput : public NonCopyable {
private:
	GLuint handle; // vertex array object handle
	GLuint vbo[4]; // vertex buffer object handles | position, normal, texture, index
	void bufferData(unsigned int index, void* buff, size_t size);

public:
	ProgramInput();

	void setPositionData(float* buff, unsigned int len);
	void setNormalData(float* buff, unsigned int len);
	void setTextureData(float* buff, unsigned int len);
	void setIndexData(unsigned int* buff, unsigned int len);

	operator GLuint() const; // cast to GLuint

	~ProgramInput();
};

class PhongShader : public GLProgram {
private:
	// Albedo | Ambient Light | Light Direction
	GLuint uAlbedo, uAmbient, uLight;

public:
	PhongShader();
	virtual void postLink();
	void setAlbedo(const glm::vec3& albedo);
	void setAmbient(const glm::vec3& ambient);
	void setLight(const glm::vec3& light);
};


class PickShader : public GLProgram {
private:
	GLuint uTessFact;

public:
	PickShader();
	virtual void postLink();
	void setTessFact(unsigned int n);
};