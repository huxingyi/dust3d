#pragma once
#include <glm/common.hpp>

#include "MassSpringSolver.h"
#include "Renderer.h"

class UserInteraction {
protected:
	typedef glm::vec3 vec3;
	typedef std::vector<unsigned char> color;

	int i; // index of fixed point
	float* vbuff; // vertex buffer
	CgPointFixNode* fixer; // point fixer
	Renderer* renderer; // pick shader renderer
	virtual int colorToIndex(color c) const = 0;

public:
	UserInteraction(Renderer* renderer, CgPointFixNode* fixer, float* vbuff);

	void setModelview(const glm::mat4& mv);
	void setProjection(const glm::mat4& p);

	void grabPoint(int mouse_x, int mouse_y); // grab point with color c
	void movePoint(vec3 v); // move grabbed point along mouse
	void releasePoint(); // release grabbed point;
};

class GridMeshUI : public UserInteraction {
protected:
	const unsigned int n; // grid width
	virtual int colorToIndex(color c) const;
public:
	GridMeshUI(Renderer* renderer, CgPointFixNode* fixer, float* vbuff, unsigned int n);
};