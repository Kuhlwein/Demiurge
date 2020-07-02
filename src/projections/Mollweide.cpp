//
// Created by kuhlwein on 7/2/20.
//

#include "Project.h"
#include "Mollweide.h"

glm::vec2 Mollweide::mousePos(ImVec2 pos) {
	glm::vec2 st = glm::vec2(pos.x/project->getWindowWidth(),pos.y/project->getWindowHeight());

	float x_ = 2.0 * (st.x - 0.5) * pow(ZOOM,z) + x;
	float y_ = 2.0 * (st.y - 0.5 )/windowAspect * pow(ZOOM,z) + y;

	x_ = x_*2*sqrt(2);
	y_ = y_*sqrt(2)*2;
	float theta = asin(y_/sqrt(2));

	float phi = asin((2*theta+sin(2*theta))/3.14159);
	float lambda = 3.14159*x_/(2*sqrt(2)*cos(theta));

	auto v = project->getCoords();
	for (auto &e : v) e=e/180.0f*M_PI;
	float phi2 = (phi-v[0])/(v[1]-v[0]); // 0 to 1
	float theta2 = (lambda-v[2])/(v[3]-v[2]);

	return glm::vec2(theta2,phi2);
}

Shader *Mollweide::projection_shader() {
	return Shader::builder()
			.include(def_pi)
			.include(cornerCoords)
			.create(R"(
uniform float windowaspect=1.0f;
uniform float zoom = 2;
uniform vec2 xyoffset;

vec2 projection(in vec2 st) {

float x = 2.0 * (st.x - 0.5 )*zoom+ xyoffset.x;
float y = 2.0 * (st.y - 0.5 ) / windowaspect * zoom+ xyoffset.y;



x = x*2*sqrt(2);
y = y*sqrt(2)*2;
float theta = asin(y/sqrt(2));

float phi = asin((2*theta+sin(2*theta))/3.14159);
float lambda = 3.14159*x/(2*sqrt(2)*cos(theta));

if (y<-sqrt(2)) discard;
if (y>sqrt(2)) discard;
if (lambda<-3.14159) discard;
if (lambda>3.14159) discard;


phi = (phi);//3.14159;
theta = (lambda);//(2*3.14159);

float phi2 = (phi-cornerCoords[0])/(cornerCoords[1]-cornerCoords[0]); // 0 to 1
float theta2 = (theta-cornerCoords[2])/(cornerCoords[3]-cornerCoords[2]);

if (phi2<0.0f) discard;
if (phi2>1.0f) discard;
if (theta2<0.0f) discard;
if (theta2>1.0f) discard;

return vec2(theta2,phi2);
}
)","vec2 st_p = projection(st);");
}

Mollweide::Mollweide(Project *project) : AbstractCanvas(project) {

}
