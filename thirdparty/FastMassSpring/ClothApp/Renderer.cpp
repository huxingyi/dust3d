#include "Renderer.h"
#include <glm/gtc/type_ptr.hpp>

Renderer::Renderer() {}

void Renderer::setProgram(GLProgram* program) {
	assert(program != nullptr);
	assert((*program) > 0);
	this->program = program;
}

void Renderer::setProgramInput(ProgramInput* input) {
	this->input = input;
}

void Renderer::setModelview(const glm::mat4& mv) {
	assert(program != nullptr);
	assert((*program) > 0);
	glUseProgram(*program);
	program->setModelView(mv);
	glUseProgram(0);
}

void Renderer::setProjection(const glm::mat4& p) {
	assert(program != nullptr);
	assert((*program) > 0);
	glUseProgram(*program);
	program->setProjection(p);
	glUseProgram(0);
}

void Renderer::setElementCount(unsigned int n_elements) { this->n_elements = n_elements; }

void Renderer::draw() {
	assert(program != nullptr);
	assert((*program) > 0);
	glUseProgram(*program);
	glBindVertexArray(*input);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glDrawElements(GL_TRIANGLES, n_elements, GL_UNSIGNED_INT, 0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glBindVertexArray(0);
	glUseProgram(0);
}

