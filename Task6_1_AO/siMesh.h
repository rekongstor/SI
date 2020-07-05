#pragma once
class siSceneLoader;

class siMesh
{
   friend class siSceneLoader;
   std::vector<uint32_t> indices;
   std::vector<siVertex> vertices;
   std::string diffuseMap;
};

