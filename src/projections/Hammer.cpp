//
// Created by kuhlwein on 8/8/20.
//

#include "Hammer.h"

Hammer::Hammer(Project *project) : AbstractCanvas(project) {

}

glm::vec2 Hammer::inverseTransform(glm::vec2 coord) {
	float z = sqrt(1-pow(0.25*coord.x,2)-pow(0.5*coord.y,2));

	float phi = asin(coord.y*z);
	float lambda = 2*atan(z*coord.x/(2*(2*z*z-1)));

	return glm::vec2(lambda,phi);
}

Shader* Hammer::inverseShader() {
	return Shader::builder()
			.include(def_pi).create(R"(
vec2 inverseshader(vec2 coord, inout bool outOfBounds) {
	float z = sqrt(1-pow(0.25*coord.x,2)-pow(0.5*coord.y,2));

    float phi = asin(coord.y*z);
    float lambda = 2*atan(z*coord.x/(2*(2*z*z-1)));

	if (coord.x*coord.x+4*coord.y*coord.y>8) outOfBounds=true;

	return vec2(lambda,phi);
}
)");
}

glm::vec2 Hammer::getScale() {
	return glm::vec2(sqrt(8),sqrt(2)*2);
}

glm::vec2 Hammer::getLimits() {
	return glm::vec2(1,0.5);
}

bool Hammer::isInterruptible() {
	return true;
}