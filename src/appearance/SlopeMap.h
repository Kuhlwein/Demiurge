//
// Created by kuhlwein on 7/18/20.
//

#ifndef DEMIURGE_SLOPEMAP_H
#define DEMIURGE_SLOPEMAP_H

#include "Appearance.h"
#include "GradientMenu.h"

class Texture;

class SlopeMap : public Appearance {
public:
	SlopeMap();
	void prepare(Project* p) override;
	void unprepare(Project* p) override;
	Shader* getShader() override;
private:
	bool update_self(Project* p) override;

	bool first = true;

	Texture* slope_texture;
	GradientMenu* gradient;
	Shader* shader;

	float zfactor=1.0;
};


#endif //DEMIURGE_SLOPEMAP_H
