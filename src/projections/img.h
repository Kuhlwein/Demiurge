//
// Created by kuhlwein on 7/6/20.
//

#ifndef DEMIURGE_IMG_H
#define DEMIURGE_IMG_H


#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include "Vbo.h"
#include "Shader.h"
#include "Canvas.h"

class Project;

class img : public Canvas {
public:
	img(Project* project);
	~img();
	void render() override;
	void update() override;
	glm::vec2 mousePos(ImVec2 pos) override;
	Shader* projection_shader() override;

private:
	Vbo *vbo;
	float x, y, z;
	void pan(float dx, float dy);

	float FOVY; //radian
	float TANFOV;
	float Z_NEAR;
	float Z_FAR;
	float ZOOM;

	float canvasAspect;
};


#endif //DEMIURGE_IMG_H
