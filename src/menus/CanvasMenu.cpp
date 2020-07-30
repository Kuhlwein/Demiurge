//
// Created by kuhlwein on 7/10/20.
//


#include <glm/glm/glm.hpp>
#include "Project.h"
#include "CanvasMenu.h"

void handle_data(std::vector<float> &data,std::vector<float> &central_data, bool central, std::string s) {
	if (ImGui::Button(("Add##"+s).c_str())) {
		data.push_back(180);
		central_data.push_back(180);
	}
	ImGui::SameLine();
	if (ImGui::Button(("Remove##"+s).c_str()) && data.size()>3) {
		data.pop_back();
		central_data.pop_back();
	}
	for (int i=0; i<data.size()-1; i++) {
		ImGui::DragFloatRange2(("Interval "+std::to_string(i)+"##"+s).c_str(),data.data()+i,data.data()+i+1,
							   1.0f,(i==0) ? -180 : data[i-1],(i+1==data.size()-1) ? 180 : data[i+2]);
		if(!central) ImGui::DragFloat(("Central meridian "+std::to_string(i)+"##"+s).c_str(),central_data.data()+i,1.0,data[i],data[i+1]);
	}
	data.front()=-180;
	data.back()=180;
	for (int i=data.size()-2; i>=0; i--) data[i] = (data[i]>data[i+1]) ? data[i+1] : data[i];
	for (int i=0; i<data.size()-1; i++) {
		if(!central) {
			central_data[i] = central_data[i]<data[i] ? data[i] : central_data[i];
			central_data[i] = central_data[i]>data[i+1] ? data[i+1] : central_data[i];
		} else {
			central_data[i] = (data[i]+data[i+1])/2;
		}
	}

}

CanvasMenu::CanvasMenu(std::string title, AbstractCanvas *canvas) : Modal(title, [canvas](Project* p){
	static bool first = true;
	static Canvas* canvas_old;
	if (first) canvas_old = p->canvas;
	first = false;
	p->canvas = canvas;
	p->update_terrain_shader();

	static bool interruptions, oblique, separate, central=true;
	bool isInterruptible = canvas->isInterruptible();

	static glm::vec3 angles;
	ImGui::Checkbox("Rotate (Oblique projection)",&oblique);
	if (oblique) {
		ImGui::DragFloat("Longitudinal", &angles.x, 1.0f, 0, 0, "%.1f");
		ImGui::DragFloat("Latitudal", &angles.y, 1.0f, 0,0, "%.1f");
		ImGui::DragFloat("Transverse", &angles.z, 1.0f, 0,0, "%.1f");
		canvas->set_rotation(angles.x / 180 * M_PI, angles.y / 180 * M_PI, angles.z / 180 * M_PI);
		angles += 180;
		angles = glm::mod(angles,glm::vec3(360,360,360));
		angles -= 180;
	} else {
		canvas->set_rotation(0,0,0);
	}

	if (interruptions && isInterruptible || oblique) ImGui::Separator();

	static std::vector<float> data = {-180,0,180}, data_s = {-180,0,180};
	static std::vector<float> central_data = {-90,90}, central_data_s = {-90,90};

	if (isInterruptible) ImGui::Checkbox("Interruptions",&interruptions);

	if (interruptions && isInterruptible) {
		ImGui::Checkbox("Separate hemispheres",&separate);
		ImGui::Checkbox("Use central meridians",&central);

		if (separate) ImGui::Text("Northern hemisphere");
		handle_data(data,central_data,central,"N");

		if (separate) {
			ImGui::Text("Southern hemisphere");
			handle_data(data_s,central_data_s,central,"S");
			canvas->set_interruptions({data,central_data,data_s,central_data_s},true);
		} else {
			canvas->set_interruptions({data,central_data,data,central_data},true);
		}
	} else {
		canvas->set_interruptions({},false);
	}
	if (interruptions && isInterruptible) ImGui::Separator();



	if (ImGui::Button("Apply")) {
		first = true;
		return true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		p->canvas = canvas_old;
		p->update_terrain_shader();
		first = true;
		return true;
	}
	return false;
}) {

}

bool CanvasMenu::test(Project *p) {
	return false;
}
