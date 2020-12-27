#include "rnd_Mesh.h"
#include "HlslCompat.h"
#include "rnd_Dx12.h"

void rnd_Mesh::OnInit(std::vector<char>& vertices, std::vector<char>& indices, LPCWSTR name)
{
   indexBuffer.OnInit(indices, sizeof(Index), name);
   vertexBuffer.OnInit(vertices, sizeof(Vertex), name);

   renderer->ResolveUploadBuffer();

   bottomLas.OnInit(indexBuffer, vertexBuffer, name);

   indexBuffer.CreateSrv();
   vertexBuffer.CreateSrv();
}
