#pragma once
#include "rnd_ConstantBuffer.h"
class rnd_ConstBufferInitializer;


class rnd_ConstantBufferMgr
{
public:
   std::map<LPCWSTR, rnd_ConstantBuffer*> constantBuffers;

   void InitConstBuffers();
   void UpdateConstBuffers();

   rnd_ConstantBuffer* Get(LPCWSTR cbName) { return constantBuffers[cbName]; }
   rnd_ConstantBuffer* Get(const rnd_ConstBufferInitializer& cbInitializer);
};

