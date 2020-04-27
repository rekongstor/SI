/*
1) Сгенерить матрицу 1000x1000 из случайных bool элементов.
2) Найти размер наибольшей связной true области. Связными считаются элементы одного значения в строке, столбце или по диагонали.
3) Использовать стек, очередь и рекурсию для обхода областей.
4) Сравнить тайминги, конечно же.
 */


#include <iostream>
#include <chrono>
#include <map>
#include <unordered_set>
#include <vector>
#include <array>
#include <stack>
#include <queue>
#include <set>


#define n 100


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


const bool dummy = false;
std::vector<bool> matrix(n * n);
std::vector<int> support_matrix(n * n);


bool m(int i, int j) {
   if (i == 0 || j == 0 || i >= n || j >= n)
      return dummy;
   return matrix[i * n + j];
}


int &sm(int i, int j) {
   return support_matrix[i * n + j];
}


void dfs(std::map<int, std::unordered_set<int>> &graph, std::map<int, int> &colors) {
   Timer timer("DFS");
   // make color set. coloring it to 0 - white. could be a find - end() iterator but that's easier for me
   for (auto v : graph)
      colors[v.first] = 0;
   int counter = 0;
   std::stack<int> requeu;
   for (auto &v : graph) {
      if (colors[v.first] == 0) {
         ++counter;
         requeu.push(v.first);
         while (!requeu.empty()) {
            int current = requeu.top();
            colors[current] = counter; // grey
            requeu.pop();
            for (auto child : graph[current]) {
               if (colors[child] == 0) {
                  colors[child] = -1;
                  requeu.push(child);
               }
            }
         }
      }
   }
   return;
}


void recursion(int current, int counter, std::map<int, std::unordered_set<int>> &graph, std::map<int, int> &colors) {
   colors[current] = counter; // grey
   for (auto child : graph[current]) {
      if (colors[child] == 0) {
         colors[child] = -1;
         recursion(child, counter, graph, colors);
      }
   }
}


void recurrent(std::map<int, std::unordered_set<int>> &graph, std::map<int, int> &colors) {
   Timer timer("Recurrent");
   // make color set. coloring it to 0 - white. could be a find - end() iterator but that's easier for me
   for (auto v : graph)
      colors[v.first] = 0;
   int counter = 0;
   for (auto &v : graph) {
      if (colors[v.first] == 0) {
         ++counter;
         recursion(v.first, counter, graph, colors);
      }
   }
   return;
}


void bfs(std::map<int, std::unordered_set<int>> &graph, std::map<int, int> &colors) {
   Timer timer("BFS");
   // make color set. coloring it to 0 - white. could be a find - end() iterator but that's easier for me
   for (auto v : graph)
      colors[v.first] = 0;
   int counter = 0;
   std::queue<int> requeu;
   for (auto &v : graph) {
      if (colors[v.first] == 0) {
         ++counter;
         requeu.push(v.first);
         while (!requeu.empty()) {
            int current = requeu.front();
            colors[current] = counter; // grey
            requeu.pop();
            for (auto child : graph[current]) {
               if (colors[child] == 0) {
                  colors[child] = -1;
                  requeu.push(child);
               }
            }
         }
      }
   }
   return;
}


void reko() {
   std::map<int, std::unordered_set<int>> zoneSet;
   std::map<int, int> values;
   {
      Timer timer("Generating zones");
      int counter = 0;
      for (int i = 0; i < n; ++i)
         for (int j = 0; j < n; ++j) {
            if (m(i, j)) {
               if (m(i, j - 1)) {
                  sm(i, j) = sm(i, j - 1);
               }
               if (m(i - 1, j - 1)) {
                  sm(i, j) = sm(i - 1, j - 1);
               }
               if (m(i - 1, j + 1)) {
                  if (sm(i, j)) {
                     if (sm(i, j) != sm(i - 1, j + 1)) {
                        zoneSet[sm(i - 1, j + 1)].insert(sm(i, j));
                        zoneSet[sm(i, j)].insert(sm(i - 1, j + 1));
                     }
                     values[sm(i, j)]++;
                  } else {
                     sm(i, j) = sm(i - 1, j + 1);
                     values[sm(i - 1, j + 1)]++;
                  }
               } else {
                  if (m(i, j - 1)) {
                     sm(i, j) = sm(i, j - 1);
                     values[sm(i, j)]++;
                     continue;
                  }
                  if (m(i - 1, j - 1)) {
                     sm(i, j) = sm(i - 1, j - 1);
                     values[sm(i, j)]++;
                     continue;
                  }
                  if (m(i - 1, j)) {
                     sm(i, j) = sm(i - 1, j);
                     values[sm(i, j)]++;
                     continue;
                  }
                  sm(i, j) = ++counter;
                  values[counter] = 1;
               }
            }
         }
   }
   std::map<int, int> colors;
   {
      colors.clear();
      dfs(zoneSet, colors);
   }
   {
      colors.clear();
      bfs(zoneSet, colors);
   }
   {
      colors.clear();
      recurrent(zoneSet, colors);
   }
   std::map<int, int> processed;
   for (auto &v : colors)
      if (processed.find(v.second) == processed.end()) {
         processed[v.second] = v.first;
      } else {
         values[processed[v.second]] += values[v.first];
      }
   std::pair<int, int> max_key = {0, values[0]};
   for (auto p : values)
      if (p.second > max_key.second)
         max_key = p;
   std::cout << max_key.second << std::endl;

   // optional - paint zone
//   std::set<int> painted_counters;
//   painted_counters.insert(max_key.first);
//   // if our maximum was zoned
//   auto it = colors.find(max_key.first);
//   if (it != colors.end()) {
//      int max_zone = (*it).second;
//      for (auto &z : colors) {
//         if (z.first == max_key.first) // we'll skip this zone
//            continue;
//         if (z.second == max_zone)
//            painted_counters.insert(z.first);
//      }
//   }
//   {
//      for (int i = 0; i < n; ++i) {
//         for (int j = 0; j < n; ++j) {
//            std::cout << sm(i, j) << " ";
//         }
//         std::cout << std::endl;
//      }

//      for (int i = 0; i < n; ++i) {
//         for (int j = 0; j < n; ++j) {
//            if (painted_counters.find(sm(i, j)) != painted_counters.end())
//               std::cout << "2 ";
//            else
//               std::cout << m(i, j) << " ";
//         }
//         std::cout << std::endl;
//      }
//   }
   return;
}


int main() {
   for (int i = 0; i < n * n; ++i)
      matrix[i] = (rand() % 3) / 2;

   reko();
   return 0;
}
