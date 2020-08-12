//
// Created by kuhlwein on 8/10/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <iostream>
#include <thread>
#include "cpufilter.h"

cpufilterMenu::cpufilterMenu() : FilterModal("cpufilter") {
	std::cout << "\tcreating cpufilter\n";
}

void cpufilterMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	const char* items[] = { "Erode","Dilate"};
	static int current = 0;
	ImGui::Combo("Operation",&current, items,IM_ARRAYSIZE(items));
}

std::shared_ptr<BackupFilter> cpufilterMenu::makeFilter(Project *p) {
	auto morph = std::make_unique<cpufilter>(p);
	return std::make_shared<ProgressFilter>(p,[](Project* p){return p->get_terrain();},std::move(morph));
}



cpufilter::cpufilter(Project *p) {

}

void cpufilter::setup(Project *p) {
	data = p->get_terrain()->downloadDataRAW();
	selection = p->get_selection()->downloadDataRAW();
	width = p->getWidth();
	height = p->getHeight();
}

void cpufilter::run() {
	std::mutex mtx;
	std::vector<std::pair<int,int>> ofInterest;

	float c1;

std::cout << "Finding points of interest\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;
//Find points of interest
	std::function<void(std::pair<int,int>)> f = [this,&mtx,&ofInterest](std::pair<int,int> a) {
		std::vector<std::pair<int,int>> newOfInterest;
		int lower=a.first;
		for (int i=a.first; i<a.second; i++) {
			if(data[i]<=level || selection[i]==0) {
				if (i>lower) newOfInterest.emplace_back(lower,i);
				lower = i+1;
			}
		}
		//transfer to global
		mtx.lock();
		for (auto i : newOfInterest) ofInterest.emplace_back(i);
		mtx.unlock();
	};
	std::vector<std::pair<int,int>> jobs;
	for (int i=0; i<height; i++) jobs.emplace_back(i*width,(i+1)*width);
	cputools::threadpool(f,jobs);






	auto indexof = [this](int x, int y){return y*width+x;};
	auto coordof = [this](int i){return std::pair<int,int>{i%width,i/width};};




std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
	std::cout << "Connecting edges\n";
	c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

//Connect edges
	std::map<int,int> neighbours;
	struct lake {
		bool outlet;
		std::map<int,std::pair<float,int>> connections; // maps other lake id to {height of pass,point}
	};
	std::map<int,lake> lakes;

	f = [this,&mtx,coordof,indexof,&lakes,&neighbours](std::pair<int,int> a) {
		std::map<int,int> new_neighbours;
		std::map<int,lake> new_lakes;
		for (int i=a.first; i<a.second; i++) {
			//Find minimal neighbour, might be self
			float minval = MAXFLOAT;
			int min_i = i;
			for (int dx : {-1,0,1}) for (int dy : {-1,0,1}) {
				auto coord = coordof(i);
				int id = indexof(coord.first+dx,coord.second+dy);
				float val = data[id];
				if (val<minval) {
					minval = val;
					min_i = id;
				}
			}
			new_neighbours.insert({i,min_i});

			//If self or border pixel, make sink
			bool isOutlet = data[min_i]<=level || selection[min_i]==0;
			if (min_i == i || isOutlet) new_lakes.insert({i,{isOutlet,{}}});
		}

		//transfer to global
		mtx.lock();
		neighbours.insert(new_neighbours.begin(), new_neighbours.end());
		lakes.insert(new_lakes.begin(),new_lakes.end());
		mtx.unlock();
	};
	cputools::threadpool(f,ofInterest);
	selection.reset(); //TODO remove??

	std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
	std::cout << "Identify neighbours and lakes\n";
	c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

//Identify neighbours and lakes for each point
	struct pointdata {
		int lake;
		std::vector<int> children;
	};
	std::map<int,pointdata> points;

	std::function<void(int)> f2 = [this,&mtx,coordof,indexof,&neighbours,&points](int l) {
		std::map<int,pointdata> newPoints;
		std::stack<int> stack;
		stack.push(l);
		while (!stack.empty()) {
			int s = stack.top();
			stack.pop();
			newPoints[s] = {l,{}};
			for (int dx : {-1,0,1}) for (int dy : {-1,0,1}) {
				auto coord = coordof(s);
				int id = indexof(coord.first + dx, coord.second + dy);
				if (id==s || neighbours.count(id)==0) continue; //If self, or not in area of interest
				if (neighbours[id] == s) {
					newPoints[s].children.push_back(id);
					stack.push(id);
				}
			}
		}
		mtx.lock();
		points.insert(newPoints.begin(), newPoints.end());
		mtx.unlock();
	};
	std::vector<int> lakejob;
	for (auto l : lakes) lakejob.emplace_back(l.first);
	cputools::threadpool(f2,lakejob);

	std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
	std::cout << "Find possible lake connections\n";
	c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

//Find lake connections
	f2 = [this,&mtx,coordof,indexof,&lakes,&points](int l) {
		std::stack<int> stack;
		stack.push(l);
		while (!stack.empty()) {
			int s = stack.top();
			stack.pop();
			for (int n : points[s].children) stack.push(n);
			for (int dx : {-1,0,1}) for (int dy : {-1,0,1}) {
				auto coord = coordof(s);
				int id = indexof(coord.first + dx, coord.second + dy);

				if (points.count(id)==0) continue;
				int nlake = points[id].lake;
				float nheight = std::max(data[id] ,data[s]);
				if (nlake == l) continue; //If belong to same lake


				if (lakes[l].connections.count(nlake)==0) {
					lakes[l].connections.insert({nlake,{nheight,id}});
				} else if (lakes[l].connections[nlake].first>nheight) {
					lakes[l].connections[nlake] =  {nheight,id};
				}
			}
		}
	};
	cputools::threadpool(f2,lakejob);

	std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
	std::cout << "Connect lakes\n";
	c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

//Make lake connections
	std::map<int,float> placed_lakes; //maps lake to surface of lake height
	struct pass {
		float h;
		int from;
		int to;
		int location;
	};
	auto comp = [](const pass& c1, const pass& c2){return c1.h<c2.h;}; //TODO check!
	std::set<pass,decltype(comp)> candidates(comp); // maps heights of passes to {from,to} pairs

	for (auto l : lakes) {
		if(!l.second.outlet) continue; //only outlets for now
		placed_lakes.insert({l.first,0});
		for (auto c : l.second.connections) {
			int from = c.first;
			int to = l.first;
			auto dual = lakes[from].connections[to];
			candidates.insert({dual.first,from,to,dual.second});
		}
	}

	while (!candidates.empty()) {
		auto c = *candidates.begin();
		candidates.erase(c);
		if (placed_lakes.count(c.from)>0) continue; //lake already connected
		points[c.location].children.push_back(c.from);
		placed_lakes.insert({c.from,c.h});

		int to = c.from;
		for (auto c : lakes[to].connections) {
			int from = c.first;
			auto dual = lakes[from].connections[to];
			candidates.insert({dual.first,from,to,dual.second});
		}
	}


//std::cout << "Draw lakes\n";
//	for (auto l : lakes) {
//		if(l.second.outlet) continue; //only !outlets for now
//
//		std::stack<int> stack;
//		stack.push(l.first);
//		while (!stack.empty()) {
//			int s = stack.top();
//			stack.pop();
//			for (int n : points[s].children) stack.push(n);
//			if (data[s]<=placed_lakes[l.first]) data[s] = -5e7;
//		}
//	}

	std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
	std::cout << "Draw flow\n";
	c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	std::function<int(int)> rec = [&rec,&points,this](int p) {
		int sum = 1;
		for (int d : points[p].children) sum+= rec(d);
		if (data[p]>0) data[p] = pow(sum,0.6);
		return sum;
	};

	for (auto l : lakes) {
		if(!l.second.outlet) continue; //only outlets for now

		std::stack<int> stack;
		stack.push(l.first);
		//rec(l.first);
		while (!stack.empty()) {
			int s = stack.top();
			stack.pop();
			for (int n : points[s].children) stack.push(n);
			data[s] = l.first*149874874%289;
		}
	}
	std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";


	setProgress({true,1.0f});
}

void cpufilter::finalize(Project *p) {
	p->get_terrain()->uploadData(GL_RED,GL_FLOAT,data.get());
	//selection.reset();
	data.reset();
}

cpufilter::~cpufilter() {
	std::cout << "\tdestroying cpu filter\n";
}


template<typename T>
void cputools::threadpool(std::function<void(T)> f, std::vector<T> arg) {
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
