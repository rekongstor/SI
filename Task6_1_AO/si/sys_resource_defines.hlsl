#ifndef SYS_RESOURCE_DEFINES_FX
#define SYS_RESOURCE_DEFINES_FX

#ifndef __cplusplus
#include "system.hlsl"
#endif

// ----------------------------------------------------------------------------
//
// Sets and binding points
//
// ----------------------------------------------------------------------------
#define PER_FRAME_SET 0
#define PER_DRAW_CB_SET 1
#define PER_DRAW_TX_SET 1

// vulkan stuff. BINDING_NO_OFFSET for uniform buffer. BINDING_DYNAMIC_OFFSET for dynamic uniform buffer 
#define BINDING_NO_OFFSET   0  
#define BINDING_DYNAMIC_OFFSET 1  

#ifdef _AP_NX64
   #define TEXTURE_BINDING_LOCATION        13
   #define CONST_BUFFER_BINDING_LOCATION   0   // max 14 (MAX_UNIFORM_BUFFER_BINDINGS)
   #define UAV_BINDING_LOCATION            0   // max 16
   #define SAMPLER_BINDING_LOCATION        1   // max 32
   
   #define SKINDATA_BINDING_LOCATION       15
   #define TEXOBJID_BINDING_LOCATION       3
#else // for cpp and other platforms
   /*
   #define TEXTURE_BINDING_LOCATION        0
   #define CONST_BUFFER_BINDING_LOCATION   120  // 14 const buffers
   #define UAV_BINDING_LOCATION            134
   #define SAMPLER_BINDING_LOCATION        148
   */
  
   #define SAMPLER_BINDING_LOCATION        0
   #define CONST_BUFFER_BINDING_LOCATION   120  // 14 const buffers
   #define UAV_BINDING_LOCATION            134
   #define UAV_COUNTER_BINDING_LOCATION    148
   #define TEXTURE_BINDING_LOCATION        162
   #define BINDING_LOCATION_TOTAL          1024
   
   
   #define SKINDATA_BINDING_LOCATION       111  // texture
   #define TEXOBJID_BINDING_LOCATION       3    // uav texture
#endif


// ----------------------------------------------------------------------------
//
// Reflection info
//
// ----------------------------------------------------------------------------

// Resourse type
#ifdef __cplusplus
   enum ResourceType{
   // Samplers
      SAMPLER,
      SAMPLER_CMP,
   // Buffers
      CONST_BUFFER,
      INSTANCED_CONST_BUFFER,
      REGULAR_BUFFER,
      REGULAR_STRUCTURED_BUFFER,
      TEX_BUFFER,
      TEX_STRUCTURED_BUFFER,
      INSTANCED_RESOURCE_BUFFER,
      UAV_BUFFER,
      UAV_BUFFER_COUNTER,
      UAV_STRUCTURED_BUFFER,
      UAV_STRUCTURED_BUFFER_COUNTER,
   //SRV textures
      SRV_TEX_1D,
      SRV_TEX_2D,
      SRV_TEX_2D_UINT,
      SRV_TEX_2D_ARRAY,
      SRV_TEX_3D,
      SRV_TEX_3D_UINT,
      SRV_TEX_CUBE,
      SRV_TEX_CUBE_ARRAY,
   //UAV textures
      UAV_TEXTURE,
   //unused type
      LAST_TYPE
   };

   enum {
      USAGE_VS = 0x1,
      USAGE_PS = 0x2,
      USAGE_GS = 0x4,
      USAGE_CS = 0x8,
      USAGE_DS = 0x10,
      USAGE_HS = 0x20,
      USAGE_ESLS_BY_SHADER = 0x40
   };

   struct __REFLECTION_INFO;
   void __RegisterReflectionInfo (const __REFLECTION_INFO * info); 
   void __FillConstantBufferInformation (const __REFLECTION_INFO * info);
   void __RegisterCommonConstantBufferForPass (int passId, int bufInd);
   void __RegisterCommonTextureForPass (int passId, int texInd);
   
   struct __REFLECTION_INFO {
      static __REFLECTION_INFO * pLastCreatedInfo; // need for cb size
      
      ResourceType type; // = TYPE
      const char * name; // = #TEX_ID
      const int    pass; // pass this resource should bind to
	   int        _register; // = IDX (binding slot)
	   const int   set;   // = SET (per frame OR per draw)
      int        size;  // = SIZE
      int        usage; // = USAGE (type of using shaders)
      const int  bind;  // = BIND (bind with or without offset)
                
      __REFLECTION_INFO(void) : type(LAST_TYPE), pass(-1), set(-1), bind(BINDING_NO_OFFSET) {}
	   __REFLECTION_INFO(ResourceType _type, const char *_name, int _pass, int __register, int _set, int _size, int _usage, int _bind, bool _isDebugResourse = false) :
         type(_type),
         name(_name),
         pass(_pass),
         _register(__register),
         set(_set),
         size(_size),
         usage(_usage),
         bind(_bind)
         {
            __RegisterReflectionInfo(this);
            pLastCreatedInfo = this;
         }
         
                  
      int GetSlot (void) const {
         switch (type) {
            case SAMPLER:
            case SAMPLER_CMP:
               return _register + SAMPLER_BINDING_LOCATION;
            case CONST_BUFFER:
            case INSTANCED_CONST_BUFFER:
               return _register + CONST_BUFFER_BINDING_LOCATION;
            case REGULAR_BUFFER:
            case REGULAR_STRUCTURED_BUFFER:
            case TEX_BUFFER:
            case TEX_STRUCTURED_BUFFER:
               return _register + TEXTURE_BINDING_LOCATION;
            case UAV_BUFFER:
            case UAV_BUFFER_COUNTER:
            case UAV_STRUCTURED_BUFFER:
            case UAV_STRUCTURED_BUFFER_COUNTER:
               return _register + UAV_BINDING_LOCATION;
            case SRV_TEX_1D:
            case SRV_TEX_2D:
            case SRV_TEX_2D_UINT:
            case SRV_TEX_2D_ARRAY:
            case SRV_TEX_3D:
            case SRV_TEX_3D_UINT:
            case SRV_TEX_CUBE:
            case SRV_TEX_CUBE_ARRAY:
               return _register + TEXTURE_BINDING_LOCATION;
            case UAV_TEXTURE:
               return _register + UAV_BINDING_LOCATION;
         }
         ASSERT(false);
         return -1;
      }
      
      bool operator== (const __REFLECTION_INFO & info) const {
         bool equal = true;
         equal &= (type == info.type);
         equal &= (_register == info._register);
         equal &= (set == info.set);
         equal &= (size == info.size);
         equal &= (bind == info.bind);
         return equal;
      } 
   };
   
   struct __PROXY_REFLECTION_INFO_UPDATE_SIZE {
      __PROXY_REFLECTION_INFO_UPDATE_SIZE (int _size) {__REFLECTION_INFO::pLastCreatedInfo->size += _size;}
   };
   
   struct __PROXY_REFLECTION_INFO_FILL_CONST_BUF {
      __PROXY_REFLECTION_INFO_FILL_CONST_BUF (void) {__FillConstantBufferInformation(__REFLECTION_INFO::pLastCreatedInfo);}
   };
   
   struct __PROXY_REFLECTION_INFO_ADD_USED_CB {
      __PROXY_REFLECTION_INFO_ADD_USED_CB (int passId, int bufInd) {
            __RegisterCommonConstantBufferForPass(passId, bufInd);
         }
   };

   struct __PROXY_REFLECTION_INFO_ADD_USED_TEX {
      __PROXY_REFLECTION_INFO_ADD_USED_TEX(int passId, int texInd) {
         __RegisterCommonTextureForPass(passId, texInd);
      }
   };

   #ifdef __PLACE_REFLECTION_INFO
      #define MAKE_UNIQUE_NAME3(LINE) _unique_ ## LINE
      #define MAKE_UNIQUE_NAME2(LINE) MAKE_UNIQUE_NAME3(LINE)
      
      // AAV: "Unique" here is not actually unique, it is just a generated namespace name
      // It never was unique, as __LINE__ wasn't unwind here by preprocessor
      // More than that, actual unique could prevent us from sharing textures between two passes
      #define MAKE_UNIQUE_NAME(NAME)  _unique_ ## NAME
      
      #define BEGIN_REFLECTION_TABLE_COMMON\
         namespace MAKE_UNIQUE_NAME(COMMON)\
         { static const int ___PASS___ = VID_PASS_LAST;
      #define BEGIN_REFLECTION_TABLE(PASS)\
         namespace MAKE_UNIQUE_NAME(PASS)\
         { static const int ___PASS___ = VID_PASS_##PASS;         
      #define END_REFLECTION_TABLE    }

      #define ADD_REFLECTION_INFO(TYPE, NAME, IDX, SET, SIZE, USAGE, BIND)\
         static __REFLECTION_INFO __REFLECTION_INFO_##NAME (TYPE, #NAME, ___PASS___, IDX, SET, SIZE, USAGE, BIND);
      
      #define ADD_BUFFER_SIZE(NAME, SIZE)\
         static __PROXY_REFLECTION_INFO_UPDATE_SIZE __PROXY_REFLECTION_INFO_UPDATE_SIZE_##NAME (SIZE)
      #define FILL_REFLECTION_BUFFER_INFO(NAME) FILL_REFLECTION_BUFFER_INFO2(NAME)
      #define FILL_REFLECTION_BUFFER_INFO2(NAME)\
         static __PROXY_REFLECTION_INFO_FILL_CONST_BUF __PROXY_REFLECTION_INFO_FILL_CONST_BUF_ ## NAME
         
      #define DECLARE_USED_COMMON_CONST_BUFFER(BUFFER)\
         static __PROXY_REFLECTION_INFO_ADD_USED_CB __PROXY_REFLECTION_INFO_ADD_USED_CB_##BUFFER (___PASS___, BUFFER);
         
      #define DECLARE_USED_TEXTURE(DECL_PASS, TEXTURE)\
         static __PROXY_REFLECTION_INFO_ADD_USED_TEX __PROXY_REFLECTION_INFO_ADD_USED_TEX_##TEXTURE (___PASS___, MAKE_UNIQUE_NAME(DECL_PASS)::PS_##TEXTURE##_TEX);
         
      #define DECLARE_USED_COMMON_TEXTURE(TEXTURE)\
         static __PROXY_REFLECTION_INFO_ADD_USED_TEX __PROXY_REFLECTION_INFO_ADD_USED_TEX_##TEXTURE (___PASS___, MAKE_UNIQUE_NAME(COMMON)::PS_##TEXTURE##_TEX);
   #else 
      #define BEGIN_REFLECTION_TABLE_COMMON
      #define BEGIN_REFLECTION_TABLE(PASS)
      #define END_REFLECTION_TABLE 
      #define ADD_REFLECTION_INFO(TYPE, NAME, IDX, SET, SIZE, USAGE, BIND)
      #define ADD_BUFFER_SIZE(NAME, SIZE)
      #define FILL_REFLECTION_BUFFER_INFO(NAME)
      #define DECLARE_USED_COMMON_CONST_BUFFER(BUFFER)
      #define DECLARE_USED_TEXTURE(PASS, TEXTURE)
      #define DECLARE_USED_COMMON_TEXTURE(TEXTURE)
   #endif

#else
      #define BEGIN_REFLECTION_TABLE_COMMON
      #define BEGIN_REFLECTION_TABLE(PASS)
      #define END_REFLECTION_TABLE 
      #define ADD_REFLECTION_INFO(TYPE, NAME, IDX, SET, SIZE, USAGE, BIND)
      #define ADD_BUFFER_SIZE(NAME, SIZE)
      #define FILL_REFLECTION_BUFFER_INFO(NAME)
      #define DECLARE_USED_COMMON_CONST_BUFFER(BUFFER)
      #define DECLARE_USED_TEXTURE(PASS, TEXTURE)
      #define DECLARE_USED_COMMON_TEXTURE(TEXTURE)
#endif


// ----------------------------------------------------------------------------
//
// Constant buffers
//
// ----------------------------------------------------------------------------
#define STATIC_CB_REGISTER  3
#define DYNAMIC_CB_REGISTER 4

#define MAX_INSTANCES_PER_CB  30 // limited mainly by bone data
#define DEFAULT_USAGE (USAGE_VS | USAGE_PS)

#ifdef __cplusplus
   #define DECLARE_REGULAR_BUFFER(BUF_NAME, IDX, SET, SIZE)\
      ADD_REFLECTION_INFO(REGULAR_BUFFER, BUF_NAME, IDX, SET, SIZE, DEFAULT_USAGE, BINDING_NO_OFFSET); FILL_REFLECTION_BUFFER_INFO(__LINE__)
   #define DECLARE_REGULAR_BUFFER_USAGE(BUF_NAME, IDX, SET, SIZE, USAGE)\
      ADD_REFLECTION_INFO(REGULAR_BUFFER, BUF_NAME, IDX, SET, SIZE, USAGE, BINDING_NO_OFFSET); FILL_REFLECTION_BUFFER_INFO(__LINE__)
   
   #define CONST_BUFFER_BEGIN(BUF_NAME, IDX, SET)                    ADD_REFLECTION_INFO(CONST_BUFFER, BUF_NAME,                 IDX,             SET, 0, DEFAULT_USAGE, BINDING_NO_OFFSET)
   #define CONST_BUFFER_BEGIN_USAGE(BUF_NAME, IDX, SET, USAGE, BIND) ADD_REFLECTION_INFO(CONST_BUFFER, BUF_NAME,                 IDX,             SET, 0,         USAGE, BIND)
   #define CONST_BUFFER_STATIC_BEGIN(BUF_NAME)                       ADD_REFLECTION_INFO(CONST_BUFFER, BUF_NAME,  STATIC_CB_REGISTER, PER_DRAW_CB_SET, 0, DEFAULT_USAGE, BINDING_NO_OFFSET)
   #define CONST_BUFFER_STATIC_BEGIN_USAGE(BUF_NAME, USAGE)          ADD_REFLECTION_INFO(CONST_BUFFER, BUF_NAME,  STATIC_CB_REGISTER, PER_DRAW_CB_SET, 0,         USAGE, BINDING_NO_OFFSET)
   #define CONST_BUFFER_DYNAMIC_BEGIN(BUF_NAME)                      ADD_REFLECTION_INFO(CONST_BUFFER, BUF_NAME, DYNAMIC_CB_REGISTER, PER_DRAW_CB_SET, 0, DEFAULT_USAGE, BINDING_DYNAMIC_OFFSET)
   #define CONST_BUFFER_DYNAMIC_BEGIN_USAGE(BUF_NAME, USAGE)         ADD_REFLECTION_INFO(CONST_BUFFER, BUF_NAME, DYNAMIC_CB_REGISTER, PER_DRAW_CB_SET, 0,         USAGE, BINDING_DYNAMIC_OFFSET)
   //used for calculate CB size
   #define DECLARE_CONST(CONST_NAME, SIZE)                      ADD_BUFFER_SIZE(CONST_NAME, SIZE)
   #define DECLARE_CONSTI(CONST_NAME, SIZE)                     ADD_BUFFER_SIZE(CONST_NAME, SIZE)
   #define CONST_BUFFER_END                                     FILL_REFLECTION_BUFFER_INFO(__LINE__)
   
   #define INSTANCED_CONST_BUFFER_BEGIN(BUF_NAME, IDX, SET)                    ADD_REFLECTION_INFO(INSTANCED_CONST_BUFFER, BUF_NAME, IDX, SET, 0, DEFAULT_USAGE, BINDING_NO_OFFSET)
   #define INSTANCED_CONST_BUFFER_BEGIN_USAGE(BUF_NAME, IDX, SET, USAGE, BIND) ADD_REFLECTION_INFO(INSTANCED_CONST_BUFFER, BUF_NAME, IDX, SET, 0,         USAGE, BIND)
   #define INSTANCED_CONST_BUFFER_DYNAMIC_BEGIN(BUF_NAME)                      ADD_REFLECTION_INFO(INSTANCED_CONST_BUFFER, BUF_NAME, DYNAMIC_CB_REGISTER, PER_DRAW_CB_SET, 0, DEFAULT_USAGE, BINDING_DYNAMIC_OFFSET) 
   #define INSTANCED_CONST_BUFFER_END(BUF_NAME, IDX, SET)                      FILL_REFLECTION_BUFFER_INFO(__LINE__)      
      
   #define INSTANCED_STRUCT_BUFFER_BEGIN(BUF_NAME, IDX, SET)                   ADD_REFLECTION_INFO(INSTANCED_RESOURCE_BUFFER, BUF_NAME, IDX, SET, 0, DEFAULT_USAGE, BINDING_NO_OFFSET)
   #define DECLARE_STRUCT_FIELD(CONST_NAME, SIZE)                              ADD_BUFFER_SIZE(CONST_NAME, SIZE)
   #define INSTANCED_STRUCT_BUFFER_END(BUF_NAME, IDX, SET)                     FILL_REFLECTION_BUFFER_INFO(__LINE__)      
   
   #define PS_TEX_TFF(TEX_NAME) PS_##TEX_NAME##_TEX, PS_##TEX_NAME##_TFF    
   #define PS_TEX(TEX_NAME) PS_##TEX_NAME##_TEX
#else
   #ifdef _AP_ORBIS
	   #define CONSTANT_BUFFER	ConstantBuffer
   #elif defined(_AP_NX64)
	   #define CONSTANT_BUFFER uniform
   #else
	   #define CONSTANT_BUFFER cbuffer
   #endif
   
   #ifdef _API_VULKAN
      #define CONST_BUFFER_BEGIN(BUF_NAME, IDX, SET) \
         [[vk::binding(CONST_BUFFER_BINDING_LOCATION + IDX, SET)]] CONSTANT_BUFFER BUF_NAME : register(b##IDX) {
   #elif defined(_AP_NX64)
      #define CONST_BUFFER_BEGIN(BUF_NAME, IDX, SET) \
         layout(binding=IDX, row_major, std140) CONSTANT_BUFFER BUF_NAME {
   #else
      #define CONST_BUFFER_BEGIN(BUF_NAME, IDX, SET) \
         CONSTANT_BUFFER BUF_NAME : register(b##IDX) {
   #endif
   
   #define CONST_BUFFER_BEGIN_USAGE(BUF_NAME, IDX, SET, USAGE, BIND) \
      CONST_BUFFER_BEGIN(BUF_NAME, IDX, SET)
   #define CONST_BUFFER_STATIC_BEGIN(BUF_NAME) \
      CONST_BUFFER_BEGIN(BUF_NAME, STATIC_CB_REGISTER, PER_DRAW_CB_SET)
   #define CONST_BUFFER_STATIC_BEGIN_USAGE(BUF_NAME,USAGE) \
      CONST_BUFFER_BEGIN_USAGE(BUF_NAME, STATIC_CB_REGISTER, PER_DRAW_CB_SET, USAGE, BINDING_NO_OFFSET)
   #define CONST_BUFFER_DYNAMIC_BEGIN(BUF_NAME) \
      CONST_BUFFER_BEGIN(BUF_NAME, DYNAMIC_CB_REGISTER, PER_DRAW_CB_SET)
   #define CONST_BUFFER_DYNAMIC_BEGIN_USAGE(BUF_NAME, USAGE) \
      CONST_BUFFER_BEGIN_USAGE(BUF_NAME, DYNAMIC_CB_REGISTER, PER_DRAW_CB_SET, USAGE, BINDING_DYNAMIC_OFFSET)
   #define DECLARE_CONST(CONST_NAME, SIZE) \
      float4 CONST_NAME[SIZE]
   #define DECLARE_CONSTI(CONST_NAME, SIZE) \
      int4   CONST_NAME[SIZE]
   #define CONST_BUFFER_END }

   #ifdef _API_VULKAN
      #define DECLARE_REGULAR_BUFFER(BUF_NAME, IDX, SET, SIZE) \
         [[vk::binding(TEXTURE_BINDING_LOCATION + IDX, SET)]] Buffer<float4> BUF_NAME : register(t##IDX)
   #elif defined(_AP_NX64)
      #define DECLARE_REGULAR_BUFFER(BUF_NAME, IDX, SET, SIZE) \
         layout(binding=IDX, std140) buffer BUF_NAME##_BUFFER { float4 BUF_NAME[SIZE]; }
   #else
      #define DECLARE_REGULAR_BUFFER(BUF_NAME, IDX, SET, SIZE) \
         Buffer<float4> BUF_NAME : register(t##IDX)
   #endif
   
   #define DECLARE_REGULAR_BUFFER_USAGE(BUF_NAME, IDX, SET, SIZE, USAGE) \
      DECLARE_REGULAR_BUFFER(BUF_NAME, IDX, SET, SIZE)

   #if defined(_API_VULKAN) 
      #define INSTANCED_CONST_BUFFER_BEGIN(BUF_NAME, IDX, SET) \
         [[vk::binding(CONST_BUFFER_BINDING_LOCATION + IDX, SET)]] CONSTANT_BUFFER BUF_NAME : register(b##IDX) { struct T_##BUF_NAME {
   #elif defined(_AP_NX64)
      #define INSTANCED_CONST_BUFFER_BEGIN(BUF_NAME, IDX, SET) \
         struct T_##BUF_NAME {
   #else
      #define INSTANCED_CONST_BUFFER_BEGIN(BUF_NAME, IDX, SET) \
         CONSTANT_BUFFER BUF_NAME : register(b##IDX) { struct T_##BUF_NAME {
   #endif
   
   #define INSTANCED_CONST_BUFFER_BEGIN_USAGE(BUF_NAME, IDX, SET, USAGE, BIND) \
      INSTANCED_CONST_BUFFER_BEGIN(BUF_NAME, IDX, SET)
   #define INSTANCED_CONST_BUFFER_DYNAMIC_BEGIN(BUF_NAME) \
      INSTANCED_CONST_BUFFER_BEGIN(BUF_NAME, DYNAMIC_CB_REGISTER, PER_DRAW_CB_SET)
      
   #if defined(_AP_ORBIS) // Research why other branch don't work for Orbis.
      #define INST_REGS(BUF_NAME, CONST, IDX)   (BUF_NAME##_slice.CONST[IDX])
      #define INST_REGS_ARR(BUF_NAME, CONST)    (BUF_NAME##_slice.CONST)

      #ifdef COMMON_INSTANCED_CB
         #define INSTANCED_CONST_BUFFER_END(BUF_NAME, IDX, SET) \
            } BUF_NAME##_data[MAX_INSTANCES_PER_CB]; }; T_##BUF_NAME BUF_NAME##_slice;

         #define LOAD_CB_INSTANCE_DATA(BUF_NAME, id) BUF_NAME##_slice = BUF_NAME##_data[id]
         #define FORWARD_INSTANCE_ID(id, fwdId) fwdId = id

      #else
         #define INSTANCED_CONST_BUFFER_END(BUF_NAME, IDX, SET) \
            } BUF_NAME##_slice; };
         
         #define LOAD_CB_INSTANCE_DATA(BUF_NAME, id)
         #define FORWARD_INSTANCE_ID(id, fwdId)
      #endif
      
   #else
      #define INST_REGS(BUF_NAME, CONST, IDX) \
         (BUF_NAME##_data[BUF_NAME##_sliceID].CONST[IDX])
      #define INST_REGS_ARR(BUF_NAME, CONST) \
         (BUF_NAME##_data[BUF_NAME##_sliceID].CONST)

      #ifdef COMMON_INSTANCED_CB
         #ifdef _AP_NX64
            #define INSTANCED_CONST_BUFFER_END(BUF_NAME, IDX, SET) \
               }; layout(binding=IDX, row_major, std140) CONSTANT_BUFFER BUF_NAME { T_##BUF_NAME BUF_NAME##_data[MAX_INSTANCES_PER_CB]; }; \
               uint BUF_NAME##_sliceID         
         #else
            #define INSTANCED_CONST_BUFFER_END(BUF_NAME, IDX, SET) \
               } BUF_NAME##_data[MAX_INSTANCES_PER_CB]; }; \
               static uint BUF_NAME##_sliceID;
         #endif

         #define LOAD_CB_INSTANCE_DATA(BUF_NAME, id) BUF_NAME##_sliceID = id
         #define FORWARD_INSTANCE_ID(id, fwdId) fwdId = id

      #else
         #ifdef _AP_NX64
            #define INSTANCED_CONST_BUFFER_END(BUF_NAME, IDX, SET) \
               }; layout(binding=IDX, row_major, std140) CONSTANT_BUFFER BUF_NAME { T_##BUF_NAME BUF_NAME##_data[1]; }; \
               const uint BUF_NAME##_sliceID = 0
         #else
            #define INSTANCED_CONST_BUFFER_END(BUF_NAME, IDX, SET) \
               } BUF_NAME##_data[1]; }; \
               static const uint BUF_NAME##_sliceID = 0;
         #endif
         
         #define LOAD_CB_INSTANCE_DATA(BUF_NAME, id)
         #define FORWARD_INSTANCE_ID(id, fwdId)
      #endif
   #endif
   
   #define FORWARD_DATA(id, fwdId) fwdId = id   
#endif


// INSTANCED_RESOURCE_BUFFER

#ifdef __cplusplus
enum class INST_RES_BUFFER_ID {
   OBJ_DATA = 0,
   COUNT
};

enum class INST_RES_BUFFER_DATA {
   BUFFER_STRIDE = 0,
   MAX_PERSISTENT_INST,
   MAX_PER_FRAME_INST,
   COUNT
};

struct INST_RES_BUFFER_TRAITS {
   int structSize;
   int 
}
#endif

#define INSTANCED_RES_BUFFER_STRUCT_BEGIN(DATA_TYPE)              struct DATA_TYPE##_STRUCT {
   
#ifdef __cplusplus
   #define DECLARE_STRUCT_FIELD(FIELD_NAME)                       m4dV   FIELD_NAME;
   #define DECLARE_STRUCT_FIELD_ARRAY(FIELD_NAME, COUNT)          m4dV   FIELD_NAME[COUNT];
   
   #define INSTANCED_RES_BUFFER_STRUCT_END(DATA_TYPE, MAX_INST_COUNT_DYN, MAX_INST_COUNT_STAT)                          \
                                                                  };                                                    \   
                                                                  static DATA_TYPE_TRAITS
   
   
#else
   #define DECLARE_STRUCT_FIELD(FIELD_NAME)                       float4 FIELD_NAME;
   #define DECLARE_STRUCT_FIELD_ARRAY(FIELD_NAME, COUNT)          float4 FIELD_NAME[COUNT];
   
   #define INSTANCED_RES_BUFFER_STRUCT_END(DATA_TYPE, MAX_INST_COUNT_DYN, MAX_INST_COUNT_STAT)                          \
                                                                  };                                                    \
                                                                  static DATA_TYPE##_STRUCT DATA_TYPE##_BUFFER_INST_DATA;
   
   #define LOAD_RB_INST_DATA(BUF_NAME, DATA_IDX)                  BUF_NAME##_INST_DATA = BUF_NAME[DATA_IDX]
   #define GET_INST_DATA(BUF_NAME)                                BUF_NAME##_INST_DATA
#endif


// ----------------------------------------------------------------------------
//
// Samplers
//
// ----------------------------------------------------------------------------
#define PS_SMP_WRAP_POINT    0
#define PS_SMP_WRAP_LINEAR   1
#define PS_SMP_WRAP_ANISO    2
#define PS_SMP_CLAMP_POINT   3
#define PS_SMP_CLAMP_LINEAR  4
#define PS_SMP_CLAMP_ANISO   5
#define PS_SMP_BORDER_POINT  6
#define PS_SMP_BORDER_LINEAR 7
#define PS_SMP_MIRROR_LINEAR 8
#define PS_SMP_MIRROR_ANISO  9
#define MAX_PS_SAMPLERS      10

#define PS_SMP_BORDER_CMP    0 // MAX_PS_SAMPLERS+0
#define PS_SMP_CLAMP_CMP     1 // MAX_PS_SAMPLERS+1
#define MAX_PS_CMP_SAMPLERS  2

#define _MAKE_SMP_ID(ID)          PS_##ID##_SMP

#ifdef __cplusplus
   #define DECLARE_SAMPLERS(SAM_ID, IDX, SET, SIZE)      ADD_REFLECTION_INFO(SAMPLER, SAMPLER, IDX, SET, SIZE, DEFAULT_USAGE, BINDING_NO_OFFSET)
   #define DECLARE_SAMPLERS_CMP(SAM_ID, IDX, SET, SIZE)  ADD_REFLECTION_INFO(SAMPLER_CMP, SAMPLER_CMP, IDX, SET, SIZE, DEFAULT_USAGE, BINDING_NO_OFFSET)
#else           
   #ifdef _API_VULKAN
      #define DECLARE_SAMPLERS(SAM_ID, IDX, SET, SIZE) [[vk::binding(SAMPLER_BINDING_LOCATION + IDX, SET)]] SamplerState SAM_ID[SIZE] : register(s##IDX)
      #define DECLARE_SAMPLERS_CMP(SAM_ID, IDX, SET, SIZE) [[vk::binding(SAMPLER_BINDING_LOCATION + IDX, SET)]] SamplerComparisonState SAM_ID[SIZE] : register(s##IDX);
   #elif defined(_AP_NX64)
      #define DECLARE_SAMPLERS(SAM_ID, IDX, SET, SIZE)
      #define DECLARE_SAMPLERS_CMP(SAM_ID, IDX, SET, SIZE)
   #else
      #define DECLARE_SAMPLERS(SAM_ID, IDX, SET, SIZE) SamplerState SAM_ID[SIZE] : register(s##IDX)
      #define DECLARE_SAMPLERS_CMP(SAM_ID, IDX, SET, SIZE) SamplerComparisonState SAM_ID[SIZE] : register(s##IDX);
   #endif
#endif
// ----------------------------------------------------------------------------
//
// Textures
//
// ----------------------------------------------------------------------------
// Texture Fetch Flags (declared for each texture, used only by cpp code)
#define TFF_SRGB     1
#define TFF_STENCIL  1 // alias for depth-stencil textures, uses same bit as SRGB for color textures
#define TFF_BX2      2
#define _MAKE_TEX_ID(ID)          PS_##ID##_TEX
#define _MAKE_TFF_ID(ID)          PS_##ID##_TFF


#ifdef _AP_NX64
   #define DECLARE_TEXTURE_POOL(IDX, SIZE) layout(binding=TEXTURE_BINDING_LOCATION + IDX, std140) uniform BLOCK_##TEX_POOL_ID {\
         uint64_t TEXTURES_POOL[SIZE];\
         uint64_t PS_SAMPLERS[MAX_PS_SAMPLERS];\
         uint64_t PS_SAMPLERS_CMP[MAX_PS_CMP_SAMPLERS];\
      };
#else
   #define DECLARE_TEXTURE_POOL(IDX, SIZE)
#endif

#ifndef __cplusplus
   #ifdef _API_VULKAN
      #define _DECLARE_TEX(TYPE, TEX_ID, IDX, SET, SIZE, SMP, TFF, POSTFIX) \
         [[vk::binding(TEXTURE_BINDING_LOCATION + IDX, SET)]] TYPE _MAKE_TEX_ID(TEX_ID)[SIZE] : register(t##IDX); \
         static const int _MAKE_SMP_ID(TEX_ID) = SMP
         
      #define _DECLARE_STRUCTURED_BUFFER(TYPE, TEX_ID, IDX, SET) \
         [[vk::binding(TEXTURE_BINDING_LOCATION + IDX, SET)]] StructuredBuffer<TYPE> TEX_ID : register(t##IDX);
      #define _DECLARE_BUFFER(TYPE, TEX_ID, IDX, SET) \
         [[vk::binding(TEXTURE_BINDING_LOCATION + IDX, SET)]] Buffer<TYPE> TEX_ID : register(t##IDX);
   #elif defined(_AP_NX64)
      #define _DECLARE_TEX(TYPE, TEX_ID, IDX, SET, SIZE, SMP, TFF, POSTFIX) \
         static const int _MAKE_TEX_ID(TEX_ID) = IDX;\
         static const int _MAKE_SMP_ID(TEX_ID) = SMP
         
      #define _DECLARE_STRUCTURED_BUFFER(TYPE, TEX_ID, IDX, SET) \
         layout(binding=IDX) readonly buffer TEX_ID##_BUFFER { TYPE TEX_ID[]; }
      #define _DECLARE_BUFFER(TYPE, TEX_ID, IDX, SET) \
         layout(binding=IDX) readonly buffer TEX_ID##_BUFFER { TYPE TEX_ID[]; }
   #else
      #define _DECLARE_TEX(TYPE, TEX_ID, IDX, SET, SIZE, SMP, TFF, POSTFIX) \
         TYPE _MAKE_TEX_ID(TEX_ID)[SIZE] : register(t##IDX); \
         static const int _MAKE_SMP_ID(TEX_ID) = SMP
         
      #define _DECLARE_STRUCTURED_BUFFER(TYPE, TEX_ID, IDX, SET) \
         StructuredBuffer<TYPE> TEX_ID : register(t##IDX);
      #define _DECLARE_BUFFER(TYPE, TEX_ID, IDX, SET) \
         Buffer<TYPE> TEX_ID : register(t##IDX);
   #endif
#else //__cplusplus
   #define _DECLARE_TEX(TYPE, TEX_ID, IDX, SET, SIZE, SMP, TFF, POSTFIX) \
      static const int _MAKE_TEX_ID(TEX_ID) = IDX; \
      static const int _MAKE_TFF_ID(TEX_ID) = TFF;  \
      ADD_REFLECTION_INFO(SRV_##POSTFIX, TEX_ID, IDX, SET, SIZE, DEFAULT_USAGE, BINDING_NO_OFFSET) 
   
   #define _DECLARE_STRUCTURED_BUFFER(TYPE, TEX_ID, IDX, SET) \
      static const int _MAKE_TEX_ID(TEX_ID) = IDX; \
      static const int _MAKE_TFF_ID(TEX_ID) = 0;  \
      ADD_REFLECTION_INFO(TEX_STRUCTURED_BUFFER, TEX_ID, IDX, SET, 1, DEFAULT_USAGE, BINDING_NO_OFFSET) 
      
   #define _DECLARE_BUFFER(TYPE, TEX_ID, IDX, SET) \
      static const int _MAKE_TEX_ID(TEX_ID) = IDX; \
      static const int _MAKE_TFF_ID(TEX_ID) = 0;  \
      ADD_REFLECTION_INFO(TEX_BUFFER, TEX_ID, IDX, SET, 1, DEFAULT_USAGE, BINDING_NO_OFFSET);
         
#endif //__cplusplus 

#define DECLARE_TEX_1D(TEX_ID, IDX, SET, SIZE, SMP, TFF)     _DECLARE_TEX(Texture1D,        TEX_ID, IDX, SET, SIZE, SMP, TFF, TEX_1D) 
#define DECLARE_TEX_2D(TEX_ID, IDX, SET, SIZE, SMP, TFF)     _DECLARE_TEX(Texture2D,        TEX_ID, IDX, SET, SIZE, SMP, TFF, TEX_2D) 
#define DECLARE_TEX_2D_A(TEX_ID, IDX, SET, SIZE, SMP, TFF)   _DECLARE_TEX(Texture2DArray,   TEX_ID, IDX, SET, SIZE, SMP, TFF, TEX_2D_ARRAY) 
#define DECLARE_TEX_3D(TEX_ID, IDX, SET, SIZE, SMP, TFF)     _DECLARE_TEX(Texture3D,        TEX_ID, IDX, SET, SIZE, SMP, TFF, TEX_3D) 
#define DECLARE_TEX_CUBE(TEX_ID, IDX, SET, SIZE, SMP, TFF)   _DECLARE_TEX(TextureCube,      TEX_ID, IDX, SET, SIZE, SMP, TFF, TEX_CUBE) 
#define DECLARE_TEX_CUBE_A(TEX_ID, IDX, SET, SIZE, SMP, TFF) _DECLARE_TEX(TextureCubeArray, TEX_ID, IDX, SET, SIZE, SMP, TFF, TEX_CUBE_ARRAY) 

#if defined(_AP_NX64)
   #define DECLARE_TEX_2D_UINT(TEX_ID, IDX, SET, SIZE, SMP, TFF) _DECLARE_TEX(utexture2D, TEX_ID, IDX, SET, SIZE, SMP, TFF, TEX_2D_UINT) 
   #define DECLARE_TEX_3D_UINT(TEX_ID, IDX, SET, SIZE, SMP, TFF) _DECLARE_TEX(utexture3D, TEX_ID, IDX, SET, SIZE, SMP, TFF, TEX_3D_UINT) 
#else
   #define DECLARE_TEX_2D_UINT(TEX_ID, IDX, SET, SIZE, SMP, TFF) _DECLARE_TEX(Texture2D<uint4>, TEX_ID, IDX, SET, SIZE, SMP, TFF, TEX_2D_UINT) 
   #define DECLARE_TEX_3D_UINT(TEX_ID, IDX, SET, SIZE, SMP, TFF) _DECLARE_TEX(Texture3D<uint4>, TEX_ID, IDX, SET, SIZE, SMP, TFF, TEX_3D_UINT) 
#endif

#define DECLARE_BUF_FLOAT4(TEX_ID, IDX, SET, SIZE, SMP, TFF) _DECLARE_BUFFER(float4, TEX_ID, IDX, SET)
#define DECLARE_TEX_BUF(TYPE, TEX_ID, IDX, SET)              _DECLARE_BUFFER(TYPE, TEX_ID, IDX, SET)
#define DECLARE_TEX_STRUCTURED_BUF(TYPE, TEX_ID, IDX, SET)   _DECLARE_STRUCTURED_BUFFER(TYPE, TEX_ID, IDX, SET)

#ifdef _AP_NX64
   #define GET_TEX(ID)           TEXTURES_POOL[ID]
   #define GET_TEX_IDX(ID, IDX)  TEXTURES_POOL[ID + IDX]
   #define GET_SAMP(ID)          PS_SAMPLERS[ID]
   #define GET_SAMP_CMP(ID)      PS_SAMPLERS_CMP[ID]
#else
   #define GET_TEX(ID)           ID[0]
   #define GET_TEX_IDX(ID, IDX)  ID[IDX]
   #define GET_SAMP(ID)          PS_SAMPLERS[ID]
   #define GET_SAMP_CMP(ID)      PS_SAMPLERS_CMP[ID]
#endif

#define _TEX_ID(ID)         GET_TEX(_MAKE_TEX_ID(ID))
#define _TEX_IDX(ID,IDX)    GET_TEX_IDX(_MAKE_TEX_ID(ID), IDX)
#define _SAMP(ID)           GET_SAMP(_MAKE_SMP_ID(ID))
#define _SAMP_CMP(ID)       GET_SAMP_CMP(_MAKE_SMP_ID(ID))


#define _TEX_SAMP(ID)       _TEX_ID(ID), _SAMP(ID)
#define _TEX_SAMP_CMP(ID)   _TEX_ID(ID), _SAMP_CMP(ID)
#define _TEX_A_SAMP(ID)     _TEX_ID(ID), _SAMP(ID)
#define _TEXCUBE_SAMP(ID)   _TEX_ID(ID), _SAMP(ID)
#define _TEXCUBE_A_SAMP(ID) _TEX_ID(ID), _SAMP(ID)

#define SAMPLE(ID, uv)                                   _SAMPLE(_TEX_ID(ID), _SAMP(ID), uv)
#define SAMPLE_IDX(ID, idx, uv)                          _SAMPLE(_TEX_IDX(ID,idx), _SAMP(ID), uv)
#define SAMPLE_BIAS(ID, uv, bias)                        _SAMPLE_BAIS(_TEX_ID(ID), _SAMP(ID), uv, bias)
#define SAMPLE_LEVEL(ID, uv, lod)                        _SAMPLE_LEVEL(_TEX_ID(ID), _SAMP(ID), uv, lod)
#define SAMPLE_LEVEL_OFFS(ID, uv, lod, offs)             _SAMPLE_LEVEL_OFFS(_TEX_ID(ID), _SAMP(ID), uv, lod, offs)
#define SAMPLE_CUBE(ID, uv)                              _SAMPLE_CUBE(_TEX_ID(ID), _SAMP(ID), uv)
#define SAMPLE_CUBE_LEVEL(ID, uv, lod)                   _SAMPLE_CUBE_LEVEL(_TEX_ID(ID), _SAMP(ID), uv, lod)
#define SAMPLE_CUBE_ARRAY(ID, uv)                        _SAMPLE_CUBE_ARRAY(_TEX_ID(ID), _SAMP(ID), uv)
#define SAMPLE_CUBE_ARRAY_LEVEL(ID, uv, lod)             _SAMPLE_CUBE_ARRAY_LEVEL(_TEX_ID(ID), _SAMP(ID), uv, lod)
#define SAMPLE_ARRAY(ID, uv)                             _SAMPLE_ARRAY(_TEX_ID(ID), _SAMP(ID), uv)
#define SAMPLE_ARRAY_LEVEL(ID, uv, lod)                  _SAMPLE_ARRAY_LEVEL(_TEX_ID(ID), _SAMP(ID), uv, lod)
#define SAMPLE_ARRAY_GRAD(ID, uv, ddx, ddy)              _SAMPLE_ARRAY_GRAD(_TEX_ID(ID), _SAMP(ID), uv, ddx, ddy)

#define SAMPLE_CMP_LEVEL_0(ID, uv, cval)                 _SAMPLE_CMP_LEVEL_0(_TEX_ID(ID), _SAMP_CMP(ID), uv, cval)
#define SAMPLE_CMP_LEVEL_0_OFFS(ID, uv, cval, offs)      _SAMPLE_CMP_LEVEL_0_OFFS(_TEX_ID(ID), _SAMP_CMP(ID), uv, cval, offs)
#define SAMPLE_GRAD(ID, uv, ddx, ddy)                    _SAMPLE_GRAD(_TEX_ID(ID), _SAMP(ID), uv, ddx, ddy)

#define SAMPLE_1D(ID, uv)                                _SAMPLE_1D(_TEX_ID(ID), _SAMP(ID), uv)
#define SAMPLE_LEVEL_1D(ID, uv, lod)                     _SAMPLE_LEVEL_1D(_TEX_ID(ID), _SAMP(ID), uv, lod)
#define SAMPLE_3D(ID, uv)                                _SAMPLE_3D(_TEX_ID(ID), _SAMP(ID), uv)
#define SAMPLE_LEVEL_3D(ID, uv, lod)                     _SAMPLE_LEVEL_3D(_TEX_ID(ID), _SAMP(ID), uv, lod)

#define SAMPLE_SAMPLER(ID, smp, uv)                      _SAMPLE(_TEX_ID(ID), PS_SAMPLERS[smp], uv)
#define SAMPLE_IDX_LEVEL(ID, idx, uv, lod)               _SAMPLE_LEVEL(_TEX_IDX(ID,idx), _SAMP(ID), uv, lod)
#define SAMPLE_IDX_LEVEL_SAMPLER(ID, idx, uv, lod, smp)  _SAMPLE_LEVEL(_TEX_IDX(ID,idx), PS_SAMPLERS[smp], uv, lod)
#define SAMPLE_2D_FLOAT(ID, uv)                          _TEX_ID(ID)[uv]

#define LOAD_2D_FLOAT(ID, xy)                            _LOAD_2D_FLOAT(_TEX_ID(ID), xy)

#if defined(_AP_NX64)
   #define LOAD_2D_LEVEL_FLOAT(ID, xy, lod)        texelFetch(sampler2D(COMB(_TEX_ID(ID), PS_SAMPLERS[PS_SMP_WRAP_POINT])), int2(xy), lod)
   #define LOAD_3D_FLOAT(ID, uv)                   texelFetch(sampler3D(COMB(_TEX_ID(ID), PS_SAMPLERS[PS_SMP_WRAP_POINT])), int3(uv), 0)
   #define LOAD_2D_ARRAY_LEVEL_FLOAT(ID, xyz, lod) texelFetch(sampler2DArray(COMB(_TEX_ID(ID), PS_SAMPLERS[PS_SMP_WRAP_POINT])), int3(xyz), lod)
   #define LOAD_2D_UINT(ID, uv)                    texelFetch(usampler2D(COMB(_TEX_ID(ID), PS_SAMPLERS[PS_SMP_WRAP_POINT])), int2(uv), 0)
   #define LOAD_3D_UINT(ID, uv)                    texelFetch(usampler3D(COMB(_TEX_ID(ID), PS_SAMPLERS[PS_SMP_WRAP_POINT])), int3(uv), 0)

   #define GET_DIMENSIONS(ID, size)                size = textureSize(sampler2D(COMB(_TEX_ID(ID), _SAMP(ID))), 0)
   #define GET_DIMENSIONS3D(ID, size)              size = textureSize(sampler3D(COMB(_TEX_ID(ID), _SAMP(ID))), 0)
#else
   #define LOAD_2D_LEVEL_FLOAT(ID, xy, lod)        _TEX_ID(ID).Load(int3(xy, lod))
   #define LOAD_3D_FLOAT(ID, uv)                   _TEX_ID(ID).Load(int4(uv, 0))
   #define LOAD_2D_ARRAY_LEVEL_FLOAT(ID, xyz, lod) _TEX_ID(ID).Load(int4(xyz, lod))
   #define LOAD_2D_UINT(ID, uv)                    _TEX_ID(ID).Load(int3(uv, 0))
   #define LOAD_3D_UINT(ID, uv)                    _TEX_ID(ID).Load(int4(uv, 0))

   #define GET_DIMENSIONS(ID, size)                _TEX_ID(ID).GetDimensions(size.x, size.y)
   #define GET_DIMENSIONS3D(ID, size)              _TEX_ID(ID).GetDimensions(size.x, size.y, size.z)
#endif



#define LOAD_BUF(ID, offs)                ID[offs]
#define LOAD_STRUCTURED_BUF(ID, offs)     _TEX_ID(ID)[offs]
#define SAMPLE_BUF(ID, offs)              _TEX_ID(ID)[offs]

#define SAMPLE_GATHER(ID, uv)               _SAMPLE_GATHER(_TEX_ID(ID), _SAMP(ID), uv)
#define SAMPLE_GATHER_ALPHA(ID, uv)         _SAMPLE_GATHER_ALPHA(_TEX_ID(ID), _SAMP(ID), uv)
#if defined(_AP_NX64)
   #define SAMPLE_GATHER_2D_ARRAY(ID, uv)   _SAMPLE_GATHER_2D_ARRAY(_TEX_ID(ID), _SAMP(ID), uv)
   #define SAMPLE_GATHER_CUBE_ARRAY(ID, uv) _SAMPLE_GATHER_CUBE_ARRAY(_TEX_ID(ID), _SAMP(ID), uv)
#else
   #define SAMPLE_GATHER_2D_ARRAY(ID, uv)   _SAMPLE_GATHER(_TEX_ID(ID), _SAMP(ID), uv)
   #define SAMPLE_GATHER_CUBE_ARRAY(ID, uv) _SAMPLE_GATHER(_TEX_ID(ID), _SAMP(ID), uv)
#endif
#define SAMPLE_GATHER_OFFSET(ID, uv, off)  _SAMPLE_GATHER_OFFS(_TEX_ID(ID), _SAMP(ID), uv, off)


// ----------------------------------------------------------------------------
//
// Unordeded Access View (UAV) buffers
//
// ----------------------------------------------------------------------------   
#define _MAKE_UAV_ID(ID)          PS_##ID##_UAV

      
#ifndef __cplusplus
   #ifdef _API_VULKAN
      #define _DECLARE_UAV(TYPE, FORMAT, UAV_ID, IDX, SET, SIZE) \
         [[vk::binding(UAV_BINDING_LOCATION + IDX, SET)]] TYPE _MAKE_UAV_ID(UAV_ID)[SIZE] : register(u##IDX)
      #define _DECLARE_UAV_BUFFER(TYPE, UAV_ID, IDX, SET) \
         [[vk::binding(UAV_BINDING_LOCATION + IDX, SET)]] RWBuffer<TYPE> UAV_ID : register(u##IDX);
      #define _DECLARE_UAV_STRUCTURED_BUFFER(TYPE, UAV_ID, IDX, SET) \
         [[vk::binding(UAV_BINDING_LOCATION + IDX, SET)]] RWStructuredBuffer<TYPE> UAV_ID : register(u##IDX);
      #define _DECLARE_UAV_STRUCTURED_BUFFER_COUNTER(TYPE, UAV_ID, IDX, SET) \
         [[vk::binding(UAV_BINDING_LOCATION + IDX, SET), vk::counter_binding(UAV_COUNTER_BINDING_LOCATION + IDX)]] RWStructuredBuffer<TYPE> UAV_ID : register(u##IDX);
   #elif defined(_AP_NX64)
      #define _DECLARE_UAV(TYPE, FORMAT, UAV_ID, IDX, SET, SIZE) \
         layout(FORMAT, binding=UAV_BINDING_LOCATION + IDX) uniform restrict TYPE _MAKE_UAV_ID(UAV_ID)[SIZE]
      #define _DECLARE_UAV_BUFFER(TYPE, UAV_ID, IDX, SET) \
         layout(binding=UAV_BINDING_LOCATION + IDX) buffer UAV_ID##_BUFFER { TYPE UAV_ID[]; }
      #define _DECLARE_UAV_STRUCTURED_BUFFER(TYPE, UAV_ID, IDX, SET) \
         layout(binding=UAV_BINDING_LOCATION + IDX) buffer UAV_ID##_BUFFER { TYPE UAV_ID[]; }
      #define _DECLARE_UAV_STRUCTURED_BUFFER_COUNTER(TYPE, UAV_ID, IDX, SET) \
         layout(binding=UAV_BINDING_LOCATION + IDX) buffer UAV_ID##_BUFFER { uint UAV_ID##_COUNTER[8]; TYPE UAV_ID[]; }
   #else
      #define _DECLARE_UAV(TYPE, FORMAT, UAV_ID, IDX, SET, SIZE) \
         TYPE _MAKE_UAV_ID(UAV_ID)[SIZE] : register(u##IDX)
      #define _DECLARE_UAV_BUFFER(TYPE, UAV_ID, IDX, SET) \
         RWBuffer<TYPE> UAV_ID : register(u##IDX);
      #define _DECLARE_UAV_STRUCTURED_BUFFER(TYPE, UAV_ID, IDX, SET) \
         RWStructuredBuffer<TYPE> UAV_ID : register(u##IDX);
      #define _DECLARE_UAV_STRUCTURED_BUFFER_COUNTER(TYPE, UAV_ID, IDX, SET) \
         RWStructuredBuffer<TYPE> UAV_ID : register(u##IDX);
   #endif
#else  //__cplusplus
   #define _DECLARE_UAV(TYPE, FORMAT, UAV_ID, IDX, SET, SIZE) \
      static const int _MAKE_UAV_ID(UAV_ID) = IDX;  \
      ADD_REFLECTION_INFO(UAV_TEXTURE, UAV_ID, IDX, SET, SIZE, DEFAULT_USAGE, BINDING_NO_OFFSET)
   #define _DECLARE_UAV_BUFFER(TYPE, UAV_ID, IDX, SET) \
      static const int _MAKE_UAV_ID(UAV_ID) = IDX;  \
      ADD_REFLECTION_INFO(UAV_BUFFER, UAV_ID, IDX, SET, 1, DEFAULT_USAGE, BINDING_NO_OFFSET) 
   #define _DECLARE_UAV_STRUCTURED_BUFFER(TYPE, UAV_ID, IDX, SET) \
      static const int _MAKE_UAV_ID(UAV_ID) = IDX;  \
      ADD_REFLECTION_INFO(UAV_STRUCTURED_BUFFER, UAV_ID, IDX, SET, 1, DEFAULT_USAGE, BINDING_NO_OFFSET) 
   #define _DECLARE_UAV_STRUCTURED_BUFFER_COUNTER(TYPE, UAV_ID, IDX, SET) \
      static const int _MAKE_UAV_ID(UAV_ID) = IDX;  \
      ADD_REFLECTION_INFO(UAV_STRUCTURED_BUFFER_COUNTER, UAV_ID, IDX, SET, 1, DEFAULT_USAGE, BINDING_NO_OFFSET) 
#endif //__cplusplus

#define DECLARE_UAV_TEX_1D_UINT(UAV_ID, IDX, SET, SIZE)                  _DECLARE_UAV(RWTexture1D_UINT, r32ui, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_2D_UINT(UAV_ID, IDX, SET, SIZE)                  _DECLARE_UAV(RWTexture2D_UINT, r32ui, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_3D_UINT(UAV_ID, IDX, SET, SIZE)                  _DECLARE_UAV(RWTexture3D_UINT, r32ui, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_1D_UINT2(UAV_ID, IDX, SET, SIZE)                 _DECLARE_UAV(RWTexture1D_UINT2, rg32ui, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_2D_UINT2(UAV_ID, IDX, SET, SIZE)                 _DECLARE_UAV(RWTexture2D_UINT2, rg32ui, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_3D_UINT2(UAV_ID, IDX, SET, SIZE)                 _DECLARE_UAV(RWTexture3D_UINT2, rg32ui, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_2D_UINT4(UAV_ID, IDX, SET, SIZE)                 _DECLARE_UAV(RWTexture2D_UINT4, rgba32ui, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_1D_UNORM(UAV_ID, IDX, SET, SIZE)                 _DECLARE_UAV(RWTexture1D_UNORM, r8, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_2D_UNORM(UAV_ID, IDX, SET, SIZE)                 _DECLARE_UAV(RWTexture2D_UNORM, r8, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_3D_UNORM(UAV_ID, IDX, SET, SIZE)                 _DECLARE_UAV(RWTexture3D_UNORM, r8, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_1D_UNORM4(UAV_ID, IDX, SET, SIZE)                _DECLARE_UAV(RWTexture1D_UNORM4, rgba8, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_2D_UNORM4(UAV_ID, IDX, SET, SIZE)                _DECLARE_UAV(RWTexture2D_UNORM4, rgba8, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_3D_UNORM4(UAV_ID, IDX, SET, SIZE)                _DECLARE_UAV(RWTexture3D_UNORM4, rgba8, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_1D_FLOAT(UAV_ID, IDX, SET, SIZE)                 _DECLARE_UAV(RWTexture1D_FLOAT, r32f, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_2D_FLOAT(UAV_ID, IDX, SET, SIZE)                 _DECLARE_UAV(RWTexture2D_FLOAT, r32f, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_3D_FLOAT(UAV_ID, IDX, SET, SIZE)                 _DECLARE_UAV(RWTexture3D_FLOAT, r32f, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_1D_FLOAT2(UAV_ID, IDX, SET, SIZE)                _DECLARE_UAV(RWTexture1D_FLOAT2, rg32f, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_2D_FLOAT2(UAV_ID, IDX, SET, SIZE)                _DECLARE_UAV(RWTexture2D_FLOAT2, rg32f, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_3D_FLOAT2(UAV_ID, IDX, SET, SIZE)                _DECLARE_UAV(RWTexture3D_FLOAT2, rg32f, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_1D_FLOAT3(UAV_ID, IDX, SET, SIZE)                _DECLARE_UAV(RWTexture1D_FLOAT3, rgba8, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_2D_FLOAT3(UAV_ID, IDX, SET, SIZE)                _DECLARE_UAV(RWTexture2D_FLOAT3, rgba8, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_3D_FLOAT3(UAV_ID, IDX, SET, SIZE)                _DECLARE_UAV(RWTexture3D_FLOAT3, rgba8, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_TEX_2D_FLOAT4(UAV_ID, IDX, SET, SIZE)                _DECLARE_UAV(RWTexture2D_FLOAT4, rgba16f, UAV_ID, IDX, SET, SIZE) // rgba8 ??
#define DECLARE_UAV_TEX_3D_FLOAT4(UAV_ID, IDX, SET, SIZE)                _DECLARE_UAV(RWTexture3D_FLOAT4, rgba16f, UAV_ID, IDX, SET, SIZE)
#define DECLARE_UAV_BUF(TYPE, UAV_ID, IDX, SET)                          _DECLARE_UAV_BUFFER(TYPE, UAV_ID, IDX, SET)
#define DECLARE_UAV_STRUCTURED_BUF(TYPE, UAV_ID, IDX, SET)               _DECLARE_UAV_STRUCTURED_BUFFER(TYPE, UAV_ID, IDX, SET)
#define DECLARE_UAV_STRUCTURED_BUF_COUNTER(TYPE, UAV_ID, IDX, SET)       _DECLARE_UAV_STRUCTURED_BUFFER_COUNTER(TYPE, UAV_ID, IDX, SET)
#define UAV(ID)         _MAKE_UAV_ID(ID)[0]
#define UAV_IDX(ID, IDX)         _MAKE_UAV_ID(ID)[IDX]

// ----------------------------------------------------------------------------
//
// Vertex attribute binding
//
// ----------------------------------------------------------------------------   
#define _MAKE_ATTRIBUTE_NAME(ATT_NAME) ATTRIBUTE_##ATT_NAME

#ifdef GEOMETRY_SHADER
   #define ATTRIBUTES_AS_ARRAYS []
#endif // GEOMETRY_SHADER

#ifndef ATTRIBUTES_AS_ARRAYS
   #define ATTRIBUTES_AS_ARRAYS
#endif // ATTRIBUTES_AS_ARRAYS

#if defined(_API_VULKAN) || defined(_AP_NX64)
   #define V_POSITION        0
   #define V_BLENDWEIGHT     1
   #define V_BLENDWEIGHT0    1
   #define V_BLENDWEIGHT1    2
   #define V_BLENDINDICES    3
   #define V_BLENDINDICES0   3
   #define V_BLENDINDICES1   4
   #define V_NORMAL          5
   
   #define V_TANGENT0        6
   #define V_TANGENT1        7
   #define V_TANGENT2        8
   #define V_TANGENT3        9

   #define V_COLOR0          10
   #define V_COLOR1          11
   #define V_COLOR2          12
   #define V_COLOR3          13
   #define V_COLOR4          14
   #define V_COLOR5          15
   
   #define V_TEXCOORD0       16
   #define V_TEXCOORD1       17
   #define V_TEXCOORD2       18
   #define V_TEXCOORD3       19
   #define V_TEXCOORD4       20
   #define V_TEXCOORD5       21
   #define V_TEXCOORD6       22
   #define V_TEXCOORD7       23
   #define V_TEXCOORD8       24
   #define V_TEXCOORD9       25
   #define V_TEXCOORD10      26
   #define V_TEXCOORD11      27
   #define V_TEXCOORD12      28
   #define V_TEXCOORD13      29
   #define V_TEXCOORD14      30
   #define V_TEXCOORD15      31
#endif

#ifdef _AP_NX64
   #define _GET_ATTRIBUTE(STRUCT_NAME, ATT_NAME) _MAKE_ATTRIBUTE_NAME(ATT_NAME)
   #define VERTEX_ATTRIBUTE_BEGIN(STRUCT_NAME) struct STRUCT_NAME { int instanceID; }; 
   
   #define DECLARE_ATTRIBUTE(TYPE, ATT_NAME, SEMANTIC) layout(location=ATTRIB_COUNTER) in TYPE _MAKE_ATTRIBUTE_NAME(ATT_NAME) ATTRIBUTES_AS_ARRAYS
   
   #define DECLARE_SV_ATTRIBUTE(TYPE, ATT_NAME, SEMANTIC) const int _MAKE_ATTRIBUTE_NAME(SEMANTIC) = 0 /* Save a const int to allow ; after macro */
   #define VERTEX_ATTRIBUTE_END

   #define DECLARE_INTERPOLATOR(TYPE, ATT_NAME, SEMANTIC) TYPE ATT_NAME   
   
   #define _GET_VERTEXID(STRUCT_NAME, ATT_NAME) GLSL_VERTEX_INDEX
   
   #define _GET_INSTANCEID(STRUCT_NAME, ATT_NAME) GLSL_INSTANCE_INDEX
#else
   #define _GET_ATTRIBUTE(STRUCT_NAME, ATT_NAME) STRUCT_NAME.ATT_NAME
   #define VERTEX_ATTRIBUTE_BEGIN(STRUCT_NAME) struct STRUCT_NAME{
   #ifdef _API_VULKAN
      #define DECLARE_ATTRIBUTE(TYPE, ATT_NAME, SEMANTIC) [[vk::location(SEMANTIC)]] TYPE ATT_NAME : location##SEMANTIC
   #else
      #define DECLARE_ATTRIBUTE(TYPE, ATT_NAME, SEMANTIC) TYPE ATT_NAME : SEMANTIC
   #endif
   #define DECLARE_SV_ATTRIBUTE(TYPE, ATT_NAME, SEMANTIC) TYPE ATT_NAME : SEMANTIC
   #define VERTEX_ATTRIBUTE_END };
   
   #define DECLARE_INTERPOLATOR(TYPE, ATT_NAME, SEMANTIC) TYPE ATT_NAME : SEMANTIC
   
   #define _GET_VERTEXID(STRUCT_NAME, ATT_NAME) STRUCT_NAME.ATT_NAME
   #define _GET_INSTANCEID(STRUCT_NAME, ATT_NAME) STRUCT_NAME.ATT_NAME
#endif

// ----------------------------------------------------------------------------
//
// Bindless textures
//
// ---------------------------------------------------------------------------- 
#if defined(_API_VULKAN)
   #define BINDLESS_TEXTURES
#endif

#endif // SYS_RESOURCE_DEFINES_FX
