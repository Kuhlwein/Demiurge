//
// Created by kuhlwein on 8/8/20.
//

#ifndef DEMIURGE_HAMMER_H
#define DEMIURGE_HAMMER_H


#include "Canvas.h"

class Project;

class Hammer : public AbstractCanvas {
public:
	Hammer(Project* project);
	glm::vec2 inverseTransform(glm::vec2 xy) override;
	Shader* inverseShader() override;
	bool isInterruptible() override;
protected:
	glm::vec2 getScale() override;
	glm::vec2 getLimits() override;
};


#endif //DEMIURGE_HAMMER_H
