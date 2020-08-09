//
// Created by kuhlwein on 8/9/20.
//

#ifndef DEMIURGE_ROBINSON_H
#define DEMIURGE_ROBINSON_H

#include "Canvas.h"

class Project;

class Robinson : public AbstractCanvas {
public:
	Robinson(Project* project);
	glm::vec2 inverseTransform(glm::vec2 xy) override;
	Shader* inverseShader() override;
	bool isInterruptible() override;
protected:
	glm::vec2 getScale() override;
	glm::vec2 getLimits() override;
};

#endif //DEMIURGE_ROBINSON_H
