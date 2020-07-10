//
// Created by kuhlwein on 7/10/20.
//

#ifndef DEMIURGE_SINUSOIDAL_H
#define DEMIURGE_SINUSOIDAL_H


#include "Canvas.h"

class Project;

class Sinusoidal : public AbstractCanvas {
public:
	Sinusoidal(Project* project);
	glm::vec2 inverseTransform(glm::vec2 xy) override;
	Shader* inverseShader() override;
	bool isInterruptible() override;
protected:
	glm::vec2 getScale() override;
	glm::vec2 getLimits() override;
};


#endif //DEMIURGE_SINUSOIDAL_H
