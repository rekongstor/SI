#pragma once
#include "stdafx.h"
#include <wtypes.h>
#include <vector>


class Mesh
{
public:

   std::vector<Vertex> vertices;
   std::vector<DWORD> indices;
   Mesh();
};