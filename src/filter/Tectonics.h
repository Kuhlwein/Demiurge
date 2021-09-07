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
    void rotate(float theta, glm::vec3 axis);
    glm::mat4 rotation;
private:
    Texture* texture;
};

class Tectonics : public AsyncSubFilter {
public:
    Tectonics(Project *p);
	~Tectonics();
	void run() override;
private:
    Plate* plate;
    Plate* plate2;
};


#endif //DEMIURGE_TECTONICS_H
