//
// Created by kuhlwein on 4/11/20.
//


#include "Project.h"
#include "view.h"

bool view::normal_map(Project *project) {
	project->set_terrain_shader(draw_normal);
	return true;
}