//
// Created by kuhlwein on 6/13/19.
//

#ifndef DEMIURGE_SHADER_H
#define DEMIURGE_SHADER_H

#include <string>
#include <vector>

class Shader {
public:
    class builder;
    Shader(std::string include_code, std::vector<Shader*> includes, std::string main_code);
    std::string getCode();
private:
    std::string include_code;
    std::string main_code;
    std::vector<Shader*> includes;
};

class Shader::builder {
public:
    builder include(Shader* shader);
    Shader* create(std::string include_code="",std::string main_code="");
private:
    std::vector<Shader*> includes = {};
};

static Shader* def_pi = Shader::builder()
		.create("#define M_PI 3.1415926535897932384626433832795");

static Shader* cornerCoords = Shader::builder()
		.include(def_pi)
		.create(R"(
uniform float cornerCoords[4];
uniform float circumference;

vec2 tex_to_spheric(vec2 p) {
	p.x = (p.x*(cornerCoords[3]-cornerCoords[2])+cornerCoords[2]);
	p.y = (p.y*(cornerCoords[1]-cornerCoords[0])+cornerCoords[0]);
	return p;
}

vec2 spheric_to_tex(vec2 p) {
	p.x = (p.x-cornerCoords[2])/(cornerCoords[3]-cornerCoords[2]);
	p.y = (p.y-cornerCoords[0])/(cornerCoords[1]-cornerCoords[0]);
	return p;
}

vec4 spheric_to_cartesian(vec2 p) {
	return vec4(cos(p.y)*cos(p.x),cos(p.y)*sin(p.x),sin(p.y),1);
}

vec2 cartesian_to_spheric(vec4 p) {
	return vec2(atan(p.y,p.x),asin(p.z));
}

vec2 pixelsize(vec2 st) {
	vec2 geo = tex_to_spheric(st);
	return vec2((cornerCoords[3]-cornerCoords[2])*cos(geo.y),cornerCoords[1]-cornerCoords[0])*circumference/(2*M_PI) / textureSize(img,0);
}

vec2 offset(vec2 p, vec2 dp, vec2 resolution) {
	p = p + dp/resolution;

	if (cornerCoords[2]<-M_PI+1e-4 && cornerCoords[3]>M_PI-1e-3) p.x = mod(p.x+1,1);
	if (cornerCoords[0]<-M_PI/2+1e-4 && p.y<0) {
		p.y=-p.y;
		p.x = mod((p.x*(cornerCoords[3]-cornerCoords[2])+cornerCoords[2])+2*M_PI,2*M_PI)-M_PI;
		p.x = (p.x-cornerCoords[2])/(cornerCoords[3]-cornerCoords[2]);
	}
	if (cornerCoords[1]>M_PI/2-1e-4 && p.y>1) {
		p.y=2-p.y;
		p.x = mod((p.x*(cornerCoords[3]-cornerCoords[2])+cornerCoords[2])+2*M_PI,2*M_PI)-M_PI;
		p.x = (p.x-cornerCoords[2])/(cornerCoords[3]-cornerCoords[2]);
	}
//	p.x = mod(p.x,1);
	return p;
}
)");//TODO what about comment

/*
 *  Vertex shaders
 */
static Shader* vertexSetup = Shader::builder()
        .create(R"(
layout (location=0) in vec3 position;
layout (location=1) in vec2 texCoord;

out vec2 st;

uniform mat4 worldMatrix;
uniform mat4 projectionMatrix;
)");

static Shader* vertex3D = Shader::builder()
        .include(vertexSetup)
        .create("",R"(
gl_Position = projectionMatrix * worldMatrix * vec4(position, 1.0);
st = texCoord;
)");

static Shader* vertex2D = Shader::builder()
        .include(vertexSetup)
        .create("",R"(
gl_Position =  vec4(position, 1.0);
st = texCoord;
)");


/*
 * Fragment shaders
 */

static Shader* fragmentBase = Shader::builder()
        .create(R"(
in vec2 st;
uniform sampler2D img;
uniform sampler2D sel;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out float fc;
)");

static Shader* fragmentColor = Shader::builder()
		.create(R"(
in vec2 st;
uniform sampler2D img;
uniform sampler2D sel;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out vec4 fc;
)");

static Shader* mouseLocation = Shader::builder()
        .create(R"(
uniform vec2 mouse;
uniform vec2 mousePrev;
uniform float brush_size;
)");

static Shader* copy_img = Shader::builder()
        .include(fragmentBase)
        .create("uniform sampler2D to_be_copied;",R"(
fc = texture(to_be_copied, st).r;
)");

static Shader* fragment_set = Shader::builder()
        .include(fragmentBase)
        .create(R"(
uniform float value=0;
)",R"(
fc = value;
)");

/*
 * Drawing map
 */

static Shader* draw_normal = Shader::builder()
		.include(fragmentColor)
		.create(R"(
vec2 deltax = vec2(1,0)/textureSize(img,0);
vec2 deltay = vec2(0,1)/textureSize(img,0);

float x1 = texture(img, projection(st)-deltax).r;
float x2 = texture(img, projection(st)+deltax).r;
float y1 = texture(img, projection(st)-deltay).r;
float y2 = texture(img, projection(st)+deltay).r;


)",R"(
vec3 normalv = vec3(x1-x2,y2-y1,0.1);
normalv = (normalv/length(normalv)+1)/2;

fc = vec4(normalv,0);
)");

static Shader* brush_outline = Shader::builder()
		.include(mouseLocation)
        .create(R"(
void draw_brush_outline(inout vec4 fc, in vec2 st) {
	float r = geodistance(mouse,st,textureSize(img,0));
	float delta = 2*length(vec2(dFdx(r),dFdy(r)));
	if (r<brush_size && r>brush_size-delta) {
	  	//fc = vec4(1,1,1,0);
		float w = abs(r-(brush_size-0.5*delta))/(0.5*delta);
		fc = fc*(w) + vec4(1,1,1,0)*(1-w);
	}
}
)","draw_brush_outline(fc,st_p);");


static Shader* graticules = Shader::builder()
		.include(def_pi)
		.include(cornerCoords)
		.create(R"(
void draw_graticules(inout vec4 fc, in vec2 st, in float grat, in vec4 grat_color) {

st = tex_to_spheric(st)/M_PI*180;

vec2 dx = dFdx(st);
vec2 dy = dFdy(st);
dx.x = min(abs(dx.x),360-abs(dx.x));
dy.x = min(abs(dy.x),360-abs(dy.x));
float xdiff = 1.2*length(vec2(dx.x,dy.x));
float ydiff = 1.2*length(vec2(dx.y,dy.y));

float absdiff = mod(abs(st.x),grat);
float r = min(absdiff,grat-absdiff);
float w = 1-r/(xdiff);
if (r<xdiff) fc = fc*(1-w*grat_color.w) + grat_color*(w*grat_color.w);

absdiff = mod(abs(st.y),grat);
r = min(absdiff,grat-absdiff);
w = 1-r/(ydiff);
if (r<ydiff) fc = fc*(1-w*grat_color.w) + grat_color*(w*grat_color.w);

}
)","");

static Shader* selection_outline = Shader::builder()
        .create(R"(
uniform float u_time;
void draw_selection_outline(inout vec4 fc, in vec2 st) {
	float dx2 = dFdx(st.x);
    float dy2 = dFdy(st.y);
    float x1 = texture(sel, st-vec2(dx2,0)).r;
    float x2 = texture(sel, st+vec2(dx2,0)).r;
    float y1 = texture(sel, st-vec2(0,dy2)).r;
    float y2 = texture(sel, st+vec2(0,dy2)).r;

    float test = round(mod(gl_FragCoord.x/8-gl_FragCoord.y/8+u_time,1));

    if (bool(x1) != bool(x2)) fc = vec4(test,test,test,0);
    if (bool(y1) != bool(y2)) fc = vec4(test,test,test,0);
}
)","draw_selection_outline(fc,st_p);");


static Shader* texturespace_gradient = Shader::builder()
		.include(cornerCoords)
		.create(R"(
vec2 get_texture_gradient(in vec2 st) {
	vec2 resolution = textureSize(img,0);

	float a = texture(img, offset(st,-vec2(1,1),resolution)).r;
	float b = texture(img, offset(st,-vec2(0,1),resolution)).r;
	float c = texture(img, offset(st,-vec2(-1,0),resolution)).r;
	float d = texture(img, offset(st,-vec2(1,0),resolution)).r;
	float f = texture(img, offset(st,-vec2(-1,0),resolution)).r;
	float g = texture(img, offset(st,-vec2(1,-1),resolution)).r;
	float h = texture(img, offset(st,-vec2(0,-1),resolution)).r;
	float i = texture(img, offset(st,-vec2(-1,-1),resolution)).r;

	vec2 pixelwidth = pixelsize(st);
	float delta_x = (-(c + 2*f + i) + (a + 2*d + g))/(8*pixelwidth.x);
	float delta_y = ((g + 2*h + i) - (a + 2*b + c))/(8*pixelwidth.y);
	return vec2(delta_x,delta_y);
}
)","");

static Shader* get_aspect = Shader::builder()
		.include(def_pi)
		.include(texturespace_gradient)
		.create(R"(
float get_aspect(in vec2 st) {
	vec2 g = get_texture_gradient(st);
	return M_PI-atan(g.y, -g.x);
}
)","");


static Shader* get_slope = Shader::builder()
		.include(def_pi)
		.include(texturespace_gradient)
		.create(R"(
float get_slope(float z_factor, in vec2 st) {
	vec2 g = get_texture_gradient(st);
	return atan(z_factor * sqrt(pow(g.x,2) + pow(g.y,2)));
}
)","");


static Shader* distance = Shader::builder()
		.include(cornerCoords)
		.include(def_pi)
		.create(R"(
float geodistance(vec2 p1, vec2 p2, vec2 size) {
	p1 = tex_to_spheric(p1);
	p2 = tex_to_spheric(p2);
	float delta_sigma = 2*asin(sqrt( pow(sin(abs(p2.y-p1.y)/2) , 2) + cos(p1.y)*cos(p2.y)*pow(sin((p1.x-p2.x)/2),2)));
	return delta_sigma/(cornerCoords[3]-cornerCoords[2])*size.x;
}
)","");

//static Shader* offset_shader = Shader::builder() //vec(1,0) is left, vec(0,1) is up
//		.include(cornerCoords)
//		.create(R"(
//vec2 offset(vec2 p, vec2 dp, vec2 resolution) {
//	p = p + dp/resolution;
//
//	if (cornerCoords[2]<-M_PI+1e-3 && cornerCoords[3]>M_PI-1e-3) p.x = mod(p.x,1);
////	if (p.y<0) {
////	p.y=-p.y;
////	p.x=p.x-0.5;
////	}
////	if (p.y>1) {
////	p.y=2-p.y;
////	p.x=p.x-0.5;
////	}
////	p.x = mod(p.x,1);
//	return p;
//}
//)","");





#endif //DEMIURGE_SHADER_H
