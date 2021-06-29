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
uniform sampler2D pressure;
uniform sampler2D v;
uniform sampler2D divw;
uniform sampler2D divw_showcase;
uniform vec2 dimensions_SID;

vec2 get_st_p(vec2 coord, vec2 dim, inout bool a) {
	return projection(coord/dimensions_SID, a);
}

float xcoordGradient(vec2 direction,vec2 st_local, inout bool a) {
	float v1 = tex_to_spheric(get_st_p(st_local+direction,dimensions_SID,a)).x+M_PI;
	float v2 = tex_to_spheric(get_st_p(st_local-direction,dimensions_SID,a)).x+M_PI;

	float v = v1-v2;

	if(abs(v)>M_PI) v = -sign(v)*(2*M_PI-abs(v));

	return v;
}

vec2 getRotatedCoordinate(vec2 coordinate, float theta) {
	return vec2(cos(theta)*coordinate.x-sin(theta)*coordinate.y,sin(theta)*coordinate.x+cos(theta)*coordinate.y);
}

bool inArrow(vec2 coordinate, float radius, float value) {
	bool black;

	//body
	black = abs(coordinate.x)<radius*0.075*sqrt(value) && abs(coordinate.y)<(radius-1)*value-(radius-1)*0.3;
	//Head
	black = black || (coordinate.y<(radius-1)*value && coordinate.y>(radius-1)*value-(radius-1)*0.3 && abs(coordinate.y-(radius-1)*value)*sqrt(value)>abs(coordinate.x));
	return black;
}

)"),replaceSID(R"(

{

float radius = 15;
float width = radius*2+1;

vec2 coordinate = mod(st*dimensions_SID,width);

vec2 st_local = st*dimensions_SID-coordinate+radius+1;

bool a = false;
vec2 st_p = get_st_p(st_local,dimensions_SID,a);

float y = xcoordGradient(vec2(0,1),st_local,a);
float x = xcoordGradient(vec2(-1,0),st_local,a);

coordinate -= radius+1;

vec2 vel = vec2(texture(scratch1,st_p).r,texture(scratch2,st_p));

float value = texture(scratch2, st_p).r;
value = atan(-vel.y,vel.x);
float theta = value -3.141592/2 + atan(y,x);




coordinate = getRotatedCoordinate(coordinate, theta);

float ratio = 0.0001;




vec4 kk = vec4(texture(scratch2, st_p).r);



kk = vec4(st_p.x);


float arrow = abs(coordinate.x)<1 && coordinate.y>0 || (length(coordinate)<2) ? 0 : 1;

value = min(value,1);
arrow = 1;

//OVerwrite value
value = 0.9;
value = min(length(vel)*8,1.2);

if(inArrow(coordinate, radius, value)) arrow -= 0.2;

for (int i=-2; i<=2; i++) {
	for (int j=-2; j<3; j++) {
		if(inArrow(coordinate+vec2(i*0.2,j*0.2), radius, value)) arrow -= 1.0/25.0;
	}
}

kk = vec4(0);
kk.a = 1-arrow;


//if (texture(img,projection(st)).r<0) fc = vec4(texture(pressure,projection(st)).r/50000+0.5);

//if (texture(img,projection(st)).r<0) fc = vec4(texture(divw_showcase,projection(st)).r/20000+0.5);

vel = vec2(texture(scratch1,projection(st)).r,texture(scratch2,projection(st)));
if (texture(img,projection(st)).r<=0) fc = 1- vec4(length(vel)*8);




//vec2 resolution = textureSize(v,0);
//// Find neighboring velocities:
//float vNy = texture(v,offset(projection(st),vec2(0,1),resolution)).y;
//float vSy = texture(v,offset(projection(st),vec2(0,-1),resolution)).y;
//float vEx = texture(v,offset(projection(st),vec2(1,0),resolution)).x;
//float vWx = texture(v,offset(projection(st),vec2(-1,0),resolution)).x;
////Multiply by half cell size??? //TODO
//vec2 cellsize = 1/resolution*200;
//fc = vec4(0.5 + 0.5*((vEx - vWx)/cellsize.x + (vNy - vSy)/cellsize.y)/2);




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
