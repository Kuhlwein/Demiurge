//
// Created by kuhlwein on 7/5/20.
//

#ifndef DEMIURGE_EQUIRETANGULAR_H
#define DEMIURGE_EQUIRETANGULAR_H


#include "Canvas.h"

class Project;

class Equiretangular : public AbstractCanvas {
public:
	Equiretangular(Project* project);
	glm::vec2 inverseTransform(glm::vec2 xy) override;
	Shader* inverseShader() override;
protected:
	glm::vec2 getScale() override;
	glm::vec2 getLimits() override;
};


#endif //DEMIURGE_EQUIRETANGULAR_H
