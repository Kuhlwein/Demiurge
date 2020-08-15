//
// Created by kuhlwein on 7/17/20.
//

#include "SlopeMap.h"
#include "Project.h"

SlopeMap::SlopeMap() : Appearance("Slope map") {
	slope_texture = new Texture(100, 1, GL_RGBA, "gradient_slope_"+sid, GL_LINEAR);
	gradient = new GradientMenu("Slope gradient");
	gradient->setGradient(std::vector<int>{0, 0, 0, 255, 255, 255});
	gradient->toTexture(slope_texture);

	shader = Shader::builder()
			.include(fragmentColor)
			.include(def_pi)
			.include(get_slope)
			.create(replaceSID(R"(
uniform sampler2D gradient_slope_SID;
uniform float z_factor_SID;
)"),replaceSID(R"(

{
float slope =  get_slope(z_factor_SID)/M_PI*2;


vec4 kk =  texture( gradient_slope_SID ,vec2(slope,0));
fc = fc*(1-kk.a) + kk*(kk.a);
}

)"));
}

void SlopeMap::prepare(Project *p) {
	p->add_texture(slope_texture);
	int id = glGetUniformLocation(p->program->getId(),replaceSID("z_factor_SID").c_str());
	glUniform1f(id,zfactor);
}

void SlopeMap::unprepare(Project *p) {
	p->remove_texture(slope_texture);
}

Shader *SlopeMap::getShader() {
	return shader;
}

bool SlopeMap::update_self(Project *p) {
	if (first) first = false;

	ImGui::InputFloat("Z-factor",&zfactor);
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
	gradient->toTexture(slope_texture);

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
