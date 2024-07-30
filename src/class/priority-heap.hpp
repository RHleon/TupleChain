#ifndef PRIORITY_HEAP
#define PRIORITY_HEAP

#include <ll-rb-tree.hpp>

namespace pp {

struct priority_heap_t : LLRBTree<int>
{
  int max;
  priority_heap_t() : max(0) {};

  void insert(const int& priority)
  {
    LLRBTree<int>::insert(priority);
    if (priority > max) max = priority;
  }
  void erase(const int& priority)
  {
    auto node = deleteBST(m_root, priority);
    if (node) {
      fixDeleteRBTree(node);
      auto curr = node;
      while (curr->right) curr = curr->right;
      max = curr->data;
    }
  }
};

} // namespace pp

#endif // PRIORITY_HEAP
