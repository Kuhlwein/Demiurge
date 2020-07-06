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
	float windowWidth();
	float windowHeight();
	float windowAspect();
};

class AbstractCanvas : public Canvas {
public:
	AbstractCanvas(Project* project);
	~AbstractCanvas();
	void render() override;
	void update() override;
	glm::vec2 mousePos(ImVec2 pos) override;
	Shader* projection_shader() override;

protected:
	virtual glm::vec2 inverseTransform(glm::vec2 coord) = 0;
	virtual Shader* inverseShader() = 0;
	virtual glm::vec2 getScale() = 0;
	virtual glm::vec2 getLimits() = 0;

private:
	Vbo *vbo;
	float x, y, z;
	void pan(float dx, float dy);
	float ZOOM;
};


#endif //DEMIURGE_CANVAS_H
