#include "rnd_Mesh.h"
#include "HlslCompat.h"
#include "rnd_Dx12.h"

void rnd_Mesh::OnInit(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, LPCWSTR name)
{
   vertexBuffer.OnInit((void*)vertices.data(), vertices.size(), sizeof(Vertex), name);
   indexBuffer.OnInit((void*)indices.data(), indices.size(), sizeof(Index), name);

   vertexBuffer.CreateSrv();
   indexBuffer.CreateSrv();

   renderer->ResolveUploadBuffer();
   renderer->SetBarrier({ {indexBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE}, {vertexBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE} });
}
