#pragma once
#include "rnd_Texture2D.h"
#include "rnd_Texture3D.h"

class rnd_TextureMgr
{
public:
   rnd_Texture2D backBuffer[FRAME_COUNT];

   rnd_Texture2D rayTracingOutput;
   rnd_Texture3D giBuffer;

   rnd_Texture2D depthBuffer;
};

