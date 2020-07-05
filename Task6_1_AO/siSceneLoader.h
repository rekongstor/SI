#pragma once
#include "siMesh.h"
#include "siTexture2D.h"

class siSceneLoader
{
public:
   static void loadScene(LPCSTR filename, std::map<int32_t, siMesh>& meshes, std::map<std::string, siTexture2D>& textures,
                         ID3D12Device* device, const siCommandList& commandList, siDescriptorMgr* descriptorMgr);
};

