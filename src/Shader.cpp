//
// Created by kuhlwein on 6/13/19.
//

#include <iostream>
#include "Shader.h"
#include <algorithm>

Shader::Shader(std::string include_code, std::vector<Shader *> includes, std::string main_code) {
    this->include_code = include_code;
    this->includes = includes;
    this->main_code = main_code;
}

std::string Shader::getCode(){
    std::string include_code = "#version 430";
    for (Shader* s : includes) {
		include_code += "\n" + s->include_code;
    }
    include_code += "\n" + this->include_code;
	std::string main_code = "\nvoid main () {\n";
	for (Shader* s : includes) {
		main_code += "\n" + s->main_code;
	}
	main_code += "\n" + this->main_code;
    return include_code + main_code + "\n}";
}

Shader::builder Shader::builder::include(Shader *shader) {

    for (Shader* s : shader->includes) {
		bool flag = true;
    	for (auto i : includes) if (i->getCode()==s->getCode()) {
    		flag = false;
    		break;
    	}
        if(flag) {
        	include(s);
        }
    }
    if(std::count(includes.begin(), includes.end(), shader)==0) includes.push_back(shader);

	//std::cout << "includes:";
	//for (auto s : includes) std::cout << " " << s << "\n";


    return *this;
}

Shader *Shader::builder::create(std::string include_code, std::string main_code) {
    return new Shader(include_code,includes,main_code);
}
