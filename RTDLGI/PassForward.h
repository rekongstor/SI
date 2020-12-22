#pragma once

class SceneConstBuf;

class PassForward
{
   ComPtr<ID3D12RootSignature> forwardRootSignature;
   ComPtr<ID3D12PipelineState> pipelineStateObject;

   SceneConstBuf* sceneCb;
public:
   void OnInit();
   void Execute();
};
