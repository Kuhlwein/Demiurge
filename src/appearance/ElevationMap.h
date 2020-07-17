//
// Created by kuhlwein on 7/14/20.
//

#ifndef DEMIURGE_ELEVATIONMAP_H
#define DEMIURGE_ELEVATIONMAP_H


#include "Appearance.h"
#include "GradientMenu.h"
#include <imgui/imgui_color_gradient.h>
class Texture;

class ElevationMap : public Appearance {
public:
	ElevationMap();
	void prepare(Project* p) override;
	void unprepare(Project* p) override;
	Shader* getShader() override;
private:
	bool update_self(Project* p) override;

	bool first = true;
	float scale=1.0f;

	Texture* texture_land;
	Texture* texture_ocean;

	GradientMenu* gradient_land;
	GradientMenu* gradient_ocean;

	Shader* shader;
};


#endif //DEMIURGE_ELEVATIONMAP_H
