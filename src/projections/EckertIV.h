//
// Created by kuhlwein on 7/11/20.
//

#ifndef DEMIURGE_ECKERTIV_H
#define DEMIURGE_ECKERTIV_H


#include "Canvas.h"

class Project;

class EckertIV : public AbstractCanvas {
public:
	EckertIV(Project* project);
	glm::vec2 inverseTransform(glm::vec2 xy) override;
	Shader* inverseShader() override;
	bool isInterruptible() override;
protected:
	glm::vec2 getScale() override;
	glm::vec2 getLimits() override;
};


#endif //DEMIURGE_ECKERTIV_H
