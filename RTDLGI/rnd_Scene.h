#pragma once
#include "rnd_Inst.h"
#include "rnd_Mesh.h"

class rnd_Scene
{
public:
   std::vector<rnd_Mesh> meshes;
   std::map<rnd_Mesh*, rnd_Inst> instances;

   // Loads scene from FBX
   void OnInit(LPCSTR filename);
};

