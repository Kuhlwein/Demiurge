//
// Created by kuhlwein on 4/30/19.
//

#include <GL/gl3w.h>
#include "Canvas.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <iostream>
#include <imgui/imgui.h>
#include "Project.h"


Canvas::Canvas(int height, int width, Project* project) {
    this->project = project;
    canvasAspect = (float) width / height;

    std::vector<float> positions = {
            -canvasAspect, 1, 0,
            -canvasAspect, -1, 0,
            canvasAspect, -1, 0,
            canvasAspect, 1, 0
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
    z=0;

    FOVY = glm::radians(60.0); //radian
    TANFOV = glm::tan(FOVY *0.5);

    windowAspect = 1.7777;
    Z_NEAR = 0.001f;
    Z_FAR = 1000.f;
    ZOOM = 1.1;


}

void Canvas::render(int programId) {


    glm::mat4 model(1.f);


    glm::mat4 projection = glm::perspective(FOVY,windowAspect,Z_NEAR,Z_FAR);
    glm::mat4 world = glm::translate(model,glm::vec3(x,y,-pow(ZOOM,z))-Z_NEAR);


    int id = glGetUniformLocation(programId,"projectionMatrix");
    glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(projection));

    id = glGetUniformLocation(programId,"worldMatrix");
    glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(world));

    vbo->render();
}

Canvas::~Canvas() {
    delete(vbo);
}

void Canvas::pan(float dx, float dy) {
    float scaling = (float) (pow(ZOOM,z)+Z_NEAR)*TANFOV*2/project->getWindowHeight();
    x+=dx * scaling;
    y-=dy * scaling;

    if(x<-canvasAspect) x=-canvasAspect;
    else if(x>canvasAspect) x=canvasAspect;
    if(y<-1) y=-1;
    else if(y>1) y=1;
}

void Canvas::update() {
    ImGuiIO io = ImGui::GetIO();
    windowAspect = (float)project->getWindowWidth()/project->getWindowHeight();

    if(io.WantCaptureMouse) return;

    if(io.MouseDown[2]) {
        //pan
        float dx = io.MouseDelta.x, dy=io.MouseDelta.y;
        pan(dx,dy);
    }

    if(io.MouseWheel!=0) {
        float delta = io.MouseWheel;

        z+=delta;
        float deltax = (io.MousePos.x-project->getWindowWidth()*0.5f)*(ZOOM-1);
        float deltay = (io.MousePos.y-project->getWindowHeight()*0.5f)*(ZOOM-1);
        pan(delta*deltax,delta*deltay);

    }

}
