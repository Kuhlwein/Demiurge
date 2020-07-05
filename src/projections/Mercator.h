//
// Created by kuhlwein on 7/5/20.
//

#ifndef DEMIURGE_MERCATOR_H
#define DEMIURGE_MERCATOR_H

#include "Canvas.h"

class Project;

class Mercator : public AbstractCanvas {
public:
	Mercator(Project* project);
	glm::vec2 inverseTransform(glm::vec2 xy) override;
	Shader* inverseShader() override;
protected:
	glm::vec2 getScale() override;
	glm::vec2 getLimits() override;
};

#endif //DEMIURGE_MERCATOR_H
