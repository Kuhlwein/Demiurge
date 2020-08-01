//
// Created by kuhlwein on 7/18/20.
//

#ifndef DEMIURGE_FILTER_H
#define DEMIURGE_FILTER_H

#include <glm/glm.hpp>
#include <ShaderProgram.h>
#include <Shader.h>
#include <Menu.h>

class Project;
class Texture;

class Filter {
public:
	Filter() {
		noneshader = Shader::builder().create("","");
	}
	virtual ~Filter() = default;
	virtual void run(Project* p) = 0;
	virtual Shader* getShader() {return noneshader;};
	virtual bool isFinished() = 0;
protected:
	Shader* noneshader;
};

class NoneFilter : public Filter {
	void run(Project* p) override {}
	virtual Shader* getShader() override {return Shader::builder().create("","");}
	bool isFinished() override {return false;}
public:
	NoneFilter() : Filter() {}
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
	Project* p;
	Texture* tmp;
	std::function<Texture *(Project *p)> target;
};

class SubFilter {
public:
	SubFilter() {}
	virtual std::pair<bool,float> step(Project* p) = 0;
};

class ProgressFilter : public BackupFilter {
public:
	ProgressFilter(Project* p, std::function<Texture *(Project *p)> target, SubFilter* subfilter);
	virtual ~ProgressFilter();
	void progressBar(float a);
	void run(Project* p) override;
	bool isFinished() override;
private:
	float progress;
	Modal* progressModal;
	bool aborting = false;
	bool finished = false;
	SubFilter* subFilter;
};






#endif //DEMIURGE_FILTER_H
