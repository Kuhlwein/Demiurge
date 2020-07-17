//
// Created by kuhlwein on 7/17/20.
//

#ifndef DEMIURGE_GRADIENTMENU_H
#define DEMIURGE_GRADIENTMENU_H

#include <string>
#include <imgui/imgui_color_gradient.h>

class Texture;

class GradientMenu {
public:
	GradientMenu(std::string name, std::vector<int> colors = std::vector<int>{0,0,0,255,255,255});
	void update();
	void setGradient(std::vector<int> colors);
	void toTexture(Texture* texture);
private:
	std::string name;
	ImGradient gradient;

	ImGradientMark* draggingMark = nullptr;
	ImGradientMark* selectedMark = nullptr;
};


#endif //DEMIURGE_GRADIENTMENU_H
