//
// Created by kuhlwein on 7/25/20.
//

#ifndef DEMIURGE_BLURMENU_H
#define DEMIURGE_BLURMENU_H


#include <Menu.h>


class BlurMenu : public Modal {
public:
	BlurMenu();
	bool update_self(Project* p);
private:
	float radius = 1.0f;
};

class Blur : public SubFilter {
public:
	Blur(Project *p, float radius, Texture *target);
	std::pair<bool,float> step() override;
private:
	float radius;
	ShaderProgram *blurProgram;
	ShaderProgram *copyProgram;
	Texture* target;
	Texture* tex1;
	Texture* tex2;
	std::vector<float> rlist;
	int i=0;
};

class BlurTerrain : public ProgressFilter {
public:
	BlurTerrain(Project *p, float radius);
	~BlurTerrain();
	std::pair<bool,float> step();
private:
	ShaderProgram* program;
	Blur* newblur;
};




#endif //DEMIURGE_BLURMENU_H
