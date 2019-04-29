//
// Created by kuhlwein on 4/9/19.
//

#include <iostream>
#include <GL/gl3w.h>
#include <vector>
#include "Project.h"
#include "Vbo.h"

Project::Project() {


}

void Project::update(){

}

void Project::render(){

    //glUseProgram(0);


    //glBindVertexArray(VAO);

    std::vector<float> positions = {
            -0.5, 0.5, 0,
            -0.5, -0.5, 0,
            0.5, -0.5, 0,
            1, 1, 0
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

    Vbo vbo(positions, textures, indices);
    vbo.render();


    //glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT, 0);
    //glDrawArrays(GL_TRIANGLES, 0, 3);

    //std::cout << glGetError() << "\n";

    //glBindVertexArray(0);

}

void Project::cleanup() {

}

Project::~Project() {
    std::cout << "a\n";
}
