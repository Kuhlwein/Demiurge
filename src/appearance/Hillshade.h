//
// Created by kuhlwein on 7/16/20.
//

#ifndef DEMIURGE_HILLSHADE_H
#define DEMIURGE_HILLSHADE_H

class Texture;
#include "Appearance.h"
#include "GradientMenu.h"

class Hillshade : public Appearance {
public:
	Hillshade();
	void prepare(Project* p) override;
	void unprepare(Project* p) override;
	Shader* getShader() override;
private:
	bool first = true;
	bool update_self(Project* p) override;

	Shader* shader;

	float zfactor=1.0, altitude=45, azimuth=315;

	Texture* hillshade_texture;
	GradientMenu* gradient;
};


#endif //DEMIURGE_HILLSHADE_H
