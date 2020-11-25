#include "rnd_Texture.h"

void rnd_Texture::OnInit(ID3D12Resource* buffer, DXGI_FORMAT format, LPCWSTR name /*= L""*/)
{
   this->buffer = buffer;
   this->format = format;

   buffer->SetName(name);
}
