/*
1) Создать массив 100к случайных uint64_t чисел.
2) Необходимо перевернуть двоичное представление чисел.
т.е. 110100 превращается в 001011
3) Замерять тайминги.
4) Использовать хеш таблицу для ускорения работы.
5) Сравнить тайминги std::map, std::unordered_map и хеш-табицу на базе массива.
6) Использовать различные размеры ключей таблицы.
7) Вывести таблицу в которой строки соответсвуют различным реализациям хеш таблиц, столбцы - размерам ключей, а значения элементов - таймингам работы.
 */


#include <iostream>
#include <chrono>
#include <random>
#include <map>
#include <unordered_map>


#define n 100000

std::vector<uint64_t> mas(n);
std::vector<uint64_t> rev_mas(n);


struct Timer {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();


    long get() {
       return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now() - start).count();
    }
};


template<class T>
inline T reverse(T x) {
   T ret = 0;

   for (int i = 0; i < sizeof(T) * 8; ++i) {
      ret = (ret << 1) | (x & 1);
      x >>= 1;
   }
   return ret;
}


template<class T>
long map() {
   Timer timer;
   int typeSize = sizeof(uint64_t) * 8;
   int keySize = sizeof(T) * 8; // key size in bits
   std::map<T, T> m;
   for (int i = 0; i < n; ++i) {
      for (int spl = 0; spl < typeSize / keySize; ++spl) {
         T mask = ~0;
         mask &= (mas[i] >> spl * 8);
         auto it = m.find(mask);
         T rev;
         if (it == m.end()) {
            rev = m[mask] = reverse(mask);
         } else {
            rev = (*it).second;
         }
         rev_mas[i] = (rev_mas[i] << keySize) | rev;
      }
   }
   return timer.get();
}


template<class T>
long umap() {
   Timer timer;
   int typeSize = sizeof(uint64_t) * 8;
   int keySize = sizeof(T) * 8; // key size in bits
   std::unordered_map<T, T> m;
   for (int i = 0; i < n; ++i) {
      for (int spl = 0; spl < typeSize / keySize; ++spl) {
         T mask = ~0;
         mask &= (mas[i] >> spl * 8);
         auto it = m.find(mask);
         T rev;
         if (it == m.end()) {
            rev = m[mask] = reverse(mask);
         } else {
            rev = (*it).second;
         }
         rev_mas[i] = (rev_mas[i] << keySize) | rev;
      }
   }
   return timer.get();
}


template<class T>
long hash() {
   Timer timer;
   int typeSize = sizeof(uint64_t) * 8;
   int keySize = sizeof(T) * 8; // key size in bits
   std::vector<std::vector<std::pair<T, T>>> m(n);
   for (int i = 0; i < n; ++i) {
      for (int spl = 0; spl < typeSize / keySize; ++spl) {
         T mask = ~0;
         T rev;
         mask &= (mas[i] >> spl * 8);
         auto &l = m[mask % n];
         auto it = l.begin();
         while (it != l.end())
            if ((*it).first == mask)
               break;
            else
               ++it;
         if (it == l.end()) {
            rev = reverse(mask);
            l.push_back({mask, rev});
         } else {
            rev = (*it).second;
         }
         rev_mas[i] = (rev_mas[i] << keySize) | rev;
      }
   }
   return timer.get();
}


template<class T>
long brute() {
   Timer timer;
   int typeSize = sizeof(uint64_t) * 8;
   int keySize = sizeof(T) * 8; // key size in bits
   for (int i = 0; i < n; ++i) {
      for (int spl = 0; spl < typeSize / keySize; ++spl) {
         T mask = ~0;
         mask &= (mas[i] >> spl * 8);
         rev_mas[i] = (rev_mas[i] << keySize) | reverse(mask);
      }
   }
   return timer.get();
}


int main() {
   std::mt19937_64 rng;
   std::uniform_int_distribution<uint64_t> dist;
   for (int i = 0; i < n; ++i)
      mas[i] = dist(rng);
   auto pattern = " [%d] %08ld";
   printf(" Map:");
   {
      printf(pattern, 8, map<uint8_t>());
      printf(pattern, 16, map<uint16_t>());
      printf(pattern, 32, map<uint32_t>());
      printf(pattern, 64, map<uint64_t>());
      printf("\n");
   }
   printf("UMap:");
   {
      printf(pattern, 8, umap<uint8_t>());
      printf(pattern, 16, umap<uint16_t>());
      printf(pattern, 32, umap<uint32_t>());
      printf(pattern, 64, umap<uint64_t>());
      printf("\n");
   }
   printf("Hash:");
   {
      printf(pattern, 8, hash<uint8_t>());
      printf(pattern, 16, hash<uint16_t>());
      printf(pattern, 32, hash<uint32_t>());
      printf(pattern, 64, hash<uint64_t>());
      printf("\n");
   }
   printf("Brut:");
   {
      printf(pattern, 8, brute<uint8_t>());
      printf(pattern, 16, brute<uint16_t>());
      printf(pattern, 32, brute<uint32_t>());
      printf(pattern, 64, brute<uint64_t>());
      printf("\n");
   }

   return 0;
}
