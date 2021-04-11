//
// Created by kuhlwein on 7/11/20.
//


#include "Project.h"
#include "EckertIV.h"

EckertIV::EckertIV(Project *project) : AbstractCanvas(project) {

}

glm::vec2 EckertIV::inverseTransform(glm::vec2 coord) {
	float theta = asin(coord.y*sqrt(4+M_PI)/(2*sqrt(M_PI)));

	float phi = asin((theta+sin(theta)*cos(theta)+2*sin(theta))/(2+M_PI/2));
	float lambda = coord.x*sqrt(4*M_PI+M_PI*M_PI)/(2*(1+cos(theta)));

	return glm::vec2(lambda,phi);
}

Shader* EckertIV::inverseShader() {
	return Shader::builder()
			.include(def_pi).create(R"(
vec2 inverseshader(vec2 coord, inout bool outOfBounds) {
	float theta = asin(coord.y*sqrt(4+M_PI)/(2*sqrt(M_PI)));

    float phi = asin((theta+sin(theta)*cos(theta)+2*sin(theta))/(2+M_PI/2));
    float lambda = coord.x*sqrt(4*M_PI+M_PI*M_PI)/(2*(1+cos(theta)));

	if (abs(coord.y)>2*sqrt(M_PI/(4+M_PI))) outOfBounds=true;
    if (abs(lambda)>M_PI) outOfBounds=true;

	return vec2(lambda,phi);
}
)");
}

glm::vec2 EckertIV::getScale() {
	return glm::vec2(2*M_PI*2/sqrt(4*M_PI+M_PI*M_PI),2*2*sqrt(M_PI/(4+M_PI)));
}

glm::vec2 EckertIV::getLimits() {
	return glm::vec2(1,0.5);
}

bool EckertIV::isInterruptible() {
	return true;
}