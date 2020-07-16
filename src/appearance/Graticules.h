//
// Created by kuhlwein on 7/15/20.
//

#ifndef DEMIURGE_GRATICULES_H
#define DEMIURGE_GRATICULES_H

#include "Appearance.h"

class Project;

class Graticules : public Appearance {
public:
	Graticules();
	void prepare(Project* p) override;
	void unprepare(Project* p) override;
	Shader* getShader() override;
private:
	bool update_self(Project* p) override;

	Shader* shader;
	float latitudinal=15.0f, longitudinal=15.0f;
	float color[4];
};


#endif //DEMIURGE_GRATICULES_H
