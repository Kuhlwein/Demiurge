//
// Created by kuhlwein on 7/2/20.
//

#ifndef DEMIURGE_MOLLWEIDE_H
#define DEMIURGE_MOLLWEIDE_H


#include "Canvas.h"

class Project;

class Mollweide : public AbstractCanvas {
	public:
		Mollweide(Project* project);
		glm::vec2 inverseTransform(glm::vec2 xy) override;
		Shader* inverseShader() override;
protected:
		glm::vec2 getScale() override;
		glm::vec2 getLimits() override;
};


#endif //DEMIURGE_MOLLWEIDE_H
