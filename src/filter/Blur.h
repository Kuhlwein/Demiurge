//
// Created by kuhlwein on 7/25/20.
//

#ifndef DEMIURGE_BLUR_H
#define DEMIURGE_BLUR_H


#include <Menu.h>


class Blur : public Modal {
public:
	Blur();
	bool update_self(Project* p);
private:
	float radius = 1.0f;
};

class BlurFilter : public BackupFilter {
public:
	BlurFilter(Project *p, float radius);
	~BlurFilter();
	void run() override;
	void finalize() override;
	static void blur(Project* p, float radius, Texture* target);
private:
	ShaderProgram* program;
	float radius;
};


#endif //DEMIURGE_BLUR_H
