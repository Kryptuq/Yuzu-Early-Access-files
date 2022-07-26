#include <iostream>
#include <vector>
#include "jwt/stack_alloc.hpp"

template <typename T, size_t SZ = 2>
using SmallVector = std::vector<T, jwt::stack_alloc<T, SZ, alignof(T)>>;


int main()
{
  SmallVector<int>::allocator_type::arena_type a;
  SmallVector<int> v{a};

  v.push_back(1);
  v.push_back(1);
  v.push_back(1);
  v.push_back(1);
  v.push_back(1);
  v.push_back(1);

  return 0;
}
