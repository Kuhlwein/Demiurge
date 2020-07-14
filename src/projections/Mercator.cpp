//
// Created by kuhlwein on 7/5/20.
//

#include "Project.h"
#include "Mercator.h"

Mercator::Mercator(Project *project) : AbstractCanvas(project) {

}

glm::vec2 Mercator::inverseTransform(glm::vec2 coord) {
	float theta = coord.x;
	float phi = 2*atan(exp(coord.y))-M_PI/2;
	return glm::vec2(theta,phi);
}

Shader *Mercator::inverseShader() {
	return Shader::builder()
			.include(def_pi).create(R"(
vec2 inverseshader(vec2 coord) {
	float theta = coord.x;
    float phi = 2*atan(exp(coord.y))-M_PI/2;

	//if (abs(phi)>89.0/180*M_PI) discard;

	return vec2(theta,phi);
}
)");
}

glm::vec2 Mercator::getScale() {
	return glm::vec2(M_PI,M_PI);
}

glm::vec2 Mercator::getLimits() {
	return glm::vec2(1,3);
}
