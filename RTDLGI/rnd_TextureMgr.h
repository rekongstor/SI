#pragma once
#include "rnd_Texture2D.h"

class rnd_TextureMgr
{
public:
   rnd_Texture2D backBuffer[FRAME_COUNT];

   rnd_Texture2D rayTracingOutput;
};

