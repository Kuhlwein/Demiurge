//
// Created by kuhlwein on 4/9/19.
//

#ifndef DEMIURGE_PROJECT_H
#define DEMIURGE_PROJECT_H
#include <imgui/examples/libs/gl3w/GL/gl3w.h>
#include <imgui/examples/libs/glfw/include/GLFW/glfw3.h>
#include <map>
#include <set>
#include <stack>
#include <menus/AppearanceWindow.h>
#include <filter/Filter.h>
#include <memory>
#include <unordered_set>
#include <menus/LayerWindow.h>
#include <unordered_map>

#include "menus/edit.h"
#include "ShaderProgram.h"
#include "Vbo.h"
#include "projections/Canvas.h"
#include "Texture.h"
#include "Menu.h"
#include "Shader.h"
#include "UndoHistory.h"

class Project {
public:
    ShaderProgram *program;
    Canvas *canvas;
    Project(GLFWwindow* window);

    ~Project();
    void update();
    void render();

    int getWindowWidth();
    int getWindowHeight();

    int getWidth();
    int getHeight();

    void apply(ShaderProgram* program, Texture* texture, std::vector<std::pair<Texture*,std::string>> l={});

    void file_load(const std::string& file_name);
	void file_new(int w, int h);
	void file_write();

	void add_texture(Texture* texture);
	void remove_texture(Texture* texture);
	void add_alias(Texture* t, std::string alias);

	Texture* get_terrain();
	Texture* get_selection();
	Texture* get_scratch2();
	Texture* get_scratch1();

	void add_layer(Layer* l);
	void set_layer(int i);
	Layer* get_layer(int i);
	int get_current_layer();

	std::map<int,Layer*> get_layers();

	void remove_layer(int i);

	void update_terrain_shader();
	void set_terrain_shader(Shader* s);

	void undo();
	void redo();
	void add_history(UndoHistory* h);

	void setCoords(std::vector<float> v);
	std::vector<float> getCoords();

	glm::vec2 getMouse();
	glm::vec2 getMousePrev();

	void dispatchFilter(std::shared_ptr<Filter> filter);
	void finalizeFilter();

	void addAsyncTex(Texture* tex);

	void setCanvasUniforms(ShaderProgram* p);

	float circumference = 42000.0f;
private:
	Texture* asyncTex;
	bool downloadingTex = false;
	GLsync sync;


	std::shared_ptr<Filter> filter;

	std::vector<float> coords = {-M_PI/2, M_PI/2, -M_PI,M_PI};

	GLFWwindow* window;
    Texture* scratchPad = nullptr;
    Texture* scratchPad2 = nullptr;
    Texture* selection = nullptr;

    Shader* terrain_shader;
    AppearanceWindow* appearanceWindow;

    Texture* tmp = nullptr;

    Vbo* vbo;
    GLuint fbo;
	GLuint pbo;
    std::unordered_set<Texture*> textures;
    std::unordered_map<Texture*,std::string> aliasmap; //TODO multiple strings?
    void bind_textures(ShaderProgram *program, std::vector<std::pair<Texture*,std::string>> l={});

    int width, height;

	int current_layer;


	std::map<int,Layer*> layers; //Map id to layer


	std::stack<UndoHistory*> undo_list;
	std::stack<UndoHistory*> redo_list;


    std::vector<std::pair<std::string,std::vector<Menu*>>> windows;
};


#endif //DEMIURGE_PROJECT_H
