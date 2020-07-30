//
// Created by kuhlwein on 4/9/19.
//

#ifndef DEMIURGE_PROJECT_H
#define DEMIURGE_PROJECT_H
#include <GL/gl3w.h>
#include <glfw/include/GLFW/glfw3.h>
#include <map>
#include <set>
#include <stack>
#include <menus/AppearanceWindow.h>
#include <filter/Filter.h>
#include <geometry/Geometry.h>
#include <memory>
#include <imgui/examples/libs/glfw/include/GLFW/glfw3.h>

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

	void add_texture(Texture* texture);
	void remove_texture(Texture* texture);
	int get_n_textures();
	Texture* get_terrain();
	Texture* get_selection();
	Texture* get_scratch2();
	Texture* get_scratch1();

	void set_terrain(Texture* texture);

	int get_n_layers();
	void add_layer(std::pair<std::string,Texture*> l,int index=0);
	void set_layer(int i);
	std::pair<std::string,Texture*> get_layer(int i);
	int get_current_layer();
	void remove_layer(int i);
	void update_terrain_shader();
	void set_terrain_shader(Shader* s);

	void undo();
	void redo();
	void add_history(UndoHistory* h);

	void setGeometry(Geometry* g);
	Geometry* getGeometry();

	void setCoords(std::vector<float> v);
	std::vector<float> getCoords();

	glm::vec2 getMouse();
	glm::vec2 getMousePrev();

	void dispatchFilter(std::shared_ptr<Filter> filter);
	void finalizeFilter();

	void addAsyncTex(Texture* tex);

private:
	Texture* asyncTex;
	bool downloadingTex = false;
	GLsync sync;


	std::shared_ptr<Filter> filter;

	std::vector<float> coords = {-90.0f, 90.0f, -180.0f,180.0f};
    GLFWwindow* window;
	Texture* terrain = nullptr;
    Texture* scratchPad = nullptr;
    Texture* scratchPad2 = nullptr;
    Texture* selection = nullptr;

    Shader* terrain_shader;
    Geometry* geometry;
    AppearanceWindow* appearanceWindow;

    Texture* tmp = nullptr;

    Vbo* vbo;
    GLuint fbo;
	GLuint pbo;
    std::set<Texture*> textures;
    void bind_textures(ShaderProgram *program, std::vector<std::pair<Texture*,std::string>> l={});

    int width, height;
	std::vector<std::pair<std::string,Texture*>> layers;
	std::stack<UndoHistory*> undo_list;
	std::stack<UndoHistory*> redo_list;
	int current_layer;

    std::vector<std::pair<std::string,std::vector<Menu*>>> windows;
};


#endif //DEMIURGE_PROJECT_H
