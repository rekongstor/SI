#pragma once
#include <unordered_map>
#include <sstream>

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)

class rnd_DescriptorHeapMgr;
class rnd_Dx12;

namespace GlobalRootSignatureParams {
   enum Value {
      OutputViewSlot = 0,
      AccelerationStructureSlot,
      SceneConstantSlot,
      VertexBuffersSlot,
      Count
   };
}

namespace LocalRootSignatureParams {
   enum Value {
      CubeConstantSlot = 0,
      Count
   };
}


class GpuUploadBuffer
{
public:
   ComPtr<ID3D12Resource> GetResource() { return m_resource; }
   ComPtr<ID3D12Resource> m_resource;

   GpuUploadBuffer() {}
   ~GpuUploadBuffer()
   {
      if (m_resource.Get()) {
         m_resource->Unmap(0, nullptr);
      }
   }

   void Allocate(ID3D12Device* device, UINT bufferSize, LPCWSTR resourceName = nullptr)
   {
      auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

      auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
      ThrowIfFailed(device->CreateCommittedResource(
         &uploadHeapProperties,
         D3D12_HEAP_FLAG_NONE,
         &bufferDesc,
         D3D12_RESOURCE_STATE_GENERIC_READ,
         nullptr,
         IID_PPV_ARGS(&m_resource)));
      m_resource->SetName(resourceName);
   }

   uint8_t* MapCpuWriteOnly()
   {
      uint8_t* mappedData;
      // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
      CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
      ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));
      return mappedData;
   }
};

// Shader record = {{Shader ID}, {RootArguments}}
class ShaderRecord
{
public:
   ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize) :
      shaderIdentifier(pShaderIdentifier, shaderIdentifierSize)
   {
   }

   ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize, void* pLocalRootArguments, UINT localRootArgumentsSize) :
      shaderIdentifier(pShaderIdentifier, shaderIdentifierSize),
      localRootArguments(pLocalRootArguments, localRootArgumentsSize)
   {
   }

   void CopyTo(void* dest) const
   {
      uint8_t* byteDest = static_cast<uint8_t*>(dest);
      memcpy(byteDest, shaderIdentifier.ptr, shaderIdentifier.size);
      if (localRootArguments.ptr) {
         memcpy(byteDest + shaderIdentifier.size, localRootArguments.ptr, localRootArguments.size);
      }
   }

   struct PointerWithSize {
      void* ptr;
      UINT size;

      PointerWithSize() : ptr(nullptr), size(0) {}
      PointerWithSize(void* _ptr, UINT _size) : ptr(_ptr), size(_size) {};
   };
   PointerWithSize shaderIdentifier;
   PointerWithSize localRootArguments;
};

// Shader table = {{ ShaderRecord 1}, {ShaderRecord 2}, ...}
class ShaderTable : public GpuUploadBuffer
{
   uint8_t* m_mappedShaderRecords;
   UINT m_shaderRecordSize;

   // Debug support
   std::wstring m_name;
   std::vector<ShaderRecord> m_shaderRecords;

   ShaderTable() {}
public:
   ShaderTable(ID3D12Device* device, UINT numShaderRecords, UINT shaderRecordSize, LPCWSTR resourceName = nullptr)
      : m_name(resourceName)
   {
      m_shaderRecordSize = Align(shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
      m_shaderRecords.reserve(numShaderRecords);
      UINT bufferSize = numShaderRecords * m_shaderRecordSize;
      Allocate(device, bufferSize, resourceName);
      m_mappedShaderRecords = MapCpuWriteOnly();
   }

   void push_back(const ShaderRecord& shaderRecord)
   {
      ThrowIfFalse(m_shaderRecords.size() < m_shaderRecords.capacity());
      m_shaderRecords.push_back(shaderRecord);
      shaderRecord.CopyTo(m_mappedShaderRecords);
      m_mappedShaderRecords += m_shaderRecordSize;
   }

   UINT GetShaderRecordSize() { return m_shaderRecordSize; }

   // Pretty-print the shader records.
   void DebugPrint(std::unordered_map<void*, std::wstring> shaderIdToStringMap)
   {
      std::wstringstream wstr;
      wstr << L"|--------------------------------------------------------------------\n";
      wstr << L"|Shader table - " << m_name.c_str() << L": "
         << m_shaderRecordSize << L" | "
         << m_shaderRecords.size() * m_shaderRecordSize << L" bytes\n";

      for (UINT i = 0; i < m_shaderRecords.size(); i++) {
         wstr << L"| [" << i << L"]: ";
         wstr << shaderIdToStringMap[m_shaderRecords[i].shaderIdentifier.ptr] << L", ";
         wstr << m_shaderRecords[i].shaderIdentifier.size << L" + " << m_shaderRecords[i].localRootArguments.size << L" bytes \n";
      }
      wstr << L"|--------------------------------------------------------------------\n";
      wstr << L"\n";
      OutputDebugStringW(wstr.str().c_str());
   }
};

class rnd_RayTracingPipeline
{
public:
   void OnInit(rnd_Dx12* renderer);
   void UpdateCameraMatrices();
   void DoRaytracing();
   void CopyRaytracingOutputToBackbuffer();

   // DirectX Raytracing (DXR) attributes
   ComPtr<ID3D12Device5> m_dxrDevice;
   ComPtr<ID3D12GraphicsCommandList5> m_dxrCommandList;
   ComPtr<ID3D12StateObject> m_dxrStateObject;

   // Root signatures
   ComPtr<ID3D12RootSignature> m_raytracingGlobalRootSignature;
   ComPtr<ID3D12RootSignature> m_raytracingLocalRootSignature;

   struct CubeConstantBuffer
   {
      XMFLOAT4 albedo;
   };

   struct SceneConstantBuffer
   {
      XMMATRIX projectionToWorld;
      XMVECTOR cameraPosition;
      XMVECTOR lightPosition;
      XMVECTOR lightAmbientColor;
      XMVECTOR lightDiffuseColor;
   };

   // Raytracing scene
   XMVECTOR m_eye;
   XMVECTOR m_at;
   XMVECTOR m_up;
   SceneConstantBuffer m_sceneCB[2];
   CubeConstantBuffer m_cubeCB;

   // Geometry
   D3DBuffer m_indexBuffer;
   D3DBuffer m_vertexBuffer;

   // Acceleration structure
   ComPtr<ID3D12Resource> m_bottomLevelAccelerationStructure;
   ComPtr<ID3D12Resource> m_topLevelAccelerationStructure;

   // Raytracing output
   D3DBuffer raytracingOutput;

   // Shader tables
   ComPtr<ID3D12Resource> m_missShaderTable;
   ComPtr<ID3D12Resource> m_hitGroupShaderTable;
   ComPtr<ID3D12Resource> m_rayGenShaderTable;
private:
   void CreateRaytracingPipelineStateObject();
   void CreateRaytracingInterfaces();
   void CreateRootSignatures();
   void BuildGeometry();
   void BuildAccelerationStructures(rnd_Dx12* renderer);
   void CreateConstantBuffers();
   void BuildShaderTables();
   void CreateRaytracingOutputResource(rnd_Dx12* renderer);

   struct DeviceResources
   {
      ID3D12Device* device;
      ID3D12GraphicsCommandList* commandList;
      rnd_DescriptorHeapMgr* descriptorHeapMgr;
      rnd_Dx12* renderer;
   } m_deviceResources;

   union AlignedSceneConstantBuffer
   {
      SceneConstantBuffer constants;
      uint8_t alignmentPadding[D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT];
   };
   AlignedSceneConstantBuffer* m_mappedConstantData;
   ComPtr<ID3D12Resource>       m_perFrameConstants;

   void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig);
   void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
   void CreateBufferSRV(D3DBuffer* m_indexBuffer, double param2, int param3);
};