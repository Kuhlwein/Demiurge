//
// Created by kuhlwein on 7/17/20.
//

#ifndef DEMIURGE_ASPECTMAP_H
#define DEMIURGE_ASPECTMAP_H


#include "Appearance.h"
#include "GradientMenu.h"

class Texture;

class AspectMap : public Appearance {
public:
	AspectMap();
	void prepare(Project* p) override;
	void unprepare(Project* p) override;
	Shader* getShader() override;
private:
	bool update_self(Project* p) override;

	bool first = true;

	Texture* aspect_texture;
	GradientMenu* gradient;
	Shader* shader;
};


#endif //DEMIURGE_ASPECTMAP_H
