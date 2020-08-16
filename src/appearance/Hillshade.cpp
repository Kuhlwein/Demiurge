//
// Created by kuhlwein on 7/16/20.
//

#include <Shader.h>
#include <imgui/imgui.h>
#include "Hillshade.h"
#include "Project.h"

Hillshade::Hillshade() : Appearance("Hillshade") {
	hillshade_texture = new Texture(100, 1, GL_RGBA, "gradient_hillshade_"+sid, GL_LINEAR);
	gradient = new GradientMenu("Hillshade gradient");
	gradient->setGradient(std::vector<int>{0, 0, 0, 255, 255, 255});
	gradient->toTexture(hillshade_texture);
	shader = Shader::builder()
			.include(fragmentColor)
			.include(def_pi)
			.include(get_slope)
			.include(get_aspect)
			.create(replaceSID(R"(
uniform float z_factor_SID;
uniform float zenith_SID;
uniform float azimuth_SID;
uniform sampler2D gradient_hillshade_SID;
)"),replaceSID(R"(
{
float slope =  get_slope(z_factor_SID,projection(st));
float aspect = get_aspect(projection(st));

float hillshade = ((cos(zenith_SID) * cos(slope)) + (sin(zenith_SID) * sin(slope) * cos(-azimuth_SID + M_PI/2 - aspect)));


vec4 k = texture(gradient_hillshade_SID,vec2(hillshade,0));
fc = fc*(1-k.a) + k*(k.a);
}
)"));
}

bool Hillshade::update_self(Project *p) {
	if (first) first = false;

	ImGui::DragFloat("Z-factor",&zfactor);
	ImGui::DragFloat("Altitude",&altitude,1.0,0,90);
	ImGui::DragFloat("Azimuth",&azimuth,1.0,0,360);
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	gradient->update();


	int current = -1;
	const char* items[] = { "Grayscale","Green-yellow-red"};
	if(ImGui::Combo("Preset",&current,items,IM_ARRAYSIZE(items))) {
		switch (current) {
			case 0:
				gradient->setGradient(std::vector<int>{0, 0, 0, 255, 255, 255});
				break;
			case 1:
				gradient->setGradient(
						std::vector<int>{31, 70, 41, 111, 165, 67, 243, 236, 34, 246, 145, 29, 212, 50, 37});
				break;
		}
	}
	gradient->toTexture(hillshade_texture);

	if (ImGui::Button("Apply")) {
		first = true;
		return true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		first = true;
		return true;
	}
	return false;
}

Shader *Hillshade::getShader() {
	return shader;
}

void Hillshade::unprepare(Project *p) {
	p->remove_texture(hillshade_texture);
}

void Hillshade::prepare(Project *p) {
	p->add_texture(hillshade_texture);
	int id = glGetUniformLocation(p->program->getId(),replaceSID("z_factor_SID").c_str());
	glUniform1f(id,zfactor);
	id = glGetUniformLocation(p->program->getId(),replaceSID("azimuth_SID").c_str());
	glUniform1f(id,azimuth/180*M_PI);
	id = glGetUniformLocation(p->program->getId(),replaceSID("zenith_SID").c_str());
	glUniform1f(id,(90-altitude)/180*M_PI);
}
