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
	virtual void finalize() = 0;
	virtual Shader* getShader() {return noneshader;};
protected:
	Project* p;
	Shader* noneshader;
};

class NoneFilter : public Filter {
	void run() override {}
	void finalize() override {}
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
private:
	Texture* tmp;
	std::function<Texture *(Project *p)> target;
};

//class FreeSelectFilter : public BackupFilter {
//public:
//	FreeSelectFilter(Project *p);
//	~FreeSelectFilter();
//	void run() override;
//	void finalize() override;
//	Shader* getShader() override;
//private:
//	glm::vec2 first_mousepos;
//	ShaderProgram* program;
//};

//class SetSelectFilter : public Filter {
//public:
//	SetSelectFilter(Project* p);
//	void run(Project* p) override;
//	void finalize(Project* p) override;
//	bool isfinished() override;
//	Shader* getShader() override;
//private:
//	ShaderProgram* program;
//};


#endif //DEMIURGE_FILTER_H
