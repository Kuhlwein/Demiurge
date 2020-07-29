//
// Created by kuhlwein on 4/3/20.
//

#ifndef DEMIURGE_UNDOHISTORY_H
#define DEMIURGE_UNDOHISTORY_H


#include <functional>
#include "Texture.h"

class Project;

class UndoHistory {
public:
	virtual void undo(Project* p) = 0;
	virtual void redo(Project* p) = 0;

};

class ReversibleHistory : public UndoHistory {
public:
	ReversibleHistory(std::function<void(Project* p)> r,std::function<void(Project* p)> u);
	void undo(Project* p) override;
	void redo(Project* p) override;
private:
	std::function<void(Project* p)> r;
	std::function<void(Project* p)> u;
};

class SnapshotHistory : public UndoHistory {
public:
	SnapshotHistory(TextureData* data, std::function<Texture *(Project *p)> filter_target);
	~SnapshotHistory();
	void undo(Project* p) override;
	void redo(Project* p) override;
private:
	TextureData* data; //Before - After
	std::function<Texture *(Project *p)> filter_target;
};

// True reversible (layers?), Reversible snapshot, Snapeshot, reconstruct

// snapshot before, quick filter
// long filter, snapshot diff
// reversible filter


#endif //DEMIURGE_UNDOHISTORY_H