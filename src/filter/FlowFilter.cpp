//
// Created by kuhlwein on 8/12/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "FlowFilter.h"

FlowfilterMenu::FlowfilterMenu() : FilterModal("Flowfilter") {

}

void FlowfilterMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	const char* items[] = { "Erode","Dilate"};
	static int current = 0;
	ImGui::Combo("Operation",&current, items,IM_ARRAYSIZE(items));
}

std::shared_ptr<BackupFilter> FlowfilterMenu::makeFilter(Project *p) {
	auto morph = std::make_unique<FlowFilter>();
	return std::make_shared<ProgressFilter>(p,[](Project* p){return p->get_terrain();},std::move(morph));
}

FlowFilter::FlowFilter() {

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

void FlowFilter::run() {
	float c1;
	/*
	 * all bits zero -> not point of interest
	 *
	 * Bits set for neighbour, 5'th bit is self and indicates a sink/lake
	 * 1 2 3
	 * 4 5 6
	 * 7 8 9
	 *
	 * the 10'th bit indicates that this is a border sink/lake, aka a river mouth
	 */

//Find magic numbers
std::cout << "Finding points of interest\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	std::unique_ptr<float[]> data;
	dispatchGPU([this,&data](Project* p){
		coords = p->getCoords();
		//wrapx = std::abs(p->getCoords()[3]-p->getCoords()[2])>2*M_PI-1e-3;
		//std::cout << wrapx << " wrap\n";

		Shader* shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.create("",R"(
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
	});


//Find points of interest
std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Points of interest\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;
	std::mutex mtx;
	std::vector<std::pair<int,int>> ofInterest;

	std::function<void(std::pair<int,int>)> f = [this,&data,&mtx,&ofInterest](std::pair<int,int> a) {
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
	cputools2::threadpool(f,jobs);

//Make list of lakes
std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Indexing lakes\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	auto lakes = std::make_unique<std::vector<std::vector<int>>>();

	f = [this,&mtx,&lakes, &data](std::pair<int,int> a) {
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
	cputools2::threadpool(f,ofInterest);


std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Assigning lakes\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	auto lakeID = std::make_unique<float[]>(width*height);
	f = [this,b=lakeID.get()](std::pair<int,int> a){
		for (int i=a.first; i<a.second; i++) b[i]=-1;
	};
	cputools2::threadpool(f,jobs);

	std::function<void(std::vector<int>)> f2 = [this, &lakeID,&data](std::vector<int> a) {
		for (int lake : a) {
			std::stack<int> stack;
			stack.push(lake);
			while (!stack.empty()) {
				int s = stack.top();
				stack.pop();
				int* a;
				a[0] = 1073741824+lake; //The offset is important because the integer is read as a floating point number when uploaded to a texture. //TODO make something better... probably number each lake from 1 to N
				float* b = (float*)a;
				lakeID[s] = *b;
				for (auto n : neighbours(s,data[s])) {
					stack.push(n);
				}
			}
		}
	};
	cputools2::threadpool(f2,*lakes);
std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Finding possible connections\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	std::unique_ptr<float[]> passheights;
	dispatchGPU([this,&lakeID,&passheights](Project* p){
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
	float h = texture2D(img, st).r;
	fc = h+1;

	a2 = texture2D(scratch1, offset(st, vec2(1,1),resolution)).r;
	if (a2!=a && a2>0) fc = h;

	a2 = texture2D(scratch1, offset(st, vec2(0,1),resolution)).r;
	if (a2!=a && a2>0) fc = h;

	a2 = texture2D(scratch1, offset(st, vec2(-1,1),resolution)).r;
	if (a2!=a && a2>0) fc = h;

	a2 = texture2D(scratch1, offset(st, vec2(1,0),resolution)).r;
	if (a2!=a && a2>0) fc = h;

	a2 = texture2D(scratch1, offset(st, vec2(-1,0),resolution)).r;
	if (a2!=a && a2>0) fc = h;

	a2 = texture2D(scratch1, offset(st, vec2(1,-1),resolution)).r;
	if (a2!=a && a2>0) fc = h;

	a2 = texture2D(scratch1, offset(st, vec2(0,-1),resolution)).r;
	if (a2!=a && a2>0) fc = h;

	a2 = texture2D(scratch1, offset(st, vec2(-1,-1),resolution)).r;
	if (a2!=a && a2>0) fc = h;

	if (fc!=h) fc = 0;
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

		//p->get_scratch2()->swap(p->get_terrain());

	});

	struct pass {
		float h;
		int from; //Which lake is the flow from
		//float to;
		int tolocation; //In self
	};
	std::function<bool(const pass&, const pass&)> comp = [](const pass& c1, const pass& c2){return c1.h<c2.h;};
	std::unordered_map<int,std::set<pass,decltype(comp)>*> passes; //map lake to set of passes, sorted by height. Passes are from some other lake to key of map //TODO MUST BE DELETED MANUALLY!

	f2 = [this, &passheights,&data,&lakeID,&mtx,&passes,&comp](std::vector<int> a) {
		std::map<int,std::set<pass,decltype(comp)>*> passSets;
		for (int lake : a) {
			std::map<float,pass> newpasses; //map to location

			std::stack<int> stack;
			stack.push(lake);
			while (!stack.empty()) {
				int s = stack.top();
				stack.pop();

				if (passheights[s]>0) {
					float minpass = MAXFLOAT;
					int nlake = -1;
					int d = static_cast<int>(data[s]);
					for (auto n : neighbours(s,((int)pow(2,15)-1) ^ d)) {
						float bd = passheights[n];
						if (bd>0 && bd<minpass) {
							minpass = bd;
							nlake = n;
						}
					}
					if (nlake>=0) {
						float nheight = std::max(minpass,passheights[s]);
						int lid = *((int*)lakeID.get()+nlake) - 1073741824;
						if(newpasses.count(lakeID[nlake])==0) {
							newpasses[lakeID[nlake]] = {nheight,lid,s};
						} else if (nheight<newpasses[lakeID[nlake]].h) {
							newpasses[lakeID[nlake]] = {nheight,lid,s};
						}
					}
				}

				for (auto n : neighbours(s,data[s])) {
					stack.push(n);
				}
			}
			//all connections found for lake

			std::set<pass,decltype(comp)>* newp = new std::set<pass,decltype(comp)>(comp);
			for (auto pass : newpasses) newp->insert(pass.second);
			passSets[lake] = newp;

		}
		mtx.lock();
		for (auto p : passSets) passes[p.first] = p.second;
		mtx.unlock();
	};
	cputools2::threadpool(f2,*lakes);

	//Alle har connection! kun et par rivermouths der ikke har, det er ok.
	//for (auto p : passes) if(p.second->size()==0) std::cout << "lake: " << p.first << ", " << p.second->size() << ", is rivermouth: " << Nthbit(data[p.first],10) << "\n";

std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Solving connections\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	std::set<pass,decltype(comp)> candidates(comp);
	std::unordered_set<int> placed_lakes;



	for (auto a : *lakes) for (int lake : a) {
		int d = static_cast<int>(data[lake]);
		if (!Nthbit(d,10)) continue; // Only river mouths for now
		placed_lakes.insert(lake);
		//Add best connection, not to existing lake
		while (!passes[lake]->empty()) {
			pass c = *passes[lake]->begin();
			passes[lake]->erase(c);
			if (placed_lakes.count(c.from)>0) continue;
			candidates.insert(c);
			break;
		}
	}

	std::unordered_map<int,pass> connections; //maps location to pass

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
		} else { //Add new lake, and new lowest pass from that lake
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
		}
	}

std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Calculating flow\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	//reset lakeID
	f = [this,b=lakeID.get()](std::pair<int,int> a){
		for (int i=a.first; i<a.second; i++) b[i]=-1;
	};
	cputools2::threadpool(f,jobs);

	for (auto a : *lakes) for (int lake : a) {
			int d = static_cast<int>(data[lake]);
			if (!Nthbit(d,10)) continue; // Only river mouths for now

			std::stack<int> stack;
			stack.push(lake);
			while (!stack.empty()) {
				int s = stack.top();
				stack.pop();

				lakeID[s] = std::fmod(lake*11412414,289)+1;

				for (auto n : neighbours(s,data[s])) {
					stack.push(n);
				}
				if (connections.count(s)>0) {
					stack.push(connections[s].from);
				}
			}
		}

	dispatchGPU([&lakeID](Project* p){
		//p->get_terrain()->uploadData(GL_RED,GL_FLOAT,lakeID.get());
	});


	//Delete stuff
	for (auto p : passes) delete p.second;

	setProgress({true,1.0f});
	//TODO map of lake id and locations
}


bool FlowFilter::Nthbit(int num, int N) {
	return num & (1 << (N-1));
}


template<typename T>
void cputools2::threadpool(std::function<void(T)> f, std::vector<T> arg) {
	std::mutex mtx;
	uint Nthreads = std::thread::hardware_concurrency();
	auto it = arg.begin();
	auto end = arg.end();

	auto j = [f,&arg,&mtx,&it,&end](){
		while (true) {
			mtx.lock();
			if (it==end) {
				mtx.unlock();
				return;
			}
			T a = *it;
			it++;
			mtx.unlock();
			f(a);
		}
	};
	std::vector<std::unique_ptr<std::thread>> threads;
	for (uint i=0; i<Nthreads; i++) threads.push_back(std::make_unique<std::thread>(j));
	for (auto &t : threads) t->join();
	for (auto &t : threads) t.reset();

}
