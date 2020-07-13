#pragma once
#include "siConstBuffer.h"
class siPipelineState;
class siRootSignature;
class siTexture2D;

template <class T>
class siComputeShader
{
   siConstBuffer<T> constBuffer;
   siRootSignature* rootSignature = nullptr;
   siPipelineState* pipelineState = nullptr;
   siTexture2D* output = nullptr;
   siTexture2D* input = nullptr;
   siTexture2D* input2 = nullptr;

public:
   void onInit(siConstBuffer<T> buffer, siRootSignature* rootSignature, siPipelineState* pipelineState,
               siTexture2D* output, siTexture2D* input,
               siTexture2D* input2 = nullptr);
   void dispatch(ID3D12GraphicsCommandList* commandList, uint32_t width, uint32_t height);

   [[nodiscard]] const siConstBuffer<T>& getConstBuffer() const { return constBuffer; }
   void constBuffer1(const siConstBuffer<T>& constBuffer) { this->constBuffer = constBuffer; }
};

template <class T>
void siComputeShader<T>::onInit(siConstBuffer<T> buffer, siRootSignature* rootSignature, siPipelineState* pipelineState,
                                siTexture2D* output,
                                siTexture2D* input, siTexture2D* input2)
{
   this->constBuffer = buffer;
   this->rootSignature = rootSignature;
   this->pipelineState = pipelineState;
   this->output = output;
   this->input = input;
   this->input2 = input2;
}

template <class T>
void siComputeShader<T>::dispatch(ID3D12GraphicsCommandList* commandList, uint32_t width, uint32_t height)
{
   commandList->SetComputeRootSignature(rootSignature->get().Get());

   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                   input->getBuffer().Get(), D3D12_RESOURCE_STATE_COMMON,
                                   D3D12_RESOURCE_STATE_GENERIC_READ));

   if (input2)
      commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                      input2->getBuffer().Get(), D3D12_RESOURCE_STATE_COMMON,
                                      D3D12_RESOURCE_STATE_GENERIC_READ));
   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                   output->getBuffer().Get(), D3D12_RESOURCE_STATE_COMMON,
                                   D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

   commandList->SetPipelineState(pipelineState->getPipelineState().Get());
   commandList->SetGraphicsRootConstantBufferView(0, constBuffer.getGpuVirtualAddress());
   commandList->SetComputeRootDescriptorTable(1, output->getUavHandle().second);
   commandList->SetComputeRootDescriptorTable(2, input->getSrvHandle().second);
   if (input2)
      commandList->SetComputeRootDescriptorTable(3, input2->getSrvHandle().second);

   commandList->Dispatch(static_cast<UINT>(ceilf(width / 256.f)), height, 1);


   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                   output->getBuffer().Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                   D3D12_RESOURCE_STATE_PRESENT));

   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                   input->getBuffer().Get(), D3D12_RESOURCE_STATE_GENERIC_READ,
                                   D3D12_RESOURCE_STATE_COMMON));
   if (input2)
      commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                      input2->getBuffer().Get(), D3D12_RESOURCE_STATE_GENERIC_READ,
                                      D3D12_RESOURCE_STATE_COMMON));
}
