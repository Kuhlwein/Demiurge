//
// Created by kuhlwein on 8/12/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "FlowFilter.h"
#include "BlurMenu.h"

FlowfilterMenu::FlowfilterMenu() : FilterModal("Flowfilter") {

}

void FlowfilterMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	const char* items[] = { "Erode","Dilate"};
	static int current = 0;
	ImGui::Combo("Operation",&current, items,IM_ARRAYSIZE(items));
}

std::shared_ptr<BackupFilter> FlowfilterMenu::makeFilter(Project *p) {
	auto morph = std::make_unique<FlowFilter>(0.5);
	return std::make_shared<ProgressFilter>(p,[](Project* p){return p->get_terrain();},std::move(morph));
}

FlowFilter::FlowFilter(float preblur) {
	this->preblur = preblur;
}

FlowFilter::~FlowFilter() {

}

std::vector<int> FlowFilter::neighbours(int pos, int dat) {
	//data-1 -> moves to the right (negative delta is left)
	//data-width -> moves down (negative delta is up)
	std::vector<int> n;
	auto f = [this,&n](int pos, int dx, int dy) {
		int x = pos%width;
		int y = pos/width;
		x = x+dx;
		if (std::abs(coords[3]-coords[2])>2*M_PI-1e-4) {
			x = (x+width)%width;
		} else {
			if(x>=width || x<0) return;
		}
		y = y+dy;
		if(y>=height || y<0) return;
//		if (coords[0]<-M_PI/2+1e-3 && y<0) {
//			y=-y;
//			float x2 = std::fmod((((float)x)/(width-1)*(coords[3]-coords[2])+coords[2])+2*M_PI,2*M_PI)-M_PI;
//			x = round((x2-coords[2])/(coords[3]-coords[2])*(width-1));
//		}
//		if (coords[1]>M_PI/2-1e-3 && y>=height) {
//			y=2*height-y;
//			float x2 = std::fmod((((float)x)/(width-1)*(coords[3]-coords[2])+coords[2])+2*M_PI,2*M_PI)-M_PI;
//			x = round((x2-coords[2])/(coords[3]-coords[2])*(width-1));
//		}
		n.emplace_back(y*width+x);
	};
	if (Nthbit(dat,1)) f(pos,-1,-1);
	if (Nthbit(dat,2)) f(pos,0,-1);
	if (Nthbit(dat,3)) f(pos,1,-1);
	if (Nthbit(dat,4)) f(pos,-1,0);
	if (Nthbit(dat,6)) f(pos,1,0);
	if (Nthbit(dat,7)) f(pos,-1,1);
	if (Nthbit(dat,8)) f(pos,0,1);
	if (Nthbit(dat,9)) f(pos,1,1);
	return n;
}

void FlowFilter::findMagicNumbers() {
	/*
	 * Magic number meaning:
	 * all bits zero -> not point of interest
	 *
	 * Bits set for neighbour, 5'th bit is self and indicates a sink/lake
	 * 1 2 3
	 * 4 5 6
	 * 7 8 9
	 *
	 * the 10'th bit indicates that this is a border sink/lake, aka a river mouth
	 */
	Blur* blur;
	std::unique_ptr<float[]> hdata;
	dispatchGPU([this,&blur,&hdata](Project* p) {
		hdata = p->get_terrain()->downloadDataRAW(); //TODO downloaded twice? Optimize
		blur = new Blur(p, preblur, p->get_terrain());
	});
	std::pair<bool, float> progress;
	do {
		dispatchGPU([this,&blur,&progress](Project* p) {
			progress = blur->step(p);
		});
	} while (!progress.first);
	dispatchGPU([this,&blur,&progress](Project* p) {
		delete blur;
	});

	dispatchGPU([this,&hdata](Project* p){
		coords = p->getCoords();
		circumference = p->circumference;

		Shader* shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.include(get_aspect)
				.create(R"(
	float hash(vec2 p)  // replace this by something better
{
    p  = 50.0*fract( p*0.3183099 + vec2(0.71,0.113));
    return -1.0+2.0*fract( p.x*p.y*(p.x+p.y) );
}

float noise( in vec2 p )
{
    vec2 i = floor( p );
    vec2 f = fract( p );

	vec2 u = f*f*(3.0-2.0*f);

    return mix( mix( hash( i + vec2(0.0,0.0) ),
                     hash( i + vec2(1.0,0.0) ), u.x),
                mix( hash( i + vec2(0.0,1.0) ),
                     hash( i + vec2(1.0,1.0) ), u.x), u.y);
}
)",R"(

	vec2 resolution = textureSize(img,0);

	float a = texture2D(img, st).r;
	fc = 0.0;
	if (a<=0.0f) return;
	if (texture2D(sel, st).r==0) return;
	fc = 5.0;
	float a2;
	float s;
	float s2 = 1;
	//(vec(1,1)-> right/down)

	float aspect = get_aspect(st);
	float lower_aspect = floor(aspect/(2*M_PI)*8)/8*2*M_PI;
	float upper_aspect = ceil(aspect/(2*M_PI)*8)/8*2*M_PI;
	float prob = abs(aspect-lower_aspect)/M_PI*4; // prob for upper_aspect

	float q = noise(st*resolution*2)*0.5+0.5;
	if (q<prob) aspect=upper_aspect; else aspect = lower_aspect;
	//if (0.5<prob) aspect=upper_aspect; else aspect = lower_aspect;


	vec2 dir = vec2(round(cos(aspect)),-round(sin(aspect)));


	if(all(dir==vec2(1,1))) fc=9;
	if(all(dir==vec2(0,1))) fc=8;
	if(all(dir==vec2(-1,1))) fc=7;
	if(all(dir==vec2(1,0))) fc=6;
	if(all(dir==vec2(-1,0))) fc=4;
	if(all(dir==vec2(1,-1))) fc=3;
	if(all(dir==vec2(0,-1))) fc=2;
	if(all(dir==vec2(-1,-1))) fc=1;


	a2 = texture2D(img, offset(st, dir,resolution)).r;
	s2 = texture2D(sel, offset(st, dir,resolution)).r;
	if(a2<=0.0) {
		fc = 5;
	}
	if(s2==0) fc=5;

	if (a2<a) return;

	fc = 5;


	a2 = texture2D(img, offset(st, vec2(1,1),resolution)).r;
	s2 = texture2D(sel, offset(st, vec2(1,1),resolution)).r;
	if(a2<a) {
		fc = 9;
		a = a2;
		s = s2;
	}
	a2 = texture2D(img, offset(st, vec2(0,1),resolution)).r;
	s2 = texture2D(sel, offset(st, vec2(0,1),resolution)).r;
	if(a2<a) {
		fc = 8;
		a = a2;
		s = s2;
	}
	a2 = texture2D(img, offset(st, vec2(-1,1),resolution)).r;
	s2 = texture2D(sel, offset(st, vec2(-1,1),resolution)).r;
	if(a2<a) {
		fc = 7;
		a = a2;
		s = s2;
	}
	a2 = texture2D(img, offset(st, vec2(1,0),resolution)).r;
	s2 = texture2D(sel, offset(st, vec2(1,0),resolution)).r;
	if(a2<a) {
		fc = 6;
		a = a2;
		s = s2;
	}
	a2 = texture2D(img, offset(st, vec2(-1,0),resolution)).r;
	s2 = texture2D(sel, offset(st, vec2(-1,0),resolution)).r;
	if(a2<a) {
		fc = 4;
		a = a2;
		s = s2;
	}
	a2= texture2D(img, offset(st, vec2(1,-1),resolution)).r;
	s2 = texture2D(sel, offset(st, vec2(1,-1),resolution)).r;
	if(a2<a) {
		fc = 3;
		a = a2;
		s = s2;
	}
	a2 = texture2D(img, offset(st, vec2(0,-1),resolution)).r;
	s2 = texture2D(sel, offset(st, vec2(0,-1),resolution)).r;
	if(a2<a) {
		fc = 2;
		a = a2;
		s = s2;
	}
	a2 = texture2D(img, offset(st, vec2(-1,-1),resolution)).r;
	s2 = texture2D(sel, offset(st, vec2(-1,-1),resolution)).r;
	if(a2<a) {
		fc = 1;
		a = a2;
		s = s2;
	}

	//Edge of ocean
	if(a<=0.0) {
		fc = 5;
	}
	if(s==0) fc=5;

)");
		ShaderProgram* program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();

		program->bind();
		p->setCanvasUniforms(program);
		int id = glGetUniformLocation(program->getId(),"cornerCoords");
		auto coordsMod = coords; //Kind of stupid fix for pole wrapping
		coordsMod[0]+=1e-3;
		coordsMod[1]-=1e-3;
		glUniform1fv(id, 4, coordsMod.data());


		p->apply(program,p->get_scratch1());


		//p->get_terrain()->swap(p->get_scratch1());
		//return;

		delete shader;
		delete program;

		shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.create("",R"(
	vec2 resolution = textureSize(img,0);

	float a = texture2D(img, st).r;
	fc = -1.0;
	if (a<=0.0f) return;
	if (texture2D(sel, st).r==0) return;
	fc = 0.5;

	bool flag = false;
	//(vec(1,1)-> right/down)

	if(texture2D(scratch1, offset(st, vec2(1,1),resolution)).r==1) fc+=256;
	if(texture2D(scratch1, offset(st, vec2(0,1),resolution)).r==2) fc+=128;
	if(texture2D(scratch1, offset(st, vec2(-1,1),resolution)).r==3) fc+=64;
	if(texture2D(scratch1, offset(st, vec2(1,0),resolution)).r==4) fc+=32;
	if(texture2D(scratch1, st).r==5) fc+=16;
	if(texture2D(scratch1, offset(st, vec2(-1,0),resolution)).r==6) fc+=8;
	if(texture2D(scratch1, offset(st, vec2(1,-1),resolution)).r==7) fc+=4;
	if(texture2D(scratch1, offset(st, vec2(0,-1),resolution)).r==8) fc+=2;
	if(texture2D(scratch1, offset(st, vec2(-1,-1),resolution)).r==9) fc+=1;

	//If besides "bad" point, make river mouth
	if(texture2D(scratch1, offset(st, vec2(1,1),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(0,1),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(-1,1),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(1,0),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(-1,0),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(1,-1),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(0,-1),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(-1,-1),resolution)).r==0) flag=true;
	if (flag) fc +=512;
)"); //TODO less samples
		program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();
		program->bind();
		p->setCanvasUniforms(program);
		p->apply(program,p->get_scratch2());

		data = p->get_scratch2()->downloadDataRAW();
		width = p->getWidth();
		height = p->getHeight();

		p->get_terrain()->uploadData(GL_RED,GL_FLOAT,hdata.get());
	});

	setProgress({false,0.02});
}

void FlowFilter::findPointsOfInterest() {
	std::function<void(std::pair<int,int>)> f = [this](std::pair<int,int> a) {
		std::vector<std::pair<int,int>> newOfInterest;
		int lower=a.first;
		int i;
		for (i=a.first; i<a.second; i++) {
			if(data[i]<0) { //not interesting
				if (i>lower) newOfInterest.emplace_back(lower,i);
				lower = i+1;
			}
		}
		if (i>lower) newOfInterest.emplace_back(lower,i);
		//transfer to global
		mtx.lock();
		for (auto i : newOfInterest) ofInterest.emplace_back(i);
		mtx.unlock();
	};
	std::vector<std::pair<int,int>> jobs;
	for (int i=0; i<height; i++) jobs.emplace_back(i*width,(i+1)*width);
	threadpool(f,jobs,0.031);
}

void FlowFilter::indexLakes(std::vector<std::vector<int>> *lakes) {
	std::function<void(std::pair<int,int>)> f = [this,&lakes](std::pair<int,int> a) {
		std::vector<int> new_lakes;
		for (int i=a.first; i<a.second; i++) {
			int d = static_cast<int>(data[i]);
			if (Nthbit(d,5)) {
				new_lakes.emplace_back(i);
			}
		}
		mtx.lock();
		lakes->emplace_back(new_lakes);
		mtx.unlock();
	};
	threadpool(f,ofInterest,0.04);
}

void FlowFilter::assignLakeIds(std::vector<std::vector<int>> *lakes) {
	lakeID = std::make_unique<float[]>(width*height);
	std::function<void(std::pair<int,int>)> f = [this,b=lakeID.get()](std::pair<int,int> a){
		for (int i=a.first; i<a.second; i++) b[i]=-1;
	};
	std::vector<std::pair<int,int>> jobs;
	for (int i=0; i<height; i++) jobs.emplace_back(i*width,(i+1)*width);
	threadpool(f,jobs,0.07);

	std::function<void(std::vector<int>)> f2 = [this](std::vector<int> a) {
		for (int lake : a) {
			std::function<void(int)> f = [this,&f,&lake](int s){
				int a[0];
				a[0] = 1073741824+lake; //The offset is important because the integer is read as a floating point number when uploaded to a texture. //TODO make something better... probably number each lake from 1 to N
				float* b = (float*)a;
				lakeID[s] = *b;
				for (auto n : neighbours(s,data[s])) {
					f(n);
				}
			};
			f(lake);

//			std::stack<int> stack;
//			stack.push(lake);
//			while (!stack.empty()) {
//				int s = stack.top();
//				stack.pop();
//				int* a;
//				a[0] = 1073741824+lake; //The offset is important because the integer is read as a floating point number when uploaded to a texture. //TODO make something better... probably number each lake from 1 to N
//				float* b = (float*)a;
//				lakeID[s] = *b;
//				for (auto n : neighbours(s,data[s])) {
//					stack.push(n);
//				}
//			}
		}
	};
	threadpool(f2,*lakes,0.151);
}

void FlowFilter::findAllConnections(std::vector<std::vector<int>> *lakes) {
	std::unique_ptr<float[]> passheights;
	std::unique_ptr<float[]> height;
	dispatchGPU([this,&passheights,&height](Project* p){
		p->get_scratch1()->uploadData(GL_RED,GL_FLOAT,lakeID.get());
		Shader* shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.create("",R"(
	vec2 resolution = textureSize(img,0);

	fc = 0;

	float a = texture2D(scratch1, st).r;
	if (a<0.0f) return;

	//(vec(1,1)-> right/down)

	float a2;
	//float h = texture2D(img, st).r;
	//fc = h+1;

	a2 = texture2D(scratch1, offset(st, vec2(1,1),resolution)).r;
	if (a2!=a && a2>0) fc += 256;

	a2 = texture2D(scratch1, offset(st, vec2(0,1),resolution)).r;
	if (a2!=a && a2>0) fc += 128;

	a2 = texture2D(scratch1, offset(st, vec2(-1,1),resolution)).r;
	if (a2!=a && a2>0) fc += 64;

	a2 = texture2D(scratch1, offset(st, vec2(1,0),resolution)).r;
	if (a2!=a && a2>0) fc += 32;

	a2 = texture2D(scratch1, offset(st, vec2(-1,0),resolution)).r;
	if (a2!=a && a2>0) fc += 8;

	a2 = texture2D(scratch1, offset(st, vec2(1,-1),resolution)).r;
	if (a2!=a && a2>0) fc += 4;

	a2 = texture2D(scratch1, offset(st, vec2(0,-1),resolution)).r;
	if (a2!=a && a2>0) fc += 2;

	a2 = texture2D(scratch1, offset(st, vec2(-1,-1),resolution)).r;
	if (a2!=a && a2>0) fc += 1;

	//if (fc!=h) fc = 0;
)");
		ShaderProgram* program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();

		program->bind();
		p->setCanvasUniforms(program);

		p->apply(program,p->get_scratch2());
		delete shader;
		delete program;
		passheights = p->get_scratch2()->downloadDataRAW();
		height = p->get_terrain()->downloadDataRAW();

		//terrain is height
		//scratch1 is lakeID
		//scratch2 is connections //TODO

		//p->get_scratch2()->swap(p->get_terrain());

	});

	std::function<void(std::vector<int>)> f2 = [this, &passheights, &height](std::vector<int> a) {
		std::map<int,std::set<pass,decltype(comp)>*> passSets;
		std::unordered_map<float,pass> newpasses; //map to location
		for (int lake : a) {
			newpasses.clear();

			std::stack<int> stack;
			stack.push(lake);
			while (!stack.empty()) {
				int s = stack.top();
				stack.pop();

				if (passheights[s]>0) {
					float minpass = MAXFLOAT;
					int nlake = -1;
					for (auto n : neighbours(s,passheights[s])) {
						float bd = height[n];
						int lid = *((int*)lakeID.get()+n) - 1073741824;
						if (lid!=lake && bd>0 && bd<minpass) {
							minpass = bd;
							nlake = n;
						}
					}
					int lid = *((int*)lakeID.get()+nlake) - 1073741824;
					if (minpass<MAXFLOAT && !Nthbit(data[lid],10)) {
						float nheight = std::max(minpass,height[s]);

						if(newpasses.count(lid)==0) {
							newpasses[lid] = {nheight,lid,s};
						} else if (nheight<newpasses[lid].h) {
							newpasses[lid] = {nheight,lid,s};
						}
					}
				}

				for (auto n : neighbours(s,data[s])) {
					stack.push(n);
				}
			}

			std::set<pass,decltype(comp)>* newp = new std::set<pass,decltype(comp)>(comp);
			for (auto pass : newpasses) newp->insert(pass.second);
			passSets[lake] = newp;

		}
		mtx.lock();
		for (auto p : passSets) {
			passes[p.first] = p.second;
		}
		mtx.unlock();
	};
	threadpool(f2,*lakes,0.44);

	int totalpasses=0;
	int rivermouth=0;
	for(auto a : passes) for(auto b : *a.second) {
		int from = b.from;
		totalpasses++;
		if (Nthbit(data[from],10)) rivermouth++;
	}
	std::cout << "total: " << totalpasses << ", rivermouth: " << rivermouth << "\n";
}

void FlowFilter::solvingConnections(std::vector<std::vector<int>> *lakes, std::unordered_map<int,pass> &connections) {
	std::set<pass,decltype(comp)> candidates(comp);
	std::unordered_set<int> placed_lakes;

	int totallakes = 0;
	for (auto &a : *lakes) totallakes+= a.size();

	int count = 0;
	for (auto a : *lakes) for (int lake : a) {
			int d = static_cast<int>(data[lake]);
			if (!Nthbit(d,10)) continue; // Only river mouths for now
			count++;
			placed_lakes.insert(lake);
			//Add best connection, not to existing lake
			while (!passes[lake]->empty()) {
				pass c = *passes[lake]->begin();
				passes[lake]->erase(c);
				if (placed_lakes.count(c.from)>0) continue;
				if (Nthbit(c.from,10)) continue;
				candidates.insert(c);
				break;
			}
			if (count%100==0) setProgress({false,0.44+0.22*((float)count/totallakes)});
		}

	//actually make connections
	while(!candidates.empty()) {
		auto p = *candidates.begin();
		candidates.erase(p);

		if (placed_lakes.count(p.from)>0) { //Lake already added, find new lowest pass and add to candidates
			int lake = *((int*)lakeID.get()+p.tolocation) - 1073741824;
			while (!passes[lake]->empty()) {
				pass c = *passes[lake]->begin();
				passes[lake]->erase(c);
				if (placed_lakes.count(c.from)>0) continue;
				candidates.insert(c);
				break;
			}
		} else { //Add new lake, and new lowest pass from that lake, also new lowest pass from old lake
			placed_lakes.insert(p.from);
			connections[p.tolocation] = p;
			int lake = p.from;
			while (!passes[lake]->empty()) {
				pass c = *passes[lake]->begin();
				passes[lake]->erase(c);
				if (placed_lakes.count(c.from)>0) continue;
				candidates.insert(c);
				break;
			}
			lake = *((int*)lakeID.get()+p.tolocation) - 1073741824;
			while (!passes[lake]->empty()) {
				pass c = *passes[lake]->begin();
				passes[lake]->erase(c);
				if (placed_lakes.count(c.from)>0) continue;
				candidates.insert(c);
				break;
			}
			count++;
			if (count%100==0) setProgress({false,0.44+0.22*((float)count/totallakes)});
		}
	}
}

void FlowFilter::calculateflow(std::vector<std::vector<int>> *lakes, std::unordered_map<int,pass> &connections) {
	//reset lakeID
	std::function<void(std::pair<int,int>)> f = [this,b=lakeID.get()](std::pair<int,int> a){
		for (int i=a.first; i<a.second; i++) b[i]=-1;
	};
	std::vector<std::pair<int,int>> jobs;
	for (int i=0; i<height; i++) jobs.emplace_back(i*width,(i+1)*width);
	threadpool(f,jobs,getProgress().second+0.01);


	std::function<float(int)> rec = [&rec,this,&connections](int p) {
		float y = (float)(p/width)/height;
		float geoy = (y*(coords[1]-coords[0])+coords[0]);
		float pixelwidthx = circumference*(coords[3]-coords[2])/(2*M_PI) / width;
		float pixelwidthy = circumference*(coords[1]-coords[0])/(2*M_PI) / height;

		float sum = pixelwidthy*pixelwidthx*cos(geoy) * 1e-5;
		for (auto n : neighbours(p, data[p])) {
			sum+=rec(n);
		}
		if (connections.count(p) > 0) {
			sum+=rec(connections[p].from);
		}
		lakeID[p] = sum;
		return sum;
	};

	std::function<void(std::vector<int>)> f2 = [this,&connections,&rec](std::vector<int> a) {
		std::stack<int> stack;
		for (int lake : a) {
			int d = static_cast<int>(data[lake]);
			if (!Nthbit(d, 10)) continue; // Only river mouths for now


			rec(lake);
//			stack.push(lake);
//			while (!stack.empty()) {
//				int s = stack.top();
//				stack.pop();
//
//				lakeID[s] = std::fmod(abs(lake * 11412414), 289) + 1;
//
//				for (auto n : neighbours(s, data[s])) {
//					stack.push(n);
//				}
//				if (connections.count(s) > 0) {
//					stack.push(connections[s].from);
//				}
//			}
		}
	};
	threadpool(f2,*lakes,0.98);

//	std::unique_ptr<float[]> height;
//	dispatchGPU([this,&height](Project* p){
//		height = p->get_terrain()->downloadDataRAW();
//	});


	//Draw lakes?
//	std::function<void(int,float,float)> lakefill = [this,&connections,&lakefill,&height](int lake, float waterheight, float waterlevel) {
//		std::stack<int> stack;
//		std::vector<pass> new_connections;
//		stack.push(lake);
//		while (!stack.empty()) {
//			auto p = stack.top();
//			stack.pop();
//			if (height[p] <= waterheight) lakeID[p] = std::max(lakeID[p], waterlevel);
//
//			for (auto n : neighbours(p,data[p])) {
//				stack.push(n);
//			}
//			if (connections.count(p) > 0) {
//				new_connections.emplace_back(connections[p]);
//			}
//		}
//		for (auto c : new_connections) {
//			if (waterheight>c.h) {
//				lakefill(c.from,waterheight,waterlevel);
//			} else {
//				lakefill(c.from,c.h,lakeID[c.from]);
//			}
//
//		}
//	};
//
//	f2 = [this,&connections,&rec,&lakefill](std::vector<int> a) {
//		for (int lake : a) {
//			if (!Nthbit(data[lake],10)) continue;
//			lakefill(lake,0.0f,lakeID[lake]);
//		}
//	};
//	threadpool(f2,*lakes,0.98);




//	for (auto c : connections) {
//		int lake = c.second.from;
//		float height = c.second.h;
//		std::stack<int> stack;
//		stack.push(lake);
//		while(!stack.empty()) {
//			auto p = stack.top();
//			stack.pop();
//			lakeID[p] = lakeID[lake];
//			for (auto n : neighbours(p, data[p])) {
//				stack.push(n);
//			}
//		}
//	}

	dispatchGPU([this](Project* p){
		p->get_terrain()->uploadData(GL_RED,GL_FLOAT,lakeID.get());
	});
}

void FlowFilter::run() {
	float c1;

//Find magic numbers
std::cout << "Finding magic numbers\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	findMagicNumbers();
	//setProgress({true,1});
	//return;


//Find points of interest
std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Points of interest\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;


	findPointsOfInterest();


//Make list of lakes
std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Indexing lakes\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	auto lakes = std::make_unique<std::vector<std::vector<int>>>();

	indexLakes(lakes.get());


std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Assigning lakes\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	assignLakeIds(lakes.get());

std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Finding possible connections\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	//std::function<bool(const pass&, const pass&)> comp = [](const pass& c1, const pass& c2){return c1.h<c2.h;};
	//std::unordered_map<int,std::set<pass,decltype(comp)>*> passes; //map lake to set of passes, sorted by height. Passes are from some other lake to key of map //TODO MUST BE DELETED MANUALLY!
	findAllConnections(lakes.get());


std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Solving connections\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	std::unordered_map<int,pass> connections; //maps location to pass
	solvingConnections(lakes.get(),connections);

std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Calculating flow\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	calculateflow(lakes.get(),connections);


	//Delete stuff
	for (auto p : passes) delete p.second;
std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";


	setProgress({true,1.0f});
	//TODO map of lake id and locations
}


bool FlowFilter::Nthbit(int num, int N) {
	return num & (1 << (N-1));
}



