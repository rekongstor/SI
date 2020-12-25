#pragma once
class SceneConstBuf;
class CubeConstBuf;
class SceneConstBuffer;

class PassRaytracing
{
public:
   CubeConstBuf* cubeCb;
   SceneConstBuf* sceneCb;
   ComPtr<ID3D12RootSignature> m_raytracingLocalRootSignature;
   ComPtr<ID3D12StateObject> m_dxrStateObject;


   // Shader tables
   ComPtr<ID3D12Resource> m_missShaderTable;
   ComPtr<ID3D12Resource> m_hitGroupShaderTable;
   ComPtr<ID3D12Resource> m_rayGenShaderTable;

   // Raytracing scene
   XMVECTOR m_eye;
   XMVECTOR m_at;
   XMVECTOR m_up;


   void OnInit();

   void CreateRootSignature();

   void Execute();

   void CreateRaytracingPipelineStateObject();
   void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
   void BuildShaderTables();
};

