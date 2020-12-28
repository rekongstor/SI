#include "PassForward.h"

#include <PassForwardPS.hlsl.h>
#include <PassForwardVS.hlsl.h>

#include "rnd_Dx12.h"
#include "SceneConstBuf.h"

void PassForward::OnInit()
{
   // Create RS
   forwardRootSignature = renderer->rootSignatureMgr.CreateRootSignature({ CBV(0), SRV(0, 0), DescTable({DescRange(RngType::SRV, 1, 1) }) },
      { CD3DX12_STATIC_SAMPLER_DESC(0) },
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
   );

   sceneCb = dynamic_cast<SceneConstBuf*>(renderer->constantBufferMgr.Get(SCENE_CB));

   // Create PSO
   std::vector<DXGI_FORMAT> rtvFormats { renderer->swapChainFormat };
   int renderTargetsCount = rtvFormats.size();

   D3D12_SHADER_BYTECODE vertexShaderByteCode = { g_pPassForwardVS, ARRAYSIZE(g_pPassForwardVS) };
   D3D12_SHADER_BYTECODE pixelShaderByteCode = { g_pPassForwardPS, ARRAYSIZE(g_pPassForwardPS) };

   D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
   {
      "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
      0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
   },
   {
      "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
      0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
   },
   {
      "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
      0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
   }
   };
   D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = { inputElementDesc, _countof(inputElementDesc) };


   D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
   ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
   auto rs = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
   rs.CullMode = D3D12_CULL_MODE_FRONT;
   psoDesc.InputLayout = inputLayoutDesc;
   psoDesc.pRootSignature = forwardRootSignature.Get();
   psoDesc.VS = vertexShaderByteCode;
   psoDesc.PS = pixelShaderByteCode;
   psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
   for (uint32_t i = 0; i < renderTargetsCount; ++i)
      psoDesc.RTVFormats[i] = rtvFormats[i];
   psoDesc.SampleDesc.Count = 1;
   psoDesc.SampleDesc.Quality = 0;
   psoDesc.SampleMask = UINT_MAX;
   psoDesc.RasterizerState = rs;
   psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
   psoDesc.NumRenderTargets = renderTargetsCount;
   const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
   { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
   D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
   depthStencilDesc.DepthEnable = true;
   depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
   depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
   depthStencilDesc.StencilEnable = false;
   depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
   depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
   depthStencilDesc.FrontFace = defaultStencilOp;
   depthStencilDesc.BackFace = defaultStencilOp;

   psoDesc.DepthStencilState = depthStencilDesc;
   psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

   ThrowIfFailed(renderer->Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject)));
   pipelineStateObject.Get()->SetName(L"Drawing PSO");
}

void PassForward::Execute()
{
   renderer->SetBarrier({ { renderer->BackBuffer() , (D3D12_RESOURCE_STATE_RENDER_TARGET) } });

   renderer->CommandList()->RSSetViewports(1, &renderer->viewport);
   renderer->CommandList()->RSSetScissorRects(1, &renderer->scissorRect);

   // Root signature [0]. Drawing meshes
   renderer->CommandList()->SetGraphicsRootSignature(forwardRootSignature.Get());
   renderer->CommandList()->SetPipelineState(pipelineStateObject.Get());
   renderer->CommandList()->SetGraphicsRootConstantBufferView(0, sceneCb->buffer->GetGPUVirtualAddress());
   renderer->CommandList()->SetGraphicsRootDescriptorTable(2, renderer->textureMgr.rayTracingOutput.srvHandle.second);

   renderer->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   ID3D12DescriptorHeap* heap[] = { renderer->cbvSrvUavHeap.Get() };
   renderer->CommandList()->SetDescriptorHeaps(1, heap);
   auto& backBuffer = renderer->BackBuffer().rtvHandle;
   auto& depthBuffer = renderer->textureMgr.depthBuffer.dsvHandle;
   renderer->CommandList()->OMSetRenderTargets(1, &backBuffer.first, false, &depthBuffer.first);
   auto& instBuffer = renderer->scene.instancesDataBuffer[renderer->currentFrame];

   for (auto& i : renderer->scene.instances)
   {
      renderer->CommandList()->IASetVertexBuffers(0, 1, &i.first->vertexBuffer.vertexBufferView);
      renderer->CommandList()->IASetIndexBuffer(&i.first->indexBuffer.indexBufferView);
      renderer->CommandList()->SetGraphicsRootShaderResourceView(1, instBuffer.buffer->GetGPUVirtualAddress() + i.second.instIdx * instBuffer.sizeOfElement);
      renderer->CommandList()->DrawIndexedInstanced(i.first->indexBuffer.width, 1, 0, 0, 0);
   }
}
