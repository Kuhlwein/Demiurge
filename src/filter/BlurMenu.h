//
// Created by kuhlwein on 7/25/20.
//

#ifndef DEMIURGE_BLURMENU_H
#define DEMIURGE_BLURMENU_H


#include <Menu.h>
#include "FilterModal.h"

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

class BlurMenu : public FilterModal {
public:
	BlurMenu();
	void update_self(Project* p) override;
	std::shared_ptr<Filter> makeFilter(Project* p) override;
private:
	float radius = 1.0f;
	//std::shared_ptr<Filter> filter;
};



class BlurTerrain : public ProgressFilter {
public:
	BlurTerrain(Project *p, float radius);
	~BlurTerrain();
	std::pair<bool,float> step();
private:

	Blur* newblur;
};




#endif //DEMIURGE_BLURMENU_H
