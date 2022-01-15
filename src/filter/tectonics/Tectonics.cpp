//
// Created by kuhlwein on 6/29/21.
//

#include <iostream>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Project.h"
#include "Tectonics.h"



void TectonicsMenu::update_self(Project *p) {

}

std::shared_ptr<BackupFilter> TectonicsMenu::makeFilter(Project *p) {
	return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_terrain();}, std::move(std::make_unique<Tectonics>(p)));
}

TectonicsMenu::TectonicsMenu() : FilterModal("Tectonics") {

}

Tectonics::~Tectonics() {

}

Tectonics::Tectonics(Project *p) {
    plates.push_back(new Plate(p->getWidth(),p->getHeight()));
    plates.push_back(new Plate(p->getWidth(),p->getHeight()));
    plates[0]->updateRotationBy((float)0.01, glm::vec3(0, 0, 1));
    //plates[0]->updateRotationBy((float)0.01, glm::vec3(1, 0, 0));
    plates[1]->updateRotationBy(-(float)0.01, glm::vec3(0, 0, 1));

    a = new Texture(p->getWidth()*1,p->getWidth()*1,GL_RGBA32F,"");
    b = new Texture(p->getWidth()*1,p->getWidth()*1,GL_RGBA32F,"");
    c = new Texture(p->getWidth(),p->getWidth(),GL_RGBA32F,"");



	Shader* shader = Shader::builder()
			.include(fragmentColor)
			.create("",R"(
    vec4 a = texture(img, st);
    float h = a.x>0 ? a.x : -1;
	fc =  st.x>0.5 ? vec4(h,1.0,0,0) : vec4(0,-1,0,0);
)");
	setzero = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	setzero->bind();
	p->setCanvasUniforms(setzero);
	p->apply(setzero,plates[0]->getTexture());

	shader = Shader::builder()
            .include(fragmentColor)
            .create("",R"(
    vec4 a = texture(img, st);
    float h = a.x>0 ? a.x : -2;
	fc =  st.x<0.5 ? vec4(h,0.5,0,0) : vec4(0,-1,0,0);
)");
    setzero = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();

    setzero->bind();
    p->setCanvasUniforms(setzero);
    p->apply(setzero,plates[1]->getTexture());


    tectonicSamplingShader = Shader::builder()
            .create(R"(
uniform sampler2D plate;

uniform mat3 rotationmatrix;
uniform mat3 inverserotationmatrix;
uniform float plateIndex;
uniform vec3 plateVelocity;

vec4 plateTexture(sampler2D img, vec2 st) {
    vec2 spheric = tex_to_spheric(st);
    vec3 cartesian = spheric_to_cartesian(spheric);

    cartesian = rotationmatrix*cartesian;

    spheric = cartesian_to_spheric(cartesian);
    vec2 st_ = spheric_to_tex(spheric);

	return texture(img,st_);
}

vec4 inverseplateTexture(sampler2D img, vec2 st) {
    vec2 spheric = tex_to_spheric(st);
    vec3 cartesian = spheric_to_cartesian(spheric);

    cartesian = inverserotationmatrix*cartesian;

    spheric = cartesian_to_spheric(cartesian);
    vec2 st_ = spheric_to_tex(spheric);

	return texture(img,st_);
}
)","");

    /*
     * SET SHADERS
     */

    shader = Shader::builder()
            .include(fragmentColor)
            .include(cornerCoords)
            .include(tectonicSamplingShader)
            .create("",R"(
    //plate id, height, age, collision
	fc = vec4(0.0,-1,-1,0);
)");
    setzero = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();

    shader = Shader::builder()
            .include(fragmentColor)
            .include(cornerCoords)
            .include(tectonicSamplingShader)
            .include(directional)
            .create(R"(
    uniform sampler2D foldtex;
)",R"(
    fc = texture(foldtex,st);
	float index = plateIndex;

    vec2 spheric_coord = tex_to_spheric(st);
	vec3 cartesian_coord = spheric_to_cartesian(spheric_coord);
    vec3 v = cross(cartesian_coord,plateVelocity);
    vec2 spheric_v = cartesian_to_v(v,spheric_coord);
    float theta = atan(spheric_v.y,spheric_v.x);

    vec4 p = plateTexture(plate,st);
    float plateHeight = p.x;
    float plateAge = p.y;

    float previousAge = fc.z;
    float previousHeight = fc.y;

    //mark collision
    bool overlap = plateAge>=0 && previousAge>=0;
    if (overlap) fc.a = 1;

    bool oceanOnLand = plateHeight <= 0 && previousHeight>0;
    //if (plateAge >= 0 && !oceanOnLand) fc = vec4(index,plateHeight,plateAge,fc.a);

    if (plateAge >= 0 && !overlap) fc = vec4(index,plateHeight,plateAge,fc.a);

    bool landOnOcean = plateHeight>0 && previousHeight<=0;
    bool plateYounger = plateAge<previousAge;
    bool plateOnTop = (plateYounger && plateHeight<=0 && previousHeight<=0) || (!plateYounger && plateHeight>0 && previousHeight>0) || landOnOcean;
    if (overlap && plateOnTop) fc = vec4(index,plateHeight,plateAge,fc.a);
)");
    foldShader = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();
}

void Tectonics::run() {
    for (int i=0; i<90; i++) {
        for (Plate* p : plates) {
            p->rotate();
        }



        dispatchGPU([this](Project* p) {

            //Sets b to [plate index, height, age, and collision bool]
            fold(setzero, foldShader, a, b, p);

            //Create new crust with age=0
            oceanSpreading(p);

            /*
             * Collision
             */
            collision(p);


            /*
            // Render
            */

            auto shader = Shader::builder()
                    .include(fragmentBase)
                    .include(cornerCoords)
                    .create(R"(
    uniform sampler2D foldtex;
)",R"(
	fc = texture(foldtex,st).y;
    //if(fc==0) fc = texture(foldtex,st).x;
    //fc = texture(foldtex,st).z;

    if (texture(foldtex,st).a>0) fc = texture(foldtex,st).a/0.002;

    //fc = texture(foldtex,st).z;
)");

            ShaderProgram* foldShader = ShaderProgram::builder()
                    .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
                    .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
                    .link();
            foldShader->bind();

            p->setCanvasUniforms(foldShader);

            p->apply(foldShader,p->get_terrain(),{{c,"foldtex"}});

            /*
             * Back to plates
             */

            auto shader3 = Shader::builder()
                    .include(fragmentColor)
                    .include(cornerCoords)
                    .include(distance)
                    .include(directional)
                    .include(tectonicSamplingShader)
                    .create(R"(
    uniform sampler2D foldtex;
    uniform sampler2D oldPlate;
)",R"(
    vec4 a = inverseplateTexture(foldtex,st);

    fc = texture(oldPlate,st);
    //fc.y = fc.y + 1; //Increment age


    // delete stuff? (index does not match, if deleting land, must not be replaced by land)
    if(a.x != plateIndex && !(a.y<=0 && fc.x>0)) fc = vec4(0,-1,0,0);

    //create new, age of old must be negative (nonexisting),
    if(fc.y<0 && a.x == plateIndex) fc = vec4(-plateIndex,1,0,0);

)");

            ShaderProgram* unfoldshader = ShaderProgram::builder()
                    .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
                    .addShader(shader3->getCode(), GL_FRAGMENT_SHADER)
                    .link();

            float index = 1; //Index 0 is no plate
            for (Plate* plate : plates) {
                c->swap(plate->getTexture());

                unfoldshader->bind();
                p->setCanvasUniforms(unfoldshader);
                plate->setPlateUniforms(unfoldshader,index++);


                p->apply(unfoldshader,plate->getTexture(),{{b,"foldtex"},{c,"oldPlate"}});
            }


        });
    }

    setProgress({true,1});
}

//Apply operations to t1
void Tectonics::fold(ShaderProgram *zero, ShaderProgram *operation, Texture* t1, Texture* t2, Project *p) {
    zero->bind();
    p->apply(zero,t1);
    t1->swap(t2);

    float index = 1; //Index 0 is no plate
    for (Plate* plate : plates) {

        operation->bind();
        plate->setPlateUniforms(operation,index++);
        p->setCanvasUniforms(operation);

        p->apply(operation,t1,{{t2,"foldtex"},{plate->getTexture(),"plate"}});
        t1->swap(t2);
    }
}

void Tectonics::oceanSpreading(Project* p) {
    //set distance for all to zero, set plate for all to plate
    //Check neighbours distance, if shorter than current or currently not part of plate, update current plate and distance
    //ONLY if direction of plate movement matches!!

    //When finished, do backwards from borders between plates


    auto shader2 = Shader::builder()
            .include(fragmentColor)
            .include(cornerCoords)
            .include(distance)
            .include(directional)
            .create(R"(
    uniform sampler2D foldtex;
    uniform float radius;
)",R"(
	fc = texture(foldtex,st);

    vec2 resolution = textureSize(foldtex,0);
    float phi = tex_to_spheric(st).y;
    float factor = 1/cos(abs(phi));
    int N = 16;


    for (int i=0; i<N; i++) {
        vec2 neighbour = offset(st, vec2(cos(2*3.14159*i/N)*radius*factor,sin(2*3.14159*i/N)*radius),resolution);
        vec4 a = texture(foldtex, neighbour);

        //float theta = atan(sin(2*3.14159*i/N),cos(2*3.14159*i/N));

        //vec3 ndiff = spheric_to_cartesian(tex_to_spheric(neighbour)) - spheric_to_cartesian(tex_to_spheric(st));
        //vec2 v = cartesian_to_v(ndiff,tex_to_spheric(st));
        //float ntheta = atan(v.y,v.x);

        float nz = a.z+geodistance(neighbour,st,resolution);

        //&& abs(ntheta-a.y)<2*M_PI/N

        if((nz < fc.z || fc.x==0) && a.x != 0) fc = vec4(a.x, -1.1, nz, 0);
    }
)");

    ShaderProgram* foldShader2 = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader2->getCode(), GL_FRAGMENT_SHADER)
            .link();

    int N = 5;
    for (int i=0; i<N; i++) {
        foldShader2->bind();
        p->setCanvasUniforms(foldShader2);
        int id = glGetUniformLocation(foldShader2->getId(),"radius");
        glUniform1f(id,pow(2,i));
        p->apply(foldShader2,a,{{b,"foldtex"}});
        a->swap(b);
    }
    for (int i=N; i>0; i--) {
        foldShader2->bind();
        p->setCanvasUniforms(foldShader2);
        int id = glGetUniformLocation(foldShader2->getId(),"radius");
        glUniform1f(id,pow(2,i));
        p->apply(foldShader2,a,{{b,"foldtex"}});
        a->swap(b);
    }
}

void Tectonics::collision(Project* p) {
    Shader* shader = Shader::builder()
            .include(fragmentColor)
            .create("",R"(
	fc =  vec4(0,0,0,0);
)");
    ShaderProgram* setzero = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();

    shader = Shader::builder()
            .include(fragmentColor)
            .include(cornerCoords)
            .include(tectonicSamplingShader)
            .include(directional)
            .create(R"(
    uniform sampler2D foldtex;
    uniform sampler2D plateIndices;
)",R"(
    fc = texture(foldtex,st);
	float index = plateIndex;



    vec4 p = texture(plateIndices,st);



    if (p.x == plateIndex) fc = vec4(plateVelocity.x,plateVelocity.y,plateVelocity.z,0);
)");
    ShaderProgram* setrotation = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();

    setzero->bind();
    p->apply(setzero,a);
    a->swap(c);
    float index = 1; //Index 0 is no plate
    for (Plate* plate : plates) {
        setrotation->bind();
        plate->setPlateUniforms(setrotation,index++);
        p->setCanvasUniforms(setrotation);

        p->apply(setrotation,a,{{c,"foldtex"},{b,"plateIndices"}});
        a->swap(c);
    }


    shader = Shader::builder()
            .include(fragmentColor)
            .include(cornerCoords)
            .create(R"(
    uniform sampler2D foldtex;
    uniform sampler2D plateIndices;
)",R"(
    float index = texture(plateIndices,st).x;

    vec3 v = texture(foldtex,st).xyz;
    vec3 otherv;

    vec3 p = vec3(0);
    vec3 otherp = vec3(0);
    int count = 0;
    int othercount = 0;
    vec2 res = textureSize(foldtex,0);


    int N = 16;
    float phi = tex_to_spheric(st).y;
    float factor = 1/cos(abs(phi));
    //for (int xx=0; xx<N; xx++) {
        //int i = int(cos(2*3.14159*xx/N)*5*factor);
        //int j = int(sin(2*3.14159*xx/N)*5);


    for(int i=-1; i<=1; i++) for(int j=-1; j<=1; j++) {
        vec2 o = offset(st,vec2(i,j),res);

        float n_index = texture(plateIndices,o).x;
        vec3 n_v = texture(foldtex,o).xyz;
        vec2 spheric_coord = tex_to_spheric(o);
	    vec3 n_p = spheric_to_cartesian(spheric_coord);

        if(n_index==index) {
            p += n_p;
            count++;
        } else {
            otherp += n_p;
            othercount++;
            otherv = n_v;
        }
    }

    p = p/count;
    otherp = otherp/othercount;

    v = cross(v,p);
    otherv = cross(otherv,p);

    vec3 d = othercount>0 ? v-otherv : vec3(0);
    vec3 deltap = p-otherp;

    float magnitude = dot(d, normalize(deltap));
    if (texture(plateIndices,st).a<=0) magnitude = 0;
    if (othercount==0) magnitude = 0;

    fc = texture(plateIndices,st);
    fc.a = magnitude;
)");
    ShaderProgram* collisionspeed = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();
    collisionspeed->bind();
    p->setCanvasUniforms(collisionspeed);
    p->apply(collisionspeed,a,{{c,"foldtex"},{b,"plateIndices"}});
    a->swap(c);
}