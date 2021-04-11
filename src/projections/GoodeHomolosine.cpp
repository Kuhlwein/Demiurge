//
// Created by kuhlwein on 7/10/20.
//

#include "Project.h"
#include "GoodeHomolosine.h"

GoodeHomolosine::GoodeHomolosine(Project *project) : AbstractCanvas(project) {

}

glm::vec2 GoodeHomolosine::inverseTransform(glm::vec2 coord) {

	float phi = coord.y;
	float lambda = coord.x/cos(phi);



	float k = 1.19321014759578607280098010649700264274;
	float k2 = 0.930871;
	coord = coord * k;
	coord.x = coord.x*k2;
	coord.y = ((std::abs(coord.y)-0.711*k)*k2+0.711*k)*(coord.y<0 ? -1.0f : 1.0f);
	float theta = asin(coord.y*2/M_PI);

	lambda = std::abs(phi)>0.711 ? 2*sqrt(2)*coord.x/(2*sqrt(2)*cos(theta)) : lambda;
	phi = std::abs(phi)>0.711 ? asin((2*theta+sin(2*theta))/M_PI) : phi;


	return glm::vec2(lambda,phi);
}

Shader* GoodeHomolosine::inverseShader() {
	return Shader::builder()
			.include(def_pi).create(R"(
vec2 inverseshader(vec2 coord, inout bool outOfBounds) {


    float phi = coord.y;
    float lambda = coord.x/cos(phi);

float k = 1.19321014759578607280098010649700264274;
float k2 = 0.930871;
coord = coord * k;
coord.x = coord.x*k2;
coord.y = ((abs(coord.y)-0.711*k)*k2+0.711*k)*sign(coord.y);
float theta = asin(coord.y*2/M_PI);

	lambda = abs(phi)>0.711 ? 2*sqrt(2)*coord.x/(2*sqrt(2)*cos(theta)) : lambda;
	phi = abs(phi)>0.711 ? asin((2*theta+sin(2*theta))/M_PI) : phi;



    if (lambda<-3.14159) outOfBounds=true;
    if (lambda>3.14159) outOfBounds=true;
	if (abs(coord.y)>M_PI/2) outOfBounds=true;


	return vec2(lambda,phi);
}
)");
}

glm::vec2 GoodeHomolosine::getScale() {
	return glm::vec2(M_PI,M_PI);
}

glm::vec2 GoodeHomolosine::getLimits() {
	return glm::vec2(1,0.5);
}

bool GoodeHomolosine::isInterruptible() {
	return true;
}
