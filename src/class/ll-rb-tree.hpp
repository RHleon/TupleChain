#ifndef LL_RB_TREE_HPP
#define LL_RB_TREE_HPP

#include <cstdlib>
#include <common.hpp>

namespace pp {

using namespace std;
  
template<typename T>
class LLRBTree
{
public:
  enum Color {RED, BLACK, DOUBLE_BLACK};
  
  struct Node {
    T data;
    int color;
    int count;
    Node *left, *right, *parent;
    Node(const T& d)
      : data(d), color(RED), count(1)
      , left(nullptr), right(nullptr), parent(nullptr) {};
  };
  typedef Node* node_t;
  
protected:
  node_t m_root;

protected:
  int getColor(const node_t& node)
  {
    return node ? node->color : BLACK;
  }
  
  void setColor(const node_t& node, int color)
  {
    if (node) node->color = color;
  }

  node_t minValueNode(const node_t& node)
  {
    auto curr = node;
    while (curr->left) curr = curr->left;
    return curr;
  }
  
  node_t maxValueNode(const node_t& node)
  {
    auto curr = node;
    while (curr->right) curr = curr->right;
    return curr;
  }

  int getBlackHeight(const node_t& node)
  {
    int res = 0;
    auto curr = node;
    while (curr) {
      if (getColor(curr) == BLACK) {
	res ++;
      }
      curr = curr->left;
    }
    return res;
  }

  void rotateLeft(const node_t& ptr)
  {
    auto right_child = ptr->right;
    ptr->right = right_child->left;

    if (ptr->right != nullptr)
      ptr->right->parent = ptr;

    right_child->parent = ptr->parent;

    if (ptr->parent == nullptr)
      m_root = right_child;
    else if (ptr == ptr->parent->left)
      ptr->parent->left = right_child;
    else
      ptr->parent->right = right_child;

    right_child->left = ptr;
    ptr->parent = right_child;
  }
  
  void rotateRight(const node_t& ptr)
  {
    auto left_child = ptr->left;
    ptr->left = left_child->right;

    if (ptr->left != nullptr)
      ptr->left->parent = ptr;

    left_child->parent = ptr->parent;

    if (ptr->parent == nullptr)
      m_root = left_child;
    else if (ptr == ptr->parent->left)
      ptr->parent->left = left_child;
    else
      ptr->parent->right = left_child;

    left_child->right = ptr;
    ptr->parent = left_child;   
  }

  void fixInsertRBTree(node_t ptr)
  {
    node_t parent = nullptr;
    node_t grandparent = nullptr;
    while (ptr != m_root && getColor(ptr) == RED && getColor(ptr->parent) == RED) {
      parent = ptr->parent;
      grandparent = parent->parent;
      if (parent == grandparent->left) {
	auto uncle = grandparent->right;
	if (getColor(uncle) == RED) {
	  setColor(uncle, BLACK);
	  setColor(parent, BLACK);
	  setColor(grandparent, RED);
	  ptr = grandparent;
	} else {
	  if (ptr == parent->right) {
	    rotateLeft(parent);
	    ptr = parent;
	    parent = ptr->parent;
	  }
	  rotateRight(grandparent);
	  swap(parent->color, grandparent->color);
	  ptr = parent;
	}
      } else {
	auto uncle = grandparent->left;
	if (getColor(uncle) == RED) {
	  setColor(uncle, BLACK);
	  setColor(parent, BLACK);
	  setColor(grandparent, RED);
	  ptr = grandparent;
	} else {
	  if (ptr == parent->left) {
	    rotateRight(parent);
	    ptr = parent;
	    parent = ptr->parent;
	  }
	  rotateLeft(grandparent);
	  swap(parent->color, grandparent->color);
	  ptr = parent;
	}
      }
    }
    setColor(m_root, BLACK);
  }

  void fixDeleteRBTree(const node_t& node)
  {
    if (node == nullptr)
      return;

    if (node == m_root) {
      m_root = nullptr;
      return;
    }

    if (getColor(node) == RED || getColor(node->left) == RED || getColor(node->right) == RED) {
      auto child = node->left != nullptr ? node->left : node->right;

      if (node == node->parent->left) {
	node->parent->left = child;
	if (child != nullptr)
	  child->parent = node->parent;
	setColor(child, BLACK);
	delete (node);
      } else {
	node->parent->right = child;
	if (child != nullptr)
	  child->parent = node->parent;
	setColor(child, BLACK);
	delete (node);
      }
    } else {
      node_t sibling = nullptr;
      node_t parent = nullptr;
      auto ptr = node;
      setColor(ptr, DOUBLE_BLACK);
      while (ptr != m_root && getColor(ptr) == DOUBLE_BLACK) {
	parent = ptr->parent;
	if (ptr == parent->left) {
	  sibling = parent->right;
	  if (getColor(sibling) == RED) {
	    setColor(sibling, BLACK);
	    setColor(parent, RED);
	    rotateLeft(parent);
	  } else {
	    if (getColor(sibling->left) == BLACK && getColor(sibling->right) == BLACK) {
	      setColor(sibling, RED);
	      if(getColor(parent) == RED)
		setColor(parent, BLACK);
	      else
		setColor(parent, DOUBLE_BLACK);
	      ptr = parent;
	    } else {
	      if (getColor(sibling->right) == BLACK) {
		setColor(sibling->left, BLACK);
		setColor(sibling, RED);
		rotateRight(sibling);
		sibling = parent->right;
	      }
	      setColor(sibling, parent->color);
	      setColor(parent, BLACK);
	      setColor(sibling->right, BLACK);
	      rotateLeft(parent);
	      break;
	    }
	  }
	} else {
	  sibling = parent->left;
	  if (getColor(sibling) == RED) {
	    setColor(sibling, BLACK);
	    setColor(parent, RED);
	    rotateRight(parent);
	  } else {
	    if (getColor(sibling->left) == BLACK && getColor(sibling->right) == BLACK) {
	      setColor(sibling, RED);
	      if (getColor(parent) == RED)
		setColor(parent, BLACK);
	      else
		setColor(parent, DOUBLE_BLACK);
	      ptr = parent;
	    } else {
	      if (getColor(sibling->left) == BLACK) {
		setColor(sibling->right, BLACK);
		setColor(sibling, RED);
		rotateLeft(sibling);
		sibling = parent->left;
	      }
	      setColor(sibling, parent->color);
	      setColor(parent, BLACK);
	      setColor(sibling->left, BLACK);
	      rotateRight(parent);
	      break;
	    }
	  }
	}
      }
      if (node == node->parent->left)
	node->parent->left = nullptr;
      else
	node->parent->right = nullptr;
      delete(node);
      setColor(m_root, BLACK);
    }
  }
  
  node_t insertBST(const node_t& root, const node_t& ptr)
  {
    if (root == nullptr) {
      return ptr;
    }

    if (ptr->data < root->data) {
      root->left = insertBST(root->left, ptr);
      root->left->parent = root;
    } else if (ptr->data > root->data) {
      root->right = insertBST(root->right, ptr);
      root->right->parent = root;
    }

    root->count ++;
    return root;
  }

  node_t deleteBST(const node_t& root, const T& data) {
    if (root == nullptr)
      return root;

    if (data < root->data)
      return deleteBST(root->left, data);

    if (data > root->data)
      return deleteBST(root->right, data);

    if (-- root->count)
      return nullptr;

    if (root->left == nullptr || root->right == nullptr)
      return root;

    auto temp = minValueNode(root->right);
    root->data = temp->data;
    return deleteBST(root->right, temp->data);
  }
  
  void display(const node_t& ptr, int indent) const
  {
    if (ptr == nullptr)
      return;

    for (int i = 0; i < indent; i ++) LOG_DEBUG("\t");
    LOG_DEBUG("[%s: %p]\n",
	      (ptr->color == DOUBLE_BLACK ? "D" : (ptr->color == RED ? "R" : "B")),
	      &(ptr->data));
    display(ptr->left, indent + 1);
    display(ptr->right, indent + 1);
  }

  node_t release(const node_t& root) {
    if (root) {
      root->left = release(root->left);
      root->right = release(root->right);
      delete root;
    }
    return nullptr;
  }

  node_t find(const node_t& curr, const T& data) const
  {
    if (!curr) return nullptr;
    if (curr->data == data) return curr;
    if (curr->data < data) return find(curr->right, data);
    return find(curr->left, data);
  }

  int size(const node_t& root) const
  {
    if (!root) return 0;
    return 1 + size(root->left) + size(root->right);
  }
  
public:
  LLRBTree() : m_root(nullptr) {};
  
  ~LLRBTree() { m_root = release(m_root); }
  
  void insert(const T& data)
  {
    auto node = new Node(data);
    m_root = insertBST(m_root, node);
    fixInsertRBTree(node);
  }
  
  void erase(const T& data)
  {
    auto node = deleteBST(m_root, data);
    fixDeleteRBTree(node);
  }
  
  node_t find(const T& data) const { return find(m_root, data); }
  void display() const { display(m_root, 0); }
  const node_t& getRoot() const { return m_root; }
  int size() const { return size(m_root); }
};

} // namespace pp

#endif // LL_RB_TREE_HPP
