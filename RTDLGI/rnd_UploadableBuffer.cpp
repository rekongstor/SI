#include "rnd_UploadableBuffer.h"

void rnd_UploadableBuffer::CleanUploadData()
{
   cpuBuffer.clear();
   cpuBuffer.resize(0);
}
