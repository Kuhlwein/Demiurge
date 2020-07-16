//
// Created by kuhlwein on 7/14/20.
//

#include "Project.h"
#include <Texture.h>
#include <algorithm>
#include "ElevationMap.h"


//template<typename Type>
void set_gradient(ImGradient *gradient, std::vector<int> colors) {
	gradient->getMarks().clear();
	for (long unsigned int i=0; i<colors.size()/3; i++) {
		gradient->addMark(float(i)/(colors.size()/3-1),ImColor(colors[i*3],colors[i*3+1],colors[i*3+2],255));
	}
}

void gradient_to_texture(ImGradient *gradient, Texture* texture) {
	auto data = new unsigned char[100 * 4];
	for (int i=0; i<100; i++) {
		float c[4];
		gradient->computeColorAt(float(i)/100.0f,c);
		data[4*i]=int(c[0]*255);
		data[4*i+1]=char(c[1]*255);
		data[4*i+2]=char(c[2]*255);
		data[4*i+3]=char(c[3]*255);
	}
	texture->uploadData(GL_RGBA, GL_UNSIGNED_BYTE, data);
}

ElevationMap::ElevationMap() : Appearance("Elevation map") {
	texture_land = new Texture(100, 1, GL_RGBA, "gradient_land_"+sid, GL_LINEAR);
	texture_ocean = new Texture(100,1,GL_RGBA,"gradient_ocean_"+sid,GL_LINEAR);
	set_gradient(&gradient,std::vector<int>{127,127,127, 255,255,255});
	set_gradient(&gradient_ocean,std::vector<int>{0,0,0, 127,127,127});
	gradient_to_texture(&gradient,texture_land);
	gradient_to_texture(&gradient_ocean,texture_ocean);

	shader = Shader::builder()
			.include(fragmentColor)
			.create(replaceSID(R"(
uniform sampler2D gradient_land_SID;
uniform sampler2D gradient_ocean_SID;
)"),replaceSID(R"(
float h_SID = texture(img, st_p).r;
if (h_SID>0) {
    vec4 k = texture(gradient_land_SID,vec2(h_SID,0));
    fc = fc*(1-k.a) + k*(k.a);
} else {
    fc = texture(gradient_ocean_SID,vec2(1+h_SID,0));
}
)"));
}

bool ElevationMap::update_self(Project *p) {
	if (first) first = false;

	if(ImGui::GradientButton(&gradient))
		ImGui::OpenPopup("Terrain editor");
	if (ImGui::BeginPopupModal("Terrain editor", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		static ImGradientMark* draggingMark = nullptr;
		static ImGradientMark* selectedMark = nullptr;
		ImGui::GradientEditor(&gradient, draggingMark, selectedMark);
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	int current = -1;
	const char* items[] = { "Grayscale","Atlas", "Green-yellow-red","Tropic","Contrast","Terrain"};
	if(ImGui::Combo("Preset",&current,items,IM_ARRAYSIZE(items))) {
		switch (current) {
			case 0:
				set_gradient(&gradient,std::vector<int>{127,127,127, 255,255,255});
				break;
			case 1:
				set_gradient(&gradient,std::vector<int>{172,208,165, 148,191,139, 168,198,143, 189,204,150, 209,215,171, 225,228,181, 239,235,192, 232,225,182, 222,214,163, 211,202,157, 202,185,130, 195,167,107, 185,152,90, 170,135,83, 172,154,124, 186,174,154, 202,195,184, 224,222,216, 245,244,242});
				break;
			case 2:
				set_gradient(&gradient,std::vector<int>{31,70,41, 111,165,67, 243,236,34, 246,145,29, 212,50,37});
				break;
			case 3:
				set_gradient(&gradient, std::vector<int>{1,64,76, 47,93,49, 95,124,21, 176,159,28, 254,229,151});
				break;
			case 4:
				set_gradient(&gradient, std::vector<int>{2,46,6, 0,154,0, 46,199,0, 162,227,39, 246,253,82, 215,180,46, 177,95,22, 121,5,0, 237,224,216});
				break;
			case 5:
				set_gradient(&gradient, std::vector<int>{8,9,5, 51,51,33, 32,60,40, 40,86,57, 55,116,76, 113,165,100, 160,184,110, 217,207,120, 211,185,104, 190,148,78, 186,122,59, 213,127,63});
				break;
		}
	}
	current = -1;

	ImGui::Spacing();

	if(ImGui::GradientButton(&gradient_ocean))
		ImGui::OpenPopup("Ocean editor");
	if (ImGui::BeginPopupModal("Ocean editor", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		static ImGradientMark* draggingMark2 = nullptr;
		static ImGradientMark* selectedMark2 = nullptr;
		ImGui::GradientEditor(&gradient_ocean, draggingMark2, selectedMark2);
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
	const char* items2[] = { "Grayscale","Atlas","Blue","Sand","Deep"};
	if(ImGui::Combo("Preset##2",&current,items2,IM_ARRAYSIZE(items2))) {
		switch (current) {
			case 0:
				set_gradient(&gradient_ocean,std::vector<int>{0,0,0, 127,127,127});
				break;
			case 1:
				set_gradient(&gradient_ocean,std::vector<int>{113,171,215, 121,178,222, 132,185,227, 141,193,234, 150,201,240, 161,210,247, 172,219,251, 185,227,255, 198,236,255, 216,242,254});
				break;
			case 2:
				set_gradient(&gradient_ocean,std::vector<int>{44,27,77, 40,85,139, 123,141,220, 198,192,243, 254,254,255});
				break;
			case 3:
				set_gradient(&gradient_ocean,std::vector<int>{0,7,76, 51,95,152, 108,142,147, 182,195,145, 254,254,253});
				break;
			case 4:
				set_gradient(&gradient_ocean,std::vector<int>{0,0,0, 22,59,94, 84,126,191, 138,161,202, 253,253,254});
				break;
		}
	}

	if (ImGui::Button("Apply")) {
		gradient_to_texture(&gradient,texture_land);
		gradient_to_texture(&gradient_ocean,texture_ocean);
		//p->set_terrain_shader(shader);
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

void ElevationMap::prepare(Project *p) {
	gradient_to_texture(&gradient,texture_land);
	gradient_to_texture(&gradient_ocean,texture_ocean);
	p->add_texture(texture_land);
	p->add_texture(texture_ocean);
}

void ElevationMap::unprepare(Project *p) {
	p->remove_texture(texture_land);
	p->remove_texture(texture_ocean);
}


Shader *ElevationMap::getShader() {
	return shader;
}
