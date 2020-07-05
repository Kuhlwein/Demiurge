//
// Created by kuhlwein on 4/9/19.
//

#include <iostream>
#include <GL/gl3w.h>
#include <vector>
#include "Project.h"
#include "Vbo.h"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Texture.h"
#include "Shader.h"
#include "selection.h"
#include "view.h"
#include "edit.h"

//todo remove

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <imgui/imgui.h>
#include <projections/Orthographic.h>
#include <projections/Mollweide.h>
#include <projections/Mercator.h>
#include <projections/Equiretangular.h>


void Project::file_load(const std::string& filename) {
	int w, h, comp;
	stbi_info(filename.c_str(),&w,&h,&comp);
	unsigned char* image = stbi_load(filename.c_str(), &w, &h, &comp, STBI_rgb);

	file_new(w,h);

	get_terrain()->uploadData(GL_RGB,GL_UNSIGNED_BYTE,image);
}

void Project::file_new(int w, int h) {
	width=w;
	height = h;
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	remove_texture(terrain);
	delete terrain;
	terrain = new Texture(w,h,GL_R32F,"img");
	remove_texture(scratchPad2);
	delete scratchPad2;
	scratchPad2 = new Texture(w,h,GL_R32F,"scratch2");
	remove_texture(scratchPad);
	delete scratchPad;
	scratchPad = new Texture(w,h,GL_R32F,"scratch1");
	remove_texture(selection);
	delete selection;
	selection = new Texture(w,h,GL_R32F,"sel");

	for (auto t : {terrain,scratchPad, scratchPad2, selection}) add_texture(t);
	layers = {{"Default",terrain}};
	current_layer = 0;

	ShaderProgram *program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();
	apply(program,get_terrain());
	int id = glGetUniformLocation(program->getId(),"value");
	glUniform1f(id,1.0f);
	apply(program,get_selection());

	while(!undo_list.empty()) undo_list.pop();
	while(!redo_list.empty()) redo_list.pop();

	canvas = new img(this);
	update_terrain_shader();

}

Project::Project(GLFWwindow* window) {
    ImGui::GetIO().IniFilename = NULL;

    this->window = window;

	//Used for rendering to external framebuffer
    glGenFramebuffers(1,&fbo);

    std::vector<float> positions = {
            -1, -1, 0,
            -1, 1, 0,
            1, 1, 0,
            1, -1, 0
    };
    std::vector<float> textures = {
            0.0, 0.0,
            0.0, 1,
            1, 1,
            1, 0
    };
    std::vector<int> indices = {
            0, 1, 3,
            2, 3, 1
    };
    vbo = new Vbo(positions, textures, indices);

	std::vector<Menu*> file_menu = {};
	file_menu.push_back(new Modal("New...", testnamespace::file_new));
	file_menu.push_back(new Modal("Load...", testnamespace::file_load));

	std::vector<Menu*> edit_menu = {};
	edit_menu.push_back(new Menu("Undo",edit::undo));
	edit_menu.push_back(new Menu("Redo",edit::redo));
	edit_menu.push_back(new SeparatorMenu());
	edit_menu.push_back(new Modal("Preferences",edit::preferences));



	std::vector<Menu*> view_menu = {};

	auto projection = new SubMenu("Projection");
	projection->addMenu(new Menu("None", [](Project* p) {
		p->canvas = new img(p);
		p->update_terrain_shader();
		return true;
	}));
	projection->addMenu(new Menu("Equiretangular", [](Project* p) {
		p->canvas = new Equiretangular(p);
		p->update_terrain_shader();
		return true;
	}));
	projection->addMenu(new Menu("Orthographic", [](Project* p){
		p->canvas = new Orthographic(p);
		p->update_terrain_shader();
		return true;
	}));
	projection->addMenu(new Menu("Mollweide", [](Project* p){
		p->canvas = new Mollweide(p);
		p->update_terrain_shader();
		return true;
	}));
	projection->addMenu(new Menu("Mercator", [](Project* p){
		p->canvas = new Mercator(p);
		p->update_terrain_shader();
		return true;
	}));
	view_menu.push_back(projection);
	view_menu.push_back(new Modal("Gradient...", view::gradient));
	view_menu.push_back(new Menu("Normal map...", view::normal_map));



	std::vector<Menu*> windows_menu = {};
	windows_menu.push_back(new Window("Brush", testnamespace::brush));
	windows_menu.push_back(new Window("Layers", testnamespace::layers));


	std::vector<Menu*> selection_menu = {};

	//SubMenu* sub = new SubMenu("test submenu");

	selection_menu.push_back(new Menu("All", [](Project* p){return selection::set(p,1.0f);}));
	selection_menu.push_back(new Menu("None", [](Project* p){return selection::set(p,0.0f);}));
	selection_menu.push_back(new Menu("Invert",selection::invert));
	selection_menu.push_back(new Modal("By height", selection::by_height));
	selection_menu.push_back(new SeparatorMenu());
	selection_menu.push_back(new Modal("Blur",selection::blur));

	//selection_menu.push_back(sub);

	windows.emplace_back("File",file_menu);
	windows.emplace_back("Edit",edit_menu);
	windows.emplace_back("Select",selection_menu);
	windows.emplace_back("View",view_menu);
	windows.emplace_back("Windows",windows_menu);





	geometryShader = new NoneShader(this);

	canvas = new img(this);
	set_terrain_shader(draw_grayscale);

	brush_tex = new Texture(brush_tex_size,brush_tex_size,GL_R32F,"brush_tex",GL_LINEAR);
	add_texture(brush_tex);
	set_brush(brush_hardness);


	file_new(500,500);
}

void Project::update() {
    /*
     File
     Edit
     Select
     View
     Render
     filters
    */

    if(ImGui::BeginMainMenuBar()) {
		for(auto p : windows) {
			if (ImGui::BeginMenu(p.first.c_str(),true)) {
				for (Menu* w : p.second) w->menu();
				ImGui::EndMenu();
			}
		}
        ImGui::EndMainMenuBar();
    }
	for(auto p : windows) {
		for (Menu* w : p.second) w->update(this);
	}

	if (is_filtering) run_filter();

    canvas->update();

	update_self();
}

void Project::update_self() {
	ImGuiIO io = ImGui::GetIO();

	int z_key = 47;
	if (io.KeysDown[47] && io.KeyCtrl && io.KeysDownDuration[z_key] == 0.0f && io.KeyShift) {
		redo();
	} else if (io.KeysDown[47] && io.KeyCtrl && io.KeysDownDuration[z_key] == 0.0f) {
		undo();
	}


	//Brushing
	static bool first = true;
	static std::vector<std::pair<glm::vec2,glm::vec2>> brush_strokes;

	glm::vec2 texcoord = getMouse();
	glm::vec2 texcoordPrev = getMousePrev();

	int id = glGetUniformLocation(program->getId(),"mouse");
	glUniform2f(id,texcoord.x,texcoord.y);
	id = glGetUniformLocation(program->getId(),"mousePrev");
	glUniform2f(id,texcoordPrev.x,texcoordPrev.y);

	if(io.WantCaptureMouse) return;

	if(io.MouseDown[0] & first) {
		brush_strokes.erase(brush_strokes.begin(),brush_strokes.end());
		initbrush();
		first = false;
	} else if (io.MouseDown[0]) {
		brush(texcoord,texcoordPrev);
		brush_strokes.push_back({texcoord,texcoordPrev});
	} else if (!first) {
		std::cout << "last\n";
		//project->brush(texcoord,texcoordPrev);
		brush_strokes.push_back({texcoord,texcoordPrev});
		auto test = brush_strokes;

		float hardness = brush_hardness;
		float brush_size2 = brush_size;
		auto r = [test,hardness,brush_size2](Project* p) {
			p->initbrush();
			auto data = p->get_brush_tex()->downloadData();
			p->set_brush(hardness);
			float oldsize = p->brush_size;
			p->brush_size = brush_size2;
			for (int i=0; i<test.size()-1; i++) p->brush(test[i].first,test[i].second);
			p->brush_size = oldsize;
			p->get_brush_tex()->uploadData(GL_RED,GL_FLOAT,data);
		};
		auto u = [test,hardness,brush_size2](Project* p) {
			p->initbrush();

			auto data = p->get_brush_tex()->downloadData();
			p->set_brush(hardness);
			float oldsize = p->brush_size;
			p->brush_size = brush_size2;

			for (int i=0; i<test.size()-1; i++) p->brush(test[i].first,test[i].second,true);
			ShaderProgram *program = ShaderProgram::builder()
					.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
					.addShader(fix_0->getCode(), GL_FRAGMENT_SHADER)
					.link();
			p->apply(program,p->get_scratch1());
			p->get_terrain()->swap(p->get_scratch1());

			p->brush_size = oldsize;
			p->get_brush_tex()->uploadData(GL_RED,GL_FLOAT,data);
		};

		auto hist = new ReversibleHistory(r,u);
		add_history(hist);
		first = true;
	}
}

void Project::render() {
    program->bind();
    bind_textures(program);
	int id = glGetUniformLocation(program->getId(),"brush_size");
	glUniform1f(id,brush_size);

//	id = glGetUniformLocation(program->getId(),"cornerCoords");
//	auto v = getCoords();
//	for (auto &e : v) e=e/180.0f*M_PI;
//	glUniform1fv(id, 4, v.data());

	(geometryShader->get_setup())(glm::vec2(0,0), glm::vec2(0,0),program);

    canvas->render();
}

Project::~Project() {
    delete(program);
    delete(canvas);
}

int Project::getWindowWidth() {
    int width, height;
    glfwGetWindowSize(window,&width,&height);
    return width;
}

int Project::getWindowHeight() {
    int width, height;
    glfwGetWindowSize(window,&width,&height);
    return height;
}

void Project::brush(glm::vec2 pos, glm::vec2 prev, bool flag) {
    //Paint to scratchpad2

    Shader* brush_shader = Shader::builder()
    		.include(fragmentBase)
    		.include(geometryShader->get_shader())
    		.create("",R"(
vec2 vstop;
vec2 vstart;
brush_calc(vstart,vstop);
fc = texture(scratch2,st).r + texture(sel,st).r*(texture(brush_tex,vstop).r - texture(brush_tex,vstart).r);
)");

    ShaderProgram *program2 = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(brush_shader->getCode(), GL_FRAGMENT_SHADER)
            .link();

    program2->bind();
    int id = glGetUniformLocation(program2->getId(),"mouse");
    glUniform2f(id,pos.x,pos.y);
    id = glGetUniformLocation(program2->getId(),"mousePrev");
    glUniform2f(id,prev.x,prev.y);
	id = glGetUniformLocation(program2->getId(),"brush_size");
	glUniform1f(id,brush_size);

	(geometryShader->get_setup())(pos,prev,program2);

    apply(program2,terrain);
    terrain->swap(scratchPad2);

    //Transfer to terrain
	ShaderProgram *program3;
    if (!flag) {
		program3 = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(R"(
#version 430
in vec2 st;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out float fc;

void main () {
    fc = texture(scratch1, st).r + min(texture(scratch2,st).r,0.3);
}
    )", GL_FRAGMENT_SHADER)
				.link();
    } else {
		program3 = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(R"(
#version 430
in vec2 st;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out float fc;

void main () {
    fc = texture(scratch1, st).r - min(texture(scratch2,st).r,0.3);
}
    )", GL_FRAGMENT_SHADER)
				.link();
    }


    apply(program3,terrain);
}

void Project::apply(ShaderProgram *program, Texture *texture, std::vector<std::pair<Texture*,std::string>> l) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, texture->getWidth(), texture->getHeight());
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture->getId(),0);
    program->bind();
    bind_textures(program, l);

    vbo->render();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Project::initbrush() {
    ShaderProgram *program = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
            .link();
    apply(program,scratchPad,{{terrain,"to_be_copied"}});

    ShaderProgram *program2 = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
            .link();
    apply(program2,scratchPad2);
}

void Project::bind_textures(ShaderProgram *program,std::vector<std::pair<Texture*,std::string>> l) {
	int i=0;
	for (auto t : textures) t->bind(program,i++);
	for (auto p : l) p.first->bind(program,i++,p.second);
}

void Project::add_texture(Texture *texture) {
	textures.insert(texture);
}

void Project::remove_texture(Texture *texture) {
	textures.erase(texture);
}

Texture *Project::get_terrain() {
	return terrain;
}

void Project::set_terrain(Texture *texture) {
	remove_texture(terrain);
	terrain = texture;
	add_texture(texture);
}

int Project::getWidth() {
	return width;
}

int Project::getHeight() {
	return height;
}

int Project::get_n_layers() {
	return layers.size();
}

void Project::add_layer(std::pair<std::string,Texture*> l,int index) {
	if (index>0) layers.insert(layers.begin()+index,l);
	else layers.push_back(l);
}

std::pair<std::string,Texture*> Project::get_layer(int i) {
	return layers[i];
}

int Project::get_current_layer() {
	return current_layer;
}

void Project::set_layer(int i) {
	current_layer = i;
	set_terrain(layers[i].second);
}

void Project::remove_layer(int i) {
	if (i>0) {
		layers.erase(layers.begin()+i);
		if (get_current_layer()>get_n_layers()-1) set_layer(get_n_layers()-1);
	}
}

void Project::update_terrain_shader() {
	delete program;

	Shader* shader = Shader::builder()
			.include(fragmentColor)
			.include(geometryShader->get_shader())
			.include(canvas->projection_shader())
			.include(terrain_shader)
			.include(brush_outline)
			.include(selection_outline)
			.create();
	program = ShaderProgram::builder()
			.addShader(vertex3D->getCode(),GL_VERTEX_SHADER)
			.addShader(shader->getCode(),GL_FRAGMENT_SHADER)
			.link();
}

Texture *Project::get_selection() {
	return selection;
}

void Project::add_history(UndoHistory* h) {
	undo_list.push(h);
	while(!redo_list.empty()) {
		redo_list.pop();
	}
}

void Project::undo() {
	if (undo_list.empty()) return;
	std::cout << "undoing in project\n";
	UndoHistory* h = undo_list.top();
	undo_list.pop();
	h->undo(this);
	redo_list.push(h);

}

void Project::redo() {
	if (redo_list.empty()) return;
	std::cout << "redoing in project\n";
	UndoHistory* h = redo_list.top();
	redo_list.pop();
	h->redo(this);
	undo_list.push(h);
}

int Project::get_n_textures() {
	return textures.size();
}

Texture *Project::get_scratch2() {
	return scratchPad2;
}

Texture *Project::get_scratch1() {
	return scratchPad;
}

void Project::add_filter(std::function<float(Project *p)> s, std::function<Texture *(Project *p)> t) {
	filter = s;
	filter_target = t;
	is_filtering = true;

	take_backup(filter_target);

	//apply filter
}

void Project::run_filter() {
	float o = filter(this);
	if (o<1.0f) {
		auto load = [o](Project* p) {
			ImGui::ProgressBar(o,ImVec2(360,0));
			return ImGui::Button("Cancel");
		};
		Menu* w = new Modal("Applying filter", load); //MEMORY LEAK HERE PROBABLY
		w->open();
		bool open = w->update(this);
		delete w;

		if (!open) {
			is_filtering = false;
			filter_target(this)->swap(tmp);
			delete tmp;
		}

	} else {
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
		add_texture(tmp);
		apply(program2, get_scratch1(),{{filter_target(this),"target"}});
		remove_texture(tmp);
		delete (tmp);
		void *data = get_scratch1()->downloadData();
		for (int i=0; i<width*height; i++) std::cout << ((float*)data)[i] << " ";

		auto h = new SnapshotHistory(data,filter_target);
		add_history(h);
		is_filtering = false;
	}
}

void Project::preview(std::function<float(Project* p)> s, std::function<Texture*(Project* p)> t) {
	filter_target = t;
	if (!is_previewing) {
		is_previewing = true;
		take_backup(filter_target);
	}
	ShaderProgram *program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
	std::cout << "Apply original\n";
	apply(program,t(this),{{tmp,"to_be_copied"}});
	s(this);
}

void Project::stop_preview() {
	if (is_previewing) {
		filter_target(this)->swap(tmp);
		delete tmp;
	}
	is_previewing = false;
}

void Project::take_backup(std::function<Texture*(Project* p)> t) {
	std::cout << "creating backup\n";
	tmp = new Texture(getWidth(),getHeight(),GL_R32F,"tmp");

	ShaderProgram *program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
	std::cout << "Apply original\n";
	apply(program,tmp,{{t(this),"to_be_copied"}});
}

void Project::add_reversible_filter(std::function<float(Project *p)> r, std::function<float(Project *p)> u) {
	auto hist = new ReversibleHistory(r,u);
	hist->redo(this);
	add_history(hist);
}

void Project::set_brush(float hardness) {
	float* data = new float[brush_tex_size*brush_tex_size];

	auto brush_val = [hardness](float x) {
		double R = 1.0f;
		double phi = x/R;
		double result;
		if (phi<=hardness) result=1; else result=0;
		double c = M_PI*phi/(2*(1-hardness))+M_PI/2*(1-1/(1-hardness));
		if (phi>hardness) result = pow(cos(c),2);
		return (float) result;
	};

	for (int i=0; i<brush_tex_size; i++) {
		float d = i/((float)brush_tex_size-1);
		float width = sqrt(1-pow(d,2));
		float current = -width;
		float step = (2*width)/(brush_tex_size-1);

		float r = sqrt(pow(d,2)+pow(current+i*step,2));
		float current_val = brush_val(r);
		float sum = 0;

		for (int j=0; j<brush_tex_size; j++) {
			current+=step;
			float r = sqrt(pow(d,2)+pow(current,2));
			float new_val = brush_val(r);
			sum+= (current_val+new_val)/2*step;
			data[i*brush_tex_size+j] = sum;
			current_val = new_val;
		}
	}
	brush_tex->uploadData(GL_RED,GL_FLOAT,data);
}

Texture* Project::get_brush_tex() {
	return brush_tex;
}

void Project::set_terrain_shader(Shader *s) {
	terrain_shader = s;
	update_terrain_shader();
}

void Project::setGeometryShader(GeometryShader *g) {
	delete geometryShader;
	geometryShader = g;
	update_terrain_shader();
}

void Project::setCoords(std::vector<float> v) {
	coords = v;
}

std::vector<float> Project::getCoords() {
	return coords;
}

Shader *Project::getGeometryShader() {
	return geometryShader->get_shader();
}

glm::vec2 Project::getMouse() {
	ImGuiIO io = ImGui::GetIO();
	return canvas->mousePos(io.MousePos);
}

glm::vec2 Project::getMousePrev() {
	ImGuiIO io = ImGui::GetIO();
	return canvas->mousePos(ImVec2(io.MousePos.x - io.MouseDelta.x,io.MousePos.y - io.MouseDelta.y));
}












