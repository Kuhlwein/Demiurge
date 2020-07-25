//
// Created by kuhlwein on 7/25/20.
//

#ifndef DEMIURGE_INVERSESELECT_H
#define DEMIURGE_INVERSESELECT_H

#include <Menu.h>
#include <filter/Filter.h>

class InverseSelect : public Modal {
public:
	InverseSelect();
	bool update_self(Project* p);
};

class SelectInverseFilter : public BackupFilter {
public:
	SelectInverseFilter(Project *p);
	~SelectInverseFilter();
	void run() override;
	void finalize() override;
private:
	ShaderProgram* program;
};


#endif //DEMIURGE_INVERSESELECT_H
