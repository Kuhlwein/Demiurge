//
// Created by kuhlwein on 8/15/20.
//

#ifndef DEMIURGE_LAYERWINDOW_H
#define DEMIURGE_LAYERWINDOW_H

#include <Menu.h>

class Project;

class Layer {
public:
	Layer(int w, int h);
	~Layer();
	Texture* getTexture();
	std::string getName();
	void setName(std::string s);
	const int id;

private:
	static int id_counter;
	Texture* texture;
	std::string name = "New layer";
};

class LayerWindow : public Window {
public:
	LayerWindow();
	//bool update(Project* p) override;
};


#endif //DEMIURGE_LAYERWINDOW_H
