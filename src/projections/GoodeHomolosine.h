//
// Created by kuhlwein on 7/10/20.
//

#ifndef DEMIURGE_GOODEHOMOLOSINE_H
#define DEMIURGE_GOODEHOMOLOSINE_H

#include "Canvas.h"

class Project;

class GoodeHomolosine : public AbstractCanvas {
public:
	GoodeHomolosine(Project* project);
	glm::vec2 inverseTransform(glm::vec2 xy) override;
	Shader* inverseShader() override;
	bool isInterruptible() override;
protected:
	glm::vec2 getScale() override;
	glm::vec2 getLimits() override;
};

#endif //DEMIURGE_GOODEHOMOLOSINE_H
