//
// Created by kuhlwein on 7/15/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <cmath>
#include "Graticules.h"
#include "Shader.h"

bool Graticules::update_self(Project *p) {

	ImGui::InputFloat("Longitudinal spacing",&longitudinal);
	ImGui::InputFloat("Latitudinal spacing",&latitudinal);

	if(std::fmod(180.0f,longitudinal)>1e-6 || std::fmod(180.0f,latitudinal)>1e-6) {
		ImGui::Text("The chosen spacings might not grant equidistant graticules!");
	}

	ImGui::ColorEdit4("Graticule color",color,ImGuiColorEditFlags_AlphaBar);

	if (ImGui::Button("Apply")) {
		return true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		return true;
	}
	return false;
}

Shader *Graticules::getShader() {
	return shader;
}

Graticules::Graticules() : Appearance("Graticules") {
	for(float &c : color) c = 1.0f;
	shader = Shader::builder()
			.include(def_pi)
			.include(cornerCoords)
			.include(graticules)
			.create(replaceSID(R"(
uniform float grat_SID;
uniform vec4 grat_color_SID;

)"),replaceSID("draw_graticules(fc,st_p,grat_SID,grat_color_SID);"));
}

void Graticules::prepare(Project *p) {
	int id = glGetUniformLocation(p->program->getId(),replaceSID("grat_SID").c_str());
	glUniform1f(id,longitudinal);
	id = glGetUniformLocation(p->program->getId(),replaceSID("grat_color_SID").c_str());
	glUniform4fv(id,1,color);
}

void Graticules::unprepare(Project *p) {

}
