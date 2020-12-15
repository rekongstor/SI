#pragma once
// Adding new ConstBuffers:
// 1) Add class using template from CB
// 2) Modify struct for your buffer
// 3) Change ConstBufferInitializer. This will be used as a key in rnd_ConstantBufferMgr and as a name in D3D. You can initialize multiple buffers
// 4) Add include to rnd_ConstantBufferMgr
// 5) You can make ConstBuffer immutable by inheriting from rnd_ImmutableConstBuffer<struct>
class rnd_ConstantBuffer
{
public:
   const char* name;
   ComPtr<ID3D12Resource> buffer;

   virtual void OnInit(LPCWSTR name);
   virtual void Update();
};

template<class T>
class rnd_ImmutableConstBuffer : public rnd_ConstantBuffer
{
public:
   void OnInit(LPCWSTR name) = 0;
   union AlignedData
   {
      T buffer;
      uint8_t alignment[AlignConst(sizeof(T), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)];
   };
   AlignedData* mappedData;
};


class rnd_ConstBufferInitializer
{
public:
   LPCWSTR name;
   rnd_ConstantBuffer* pBuffer;

   rnd_ConstBufferInitializer(LPCWSTR name, rnd_ConstantBuffer* pBuffer);
};

template<class T>
class ConstBufInitializer : public rnd_ConstBufferInitializer
{
public:
      T constBuf;

   ConstBufInitializer(LPCWSTR name) : rnd_ConstBufferInitializer(name, &constBuf) {}
};


extern std::vector<rnd_ConstBufferInitializer*> cbsNames;