/*
1) Сделать 8 отсортированных массивов из 100к элементов. 4 сортируешь вставкой, 4 qsort
2) Смерджить в 1 отсортированный массив
	2а) кучей
	2б) мерджСорт
3) в дебажной конфигурации кода сделать функцию которая проверяет массив на отсортированность
4) замерять тайминги на создании массивов и на мердж для разных сортировок
5*) померять перфоманс и найти узкие места и подумать можно ли их починить и улучшить тайминги
 */


#include <iostream>
#include <chrono>
#include <vector>
#include <stdexcept>
#include <stack>
#include <cmath>
#include <random>


#define n 100000


struct Timer {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();


    Timer(const char *text) {
       std::cout << text << std::endl;
    }


    ~Timer() {
       std::cout << "Time elapsed: " << std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    }
};


inline void swap(int &first, int &second) {
   int tmp = first;
   first = second;
   second = tmp;
}


void gen(std::vector<int> &arr) {
   std::mt19937 rng(4221);
   std::uniform_int_distribution<int> dist;
   for (auto &el : arr)
      el = dist(rng);
}


void debug_check_ascending_sorted(std::vector<int> &arr) {
#ifndef NDEBUG
   for (auto it = arr.begin() + 1; it != arr.end(); ++it)
      if (*it < *(it - 1))
         throw (std::logic_error("The array is not sorted"));
#endif
}


void qsort(std::vector<int> &arr) {
   Timer timer("QSorting:");
   int tmp;
   std::stack<std::pair<int, int>> tasks; // {arr.start; length}
   tasks.push({0, arr.size()});
   while (!tasks.empty()) {
      auto current = tasks.top();
      tasks.pop();
      if (current.second > 1) {
         int end = current.first + current.second;
         int splitElemIndex = rand() % current.second;
         int splitElem = arr[current.first + splitElemIndex];
         swap(arr[end - 1], arr[current.first + splitElemIndex]);
         // Partition
         int arrSize = 0;
         for (int i = current.first; i < end - 1; ++i) {
            if (arr[i] <= splitElem) {
               swap(arr[i], arr[current.first + arrSize]);
               ++arrSize;
            }
         }
         int arr2Size = current.second - 1 - arrSize;
         swap(arr[end - 1], arr[end - arr2Size - 1]);

         if (arrSize > 1)
            tasks.push({current.first, arrSize});
         if (arr2Size > 1)
            tasks.push({end - arr2Size, arr2Size});
      }
   }

   debug_check_ascending_sorted(arr);
}


void insertion(std::vector<int> &arr) {
   Timer timer("InSorting:");
   for (int i = 1; i < arr.size(); ++i)
      for (int j = i - 1; j >= 0; --j)
         if (arr[j] > arr[j + 1]) {
            swap(arr[j], arr[j + 1]);
         }

   debug_check_ascending_sorted(arr);
}


void heap(std::vector<int> &arr) {
   Timer timer("Heap:");
   int heapSize = arr.size();
   auto Parent = [](int x) {
       return (x - 1) / 2;
   };
   auto RightChild = [](int x) {
       return x * 2 + 1;
   };
   auto LeftChild = [](int x) {
       return x * 2 + 2;
   };
   // Building the heap
   for (int i = 1; i < arr.size(); ++i) {
      int x = i;
      while (arr[Parent(x)] < arr[x]) {
         swap(arr[Parent(x)], arr[x]);
         x = Parent(x);
      }
   }

   // Heap sort
   for (int i = arr.size() - 1; i > 0; --i) {
      swap(arr[i], arr[0]);
      --heapSize;
      int x = 0;
      while (LeftChild(x) < heapSize) {
         if (RightChild(x) < heapSize) {
            if (arr[x] < arr[LeftChild(x)] || arr[x] < arr[RightChild(x)]) {
               x = arr[LeftChild(x)] > arr[RightChild(x)] ? LeftChild(x) : RightChild(x);
               swap(arr[Parent(x)], arr[x]);
               continue;
            }
            break;
         } else {
            if (arr[x] < arr[LeftChild(x)]) {
               x = LeftChild(x);
               swap(arr[Parent(x)], arr[x]);
               continue;
            }
            break;
         }
      }
   }

   debug_check_ascending_sorted(arr);
}


void recursiveMerge(std::vector<int> &arr, std::vector<int> &tmp, int start, int size1, int size2) {
   if (size1 * size2 == 0)
      return;
   if (size1 > 2)
      recursiveMerge(arr, tmp, start, size1 / 2, size1 - size1 / 2);
   if (size2 > 2)
      recursiveMerge(arr, tmp, start + size1, size2 / 2, size2 - size2 / 2);
   int counter = -1;
   int arr1 = start;
   int arr2 = arr1 + size1;
   int s1 = size1;
   int s2 = size2;
   while (s1 > 0 && s2 > 0) {
      if (arr[arr1] < arr[arr2]) {
         tmp[++counter] = arr[arr1++];
         --s1;
      } else {
         tmp[++counter] = arr[arr2++];
         --s2;
      }
   }
   while (s1--)
      tmp[++counter] = arr[arr1++];
   while (s2--)
      tmp[++counter] = arr[arr2++];
   std::copy(tmp.begin(), tmp.begin() + size1 + size2, arr.begin() + start);
}


void merge(std::vector<int> &arr) {
   Timer timer("Merge:");
   std::vector<int> tmp_array(arr.size());
   recursiveMerge(arr, tmp_array, 0, arr.size() / 2, arr.size() - arr.size() / 2);

   debug_check_ascending_sorted(arr);
}


int main() {
   std::vector<int> arr[8];
   {
      Timer timer("Arrays generation:");
      for (auto &v : arr) {
         v.resize(n);
         gen(v);
      }
   }
   for (int i = 0; i < 4; ++i)
      qsort(arr[i]);

   for (int i = 4; i < 8; ++i)
      insertion(arr[i]);

   std::vector<int> finalArray;
   {
      Timer timer("Final array:");
      finalArray.resize(8 * n);
   }
   {
      Timer timer("Final array copying for heap:");
      for (int i = 0; i < 8; ++i)
         std::copy(arr[i].begin(), arr[i].end(), finalArray.begin() + i * n);
   }
   {
      heap(finalArray);
   }

   {
      Timer timer("Final array copying for merge:");
      for (int i = 0; i < 8; ++i)
         std::copy(arr[i].begin(), arr[i].end(), finalArray.begin() + i * n);
   }
   {
      merge(finalArray);
   }

   return 0;
}
