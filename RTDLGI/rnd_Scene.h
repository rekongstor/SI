#pragma once
#include "rnd_Instance.h"
#include "rnd_Mesh.h"
#include "rnd_TopLAS.h"

class rnd_Scene
{
public:
   std::vector<rnd_Mesh> meshes;
   std::map<rnd_Mesh*, rnd_Instance> instances;
   rnd_TopLAS topLayerAS;

   // Loads scene from FBX
   void OnInit(LPCSTR filename);
};

