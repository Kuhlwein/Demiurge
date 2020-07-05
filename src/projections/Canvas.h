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



class AbstractCanvas : public Canvas {
public:
	AbstractCanvas(Project* project, glm::vec2 scale = glm::vec2(1,1));
	~AbstractCanvas();
	void render() override;
	void update() override;
	glm::vec2 mousePos(ImVec2 pos) override;
	Shader* projection_shader() override;
	virtual glm::vec2 inverseTransform(glm::vec2 xy) = 0;
	virtual Shader* inverseShader() = 0;


protected:
	Vbo *vbo;
	float x, y, z;
	void pan(float dx, float dy);

	float windowAspect;
	int windowWidth;
	int windowHeight;
	float ZOOM;
	glm::vec2 scale;
};


#endif //DEMIURGE_CANVAS_H
