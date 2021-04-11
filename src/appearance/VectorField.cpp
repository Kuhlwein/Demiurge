//
// Created by kuhlwein on 4/11/21.
//

#include "VectorField.h"
#include "Shader.h"
#include "Project.h"

VectorField::VectorField() : Appearance("Vectorfield") {
	shader = Shader::builder()
			.include(fragmentColor)
			.include(def_pi)
			.include(get_slope)
			.create(replaceSID(R"(
uniform vec2 dimensions_SID;

vec2 get_st_p(vec2 coord, vec2 dim, inout bool a) {
	return projection(coord/dimensions_SID, a);
}

float xcoordGradient(vec2 direction,vec2 st_local, inout bool a) {
	float v1 = tex_to_spheric(get_st_p(st_local+direction,dimensions_SID,a)).x+M_PI;
	float v2 = tex_to_spheric(get_st_p(st_local-direction,dimensions_SID,a)).x+M_PI;
	//if(abs(v1)>=M_PI/2) v1 -= sign(v1)*M_PI;
	//if(abs(v2)>M_PI/2) v2 -= sign(v2)*M_PI;

	float v = v1-v2;

	if(abs(v)>M_PI) v = -sign(v)*(2*M_PI-abs(v));

	return v;
}

)"),replaceSID(R"(

{

float radius = 10;
float width = radius*2+1;

vec2 coordinate = mod(st*dimensions_SID,width);

vec2 st_local = st*dimensions_SID-coordinate+radius+1;

bool a = false;
vec2 st_p = get_st_p(st_local,dimensions_SID,a);

float y = xcoordGradient(vec2(0,1),st_local,a);
float x = xcoordGradient(vec2(-1,0),st_local,a);

coordinate -= radius+1;

float value = texture(img, st_p).r;
float theta = 3.14159*2*value*0 + atan(y,x);




coordinate = vec2(cos(theta)*coordinate.x-sin(theta)*coordinate.y,sin(theta)*coordinate.x+cos(theta)*coordinate.y);




vec4 kk = vec4(texture(img, st_p).r);



kk = vec4(st_p.x);
kk = vec4(abs(coordinate.x)<1 && coordinate.y>0 || (length(coordinate)<2) ? 1 : 0);
kk.a = (abs(coordinate.x)<1  && coordinate.y>0 || (length(coordinate)<2) ? 1 : 0);




if(!a) fc = fc*(1-kk.a) + kk*(kk.a);
//fc = vec4(x/3.1415/2/2);
}

)"));
}

void VectorField::prepare(Project *p) {
	int id = glGetUniformLocation(p->program->getId(),replaceSID("dimensions_SID").c_str());
	glUniform2f(id,p->getWindowWidth(),p->getWindowHeight());
}

void VectorField::unprepare(Project *p) {

}

Shader *VectorField::getShader() {
	return shader;
}

bool VectorField::update_self(Project *p) {

	return false;
}
