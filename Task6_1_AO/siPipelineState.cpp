#include "siPipelineState.h"
#include <d3dcompiler.h>
#include <fstream>

std::vector<byte> LoadShaderFile(LPCWSTR filename)
{
   std::vector<byte> filedata;

   // open the file
   std::ifstream VertexFile(filename, std::ios::in | std::ios::binary | std::ios::ate);

   // if open was successful
   if (VertexFile.is_open())
   {
      // find the length of the file
      int Length = (int)VertexFile.tellg();

      // collect the file data
      filedata.resize(Length);
      VertexFile.seekg(0, std::ios::beg);
      VertexFile.read(reinterpret_cast<char*>(filedata.data()), Length);
      VertexFile.close();
   }

   return filedata;
}

bool fileExists(LPCWSTR filename)
{
   std::ifstream file(filename);
   return file.good();
}

void siPipelineState::createPso(
   ID3D12Device* device,
   const ComPtr<ID3D12RootSignature>& rootSignature,
   LPCWSTR vsFileName,
   LPCWSTR psFileName,
   const DXGI_FORMAT* rtvFormats,
   uint32_t renderTargetsCount,
   DXGI_SAMPLE_DESC sampleDesc,
   const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputElementDesc)
{
   assert(renderTargetsCount <= 8);
   HRESULT hr;

   ComPtr<ID3DBlob> vertexShader;
   hr = D3DCompileFromFile(vsFileName,
                           nullptr,
                           nullptr,
                           "main",
                           "vs_5_1",
                           D3DCOMPILE_OPTIMIZATION_LEVEL3,
                           NULL,
                           &vertexShader,
                           nullptr);
   assert(hr == S_OK);

   ComPtr<ID3DBlob> pixelShader;
   hr = D3DCompileFromFile(psFileName,
                           nullptr,
                           nullptr,
                           "main",
                           "ps_5_1",
                           D3DCOMPILE_OPTIMIZATION_LEVEL3,
                           NULL,
                           &pixelShader,
                           nullptr);
   assert(hr == S_OK);

   D3D12_SHADER_BYTECODE vertexShaderByteCode = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
   D3D12_SHADER_BYTECODE pixelShaderByteCode = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};


   D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {inputElementDesc.data(), inputElementDesc.size()};


   D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
   ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
   auto rs = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
   rs.CullMode = D3D12_CULL_MODE_NONE;
   psoDesc.InputLayout = inputLayoutDesc;
   psoDesc.pRootSignature = rootSignature.Get();
   psoDesc.VS = vertexShaderByteCode;
   psoDesc.PS = pixelShaderByteCode;
   psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
   for (uint32_t i = 0; i < renderTargetsCount; ++i)
      psoDesc.RTVFormats[i] = rtvFormats[i];
   psoDesc.SampleDesc = sampleDesc;
   psoDesc.SampleMask = UINT_MAX;
   psoDesc.RasterizerState = rs;
   psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
   psoDesc.NumRenderTargets = renderTargetsCount;
   const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
   { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
   D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
   depthStencilDesc.DepthEnable = true;
   depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
   depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
   depthStencilDesc.StencilEnable = false;
   depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
   depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
   depthStencilDesc.FrontFace = defaultStencilOp;
   depthStencilDesc.BackFace = defaultStencilOp;

   psoDesc.DepthStencilState = depthStencilDesc;
   psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

   hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
   assert(hr == S_OK);
   pipelineStateObject.Get()->SetName(L"Drawing PSO");
}

void siPipelineState::createPso(
   ID3D12Device* device,
   const ComPtr<ID3D12RootSignature>& rootSignature,
   LPCWSTR csFileName)
{
   HRESULT hr;
   std::vector<byte> byteCode;
   D3D12_SHADER_BYTECODE CSShaderByteCode;
   wchar_t csoFilename[256];
   wcscpy_s(csoFilename, csFileName);
   size_t length = wcslen(csoFilename);
   csoFilename[length - 4] = L'c';
   csoFilename[length - 3] = L's';
   csoFilename[length - 2] = L'o';
   csoFilename[length - 1] = 0;

   ID3DBlob* computeShader;
   if (csoFilename && fileExists(csoFilename))
   {
      std::cout << "Loading cso shader " << csoFilename << std::endl;
      byteCode = LoadShaderFile(csoFilename);
      CSShaderByteCode = { byteCode.data(), byteCode.size() };
   }
   else
   {
      hr = D3DCompileFromFile(csFileName,
         nullptr,
         D3D_COMPILE_STANDARD_FILE_INCLUDE,
         "main",
         "cs_5_1",
         D3DCOMPILE_OPTIMIZATION_LEVEL3,
         NULL,
         &computeShader,
         nullptr);
      assert(hr == S_OK);
      CSShaderByteCode = { computeShader->GetBufferPointer(), computeShader->GetBufferSize() };
   }



   D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc;
   ZeroMemory(&psoDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
   psoDesc.pRootSignature = rootSignature.Get();
   psoDesc.CS = CSShaderByteCode;
   psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
   psoDesc.NodeMask = 0;

   hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
   assert(hr == S_OK);
   pipelineStateObject.Get()->SetName(csFileName);
}

const ComPtr<ID3D12PipelineState>& siPipelineState::getPipelineState() const
{
   return pipelineStateObject;
}
