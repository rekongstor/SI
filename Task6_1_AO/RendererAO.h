#pragma once
#include "stdafx.h"

class Window;

class RendererAO
{
   Window* window = nullptr;

   CAULDRON_DX12::Device* device = nullptr;
public:
   void onInit();
   void onUpdate();
   void onDestroy();
   explicit RendererAO(Window* window);
};
