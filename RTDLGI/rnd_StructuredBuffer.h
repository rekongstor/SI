#pragma once
#include "rnd_UploadableBuffer.h"

class rnd_StructuredBuffer : public rnd_Buffer, public Buffer1D
{
public:
   void OnInit(int reservedElements, int sizeOfElement, LPCWSTR name);

   char* mappedData;
   int reservedElements;
   int sizeOfElement;
   DescHandlePair srvHandle;

   template <class T>
   int AddBuffer(T elem)
   {
      char* el = mappedData + (sizeOfElement * reservedElements);
      memcpy(el, &elem, sizeOfElement);
      return reservedElements++;
   }

   template <class T>
   void UpdateBuffer(T elem, int shift)
   {
      char* el = mappedData + (sizeOfElement * shift);
      memcpy(el, &elem, sizeOfElement);
   }
};
