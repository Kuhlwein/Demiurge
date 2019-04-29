//
// Created by kuhlwein on 4/9/19.
//

#ifndef DEMIURGE_PROJECT_H
#define DEMIURGE_PROJECT_H
#include <GL/gl3w.h>
#include "ShaderProgram.h"


class Project {
    public:
    GLuint a;
    GLuint id;
    GLuint programId;
    ShaderProgram *program;
    GLuint VAO;
    Project();
    ~Project();
    void update();
    void render();
    void cleanup();
};


#endif //DEMIURGE_PROJECT_H
