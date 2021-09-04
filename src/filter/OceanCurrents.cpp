//
// Created by kuhlwein on 4/12/21.
//

#include <iostream>
#include "Project.h"
#include "OceanCurrents.h"

OceanCurrentsMenu::OceanCurrentsMenu() : FilterModal("OceanCurrents") {

}

void OceanCurrentsMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	ImGui::DragFloat("Pressure", &pressure, 0.01f, 0.1f, 100.0f, "%.2f", 1.0f);
}

std::shared_ptr<BackupFilter> OceanCurrentsMenu::makeFilter(Project *p) {
	return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_terrain();}, std::move(std::make_unique<OceanCurrents>(p,pressure)));
}

OceanCurrents::OceanCurrents(Project *p, float pressure_) {
	 /*
	 * WIND should never slow down!
	 * progressive size for all textures
	 * Possibly other approach to relaxation??
	 */

	/*
	 * Wind stuff
	 * fast current is almost 10km/h (2.7 m/s)
	 * only 6.5 km/h on average (1.8 m/s)
	 * eastern currents slow at 0.5km/h (0.15m/s)
	 * "a wind that blows for 3-4 days will result in current about 2% of the wind speed?"
	 * typical wind speed is 18-45km/h to (5-12 m/s)
	 *
	 * wind stress goes as propto v^2
	 */


    v = new Texture(p->getWidth(),p->getHeight(),GL_RG32F,"v",GL_LINEAR);
    v_scratch = new Texture(p->getWidth(),p->getHeight(),GL_RG32F,"v",GL_LINEAR);
    pressure = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"pressure",GL_LINEAR);
    scratch = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"scratch",GL_LINEAR);

    pressurefactor = pressure_;


	jacobi_iterations = 5000;



	divw = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"divw",GL_LINEAR);
	p->add_texture(v);
	p->add_texture(v_scratch);
	p->add_texture(pressure);
	p->add_texture(scratch);
	p->add_texture(divw);

	divw_showcase = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"divw_showcase",GL_NEAREST);
	p->add_texture(divw_showcase);

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

	//p->apply(setzero,pressure);


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

void OceanCurrents::resize(int width, int height, Project* p) {
    Shader* shader = Shader::builder()
            .create(R"(
in vec2 st;
uniform sampler2D img;
out float fc;
)",R"(
	fc = texture(img,st).r;
)");
    ShaderProgram* setzero2 = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();


    Texture* v = new Texture(width,height,GL_RG32F,"v",GL_LINEAR);
    Texture* v_scratch = new Texture(width,height,GL_RG32F,"v_scratch",GL_LINEAR);
    Texture* pressure = new Texture(width,height,GL_R32F,"pressure",GL_LINEAR);
    Texture* scratch = new Texture(width,height,GL_R32F,"scratch",GL_LINEAR);

    Texture* terrain = new Texture(width,height,GL_R32F,"img");

    setzero2->bind();
    p->setCanvasUniforms(setzero2);
    p->apply(setzero2,terrain);

    //p->remove_texture(p->get_terrain());
    //p->add_texture(terrain);
    p->get_terrain()->swap(terrain);

    p->remove_texture(this->pressure);
    p->remove_texture(this->scratch);
    this->pressure = pressure;
    this->scratch = scratch;
    p->add_texture(this->pressure);
    p->add_texture(this->scratch);

    p->remove_texture(this->v);
    p->remove_texture(this->v_scratch);
    this->v = v;
    this->v_scratch = v_scratch;
    p->add_texture(this->v);
    p->add_texture(this->v_scratch);

}

void OceanCurrents::run() {



	for(int j=0; j<10000; j++) {
		dispatchGPU([this](Project* p) {

			advect(p);

			diffusion(p);

			divergence(p,divw);
		});


			jacobi();
		dispatchGPU([j,this](Project* p) {
			subDiv(p);

			divergence(p,divw_showcase);




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

            if(j==2) resize(600,200,p);

		});
	}

	while(true);

	dispatchGPU([this](Project* p) {
		p->remove_texture(v);
		p->remove_texture(v_scratch);
		//p->remove_texture(pressure);
		//p->remove_texture(scratch);
		//TODO remove pressure
	});

	setProgress({true,1.0});
}

OceanCurrents::~OceanCurrents() {
	std::cout << v->getWidth() << " width\n";
	delete v;
	delete v_scratch;
	//delete pressure;
	//delete scratch;
	//TODO delete pressure
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

	vec3 parallel_comp = normalize(cross(vec3(0,0,1),cartesian_coord));

	return vec2(dot(v,parallel_comp),dot(v,y_comp));
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

	float ratio = 0.0001;

	float timestep = 24; //hour

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



	vec3 omega = vec3(0,0,1.0/24.0); // hour^-1
	vec3 coriolis_a = -2*cross(omega,v_to_cartesian(newV, spheric_coord));
	newV = newV + cartesian_to_v(coriolis_a,spheric_coord)*timestep/5000 * 0.0;




	fc = Dissipation * newV;




	//if (abs(st.y-0.5)<0.05 && st.x>0.75) fc = fc*0.3 + 0.7*vec2(-1, 0);

	//if (abs(st.x-0.5)<0.01 && st.y>0.5) fc = fc*0.3 + 0.7*vec2(0, 1);

	float phi = 2*(st.y-0.5)*3.14159;
	vec2 wind = 10*vec2(-cos(phi*3/2),sin(phi*3/2));
	if (abs(phi*3/2)>3.14159) wind.x = -wind.x;
	if (abs(phi)>3.14159*1/3 && abs(phi)<3.1459*2/3) wind.y = -wind.y;
	if (phi<0) wind.y=-wind.y;

	vec2 stress = 1.0+0.0001*pow(abs(wind-fc),vec2(2));
	fc = fc + wind*(1-pow(stress,-vec2(1.0)/24.0*2))  - fc*(1-pow(vec2(0.4),vec2(1.0/24)));
	//fc = fc*0.99985 + 0.00015*wind;
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


void OceanCurrents::divergence(Project *p, Texture* divw) {
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
uniform float pressurefactor = 100;

vec2 get_velocity(vec2 st, vec2 o) {
	vec2 resolution = textureSize(pressure,0);
	vec2 st_o = offset(st,o,resolution);

	vec2 d = pixelsize(st);
	vec2 d_o = pixelsize(st_o);

	vec2 sph = tex_to_spheric(st);
	vec2 sph_o = tex_to_spheric(st_o);

	vec2 v = texture(v,st_o).xy;

	//TODO better implementation
	if (abs(abs(sph.x-sph_o.x)-3.14159)<0.1) v = -v;
	//v = cartesian_to_v(v_to_cartesian(v, sph_o),sph);

	return v * (d_o.x*d_o.y)*pressurefactor; //Multiply pixel area
}

)", R"(
vec2 resolution = textureSize(pressure,0);

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
	int id = glGetUniformLocation(divProgram->getId(),"pressurefactor");
	glUniform1f(id,pressurefactor);
	p->apply(divProgram, divw);

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
	vec2 resolution = textureSize(pressure,0);



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
	float oC = texture(img,st).r;

    // Use center pressure for solid cells:
    if (oN > 0) pN = pC;
    if (oS > 0) pS = pC;
    if (oE > 0) pE = pC;
    if (oW > 0) pW = pC;


	float bC = texture(divw,st).r;

	vec2 pixelwidth2 = pow(pixelsize(st)/420,vec2(2,2));
	float Beta = 2*(1/pixelwidth2.x+1/pixelwidth2.y);
    fc = ((pW + pE)/pixelwidth2.x + (pS + pN)/pixelwidth2.y - bC) / Beta;
	if(oC > 0) fc = 0;
)");

		jacobiProgram = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(divergence->getCode(), GL_FRAGMENT_SHADER)
				.link();

		//Zero pressure
		p->remove_texture(pressure);
		p->remove_texture(scratch);
		p->add_texture(pressure);
		p->add_texture(scratch);

		setzero->bind();
		p->setCanvasUniforms(setzero);
		p->apply(setzero, pressure);
	});

	dispatchGPU([this](Project* p) {
        for (int i = 0; i < jacobi_iterations; i++) {

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
uniform float pressurefactor = 100;

)", R"(
vec2 resolution = textureSize(pressure,0);



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

	float oNE = texture(img,offset(st,vec2(1,1),resolution)).r;
	float oNW = texture(img,offset(st,vec2(-1,1),resolution)).r;
	float oSE = texture(img,offset(st,vec2(1,-1),resolution)).r;
	float oSW = texture(img,offset(st,vec2(-1,-1),resolution)).r;

	vec2[] offsets = vec2[8](vec2(1,0),vec2(1,1),vec2(0,1),vec2(-1,1),vec2(-1,0),vec2(-1,-1),vec2(0,-1),vec2(1,-1));

    // Use center pressure for solid cells:
    vec2 obstV = vec2(0);
    vec2 vMask = vec2(1);

    if (oN > 0) { pN = pC; obstV.y = 0; vMask.y = 0; }
    if (oS > 0) { pS = pC; obstV.y = 0; vMask.y = 0; }
    if (oE > 0) { pE = pC; obstV.x = 0; vMask.x = 0; }
    if (oW > 0) { pW = pC; obstV.x = 0; vMask.x = 0; }


	vec2 oldV = texture(v,st).xy;

	vec2 pixelwidth = pixelsize(st)/420;
	fc = oldV - 0.5*vec2(pE-pW,pN-pS)/pixelwidth / pixelsize(st).x/pixelsize(st).y /pressurefactor; //TODO store variable for pixel area

	// Enforce the free-slip boundary condition:

	//fc = vMask*fc + obstV;



	float o_array[8];
	for(int i=0; i<8; i++) {
		o_array[i] = texture(img,offset(st,offsets[i],resolution)).r;
	}

	float lower = mod(floor((atan(fc.y,fc.x)/M_PI+1)/2*8+4),8);
	float upper = mod(ceil((atan(fc.y,fc.x)/M_PI+1)/2*8+4),8);
	float theta = mod((atan(fc.y,fc.x)/M_PI+1)/2*8+4,8);
	bool isBorder = o_array[int(lower)]>0 || o_array[int(upper)]>0;

	vec2 nfc = fc;
	float difference = 2*M_PI;
	for(int i=0; i<8; i++) {
		vec2 o = offsets[i];
		float thetai = mod((atan(o.y,o.x)/M_PI+1)/2*8+4,8);
		float angleToVel = min((2 * M_PI) - abs(thetai - theta), abs(thetai - theta));
		if (angleToVel<difference && o_array[int(round(thetai))]<=0) {
			difference = angleToVel;
			nfc = o/length(o)*length(nfc);
		}
	}


	if (isBorder) fc = nfc;



)");
	ShaderProgram *subdiv = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(subgradient->getCode(), GL_FRAGMENT_SHADER)
			.link();

	subdiv->bind();
	p->setCanvasUniforms(subdiv);
	int id = glGetUniformLocation(subdiv->getId(),"pressurefactor");
	glUniform1f(id,pressurefactor);
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

	for (int i = 0; i < 50; ++i) {

		jacobi->bind();
		p->setCanvasUniforms(jacobi);
		p->apply(jacobi, v_scratch);
		v_scratch->swap(v);
	}
}

