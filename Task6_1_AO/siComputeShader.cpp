#include "siComputeShader.h"
#include "siRootSignature.h"
#include "siPipelineState.h"
#include "siTexture2D.h"

void siComputeShader::onInit(siRootSignature* rootSignature, siPipelineState* pipelineState,
                             siTexture2D* output,
                             siTexture2D* input, siTexture2D* input2, D3D12_GPU_VIRTUAL_ADDRESS constBufferAddress)
{
   this->rootSignature = rootSignature;
   this->pipelineState = pipelineState;
   this->output = output;
   this->input = input;
   this->input2 = input2;
   this->constBufferAddress = constBufferAddress;
}

void siComputeShader::dispatch(ID3D12GraphicsCommandList* commandList, uint32_t width, uint32_t height, D3D12_RESOURCE_STATES outputState, D3D12_RESOURCE_STATES inputState)
{
   commandList->SetComputeRootSignature(rootSignature->get().Get());

   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
      input->getBuffer().Get(), inputState,
      D3D12_RESOURCE_STATE_GENERIC_READ));
   if (input2)
      commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
         input2->getBuffer().Get(), inputState,
         D3D12_RESOURCE_STATE_GENERIC_READ));

   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
      output->getBuffer().Get(), outputState,
      D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

   commandList->SetPipelineState(pipelineState->getPipelineState().Get());
   uint32_t slot = 0;
   commandList->SetGraphicsRootConstantBufferView(slot++, constBufferAddress);
   commandList->SetComputeRootDescriptorTable(slot++, output->getUavHandle().second);
   commandList->SetComputeRootDescriptorTable(slot++, input->getSrvHandle().second);

   commandList->Dispatch(static_cast<UINT>(ceilf(width / 8.f)), static_cast<UINT>(ceilf(height / 8.f)), 1);


   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
      output->getBuffer().Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
      outputState));

   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
      input->getBuffer().Get(), D3D12_RESOURCE_STATE_GENERIC_READ,
      inputState));
   if (input2)
      commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
         input2->getBuffer().Get(), D3D12_RESOURCE_STATE_GENERIC_READ,
         inputState));
}
