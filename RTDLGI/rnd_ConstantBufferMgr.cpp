#include "rnd_ConstantBufferMgr.h"
#include "rnd_ConstantBuffer.h"

std::vector<rnd_ConstBufferInitializer*> cbsNames;

void rnd_ConstantBufferMgr::InitConstBuffers()
{
   for (auto& cb : cbsNames) {
      cb->pBuffer->OnInit(cb->name);
      constantBuffers[cb->name] = cb->pBuffer;
   }
   cbsNames.clear();
}

void rnd_ConstantBufferMgr::UpdateConstBuffers()
{
   for (auto& cb : constantBuffers) {
      cb.second->Update();
   }
}

rnd_ConstantBuffer* rnd_ConstantBufferMgr::Get(const rnd_ConstBufferInitializer& cbInitializer)
{
   return constantBuffers[cbInitializer.name];
}
