//
// Created by kuhlwein on 4/30/19.
//

#ifndef DEMIURGE_CANVAS_H
#define DEMIURGE_CANVAS_H


#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include "Vbo.h"
#include "Shader.h"

class Project;

class Canvas {
public:
    Canvas(Project* project);
    virtual void render() = 0;
    virtual void update() = 0;
	virtual glm::vec2 mousePos(ImVec2 pos) = 0;
	virtual Shader* projection_shader() = 0;

protected:
    Project* project;
};

class Equiretangular : public Canvas {
public:
	Equiretangular(Project* project);
	~Equiretangular();
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
	float windowAspect;
	int windowWidth;
	int windowHeight;
	float Z_NEAR;
	float Z_FAR;
	float ZOOM;

	float canvasAspect;
};

class Globe : public Canvas {
public:
	Globe(Project* p);
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


#endif //DEMIURGE_CANVAS_H
