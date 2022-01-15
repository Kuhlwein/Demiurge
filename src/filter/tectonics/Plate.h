//
// Created by kuhlwein on 1/15/22.
//

#ifndef DEMIURGE_PLATE_H
#define DEMIURGE_PLATE_H

#include "../Filter.h"

class Plate {
    public:
        Plate(int width, int height);
        ~Plate();
        Texture* getTexture();
        void rotate();
        void setPlateUniforms(ShaderProgram* shaderProgram, int indextsh);
        void updateRotationBy(float theta, glm::vec3 axis);
    private:
        //crust thickness
        //Crust age (negative when not part of plate)
        //local ridge/fold direction
        //(orogeny type???) [ocean=0, subduction=1, continental collision=2]
        Texture* texture;
        glm::mat4 rotation;
        glm::vec3 angularVelocity;
};


#endif //DEMIURGE_PLATE_H
