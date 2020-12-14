#include "rnd_ConstantBuffer.h"

rnd_ConstBufferInitializer::rnd_ConstBufferInitializer(LPCWSTR name, rnd_ConstantBuffer* pBuffer) : name(name), pBuffer(pBuffer)
{
   cbsNames.push_back(this);
}

void rnd_ConstantBuffer::OnInit(LPCWSTR name)
{
   throw (std::exception("Not implemented"));
}

void rnd_ConstantBuffer::Update()
{
   throw (std::exception("Not implemented"));
}
