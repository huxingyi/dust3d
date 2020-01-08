#include "UserInteraction.h"
#include <GL/glew.h>
#include <iostream>
#include <cmath>

UserInteraction::UserInteraction(Renderer* renderer, CgPointFixNode* fixer, float* vbuff) 
	: renderer(renderer), vbuff(vbuff), fixer(fixer), i(-1) {}

void UserInteraction::setModelview(const glm::mat4& mv) { renderer->setModelview(mv); }
void UserInteraction::setProjection(const glm::mat4& p) { renderer->setProjection(p); }

void UserInteraction::grabPoint(int mouse_x, int mouse_y){
	// render scene
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_FRAMEBUFFER_SRGB);
	renderer->draw();
	glFlush();

	// read color
	color c(3);
	glReadPixels(mouse_x, mouse_y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &c[0]);
	i = colorToIndex(c);
	if (i != -1) fixer->fixPoint(i);

	// return to normal state
	glClearColor(0.25f, 0.25f, 0.25f, 0);
	glEnable(GL_FRAMEBUFFER_SRGB);
}

void UserInteraction::releasePoint() { if (i == -1) return; fixer->releasePoint(i); i = -1; }
void UserInteraction::movePoint(vec3 v) {
	if (i == -1) return;
	fixer->releasePoint(i);
	for(int j = 0; j < 3; j++)
		vbuff[3 * i + j] += v[j];
	fixer->fixPoint(i);
}

GridMeshUI::GridMeshUI(Renderer* renderer, CgPointFixNode* fixer, float* vbuff, unsigned int n)
	: UserInteraction(renderer, fixer, vbuff), n(n) {}

int GridMeshUI::colorToIndex(color c) const {
	if (c[2] != 51) return -1;
	int vx = std::round((n - 1) * c[0] / 255.0);
	int vy = std::round((n - 1) * c[1] / 255.0);
	return n * vy + vx;
}