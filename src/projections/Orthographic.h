//
// Created by kuhlwein on 7/2/20.
//

#ifndef DEMIURGE_ORTHOGRAPHIC_H
#define DEMIURGE_ORTHOGRAPHIC_H

#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include "Vbo.h"
#include "Shader.h"
#include "Canvas.h"

class Project;

class Orthographic : public Canvas {
public:
	Orthographic(Project* p);
	void render() override;
	void update() override;
	glm::vec2 mousePos(ImVec2 pos) override;
	Shader* projection_shader() override;
private:
	Vbo *vbo;
	float windowAspect;
	float delta_phi=M_PI/2;
	float delta_theta=-M_PI/2;
	float z;
	double ZOOM;
};


#endif //DEMIURGE_ORTHOGRAPHIC_H
