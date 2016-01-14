#ifndef TREERENDERING_H
#define TREERENDERING_H

#include <glm/glm.hpp>
#include <vector>

#include "Tree.h"
#include <Rendering/VertexArrayObjects.h>

class aiScene;

namespace TreeAnimation
{

Renderable* generateRenderable(TreeAnimation::Tree::Branch* branch, const aiScene* branchModel = NULL);

Renderable* generateFoliage(TreeAnimation::Tree::Branch* branch, int numLeafs);

} // TreeAnimation
#endif