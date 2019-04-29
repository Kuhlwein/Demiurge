//
// Created by kuhlwein on 4/9/19.
//

#ifndef DEMIURGE_PROJECT_H
#define DEMIURGE_PROJECT_H
#include <GL/gl3w.h>


class Project {
    public:
    GLuint a;
    GLuint id;
    GLuint programId;
    int shaderProgram;
    GLuint VAO;
    Project();
    ~Project();
    void update();
    void render();
    void cleanup();
};


#endif //DEMIURGE_PROJECT_H
