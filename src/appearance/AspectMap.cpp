//
// Created by kuhlwein on 7/17/20.
//

#include "AspectMap.h"
#include "Project.h"

AspectMap::AspectMap() : Appearance("Aspect map") {
	aspect_texture = new Texture(100, 1, GL_RGBA, "gradient_aspect_"+sid, GL_LINEAR);
	gradient = new GradientMenu("Aspect gradient");
	gradient->setGradient(std::vector<int>{0, 0, 0, 255, 255, 255});
	gradient->toTexture(aspect_texture);

	shader = Shader::builder()
			.include(fragmentColor)
			.include(def_pi)
			.include(get_aspect)
			.create(replaceSID(R"(
uniform sampler2D gradient_aspect_SID;
)"),replaceSID(R"(

float aspect = get_aspect(projection(st))/2/M_PI;


vec4 kk =  texture( gradient_aspect_SID ,vec2(aspect,0));
fc = fc*(1-kk.a) + kk*(kk.a);


)"));
}

void AspectMap::prepare(Project *p) {
	p->add_texture(aspect_texture);
}

void AspectMap::unprepare(Project *p) {
	p->remove_texture(aspect_texture);
}

Shader *AspectMap::getShader() {
	return shader;
}

bool AspectMap::update_self(Project *p) {
	if (first) first = false;

	gradient->update();


	int current = -1;
	const char* items[] = { "Grayscale","HSV"};
	if(ImGui::Combo("Preset",&current,items,IM_ARRAYSIZE(items))) {
		switch (current) {
			case 0:
				gradient->setGradient(std::vector<int>{0, 0, 0, 255, 255, 255});
				break;
			case 1:
				gradient->setGradient(
						std::vector<int>{255,0,0, 255,186,0, 132,255,0, 0,255,60, 0,255,255, 0,66,255, 126,0,255, 255,0,192, 255,0,0});
				break;
		}
	}
	gradient->toTexture(aspect_texture);

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
