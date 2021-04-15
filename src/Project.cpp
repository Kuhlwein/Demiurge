//
// Created by kuhlwein on 4/9/19.
//

#include <iostream>
#include <vector>
#include "Project.h"
#include "Vbo.h"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Texture.h"
#include "Shader.h"

//todo remove

#include <glm/glm/glm.hpp>
#include <imgui/imgui.h>
#include <filter/BlurMenu.h>
#include <thread>
#include <filter/OffsetMenu.h>
#include <filter/ScaleMenu.h>
#include <filter/Morphological.h>
#include <select/selection.h>
#include <filter/GradientNoise.h>
#include <projections/Equiretangular.h>
#include <filter/cpufilter.h>
#include <filter/FlowFilter.h>
#include <menus/LayerWindow.h>
#include <filter/ThermalErosion.h>
#include <filter/DeTerrace.h>
#include <filter/OceanCurrents.h>


#include "projections/Projections.h"
#include "menus/BrushWindow.h"
#include "menus/AppearanceWindow.h"

void Project::file_load(const std::string& filename) {
	int w, h, comp;
	stbi_info(filename.c_str(),&w,&h,&comp);
	unsigned char* image = stbi_load(filename.c_str(), &w, &h, &comp, STBI_rgb);

	file_new(w,h);

	get_terrain()->uploadData(GL_RGB,GL_UNSIGNED_BYTE,image);
	delete image;
}

void Project::file_write() {
	auto image = get_terrain()->downloadDataRAW();
	char data[getWidth()*getHeight()];
	for (int i=0; i<getWidth()*getHeight(); i++) {
		int a = 255.0f*image[i];
		data[i] = a;
	}
	std::cout << image[0] << " <-- data[0]\n";

	stbi_write_png("output.png",getWidth(),getHeight(),1,data,getWidth()*1);

}

void Project::file_new(int w, int h) {
	for (auto l : layers) {
		remove_texture(l.second->getTexture());
		delete l.second;
	}

	width=w;
	height = h;
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	remove_texture(scratchPad2);
	delete scratchPad2;
	scratchPad2 = new Texture(w,h,GL_R32F,"scratch2");
	remove_texture(scratchPad);
	delete scratchPad;
	scratchPad = new Texture(w,h,GL_R32F,"scratch1");
	remove_texture(selection);
	delete selection;
	selection = new Texture(w,h,GL_R32F,"sel");

	for (auto t : {scratchPad, scratchPad2, selection}) add_texture(t);
	auto l = new Layer(w,h);
	add_texture(l->getTexture());
	layers = {{l->id,l}};
	current_layer = l->id;

	ShaderProgram *program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();
	std::cout << "before apply\n";
	apply(program,get_terrain());
	std::cout << "after apply\n";
	int id = glGetUniformLocation(program->getId(),"value");
	glUniform1f(id,1.0f);
	apply(program,get_selection());

	while(!undo_list.empty()) undo_list.pop();
	while(!redo_list.empty()) redo_list.pop();

	canvas = new Equiretangular(this);
	update_terrain_shader();

	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	glBufferData(GL_PIXEL_PACK_BUFFER, getWidth() * getHeight() * 4, NULL, GL_STREAM_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
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
	file_menu.push_back(new Menu("Write...",testnamespace::file_write));

	std::vector<Menu*> edit_menu = {};
	edit_menu.push_back(new Menu("Undo",edit::undo));
	edit_menu.push_back(new Menu("Redo",edit::redo));
	edit_menu.push_back(new SeparatorMenu());
	edit_menu.push_back(new Modal("Preferences",edit::preferences));

	std::vector<Menu*> windows_menu = {};
	windows_menu.push_back(new BrushWindow("Brush",this));
	appearanceWindow = new AppearanceWindow();
	windows_menu.push_back(appearanceWindow);
	windows_menu.push_back(new LayerWindow());

	std::vector<Menu*> filter_menu = {};
	filter_menu.push_back(new BlurMenu());
	filter_menu.push_back(new cpufilterMenu());
	filter_menu.push_back(new FlowfilterMenu());
	filter_menu.push_back(new ThermalErosionMenu());
	auto math = new SubMenu("Mathematical");
	math->addMenu(new OffsetMenu());
	math->addMenu(new ScaleMenu());
	math->addMenu(new DeTerraceMenu());
	math->addMenu(new OceanCurrentsMenu());
	filter_menu.push_back(math);
	filter_menu.push_back(new MorphologicalMenu());
	filter_menu.push_back(new GradientNoiseMenu());

	windows.emplace_back("File",file_menu);
	windows.emplace_back("Edit",edit_menu);
	windows.emplace_back("Select",selection::get_selection_menu());
	windows.emplace_back("Projections",projections::get_projection_menu(this));
	windows.emplace_back("Filter",filter_menu);
	windows.emplace_back("Windows",windows_menu);

	filter = std::make_unique<NoneFilter>();

	canvas = new Equiretangular(this);
	appearanceWindow->setShader(this);

	glGenBuffers(1, &pbo);

	file_new(1000,500);
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

	//TODO something
	//filter->run();

    canvas->update();

	ImGuiIO io = ImGui::GetIO();

	int z_key = 47;
	if (io.KeysDown[47] && io.KeyCtrl && io.KeysDownDuration[z_key] == 0.0f && io.KeyShift) {
		redo();
	} else if (io.KeysDown[47] && io.KeyCtrl && io.KeysDownDuration[z_key] == 0.0f) {
		undo();
	}

	if (downloadingTex) {
		GLint result;
		glGetSynciv(sync, GL_SYNC_STATUS, sizeof(result), NULL, &result);

		if(result == GL_SIGNALED){
			glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);

			void* mappedBuffer = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

			std::cout << ((float*)mappedBuffer)[0] << " <- mapped buffer\n";

			auto data = std::make_unique<float[]>(width*height);
			memcpy(data.get(),mappedBuffer,width*height*sizeof(float));
			auto tdata = new TextureData(std::move(data),width,height);

			add_history(new SnapshotHistory(tdata,[](Project* p){return p->get_terrain();}));
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
			downloadingTex = false;
			glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
			delete asyncTex;
		} else {
			std::cout << "waiting for sync\n";
		}
	}

}

void Project::render() {
    program->bind();
    bind_textures(program);
    appearanceWindow->prepare(this); //TODO could be more efficient, only update when changes! Full update should be reserved for setting new shaderprogram

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

void Project::apply(ShaderProgram *program, Texture *texture, std::vector<std::pair<Texture*,std::string>> l) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, texture->getWidth(), texture->getHeight());
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture->getId(),0);
    program->bind();
    bind_textures(program, l);
    vbo->render();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Project::bind_textures(ShaderProgram *program,std::vector<std::pair<Texture*,std::string>> l) {
	int i=0;
	for (auto t : textures) {
		t->bind(program,i++);
//		if (aliasmap.count(t)>0) {
//			//Texture has alias!
//			t->bind(program,i-1,aliasmap[t]);
//			aliasmap.erase(t);
//		}
	}
//	for(auto t : aliasmap) {
//		t.first->bind(program,i++,t.second);
//	}
//	aliasmap.clear();
	for (auto p : l) p.first->bind(program,i++,p.second);
}

void Project::add_texture(Texture *texture) {
	textures.insert(texture);
}

void Project::remove_texture(Texture *texture) {
	textures.erase(texture);
}

Texture *Project::get_terrain() {
	return get_layer(get_current_layer())->getTexture();
}

int Project::getWidth() {
	return width;
}

int Project::getHeight() {
	return height;
}

void Project::add_layer(Layer* l) {
	layers.insert({l->id,l});
}

Layer* Project::get_layer(int i) {
	return layers[i];
}

int Project::get_current_layer() {
	return current_layer;
}

void Project::set_layer(int i) {
	remove_texture(layers[current_layer]->getTexture());
	current_layer = i;
	add_texture(layers[current_layer]->getTexture());
}

void Project::update_terrain_shader() {
	Shader::builder builder = Shader::builder()
			.include(fragmentColor)
			.include(distance)
			.include(canvas->projection_shader())
			.include(terrain_shader);

			builder.include(filter->getShader());

			Shader* shader = builder
			.include(brush_outline)
			.include(selection_outline)
			.create();

	program = ShaderProgram::builder()
			.addShader(vertex3D->getCode(),GL_VERTEX_SHADER)
			.addShader(shader->getCode(),GL_FRAGMENT_SHADER)
			.link();

	appearanceWindow->prepare(this);
}

Texture *Project::get_selection() {
	return selection;
}

void Project::add_history(UndoHistory* h) {
	undo_list.push(h);
	while(!redo_list.empty()) {
		auto r = redo_list.top();
		redo_list.pop();
		delete r;
	}
}

void Project::undo() {
	if (undo_list.empty()) return;
	UndoHistory* h = undo_list.top();
	undo_list.pop();
	h->undo(this);
	redo_list.push(h);

}

void Project::redo() {
	if (redo_list.empty()) return;
	UndoHistory* h = redo_list.top();
	redo_list.pop();
	h->redo(this);
	undo_list.push(h);
}

Texture *Project::get_scratch2() {
	return scratchPad2;
}

Texture *Project::get_scratch1() {
	return scratchPad;
}

void Project::set_terrain_shader(Shader *s) {
	terrain_shader = s;
	update_terrain_shader();
}

void Project::setCoords(std::vector<float> v) {
	for (auto &e : v) e=e/180.0f*M_PI;
	coords = v;
}

std::vector<float> Project::getCoords() {
	return coords;
}

glm::vec2 Project::getMouse() {
	ImGuiIO io = ImGui::GetIO();
	return canvas->mousePos(io.MousePos);
}

glm::vec2 Project::getMousePrev() {
	ImGuiIO io = ImGui::GetIO();
	return canvas->mousePos(ImVec2(io.MousePos.x - io.MouseDelta.x,io.MousePos.y - io.MouseDelta.y));
}

void Project::dispatchFilter(std::shared_ptr<Filter> filter) {
	this->filter = std::move(filter);
	update_terrain_shader();
}

void Project::finalizeFilter() {
	filter = std::make_unique<NoneFilter>();
	update_terrain_shader();
}

void Project::addAsyncTex(Texture *tex) {
	downloadingTex = true;
	asyncTex = tex;

	asyncTex->bind(0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	//TODO calculate brush position boundaries, then use SubImage
	glGetTexImage(GL_TEXTURE_2D,0,GL_RED,GL_FLOAT,nullptr);

	sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
}

void Project::setCanvasUniforms(ShaderProgram *p) {
	int id = glGetUniformLocation(p->getId(),"cornerCoords");
	glUniform1fv(id, 4, coords.data());
	id = glGetUniformLocation(p->getId(),"circumference");
	glUniform1f(id, circumference);
}

std::map<int, Layer *> Project::get_layers() {
	return layers;
}

void Project::remove_layer(int i) {
	for (auto l : layers) {
		if (l.first==i) continue;
		set_layer(l.first);
		break;
	}

	layers.erase(i);
}

void Project::add_alias(Texture *t, std::string alias) {
	aliasmap[t] = alias;
}






