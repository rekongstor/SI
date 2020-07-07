struct VSInput
{
   float4 position : POSITION;
   float4 normal : NORMAL;
   float2 uv : TEXCOORD;
};

struct PSInput
{
   float4 position : SV_POSITION;
   float4 normal : NORMAL0;
   float4 view : NORMAL1;
   float2 uv : TEXCOORD;
};

struct InstanceData
{
   float4x4 world;
   float4x4 worldIt;
};


cbuffer cbPass : register(b0)
{
float4x4 vpMatrix;
float4 camPos;
float4 lightDirection;
float4 lightColor;
float4 ambientColor;
}

StructuredBuffer<InstanceData> gInstanceData : register(t0, space0);

struct quaternion
{
   float w;
   float x;
   float y;
   float z;
};

float4x4 transMatrix(float4 t)
{
   return float4x4(
      1.f, 0.f, 0.f, 0.f,
      0.f, 1.f, 0.f, 0.f,
      0.f, 0.f, 1.f, 0.f,
      t.x, t.y, t.z, 1.f
   );
}

float4x4 scaleMatrix(float4 s)
{
   return float4x4(
      s.x, 0.f, 0.f, 1.f,
      0.f, s.y, 0.f, 1.f,
      0.f, 0.f, s.z, 1.f,
      0.f, 0.f, 0.f, 1.f
   );
}

quaternion mulQV(quaternion a, float4 b)
{
   quaternion q = {
      -a.x * b.x - a.y * b.y - a.z * b.z,
      a.w * b.x + a.y * b.z - a.z * b.y,
      a.w * b.y - a.x * b.z + a.z * b.x,
      a.w * b.z + a.x * b.y - a.y * b.x
   };
   return q;
}

quaternion mulQQ(quaternion a, quaternion b)
{
   quaternion q = {
      a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
      a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
      a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
      a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w
   };
   return q;
}

quaternion revQ(quaternion q)
{
   quaternion qrev = { q.w, -q.x, -q.y, -q.z };
   return qrev;
}

PSInput main(VSInput input, uint instanceID : SV_InstanceID)
{
   PSInput output;
   InstanceData inst = gInstanceData[instanceID];

   output.position = mul(input.position, inst.world);
   output.position = mul(output.position, vpMatrix);
   output.normal = normalize(mul(normalize(input.normal), inst.worldIt));

   output.uv = input.uv;
   output.view = normalize(camPos - input.position);
   return output;
}
