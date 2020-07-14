//
// Created by kuhlwein on 7/14/20.
//

#ifndef DEMIURGE_APPEARANCE_H
#define DEMIURGE_APPEARANCE_H

#include <string>
#include <Menu.h>

class Project;

class Appearance : public Modal {
public:
	Appearance(std::string title);
	virtual void prepare(Project* p) = 0;
	virtual void unprepare(Project* p) = 0;
	std::string getTitle();
private:
	virtual bool update_self(Project* p) = 0;
protected:
	static int id;
	std::string sid;
};


#endif //DEMIURGE_APPEARANCE_H
