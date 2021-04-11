//
// Created by kuhlwein on 9/21/20.
//

#include <algorithm>
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

	/*
	 * scratch 2 -> pid
	 */
	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.include(pidShader)
			.create("",R"(
	fc =  uintBitsToFloat(coordToPid(st,img));
)");
	init1 = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	/*
	 * scratch 2 -> if neightbour: neighbour else self
	 */

	shader = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(pidShader)
			.create(R"(
uniform vec2 primary_offset;
)",R"(
	float epsilon = 1e-5;
	vec2 resolution = textureSize(img,0);
	vec2 o = primary_offset;
	float a;
	if (abs(texture(img,offset(st,o,resolution)).r-texture(img,st).r)<epsilon) {
		a = texture(scratch2,st).r;
	} else {
		a = texture(scratch2,offset(st,o,resolution)).r;
	}
	fc = a;
)");
	init2 = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	/*
	 * if same height:
	 * 		check distance, but only if not self.
	 * if different height (Dont do this, do a pre-scan)
	 * 		check distance
	 */

	Shader* fragment_set = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(distance)
			.include(pidShader)
			.create(R"(
uniform vec2 primary_offset;
uniform vec2 secondary_offset;
)",R"(
	vec2 resolution = textureSize(img,0);

	float min_d = -1.0;
	uint pid = floatBitsToUint(texture(scratch2,st).r);

	if (coordToPid(st,scratch2) != pid) {
		min_d = geodistance(st,pidToCoord(pid,img),resolution);
	}

	float d;
	uint newPid;
	vec2 o;

	o = secondary_offset;
	newPid = floatBitsToUint(texture(scratch2,offset(st,o,resolution)).r);
	d = texture(img,pidToCoord(newPid,img)).r;

	if (d != texture(img,st).r && floatBitsToUint(texture(scratch2,offset(st,o,resolution)).r)!=coordToPid(offset(st,o,resolution),img) && (min_d<0 || geodistance(st,pidToCoord(newPid,img),resolution)<min_d)) {
		min_d = geodistance(st,pidToCoord(newPid,img),resolution);
		pid = newPid;
	}

	o = primary_offset;
	newPid = floatBitsToUint(texture(scratch2,offset(st,o,resolution)).r);
	d = texture(img,pidToCoord(newPid,img)).r;

	if (d != texture(img,st).r && floatBitsToUint(texture(scratch2,offset(st,o,resolution)).r)!=coordToPid(offset(st,o,resolution),img) && (min_d<0 || geodistance(st,pidToCoord(newPid,img),resolution)<min_d)) {
		min_d = geodistance(st,pidToCoord(newPid,img),resolution);
		pid = newPid;
	}

	fc = uintBitsToFloat(pid);
)");
	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();
}

int intFromFloat(float* f, int index) {
	int* i = (int*) f;
	return i[index];
}

void DeTerrace::run() {
	std::unique_ptr<float[]> rightdown = get(glm::vec2(1,1),glm::vec2(1,0));
	std::unique_ptr<float[]> downright = get(glm::vec2(0,1),glm::vec2(1,1));
	std::unique_ptr<float[]> downleft = get(glm::vec2(-1,1),glm::vec2(0,1));
	std::unique_ptr<float[]> leftdown = get(glm::vec2(-1,0),glm::vec2(-1,1));
	std::unique_ptr<float[]> leftup = get(glm::vec2(-1,-1),glm::vec2(-1,0));
	std::unique_ptr<float[]> upleft = get(glm::vec2(0,-1),glm::vec2(-1,-1));
	std::unique_ptr<float[]> upright = get(glm::vec2(1,-1),glm::vec2(0,-1));
	std::unique_ptr<float[]> rightup = get(glm::vec2(1,0),glm::vec2(1,-1));

	std::unique_ptr<float[]> height;
	dispatchGPU([this,&height](Project* p) {
		height = p->get_terrain()->downloadDataRAW();
	});

	data = std::make_unique<float[]>(p->getHeight()*p->getWidth());

	std::function<void(std::pair<int,int>)> f = [this,&rightdown,&downright,&downleft,&leftdown,&leftup,&upleft, &upright, &rightup, &height](std::pair<int,int> a) {

	auto pidToCoord = [this](int pid) {
		uint a = pid - p->getWidth()*(pid/p->getWidth());
		uint b = (pid - a)/p->getWidth();
		return glm::ivec2(a,b);
	};

	auto tovec = [this,&height,&pidToCoord](int i, int id, float minheight) {
		auto xy = pidToCoord(i)-pidToCoord(id);
		if (p->getCoords()[2]<-M_PI+1e-4 && p->getCoords()[3]>M_PI-1e-3) {
			xy.x = abs(xy.x)>p->getWidth()/2 ? -xy.x/abs(xy.x)*(p->getWidth() - abs(xy.x)) : xy.x;
		}

		float ycoord = float(pidToCoord(i).y)/p->getHeight();
		float factor = (ycoord*(p->getCoords()[1]-p->getCoords()[0])+p->getCoords()[0]);
		return glm::vec3(xy.x*std::cos(factor),xy.y,std::max(height[id],minheight));
	};

	std::vector<glm::vec3> p;
	std::vector<glm::vec3> virtualPoints;
	for (int i=a.first; i<a.second; i++) {
		p.clear();
		virtualPoints.clear();
		for (auto d : {rightdown.get(), leftup.get(), downright.get(), upleft.get(), downleft.get(), upright.get(), leftdown.get(), rightup.get()}) {
			int lu = intFromFloat(d, i);
			int lu2 = intFromFloat(d, lu);
			if(i!=lu) p.push_back(tovec(i, lu, height[i]));
			if(i!=lu2) p.push_back(tovec(i, lu2, height[lu]));
		}

		float h = height[i];
		float stepSize = 0;
		for (auto point : p) {
			if (stepSize==0 || (std::abs(point.z-h)<stepSize && std::abs(point.z-h)>0)) {
				stepSize = std::abs(point.z-h);
			}
		}

		auto vec2d = [](glm::vec3 v, int sign) {
			return glm::vec2(sign*sqrt(pow(v.x,2)+pow(v.y,2)),v.z);
		};

		//Find curvature
		int curvature = 0;
		for (int j=0; j<p.size(); j+=4) {
			auto B = vec2d(p[j], 1);
			auto A = vec2d(p[j + 1], 1);
			auto C = vec2d(p[j + 2], -1);
			auto D = vec2d(p[j + 3], -1);

			if (A.y == B.y) A += A.y>h ? stepSize : -stepSize;
			curvature += A.y>B.y ? 1 : -1;
			if (C.y == D.y) D += D.y>h ? stepSize : -stepSize;
			curvature += D.y>C.y ? 1 : -1;
		}

		//Make corrections
		for (int j=0; j<p.size(); j+=4) {
			auto B = vec2d(p[j], 1);
			auto A = vec2d(p[j + 1], 1);
			auto C = vec2d(p[j + 2], -1);
			auto D = vec2d(p[j + 3], -1);

			if (A.y == B.y) {
				if (A.y > h && curvature > 0) {
					p[j + 1].z += stepSize * std::abs(curvature) / 8 * 0.5;
				} else if (A.y <= h && curvature < 0) {
					p[j + 1].z -= stepSize * std::abs(curvature) / 8 * 0.5;
				}
			}
			if (C.y == D.y) {
				if (C.y > h && curvature > 0) {
					p[j + 3].z += stepSize * std::abs(curvature) / 8 * 0.5;
				} else if (C.y <= h && curvature < 0) {
					p[j + 3].z -= stepSize * std::abs(curvature) / 8 * 0.5;
				}
			}
		}

		//Remove self-references
		for (int i=p.size()-1; i>=0; i--) {
			if(p[i].x==0 && p[i].y==0) p.erase(p.cbegin()+i);
		}
		//Remove duplicates
		std::sort(std::begin(p),std::end(p),[](glm::vec3 const &a, glm::vec3 const &b) -> bool { return a.x!=b.x ? a.x<b.x : (a.y!=b.y ? a.y<b.y : (a.z<b.z));});
		auto newEnd = std::unique(std::begin(p),std::end(p),[](glm::vec3 a, glm::vec3 b){ return a.x==b.x && a.y==b.y;});
		p.resize(std::distance(p.begin(),newEnd));


		//START
		float epsilon = 1e-6;

		const int N = p.size()+3;
		float A[N * N];
		for (int x = 0; x < N; x++)
			for (int y = 0; y < N; y++) {
				float val = 0;
				if (x < N - 3 && y < N - 3)
					val = (pow(p[x].x - p[y].x, 2) + pow(p[x].y - p[y].y, 2)) *
						  log(pow(p[x].x - p[y].x, 2) + pow(p[x].y - p[y].y, 2) + epsilon);
				if (x == N - 3 || y == N - 3) val = 1.0;
				if (x == N - 2) val = p[y].x;
				if (y == N - 2) val = p[x].x;
				if (x == N - 1) val = p[y].y;
				if (y == N - 1) val = p[x].y;
				if (x == y) val = 0;
				if (y >= N - 3 && x >= N - 3) val = 0;
				A[x * N + y] = val;
			}

		//invert A

		int P[N + 1];
		for (int i = 0; i <= N; i++) P[i] = i;

		for (int i = 0; i < N; i++) {
			float maxA = 0.0;
			int imax = i;

			for (int k = i; k < N; k++) {
				if (abs(A[k * N + i]) > maxA) {
					maxA = abs(A[k * N + i]);
					imax = k;
				}
			}

			if (imax != i) {
				int j = P[i];
				P[i] = P[imax];
				P[imax] = j;

				for (int k = 0; k < N; k++) {
					float tmp = A[i * N + k];
					A[i * N + k] = A[imax * N + k];
					A[imax * N + k] = tmp;
				}

				P[N]++;
			}

			for (int j = i + 1; j < N; j++) {
				A[j * N + i] /= A[i * N + i];

				for (int k = i + 1; k < N; k++)
					A[j * N + k] -= A[j * N + i] * A[i * N + k];
			}
		}

		float b[N];
		for (int i = 0; i < N; i++) {
			b[i] = 0;
			if (i < N - 3) b[i] = p[i].z;
		}

		float x[N];
		for (int i = 0; i < N; i++) {
			x[i] = b[P[i]];

			for (int k = 0; k < i; k++)
				x[i] -= A[i * N + k] * x[k];
		}

		for (int i = N - 1; i >= 0; i--) {
			for (int k = i + 1; k < N; k++)
				x[i] -= A[i * N + k] * x[k];

			x[i] /= A[i * N + i];
		}

		float val = x[N - 3];
		for (int i = 0; i < N - 3; i++)
			val += x[i] * (pow(p[i].x, 2) + pow(p[i].y, 2)) * log(pow(p[i].x, 2) + pow(p[i].y, 2) + epsilon);

		float DD = 0;
		for (int i = 0; i < N - 3; i++) DD += abs(p[i].x) + abs(p[i].y);

		if (std::isnan(val)) {
			std::cout << i  << " " << val << "\n";
			for(auto pp : p) {std::cout << pp.x << " " << pp.y << " " << pp.z << ",\n";}
			std::cout << "\n";
		}

		if (isnanf(val)) val = h+stepSize/2;
		val = std::max(val,h);
		val = std::min(val,h+stepSize);
		if (h<0) {
			val = std::min(val,0-epsilon);
		} else {
			val = std::max(val,0.0f);
		}

		data[i] = val;
	}
	};

	std::vector<std::pair<int,int>> jobs;
	for (int i=0; i<p->getHeight(); i++) jobs.emplace_back(i*p->getWidth(),(i+1)*p->getWidth());
	threadpool(f,jobs,0.99);



	dispatchGPU([this](Project* p) {
		Shader* shader = Shader::builder()
				.include(fragmentBase)
				.create("",R"(


	fc = 1e21;
)");
		auto reset = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();
		reset->bind();
		p->setCanvasUniforms(reset);
		p->apply(reset, p->get_scratch1());
	});

	updateDistance(std::move(rightdown));
	updateDistance(std::move(downright));
	updateDistance(std::move(downleft));
	updateDistance(std::move(leftdown));
	updateDistance(std::move(leftup));
	updateDistance(std::move(upleft));
	updateDistance(std::move(upright));
	updateDistance(std::move(rightup));


	//dispatchGPU([this](Project* p) {
	//	p->get_scratch1()->swap(p->get_terrain());
	//});

	Texture* texture;
	dispatchGPU([this,&texture,&height](Project* p) {
		p->get_terrain()->uploadData(GL_RED, GL_FLOAT, data.get());
		texture = new Texture(p->getWidth(),p->getHeight(),GL_RED,"scratch3");
		texture->uploadData(GL_RED,GL_FLOAT,height.get());
		p->add_texture(texture);
	});

	//Terrain: new terrain
	//texture: old terrain
	//scratch1: distance
	//scratch2: new calculations.
	dispatchGPU([this](Project* p) {
		Shader* shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.include(distance)
				.include(pidShader)
				.create(R"(
uniform sampler2D scratch3;
uniform vec2 o;

float pseudogaussian(float r, float sigma) {
	return 1.0/(sigma*sqrt(2*M_PI))*exp(-0.5*pow(r,2)/pow(sigma,2));
}

float oldterrain = texture(scratch3,st).r;
float newterrain = texture(img,st).r;
float d = pow(texture(scratch1,st).r/5,1.5);
vec2 resolution = textureSize(img,0);

void update(inout float weight, inout float val, vec2 o) {
	float phifactor = cos(abs(tex_to_spheric(st).y));
	//o.x = o.x/phifactor;
	float oldT = texture(scratch3,offset(st,o,resolution)).r;
	float newT = texture(img,offset(st,o,resolution)).r;
	float w = pseudogaussian(length(o),d);


	if(abs(oldT-oldterrain)>1e-6) {
		weight += 5*w;
		val += 5*w*newterrain;
	} else {
		val += w*newT;
		weight += w;
	}
}
)",R"(
	//vec2 o = vec2(1,0);


	float weight = pseudogaussian(0,d);
	float val = newterrain*weight;

	update(weight,val,o);
	update(weight,val,-o);

	update(weight,val,2*o);
	update(weight,val,-2*o);

	update(weight,val,3*o);
	update(weight,val,-3*o);

	update(weight,val,5*o);
	update(weight,val,-5*o);

	update(weight,val,8*o);
	update(weight,val,-8*o);

	fc = val/weight;
)");
		auto program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();
		program->bind();
		p->setCanvasUniforms(program);

		for (int j=0; j<10; j++) {
			int id = glGetUniformLocation(program->getId(), "o");
			glUniform2f(id,1,0);
			p->apply(program, p->get_scratch2());
			p->get_scratch2()->swap(p->get_terrain());
			glUniform2f(id,0,1);
			p->apply(program, p->get_scratch2());
			p->get_scratch2()->swap(p->get_terrain());
		}



	});


	dispatchGPU([this,&texture,&height](Project* p) {
		p->remove_texture(texture);
		delete texture;
	});

	setProgress({true,1});
}


std::unique_ptr<float[]> DeTerrace::get(glm::vec2 primary, glm::vec2 secondary) {
	std::unique_ptr<float[]> data;

	dispatchGPU([this, primary, secondary, &data](Project* p) {
		init1->bind();
		p->setCanvasUniforms(init1);

		p->apply(init1,p->get_scratch2());

		init2->bind();
		p->setCanvasUniforms(init2);
		int id = glGetUniformLocation(program->getId(), "primary_offset");
		glUniform2f(id,primary.x,primary.y);
		id = glGetUniformLocation(program->getId(), "offset_no_globe_wrap");
		glUniform1i(id,true);
		p->apply(init2,p->get_scratch1());
		p->get_scratch1()->swap(p->get_scratch2());

		int a = std::ceil(log2(std::max(p->getHeight(),p->getWidth())))-3;

		for (int i=0; i<=a; i++) {
			program->bind();
			float r = pow(2,i);
			id = glGetUniformLocation(program->getId(), "primary_offset");
			glUniform2f(id,r*primary.x,r*primary.y);
			id = glGetUniformLocation(program->getId(), "secondary_offset");
			glUniform2f(id,r*secondary.x,r*secondary.y);
			id = glGetUniformLocation(program->getId(), "offset_no_globe_wrap");
			glUniform1i(id,true);

			p->setCanvasUniforms(program);
			p->apply(program, p->get_scratch1());
			p->get_scratch1()->swap(p->get_scratch2());

		}
		for (int i=a; i>=0; i--) {
			program->bind();
			float r = pow(2,i);
			id = glGetUniformLocation(program->getId(), "primary_offset");
			glUniform2f(id,r*primary.x,r*primary.y);
			id = glGetUniformLocation(program->getId(), "secondary_offset");
			glUniform2f(id,r*secondary.x,r*secondary.y);
			id = glGetUniformLocation(program->getId(), "offset_no_globe_wrap");
			glUniform1i(id,true);

			p->apply(program, p->get_scratch1());
			p->get_scratch1()->swap(p->get_scratch2());
		}

		data = (p->get_scratch2()->downloadDataRAW()); //TODO better solution than casting
	});
	return std::move(data);
}

void DeTerrace::updateDistance(std::unique_ptr<float[]> data) {
	dispatchGPU([this,&data](Project* p) {
		p->get_scratch2()->uploadData(GL_RED,GL_FLOAT,data.get());

		Shader* shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.include(distance)
				.include(pidShader)
				.create("",R"(
	vec2 resolution = textureSize(scratch2,0);
	uint pid = floatBitsToUint(texture(scratch2,st).r);

	float d = geodistance(st,pidToCoord(pid,scratch2),resolution);
	float old = texture(scratch1,st).r;
	fc = d>0 ? min(d,old) : old;
)");
		auto program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();
		program->bind();
		p->setCanvasUniforms(program);
		p->apply(program, p->get_terrain());
		p->get_terrain()->swap(p->get_scratch1());
	});
}
