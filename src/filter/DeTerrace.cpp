//
// Created by kuhlwein on 9/21/20.
//

#include "Project.h"
#include "DeTerrace.h"
#include "BlurMenu.h"

DeTerraceMenu::DeTerraceMenu() : FilterModal("Deterrace") {

}

void DeTerraceMenu::update_self(Project *p) {

}

std::shared_ptr<BackupFilter> DeTerraceMenu::makeFilter(Project *p) {
	return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_terrain();}, std::move(std::make_unique<DeTerrace>(p, p->get_terrain())));
}

DeTerrace::DeTerrace(Project *p, Texture *target) {
	this->p = p;
	this->target = target;

	//ShaderProgram* copyProgram = ShaderProgram::builder()
//			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
//			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
//			.link();
//	p->apply(copyProgram, p->get_scratch1(), {{target, "to_be_copied"}});
}

std::pair<bool, float> DeTerrace::step(Project *p) {
	//if (i>1000) return {true,1.0f};
	//i++;

	//Blur* blur = new Blur(p, 0.3, target);erg
	//while (!blur->step(p).first);


	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.create(R"(
	void find_contour_edges(vec2 p, vec2 dp, vec2 resolution, out vec3 p1, out vec3 p2) {
		int N = 100;

		p1 = vec3(0,0,texture2D(img, st).r);
		for (int i=1; i<N; i++) {
			float h = texture2D(img, offset(st, i*dp,resolution)).r;
			if(abs(h-p1.z) > 1e-6 && p1.xy==vec2(0)) {
				p1=vec3((i-0.5)*dp.x,(i-0.5)*dp.y,max(h,p1.z));
			}
		}
		for (int i=1; i<N; i++) {
			float ii = (N+pow(i,1.5));
			float h = texture2D(img, offset(st, ii*dp,resolution)).r;
			if(abs(h-p1.z) > 1e-6 && p1.xy==vec2(0)) {
				p1=vec3(ii*dp.x,ii*dp.y,max(h,p1.z));
			}
		}
		if(p1.xy==vec2(0)) p1.xy=(N+pow(N,1.5))*dp;

		p2 = vec3(0,0,p1.z);
		for (int i=1; i<N; i++) {
			float h = texture2D(img, offset(st, i*dp+p1.xy,resolution)).r;
			if(abs(h-p2.z) > 1e-6 && p2.xy==vec2(0)) {
				p2=vec3((i-0.5)*dp.x+p1.x,(i-0.5)*dp.y+p1.y,max(h,p2.z));
			}
		}
		for (int i=1; i<N; i++) {
			float ii = (N+pow(i,1.5));
			float h = texture2D(img, offset(st, ii*dp,resolution)).r;
			if(abs(h-p2.z) > 1e-6 && p2.xy==vec2(0)) {
				p2=vec3(ii*dp.x+p1.x,ii*dp.y+p1.y,max(h,p2.z));
			}
		}
		if(p2.xy==vec2(0)) p2.xy=(N+pow(N,1.5))*dp;

		if(abs(p1.z-p2.z)<1e-6) {
			p2.xy = (p1.xy+p2.xy)*0.5;
			p2.z = p2.z + p1.z-texture2D(img, st).r;
		}
	}

)",R"(
	vec2 resolution = textureSize(img,0);

	vec3 p1;
	vec3 p2;
	vec3 p[16];
	int counter = 0;
	for(int dx=-1; dx<=1; dx++) for(int dy=-1; dy<=1; dy++) {
		if(dx==0 && dy==0) continue;
		find_contour_edges(st,vec2(dx,dy),resolution,p1,p2);
		p[counter]=p1; p[counter+1]=p2;
		counter+=2;
	}

	float epsilon = 1e-6;

	float A[19*19];
	for (int x=0; x<19; x++) for (int y=0; y<19; y++) {
		float val = 0;
		if (x<16 && y<16) val=(pow(p[x].x-p[y].x,2)+pow(p[x].y-p[y].y,2))*log(pow(p[x].x-p[y].x,2)+pow(p[x].y-p[y].y,2)+epsilon);
		if (x==16 || y==16) val=1.0;
		if (x==17) val=p[y].x;
		if (y==17) val=p[x].x;
		if (x==18) val=p[y].y;
		if (y==18) val=p[x].y;
		if (x==y) val=0;
		if (y>=16 && x>=16) val=0;
		A[x*19+y] = val;
	}

	//invert A
	const int N = 19;
	int P[N+1];
	for (int i=0; i<=N; i++) P[i] = i;

    for (int i=0; i<N; i++) {
        float maxA = 0.0;
        int imax = i;

        for (int k=i; k<N; k++) {
            if (abs(A[k*19+i]) > maxA) {
                maxA = abs(A[k*19+i]);
                imax = k;
            }
		}

        if (imax != i) {
            int j = P[i];
            P[i] = P[imax];
            P[imax] = j;

			for(int k=0; k<N; k++) {
				float tmp = A[i*19+k];
            	A[i*19+k] = A[imax*19+k];
            	A[imax*19+k] = tmp;
			}

            P[N]++;
        }

        for (int j=i+1; j<N; j++) {
            A[j*19+i] /= A[i*19+i];

            for (int k=i+1; k<N; k++)
                A[j*19+k] -= A[j*19+i]*A[i*19+k];
        }
    }

	float b[19];
	for(int i=0; i<N; i++) {
		b[i]=0;
		if(i<16) b[i] = p[i].z;
	}

	float x[19];
	for (int i=0; i<N; i++) {
        x[i] = b[P[i]];

        for (int k = 0; k < i; k++)
            x[i] -= A[i*19+k] * x[k];
    }

    for (int i=N-1; i>=0; i--) {
        for (int k=i+1; k<N; k++)
            x[i] -= A[i*19+k] * x[k];

        x[i] /= A[i*19+i];
    }

	float val = x[16];
	for (int i=0; i<16; i++) val+= x[i]*(pow(p[i].x,2)+pow(p[i].y,2))*log(pow(p[i].x,2)+pow(p[i].y,2)+epsilon);

	float DD = 0;
	for (int i=0; i<16; i++) DD+= abs(p[i].x)+abs(p[i].y);

	fc = max(val,texture2D(img, st).r);
	fc = val;
)");

	ShaderProgram* Program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->setCanvasUniforms(Program);
	p->apply(Program, p->get_scratch2());
	p->get_scratch2()->swap(target);




	return {true,i};
}
