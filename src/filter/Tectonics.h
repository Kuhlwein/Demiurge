//
// Created by kuhlwein on 6/29/21.
//

#ifndef DEMIURGE_TECTONICS_H
#define DEMIURGE_TECTONICS_H

#include "Filter.h"
#include "FilterModal.h"

class Project;

class TectonicsMenu : public FilterModal {
public:
	TectonicsMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:

};


class Plate {
public:
    Plate(int width, int height);
    ~Plate();
    Texture* getTexture();
    void rotate();
    void setPlateUniforms(ShaderProgram* shaderProgram, int indextsh);
    void updateRotationBy(float theta, glm::vec3 axis);
private:
    //crust thickness
    //Crust age
    //local ridge/fold direction
    //(orogeny type???)
    Texture* texture;
    glm::mat4 rotation;
    glm::vec3 angularVelocity;
};

class Tectonics : public AsyncSubFilter {
public:
    Tectonics(Project *p);
	~Tectonics();
	void run() override;
	void fold(ShaderProgram *zero, ShaderProgram *operation, Texture* t1, Texture* t2, Project *p);
private:
    std::vector<Plate*> plates;

    Texture* a;
    Texture* b;

    ShaderProgram* setzero;
    ShaderProgram* foldShader;
};


#endif //DEMIURGE_TECTONICS_H