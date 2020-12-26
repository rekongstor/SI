#pragma once
#include "siMesh.h"
#include "siTexture.h"

class siSceneLoader
{
public:
   static void loadScene(LPCSTR filename, std::map<int32_t, siMesh>& meshes, std::map<std::string, siTexture>& textures,
                         ID3D12Device* device, const siCommandList& commandList, siDescriptorMgr* descriptorMgr);
   static void loadScene(LPCSTR filename, std::map<int32_t, siMesh>& meshes, std::map<std::string, siTexture>& textures,
                         ID3D12Device* device, siDescriptorMgr* descriptorMgr, const siCommandList& commandList);
};
