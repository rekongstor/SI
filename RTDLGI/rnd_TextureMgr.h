#pragma once
#include "rnd_Texture.h"

class rnd_TextureMgr
{
public:
   rnd_Texture backBuffer[FRAME_COUNT];

   rnd_Texture rayTracingOutput;
};

