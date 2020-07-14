//
// Created by kuhlwein on 7/14/20.
//

#ifndef DEMIURGE_ELEVATIONMAP_H
#define DEMIURGE_ELEVATIONMAP_H


#include "Appearance.h"
#include <imgui/imgui_color_gradient.h>
class Texture;

class ElevationMap : public Appearance {
public:
	ElevationMap();
	void prepare(Project* p) override;
	void unprepare(Project* p) override;
private:
	bool update_self(Project* p) override;

	bool first = true;

	Texture* texture_land;
	Texture* texture_ocean;

	ImGradient gradient;
	ImGradient gradient_ocean;

	Shader* shader;
};


#endif //DEMIURGE_ELEVATIONMAP_H
