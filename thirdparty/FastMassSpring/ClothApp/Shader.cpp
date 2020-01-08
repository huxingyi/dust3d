#include "Shader.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// GLSHADER ///////////////////////////////////////////////////////////////////////////////////
GLShader::GLShader(GLenum shaderType) : handle(glCreateShader(shaderType)) {};

GLShader::operator GLuint() const {
	return handle;
}

GLShader::~GLShader() {
	glDeleteShader(handle);
}

void GLShader::compile(const char* source) {
	GLint compiled = 0;  // Compiled flag
	const char *ptrs[] = { source };
	const GLint lens[] = { std::strlen(source) };
	glShaderSource(handle, 1, ptrs, lens);
	glCompileShader(handle);
	glGetShaderiv(handle, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint logSize = 0;
		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logSize);
		std::vector<GLchar> errorLog(logSize);
		glGetShaderInfoLog(handle, logSize, &logSize, &errorLog[0]);
		std::cerr << &errorLog[0] << std::endl;
		throw std::runtime_error("Failed to compile shader.");
	}
}

void GLShader::compile(std::ifstream& source) {
	std::vector<char> text;
	source.seekg(0, std::ios_base::end);
	std::streampos fileSize = source.tellg();
	text.resize(fileSize);

	source.seekg(0, std::ios_base::beg);
	source.read(&text[0], fileSize);
	compile(&text[0]);
}

// GLPROGRAM //////////////////////////////////////////////////////////////////////////////////
GLProgram::GLProgram() : handle(glCreateProgram()) {}

void GLProgram::link(const GLShader& vshader, const GLShader& fshader) {
	GLint linked = 0; // Linked flag
	glAttachShader(handle, vshader);
	glAttachShader(handle, fshader);
	glLinkProgram(handle);
	glDetachShader(handle, vshader);
	glDetachShader(handle, fshader);
	glGetProgramiv(handle, GL_LINK_STATUS, &linked);
	if (!linked)
		throw std::runtime_error("Failed to link shaders.");

	// get camera uniforms
	uModelViewMatrix = glGetUniformLocation(handle, "uModelViewMatrix");
	uProjectionMatrix = glGetUniformLocation(handle, "uProjectionMatrix");

	// post link
	postLink();

}

void GLProgram::postLink() {}

void GLProgram::setUniformMat4(GLuint unif, glm::mat4 m) {
	glUseProgram(handle);
	glUniformMatrix4fv(unif,
		1, GL_FALSE, glm::value_ptr(m[0]));
	glUseProgram(0);
}


void GLProgram::setModelView(glm::mat4 m) {
	setUniformMat4(uModelViewMatrix, m);
}

void GLProgram::setProjection(glm::mat4 m) {
	setUniformMat4(uProjectionMatrix, m);
}

GLProgram::operator GLuint() const { return handle; }

GLProgram::~GLProgram() { glDeleteProgram(handle); }

// PROGRAM INPUT //////////////////////////////////////////////////////////////////////////////

ProgramInput::ProgramInput() {
	// generate buffers
	glGenBuffers(4, &vbo[0]);

	// generate vertex array object
	glGenVertexArrays(1, &handle);
	glBindVertexArray(handle);

	// positions
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// normals
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[3]);

	glBindVertexArray(0);

}

void ProgramInput::bufferData(unsigned int index, void* buff, size_t size) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo[index]);
	glBufferData(GL_ARRAY_BUFFER, size,
		buff, GL_STATIC_DRAW);
}
void ProgramInput::setPositionData(float* buff, unsigned int len) {
	bufferData(0, buff, sizeof(float) * len);
}

void ProgramInput::setNormalData(float* buff, unsigned int len) {
	bufferData(1, buff, sizeof(float) * len);
}

void ProgramInput::setTextureData(float* buff, unsigned int len) {
	bufferData(2, buff, sizeof(float) * len);
}

void ProgramInput::setIndexData(unsigned int* buff, unsigned int len) {
	bufferData(3, buff, sizeof(unsigned int) * len);
}

ProgramInput::operator GLuint() const {
	return handle;
}


ProgramInput::~ProgramInput() {
	glDeleteBuffers(4, vbo);
	glDeleteVertexArrays(1, &handle);
}

// SHADER PROGRAMS ////////////////////////////////////////////////////////////////////////////
PhongShader::PhongShader() : GLProgram() {}
void PhongShader::postLink() {
	// get uniforms
	uAlbedo = glGetUniformLocation(handle, "uAlbedo");
	uAmbient = glGetUniformLocation(handle, "uAmbient");
	uLight = glGetUniformLocation(handle, "uLight");
}
void PhongShader::setAlbedo(const glm::vec3& albedo) {
	assert(uAlbedo >= 0);
	glUseProgram(*this);
	glUniform3f(uAlbedo, albedo[0], albedo[1], albedo[2]);
	glUseProgram(0);
}
void PhongShader::setAmbient(const glm::vec3& ambient) {
	assert(uAmbient >= 0);
	glUseProgram(*this);
	glUniform3f(uAmbient, ambient[0], ambient[1], ambient[2]);
	glUseProgram(0);
}
void PhongShader::setLight(const glm::vec3& light) {
	assert(uLight >= 0);
	glUseProgram(*this);
	glUniform3f(uLight, light[0], light[1], light[2]);
	glUseProgram(0);
}


PickShader::PickShader() : GLProgram() {}

void PickShader::postLink() {
	// get uniforms
	uTessFact = glGetUniformLocation(handle, "uTessFact");
}

void PickShader::setTessFact(unsigned int n) {
	assert(uTessFact >= 0);
	glUseProgram(*this);
	glUniform1i(uTessFact, n);
	glUseProgram(0);
}