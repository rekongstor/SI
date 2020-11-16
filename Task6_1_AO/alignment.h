#pragma once

constexpr CHAR FindHigherBit(UINT32 number)
{
   BYTE res = 0;
   if (number >> 16) {
      res += 16;
      number >>= 16;
   }
   if (number >> 8) {
      res += 8;
      number >>= 8;
   }
   if (number >> 4) {
      res += 4;
      number >>= 4;
   }
   if (number >> 2) {
      res += 2;
      number >>= 2;
   }
   if (number >> 1) {
      res += 1;
      number >>= 1;
   }
   if (number) {
      res += 1;
   }
   return res;
}

constexpr CHAR FindLowerBit(UINT32 number)
{
   BYTE res = 0;
   while (number) {
      if (number & 1) {
         return res;
      }
      res += 1;
      number >>= 1;
   }
   return res;
}

constexpr UINT32 AlignSize(const UINT32 dataSize, const UINT32 alignment)
{
   return ((dataSize + (alignment - 1)) & ~(alignment - 1));
}