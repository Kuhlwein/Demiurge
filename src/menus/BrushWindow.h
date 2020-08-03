//
// Created by kuhlwein on 7/13/20.
//

#ifndef DEMIURGE_BRUSHWINDOW_H
#define DEMIURGE_BRUSHWINDOW_H


#include <Menu.h>
#include <glm/glm/fwd.hpp>


class Project;
class Texture;

class BrushWindow : public Window {
public:
	BrushWindow(std::string title, Project* p);
	bool update(Project* p) override;
private:
	bool brush_window(Project* p);
	void handle_brush(Project* p);
	void set_hardness(float hardness);
	void brush(Project* p, glm::vec2 pos, glm::vec2 prev);
	void initbrush(Project* p);

	Texture* brush_tex;
	int brush_tex_size=512;

	float brush_size=50.0f, hardness = 0.5f, flow = 1.0f, limit=1.0f, value=1.0f;
	bool limitEnabled=true;
};


#endif //DEMIURGE_BRUSHWINDOW_H
