#pragma once
#include "rnd_Buffer.h"

class rnd_UploadableBuffer : public rnd_Buffer
{
public:
   std::vector<char> cpuBuffer;

   void CleanUploadData() override;

   virtual ~rnd_UploadableBuffer() = default;
};

