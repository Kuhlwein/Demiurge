//
// Created by kuhlwein on 4/12/21.
//

#include <iostream>
#include "Project.h"
#include "OceanCurrents.h"

OceanCurrentsMenu::OceanCurrentsMenu() : FilterModal("OceanCurrents") {

}

void OceanCurrentsMenu::update_self(Project *p) {

}

std::shared_ptr<BackupFilter> OceanCurrentsMenu::makeFilter(Project *p) {
	return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_terrain();}, std::move(std::make_unique<OceanCurrents>(p)));
}

OceanCurrents::OceanCurrents(Project *p) {
	v = new Texture(p->getWidth(),p->getHeight(),GL_RG32F,"v",GL_LINEAR);
	v_scratch = new Texture(p->getWidth(),p->getHeight(),GL_RG32F,"v",GL_LINEAR);
	pressure = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"pressure",GL_LINEAR);
	scratch = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"scratch",GL_LINEAR);
	divw = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"divw",GL_LINEAR);
	p->add_texture(v);
	p->add_texture(v_scratch);
	p->add_texture(pressure);
	p->add_texture(scratch);
	p->add_texture(divw);


	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.include(pidShader)
			.create("",R"(
	fc =  0;
)");
	setzero = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	setzero->bind();
	p->setCanvasUniforms(setzero);

	p->apply(setzero,pressure);


	shader = Shader::builder()
			.create(R"(
in vec2 st;
uniform sampler2D img;
uniform sampler2D sel;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out vec2 fc;
)",R"(
	//fc =  vec2(0.5,sign(st.y-0.5)*pow(abs(st.y-0.5),2)/pow(0.5,2))/10;
	//fc = length(st-vec2(0.5,0.5))<0.05 ? vec2(0.1,0) : vec2(0,0);
	fc = vec2(0,0);
	//if (abs(0.5-st.x)<0.05 && abs(st.y-0.5)<0.1) fc = vec2(1,0);
)");
	ShaderProgram* setzero2 = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();
	setzero2->bind();
	p->setCanvasUniforms(setzero2);

	p->apply(setzero2,v);
}

void OceanCurrents::run() {



	for(int j=0; j<50; j++) {
		dispatchGPU([this](Project* p) {

			advect(p);

			diffusion(p);

			divergence(p);
		});


			jacobi();
		dispatchGPU([this](Project* p) {
			subDiv(p);




			Shader* image = Shader::builder()
					.include(fragmentBase)
					.include(cornerCoords)
					.create(R"(
uniform sampler2D v;
)",R"(
	fc = texture(v,st).x;
)");
			ShaderProgram* imageProgram = ShaderProgram::builder()
					.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
					.addShader(image->getCode(), GL_FRAGMENT_SHADER)
					.link();
			imageProgram->bind();
			p->setCanvasUniforms(imageProgram);
			p->apply(imageProgram,p->get_scratch1());

			image = Shader::builder()
					.include(fragmentBase)
					.include(cornerCoords)
					.create(R"(
uniform sampler2D v;
)",R"(
	fc = texture(v,st).y;
)");
			imageProgram = ShaderProgram::builder()
					.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
					.addShader(image->getCode(), GL_FRAGMENT_SHADER)
					.link();
			imageProgram->bind();
			p->setCanvasUniforms(imageProgram);
			p->apply(imageProgram,p->get_scratch2());




		});
	}

	while(true);

	dispatchGPU([this](Project* p) {
		p->remove_texture(v);
		p->remove_texture(v_scratch);
		p->remove_texture(pressure);
		p->remove_texture(scratch);
	});

	setProgress({true,1.0});
}

OceanCurrents::~OceanCurrents() {
	std::cout << v->getWidth() << " width\n";
	delete v;
	delete v_scratch;
	delete pressure;
	delete scratch;
	std::cout << "delete\n";
}

void OceanCurrents::advect(Project* p) {
	/*
	 * Advect
	 */
	Shader *vec2shader = Shader::builder()
			.create(R"(
in vec2 st;
uniform sampler2D img;
uniform sampler2D sel;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out vec2 fc;
)", "");

	vectorShader = Shader::builder()
			.create(R"(
vec3 v_to_cartesian(vec2 v, vec2 spheric_coord) {
	vec3 cartesian_coord = spheric_to_cartesian(spheric_coord);
	vec2 inward = normalize(cartesian_coord.xy);
	vec3 y_comp = vec3(sin(spheric_coord.y)*(-inward.x),sin(spheric_coord.y)*(-inward.y),cos(spheric_coord.y));

	vec3 parallel_comp = normalize(cross(vec3(0,0,1),cartesian_coord));
	return v.x*parallel_comp + v.y*y_comp;
}

vec2 cartesian_to_v(vec3 v, vec2 spheric_coord) {
	vec3 cartesian_coord = spheric_to_cartesian(spheric_coord);
	vec2 inward = normalize(cartesian_coord.xy);
	vec3 y_comp = vec3(sin(spheric_coord.y)*(-inward.x),sin(spheric_coord.y)*(-inward.y),cos(spheric_coord.y));

	float z_comp = v.z/y_comp.z;

	vec3 parallel_comp = normalize(cross(vec3(0,0,1),cartesian_coord));

	return vec2(((v-z_comp*y_comp)/parallel_comp).x,z_comp);
}
)","");

	Shader *advect = Shader::builder()
			.include(vec2shader)
			.include(cornerCoords)
			.include(rotation_matrix)
			.include(vectorShader)
			.create(R"(
uniform sampler2D v;
uniform sampler2D vy;
uniform sampler2D pressure;

)", R"(
vec2 resolution = textureSize(v,0);

    float solid = texture(img, st).r;
    if (solid > 0) {
        fc = vec2(0);
        return;
    }

	vec2 vel = texture(v, st).xy;

	float timestep = 1*600*2; //hour
	float Dissipation = 1;

	float distance = length(vel)*timestep;
	float arclength = 2*3.14159/circumference*distance;


	vec2 spheric_coord = tex_to_spheric(st);
	vec3 cartesian_coord = spheric_to_cartesian(spheric_coord);

	vec3 v_cart = v_to_cartesian(vel, spheric_coord);
	vec3 rotation_direction = normalize(cross(cartesian_coord,v_cart));

	cartesian_coord = rotation_matrix(-arclength,rotation_direction)*cartesian_coord;

	vec2 spheric_coord_new = cartesian_to_spheric(cartesian_coord);


	vec2 coord = spheric_to_tex(spheric_coord_new);

	vec2 newV = texture(v, coord).xy;
	vec3 testv = v_to_cartesian(newV, spheric_coord_new);
	testv = rotation_matrix(arclength,rotation_direction)*testv;
	newV = cartesian_to_v(testv,tex_to_spheric(st));

	//TODO handle 0 vector original??
	if (any(isnan(newV))) newV = vec2(0,0);

    fc = Dissipation * newV;

	//if (abs(st.y-0.5)<0.05) fc = fc*0.3 + 0.7*vec2(-1, 0);
	if (abs(st.x-0.5)<0.01 && st.y>0.5) fc = fc*0.3 + 0.7*vec2(0, 1);

	float phi = 2*(st.y-0.5)*3.14159;
	vec2 wind = vec2(-cos(phi*3/2),0);
	if (abs(phi)>3.14159*2/3) wind.x = -wind.x;
	//fc = fc*0.85 + 0.15*wind;
)");
	ShaderProgram *advectProgram = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(advect->getCode(), GL_FRAGMENT_SHADER)
			.link();

	advectProgram->bind();
	p->setCanvasUniforms(advectProgram);
	p->apply(advectProgram, v_scratch);
	v->swap(v_scratch);
}


void OceanCurrents::divergence(Project *p) {
	/*
	 * divergence
	 */
	Shader *divergence = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(vectorShader)
			.create(R"(
uniform sampler2D v;
uniform sampler2D pressure;

vec2 get_velocity(vec2 st, vec2 o) {
	vec2 resolution = textureSize(v,0);
	vec2 st_o = offset(st,o,resolution);

	vec2 d = pixelsize(st);
	vec2 d_o = pixelsize(st_o);

	vec2 sph = tex_to_spheric(st);
	vec2 sph_o = tex_to_spheric(st_o);

	vec2 v = texture(v,st_o).xy;

	//TODO better implementation
	if (abs(abs(sph.x-sph_o.x)-3.14159)<0.1) v = -v;
	//v = cartesian_to_v(v_to_cartesian(v, sph_o),sph);

	return v * (d_o.x*d_o.y)/(d.x*d.y);
}

)", R"(
vec2 resolution = textureSize(v,0);

// Find neighboring velocities:
float vNy = get_velocity(st,vec2(0,1)).y;
float vSy = get_velocity(st,vec2(0,-1)).y;
float vEx = get_velocity(st,vec2(1,0)).x;
float vWx = get_velocity(st,vec2(-1,0)).x;
float vCx = texture(v,st).x;
float vCy = texture(v,st).y;

// Find neighboring obstacles:
float oN = texture(img,offset(st,vec2(0,1),resolution)).r;
float oS = texture(img,offset(st,vec2(0,-1),resolution)).r;
float oE = texture(img,offset(st,vec2(1,0),resolution)).r;
float oW = texture(img,offset(st,vec2(-1,0),resolution)).r;

// Use obstacle velocities for solid cells:
if (oN > 0) vNy = 0;
if (oS > 0) vSy = 0;
if (oE > 0) vEx = 0;
if (oW > 0) vWx = 0;

//Multiply by half cell size??? //TODO
vec2 pixelwidth = pixelsize(st)/420;
fc = 0.5*((vEx - vWx)/pixelwidth.x + (vNy - vSy)/pixelwidth.y);
)");

	ShaderProgram *divProgram = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(divergence->getCode(), GL_FRAGMENT_SHADER)
			.link();

	divProgram->bind();
	p->setCanvasUniforms(divProgram);
	p->apply(divProgram, divw);

	setzero->bind();
	p->setCanvasUniforms(setzero);
	p->apply(setzero, pressure);
	//TODO area of pixel?????????????????????????????????????????????????????
}

void OceanCurrents::jacobi() {
	/*
		 * jacobi
		 */
	dispatchGPU([this](Project* p) {
		Shader *divergence = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.create(R"(
uniform sampler2D v;
uniform sampler2D pressure;
uniform sampler2D divw;

)", R"(
	vec2 resolution = textureSize(v,0);



	// Find neighboring pressure:
	float pN = texture(pressure,offset(st,vec2(0,1),resolution)).r;
	float pS = texture(pressure,offset(st,vec2(0,-1),resolution)).r;
	float pE = texture(pressure,offset(st,vec2(1,0),resolution)).r;
	float pW = texture(pressure,offset(st,vec2(-1,0),resolution)).r;
	float pC = texture(pressure,st).r;

    // Find neighboring obstacles:
    float oN = texture(img,offset(st,vec2(0,1),resolution)).r;
	float oS = texture(img,offset(st,vec2(0,-1),resolution)).r;
	float oE = texture(img,offset(st,vec2(1,0),resolution)).r;
	float oW = texture(img,offset(st,vec2(-1,0),resolution)).r;

    // Use center pressure for solid cells:
    if (oN > 0) pN = pC;
    if (oS > 0) pS = pC;
    if (oE > 0) pE = pC;
    if (oW > 0) pW = pC;

	float bC = texture(divw,st).r;

	vec2 pixelwidth2 = pow(pixelsize(st)/420,vec2(2,2));
	float Beta = 2*(1/pixelwidth2.x+1/pixelwidth2.y);
    fc = ((pW + pE)/pixelwidth2.x + (pS + pN)/pixelwidth2.y - bC) / Beta;
)");

		jacobiProgram = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(divergence->getCode(), GL_FRAGMENT_SHADER)
				.link();
	});

	dispatchGPU([this](Project* p) {
		for (int i = 0; i < 5000; ++i) {

			jacobiProgram->bind();
			p->setCanvasUniforms(jacobiProgram);
			p->apply(jacobiProgram, scratch);
			pressure->swap(scratch);

		}
	});
}

void OceanCurrents::subDiv(Project* p) {
	Shader *outvec = Shader::builder()
			.create(R"(
in vec2 st;
uniform sampler2D img;
uniform sampler2D sel;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out vec2 fc;
)", "");

	Shader *subgradient = Shader::builder()
			.include(outvec)
			.include(cornerCoords)
			.create(R"(
uniform sampler2D v;
uniform sampler2D vy;
uniform sampler2D pressure;

)", R"(
vec2 resolution = textureSize(v,0);



    float oC = texture(img,st).r;
    if (oC > 0) {
        fc = vec2(0,0);
        return;
    }

    // Find neighboring pressure:
	float pN = texture(pressure,offset(st,vec2(0,1),resolution)).r;
	float pS = texture(pressure,offset(st,vec2(0,-1),resolution)).r;
	float pE = texture(pressure,offset(st,vec2(1,0),resolution)).r;
	float pW = texture(pressure,offset(st,vec2(-1,0),resolution)).r;
	float pC = texture(pressure,st).r;

    // Find neighboring obstacles:
    float oN = texture(img,offset(st,vec2(0,1),resolution)).r;
	float oS = texture(img,offset(st,vec2(0,-1),resolution)).r;
	float oE = texture(img,offset(st,vec2(1,0),resolution)).r;
	float oW = texture(img,offset(st,vec2(-1,0),resolution)).r;

    // Use center pressure for solid cells:
    vec2 obstV = vec2(0);
    vec2 vMask = vec2(1);

    if (oN > 0) { pN = pC; obstV.y = 0; vMask.y = 0; }
    if (oS > 0) { pS = pC; obstV.y = 0; vMask.y = 0; }
    if (oE > 0) { pE = pC; obstV.x = 0; vMask.x = 0; }
    if (oW > 0) { pW = pC; obstV.x = 0; vMask.x = 0; }

    // Enforce the free-slip boundary condition:
	vec2 oldV = texture(v,st).xy;
	float GradientScale = 1/2;
    vec2 grad = vec2(pE - pW, pN - pS)*1 * GradientScale;
    vec2 newV = oldV - grad;
	fc = (vMask*newV) + obstV;

	vec2 pixelwidth = pixelsize(st)/420;
	fc = oldV - 0.5*vec2(pE-pW,pN-pS)/pixelwidth;



)");
	ShaderProgram *subdiv = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(subgradient->getCode(), GL_FRAGMENT_SHADER)
			.link();

	subdiv->bind();
	p->setCanvasUniforms(subdiv);
	p->apply(subdiv, v_scratch);
	v->swap(v_scratch);
}


void OceanCurrents::diffusion(Project *p) {
	/*
	 * diffiusion
	 */
	Shader *outvec = Shader::builder()
			.create(R"(
in vec2 st;
uniform sampler2D img;
uniform sampler2D sel;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out vec2 fc;
)", "");

	Shader *divergence = Shader::builder()
			.include(outvec)
			.include(cornerCoords)
			.create(R"(
uniform sampler2D v;
uniform sampler2D divW;

//TODO remove duplication
vec2 get_velocity(vec2 st, vec2 o) {
	vec2 resolution = textureSize(v,0);
	vec2 st_o = offset(st,o,resolution);

	vec2 sph = tex_to_spheric(st);
	vec2 sph_o = tex_to_spheric(st_o);

	vec2 v = texture(v,st_o).xy;

	//TODO better implementation
	if (abs(abs(sph.x-sph_o.x)-3.14159)<0.1) v = -v;
	//v = cartesian_to_v(v_to_cartesian(v, sph_o),sph);

	return v;
}

)", R"(
	vec2 resolution = textureSize(v,0);



	// Find neighboring pressure:
	vec2 pN = get_velocity(st,vec2(0,1));
	vec2 pS = get_velocity(st,vec2(0,-1));
	vec2 pE = get_velocity(st,vec2(1,0));
	vec2 pW = get_velocity(st,vec2(-1,0));
	vec2 pC = texture(v,st).xy;

    // Find neighboring obstacles:
    float oN = texture(img,offset(st,vec2(0,1),resolution)).r;
	float oS = texture(img,offset(st,vec2(0,-1),resolution)).r;
	float oE = texture(img,offset(st,vec2(1,0),resolution)).r;
	float oW = texture(img,offset(st,vec2(-1,0),resolution)).r;

    // Use center pressure for solid cells:
    if (oN > 0) pN = pC;
    if (oS > 0) pS = pC;
    if (oE > 0) pE = pC;
    if (oW > 0) pW = pC;

	float bC = texture(v,st).r;

	vec2 pixelwidth2 = 1*1/pow(pixelsize(st),vec2(2,2))*420*420;
	float Beta = 2*(pixelwidth2.x+pixelwidth2.y) * (1+1/(2*(pixelwidth2.x+pixelwidth2.y)));


    fc = ((pW + pE)*pixelwidth2.x + (pS + pN)*pixelwidth2.y - (-bC)) / Beta;
)");

	ShaderProgram *jacobi = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(divergence->getCode(), GL_FRAGMENT_SHADER)
			.link();

	for (int i = 0; i < 5; ++i) {

		jacobi->bind();
		p->setCanvasUniforms(jacobi);
		p->apply(jacobi, v_scratch);
		v_scratch->swap(v);
	}
}