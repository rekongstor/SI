#pragma once
namespace GlobalRootSignatureParams
{
   enum
   {
      OutputViewSlot = 0,
      AccelerationStructureSlot,
      SceneConstantSlot,
      VertexBuffersSlot,
      Count
   };
}

namespace LocalRootSignatureParams
{
   enum
   {
      CubeConstantSlot = 0,
      Count
   };
}


enum class RngType
{
   SRV = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
   UAV = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
   CBV = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
   SAMPLER = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER
};


struct DescRange : D3D12_DESCRIPTOR_RANGE
{
   DescRange(RngType rangeType, UINT numDescriptors, UINT baseShaderRegister,
      UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND) :
      D3D12_DESCRIPTOR_RANGE({ static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(rangeType), numDescriptors, baseShaderRegister, registerSpace, offsetInDescriptorsFromTableStart }) {}
};

struct DescTable : D3D12_ROOT_PARAMETER
{
   // Descriptor table
   DescTable(std::initializer_list<DescRange> descRanges, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) :
      D3D12_ROOT_PARAMETER({ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, {}, visibility })
   {
      DescriptorTable.NumDescriptorRanges = descRanges.size();
      DescriptorTable.pDescriptorRanges = descRanges.begin();
   }
   // TODO: Need to additionally implement std::vector to save DescRanges?
};


struct Const : D3D12_ROOT_PARAMETER
{
   Const(UINT num32BitValues, UINT shaderRegister, UINT registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) :
      D3D12_ROOT_PARAMETER({ D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS, {}, visibility })
   {
      Constants.Num32BitValues = num32BitValues;
      Constants.ShaderRegister = shaderRegister;
      Constants.RegisterSpace = registerSpace;
   }
};


struct SRV : D3D12_ROOT_PARAMETER
{
   SRV(UINT shaderRegister, UINT registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) :
      D3D12_ROOT_PARAMETER({ D3D12_ROOT_PARAMETER_TYPE_SRV, {}, visibility })
   {
      Descriptor.ShaderRegister = shaderRegister;
      Descriptor.RegisterSpace = registerSpace;
   }
};


struct CBV : D3D12_ROOT_PARAMETER
{
   CBV(UINT shaderRegister, UINT registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) :
      D3D12_ROOT_PARAMETER({ D3D12_ROOT_PARAMETER_TYPE_CBV, {}, visibility })
   {
      Descriptor.ShaderRegister = shaderRegister;
      Descriptor.RegisterSpace = registerSpace;
   }
};


struct UAV : D3D12_ROOT_PARAMETER
{
   UAV(UINT shaderRegister, UINT registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) :
      D3D12_ROOT_PARAMETER({ D3D12_ROOT_PARAMETER_TYPE_UAV, {}, visibility })
   {
      Descriptor.ShaderRegister = shaderRegister;
      Descriptor.RegisterSpace = registerSpace;
   }
};


class rnd_RootSignatureMgr
{
public:

   void OnInit();
   void SetRootSignature();
   ID3D12RootSignature* CreateRootSignature(std::initializer_list<D3D12_ROOT_PARAMETER> parameters, std::initializer_list<D3D12_STATIC_SAMPLER_DESC> staticSamplers = {}, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
};
