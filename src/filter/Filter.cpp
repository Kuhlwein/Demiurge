//
// Created by kuhlwein on 7/18/20.
//

#include "Project.h"
#include <Shader.h>
#include <iostream>
#include "Filter.h"
#include "Texture.h"

BackupFilter::BackupFilter(Project *p, std::function<Texture *(Project *p)> target) : Filter(p) {
	this->target = target;
	std::cout << "creating backup\n";
	tmp = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"tmp");

	ShaderProgram *program_backup = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->apply(program_backup,tmp,{{target(p),"to_be_copied"}});
}

BackupFilter::~BackupFilter() {

}

void BackupFilter::add_history() {
	Shader *img_tmp_diff = Shader::builder()
			.include(fragmentBase)
			.create("uniform sampler2D  tmp; uniform sampler2D target;", R"(
fc = texture(tmp,st).r - texture(target, st).r;
)");
	// find difference between backup and target
	ShaderProgram *program2 = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(img_tmp_diff->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->add_texture(tmp);
	p->apply(program2, p->get_scratch1(),{{target(p),"target"}});
	p->remove_texture(tmp);
	delete (tmp);
	void *data = p->get_scratch1()->downloadData();

	auto h = new SnapshotHistory(data,target);
	p->add_history(h);
}



