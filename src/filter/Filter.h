//
// Created by kuhlwein on 7/18/20.
//

#ifndef DEMIURGE_FILTER_H
#define DEMIURGE_FILTER_H

#include <glm/glm.hpp>
#include <ShaderProgram.h>
#include <Shader.h>

class Project;
class Texture;

class Filter {
public:
	Filter(Project* p) {
		this->p = p;
		noneshader = Shader::builder().create("","");
	}
	virtual ~Filter() = default;
	virtual void run() = 0;
	virtual Shader* getShader() {return noneshader;};
protected:
	Project* p;
	Shader* noneshader;
};

class NoneFilter : public Filter {
	void run() override {}
	virtual Shader* getShader() override {return Shader::builder().create("","");}
public:
	NoneFilter() : Filter(nullptr) {}
	~NoneFilter() = default;
};

class BackupFilter : public Filter {
public:
	BackupFilter(Project* p, std::function<Texture *(Project *p)> target);
	virtual ~BackupFilter();
	void add_history();
	void restoreUnselected();
	void restoreBackup();

protected:
	Texture* tmp;
	std::function<Texture *(Project *p)> target;
};

class ProgressFilter : public BackupFilter {
public:
	ProgressFilter(Project* p, std::function<Texture *(Project *p)> target);
	virtual ~ProgressFilter();
	void progressBar(float a);
	virtual std::pair<bool,float> step() = 0;
	void run() override;
private:
	float progress;
	Modal* progressModal;
	bool aborting = false;
};

class SubFilter {
public:
	SubFilter(Project* p) {
		this->p = p;
	}
	virtual std::pair<bool,float> step() = 0;
protected:
	Project* p;
};




#endif //DEMIURGE_FILTER_H
