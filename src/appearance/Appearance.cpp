//
// Created by kuhlwein on 7/14/20.
//

#include "Appearance.h"

Appearance::Appearance(std::string title) : Modal(title+"##"+std::to_string(id), [this](Project* p){return this->update_self(p);}) {
	sid = std::to_string(id);
	id++;
}

int Appearance::id = 0;

std::string Appearance::getTitle() {
	return title;
}

std::string Appearance::replaceSID(std::string subject) {
	size_t pos = 0;
	const std::string search = "_SID", replace="_"+sid;
	while ((pos = subject.find(search, pos)) != std::string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}



