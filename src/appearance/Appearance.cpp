//
// Created by kuhlwein on 7/14/20.
//

#include "Appearance.h"
#include <iostream>

Appearance::Appearance(std::string title) : Modal(title+"##"+std::to_string(id), [this](Project* p){return this->update_self(p);}) {
	sid = std::to_string(id);
	id++;
}

int Appearance::id = 0;

std::string Appearance::getTitle() {
	return title;
}
