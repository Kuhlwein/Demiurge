//
// Created by kuhlwein on 8/23/20.
//

#ifndef DEMIURGE_HEIGHTSELECTION_H
#define DEMIURGE_HEIGHTSELECTION_H


#include <filter/FilterModal.h>



class HeightSelectFilter : public BackupFilter {
public:
	HeightSelectFilter(Project *p, Shader *mode);
	~HeightSelectFilter() override;
	void run(Project* p) override;
	Shader* getShader() override;
	bool isFinished() override;
	void setRange(float min,float max);
	void setMode(Shader* shader);
	void finish();
private:
	ShaderProgram* program;
	Shader* mode;
	bool finished = false;
	float lower=0;
	float upper=10;
};

class HeightSelection : public Modal {
public:
	HeightSelection();
	bool update_self(Project* p);
private:
	Shader* mode;
	std::shared_ptr<HeightSelectFilter> filter;
	bool first = true;
	float range[2] = {0,5};
};


#endif //DEMIURGE_HEIGHTSELECTION_H
