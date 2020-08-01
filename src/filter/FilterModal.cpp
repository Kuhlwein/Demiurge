//
// Created by kuhlwein on 7/28/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include "FilterModal.h"
#include "BlurMenu.h"

FilterModal::FilterModal(std::string title) : Modal(title, [this](Project* p) {
	return this->update_FilterModal(p);
}) {
	filter = std::make_shared<NoneFilter>();
}

bool FilterModal::update_FilterModal(Project *p) {
	update_self(p);

	filter->run(p);

	if(ImGui::Button("Preview")) {
		if (previewing) {
			p->undo();
		}
		previewing = true;

		filter = makeFilter(p);
		p->dispatchFilter(filter);
		return false;
	}
	ImGui::SameLine();
	if(ImGui::Button("Apply")) {
		if (!previewing) {
			filter = makeFilter(p);
			p->dispatchFilter(filter);
		}
		previewing = false;
	}
	ImGui::SameLine();
	if(ImGui::Button("Close")) {
		if (previewing) p->undo();
		previewing=false;
		filter = std::make_shared<NoneFilter>();
		return true;
	}
	if (filter->isFinished() && !previewing) {
		filter = std::make_shared<NoneFilter>();
		return true;
	}

	return false;
}

FilterMenu::FilterMenu(std::string title) : Menu(title,[this](Project* p) {
	auto t = targetGetter();
	auto tmp = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"tmp");

	ShaderProgram *program_backup = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->apply(program_backup,tmp,{{t(p),"to_be_copied"}});

	this->filter(p);

	Shader *img_tmp_diff = Shader::builder()
			.include(fragmentBase)
			.create("uniform sampler2D tmp; uniform sampler2D target;", R"(
fc = texture(tmp,st).r - texture(target, st).r;
)");
	// find difference between backup and target
	ShaderProgram *program2 = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(img_tmp_diff->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->add_texture(tmp);
	p->apply(program2, p->get_scratch1(),{{t(p),"target"}});
	p->remove_texture(tmp);
	TextureData* data = p->get_scratch1()->downloadData();

	auto h = new SnapshotHistory(data,targetGetter());
	p->add_history(h);
	return true;
}) {

}

InstantFilterModal::InstantFilterModal(std::string title) : Modal(title, [this](Project* p) {
	return this->update_InstantFilterModal(p);
}) {

}

bool InstantFilterModal::update_InstantFilterModal(Project *p) {
	if (first) {
		filter = makeFilter(p);
		filter->run(p);
		first = false;
	}

	update_self(p);

	if(ImGui::Button("Apply")) {
		first = true;
		filter->add_history();
		filter.release();
		return true;
	}
	ImGui::SameLine();
	if(ImGui::Button("Cancel")) {
		first = true;
		filter->restoreBackup();
		filter.release();
		return true;
	}
	return false;
}
