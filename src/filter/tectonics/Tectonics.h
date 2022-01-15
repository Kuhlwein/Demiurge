//
// Created by kuhlwein on 6/29/21.
//

#ifndef DEMIURGE_TECTONICS_H
#define DEMIURGE_TECTONICS_H

#include "../Filter.h"
#include "../FilterModal.h"
#include "Plate.h"

class Project;

class TectonicsMenu : public FilterModal {
public:
	TectonicsMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:

};

class Tectonics : public AsyncSubFilter {
public:
    Tectonics(Project *p);
	~Tectonics();
	void run() override;
	void fold(ShaderProgram *zero, ShaderProgram *operation, Texture* t1, Texture* t2, Project *p);
	void oceanSpreading(Project* p);
    void collision(Project* p);
private:
    std::vector<Plate*> plates;

    Texture* a;
    Texture* b;
    Texture* c;

    ShaderProgram* setzero;
    ShaderProgram* foldShader;

    Shader* tectonicSamplingShader;
};


#endif //DEMIURGE_TECTONICS_H
