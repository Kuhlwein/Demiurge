//
// Created by kuhlwein on 4/9/19.
//

#ifndef DEMIURGE_PROJECT_H
#define DEMIURGE_PROJECT_H
#include <GL/gl3w.h>
#include <glfw/include/GLFW/glfw3.h>
#include <bits/unique_ptr.h>
#include <map>
#include <set>
#include <stack>
#include "edit.h"
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

	void add_filter(std::function<float(Project* p)> s, std::function<Texture*(Project* p)> t);
	void add_reversible_filter(std::function<float(Project* p)> r, std::function<float(Project* p)> u);
	void preview(std::function<float(Project* p)> s, std::function<Texture*(Project* p)> t);
	void stop_preview();

	void undo();
	void redo();
	void add_history(UndoHistory* h);

	void setGeometryShader(GeometryShader* g);
	GeometryShader* getGeometryShader();


	void setCoords(std::vector<float> v);
	std::vector<float> getCoords();

	glm::vec2 getMouse();
	glm::vec2 getMousePrev();

private:
	std::vector<float> coords = {-90.0f, 90.0f, -180.0f,180.0f};
    GLFWwindow* window;
	Texture* terrain = nullptr;
    Texture* scratchPad = nullptr;
    Texture* scratchPad2 = nullptr;
    Texture* selection = nullptr;

    Shader* terrain_shader;
    GeometryShader* geometryShader;

    Texture* tmp = nullptr;
    bool is_filtering = false;
	std::function<float(Project *p)> filter;
	std::function<Texture *(Project *p)> filter_target;

	bool is_previewing = false;

    Vbo* vbo;
    GLuint fbo;
    std::set<Texture*> textures;
    void bind_textures(ShaderProgram *program, std::vector<std::pair<Texture*,std::string>> l={});
    void run_filter();
    void take_backup(std::function<Texture*(Project* p)> t);
    int width, height;
	std::vector<std::pair<std::string,Texture*>> layers;
	std::stack<UndoHistory*> undo_list;
	std::stack<UndoHistory*> redo_list;
	int current_layer;

	void update_self();





    std::vector<std::pair<std::string,std::vector<Menu*>>> windows;


};


#endif //DEMIURGE_PROJECT_H
