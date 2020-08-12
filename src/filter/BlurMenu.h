//
// Created by kuhlwein on 7/25/20.
//

#ifndef DEMIURGE_BLURMENU_H
#define DEMIURGE_BLURMENU_H


#include <Menu.h>
#include <iostream>
#include "FilterModal.h"

class Blur : public SubFilter {
public:
	Blur(Project *p, float radius, Texture *target);
	~Blur() {
		delete tex1;
		delete tex2;
		delete blurProgram;
		delete copyProgram;
		std::cout << "\tdestroy blur\n";
	}

	std::pair<bool,float> step(Project* p) override;
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
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:
	float radius = 1.0f;
};




#endif //DEMIURGE_BLURMENU_H
