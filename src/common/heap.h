#pragma once

#include <ostream>
#include <string.h>
#include "common/common.h"

namespace bdf {

struct HeapCategory {
  enum {
    kMinHeap=0,
    kMaxHeap=1,
  };
};

template <typename Val>
struct HeapItem {
  size_t index;
  Val val;
};

template<typename T>
struct ReflectorF {
  const T& operator() (const T& t) const { return t; }
};

/* RAII */
template <
  typename Val,
  bool Order,
  typename FeatureF=ReflectorF<Val> >
class HeapBase {
 public:
  typedef HeapBase<Val, Order, FeatureF> Self;
  typedef HeapItem<Val> Node;

 public:
  static const int default_size = 4096;

 public:
  explicit HeapBase(
      size_t init_size_heap = default_size,
      FeatureF feature = FeatureF());

  /*
   * @notice : same keys are permited
   */
  void Insert(IN const Val& val, OUT size_t& sign);
  inline void Update(size_t sign, const Val& val);
  inline void UpdateTop(const Val& val);
  inline void Refresh(size_t sign);
  inline void RefreshTop();
  Val* Top() const { return 0!=num_items_ ? &(heap_items_[1]->val) : NULL; }
  inline size_t Pop();
  inline void Erase(size_t sign);
  inline Val* Item(IN size_t sign) const { return &(reinterpret_cast<Node*>(sign)->val); }
  size_t Size() const { return num_items_; }
  void Clear();
  virtual ~HeapBase();
  bool InitHeap();
 private:
  bool InitInner();
  void Resize();
  inline bool Compare(const Node* node1, const Node* node2) const;
  void AdjustNode(size_t index);

 public:
  //const
  size_t init_size_heap_;
  FeatureF feature_;
  ///

  size_t num_items_;
  size_t size_container_;
  Node** heap_items_; // heap_items_[0] is not in use

  bool init_;

  template <
    typename v_t,
    bool o_t,
    typename f_t>
  friend std::ostream& operator<<(
    std::ostream& oss,
    const HeapBase<v_t, o_t, f_t>& heap_base);
};

template <
  typename Val,
  typename FeatureF = ReflectorF<Val>,
  bool Order=HeapCategory::kMinHeap>
class SimpleHeap : public HeapBase< 
    Val,
    Order, 
    FeatureF> {}; 

#ifndef WIN32

template <
  typename Key,
  typename Val,
  bool Order=HeapCategory::kMinHeap >
class SimpleKVHeap : public HeapBase< 
    std::pair<Key, Val>, 
    Order, 
    std::_Select1st< std::pair<Key, Val> > > {
 public:
  typedef HeapBase< 
    std::pair<Key, Val>,
    Order, 
    std::_Select1st< std::pair<Key, Val> > > Father;

 public: 
  inline void Insert(IN const Key& key, IN const Val& val, OUT size_t& sign);
};

#endif // !WIN32

template < typename Val, bool Order, typename FeatureF>
HeapBase<Val, Order, FeatureF>::HeapBase(size_t init_size_heap, FeatureF feature) : 
  init_size_heap_(init_size_heap),
  feature_(feature),
  num_items_(0), 
  heap_items_(NULL), 
  init_(false) {
}

template < typename Val, bool Order, typename FeatureF>
bool HeapBase<Val, Order, FeatureF>::InitHeap(){
  if (!init_) {
    return InitInner();
  }
  return false;
}

template < typename Val, bool Order, typename FeatureF>
void HeapBase<Val, Order, FeatureF>::Insert(IN const Val& val, size_t& sign) {

  if(num_items_+1 == size_container_) {
    Resize();
  }

  Node* new_item = heap_items_[++num_items_];
  new_item->val = val;

  int iter=num_items_;
  while( iter>1 && Compare(new_item, heap_items_[iter>>1]) ) {
    heap_items_[iter] = heap_items_[iter>>1];
    heap_items_[iter]->index = iter;
    iter>>=1;
  }

  heap_items_[iter] = new_item;
  new_item->index = iter;
  sign = reinterpret_cast<size_t>(new_item);
}

template < typename Val, bool Order, typename FeatureF>
void HeapBase<Val, Order, FeatureF>::Update(size_t sign, const Val& val) {

  Node* item = reinterpret_cast<Node*>(sign);
  item->val = val;
  AdjustNode(item->index);
}

template < typename Val, bool Order, typename FeatureF>
void HeapBase<Val, Order, FeatureF>::UpdateTop(const Val& val) {

  if (unlikely(0==num_items_)) return;
  heap_items_[1]->val = val;
  AdjustNode(1);
}

template < typename Val, bool Order, typename FeatureF>
void HeapBase<Val, Order, FeatureF>::Refresh(size_t sign) {

  Node* item = reinterpret_cast<Node*>(sign);
  AdjustNode(item->index);
}

template < typename Val, bool Order, typename FeatureF>
void HeapBase<Val, Order, FeatureF>::RefreshTop() {

  AdjustNode(1);
}

template < typename Val, bool Order, typename FeatureF>
size_t HeapBase<Val, Order, FeatureF>::Pop() {
  size_t sign = reinterpret_cast<size_t>(heap_items_[1]);
  Erase(sign);
  return sign;
}

template < typename Val, bool Order, typename FeatureF>
void HeapBase<Val, Order, FeatureF>::Erase(size_t sign) {

  if (unlikely(!num_items_)) return;

  Node* item = reinterpret_cast<Node*>(sign);
  heap_items_[item->index] = heap_items_[num_items_];
  heap_items_[num_items_--] = item;
  heap_items_[item->index]->index = item->index;
  AdjustNode(item->index);
}

template < typename Val, bool Order, typename FeatureF>
void HeapBase<Val, Order, FeatureF>::Clear() {
  num_items_=0;
}

template < typename Val, bool Order, typename FeatureF>
HeapBase<Val, Order, FeatureF>::~HeapBase() {
  Clear();

  if (NULL!=heap_items_) {
    for (size_t i=0; i<size_container_; ++i) {
      if (NULL!= heap_items_[i]){
        delete heap_items_[i];
      }
    }
    free(heap_items_);
  }
}

template < typename Val, bool Order, typename FeatureF>
bool HeapBase<Val, Order, FeatureF>::InitInner() {
  heap_items_=NULL;
  num_items_=0;
  size_container_=init_size_heap_;
  heap_items_ = reinterpret_cast<Node**>(malloc(sizeof(*heap_items_) * size_container_));
  memset(heap_items_, 0, sizeof(*heap_items_) * size_container_);
  for(size_t i=0; i<size_container_; ++i) {
    heap_items_[i] = new HeapItem<Val>;
  }

  init_=true;
  return true;
}

template < typename Val, bool Order, typename FeatureF>
void HeapBase<Val, Order, FeatureF>::Resize() { 
  Node** newheap_items_ = NULL;
  int orig_size = sizeof(Node*) * size_container_;
  newheap_items_ = reinterpret_cast<Node**>(::realloc(heap_items_, (orig_size << 1)));
  for(size_t i=0; i<size_container_; ++i) {
    newheap_items_[i+size_container_] = new Node;
  }

  heap_items_=newheap_items_;
  size_container_<<=1;
}

template < typename Val, bool Order, typename FeatureF>
bool HeapBase<Val, Order, FeatureF>::Compare(const Node* node1, const Node* node2) const {
  return HeapCategory::kMinHeap == Order 
    ? feature_(node1->val) < feature_(node2->val) 
    : feature_(node1->val) > feature_(node2->val);
}

template < typename Val, bool Order, typename FeatureF>
void HeapBase<Val, Order, FeatureF>::AdjustNode(size_t index) {
  if (unlikely(index>num_items_)) return;

  Node* node = heap_items_[index];
  if( index>1 && Compare(node, heap_items_[index>>1]) ) {
    /*
     * upstairs
     */
    do {
      heap_items_[index] = heap_items_[index>>1];
      heap_items_[index]->index = index;
      index>>=1;
    } while ( index>1 && Compare(node, heap_items_[index>>1]) );

    heap_items_[index] = node;
    heap_items_[index]->index = index;
  } else {
    /*
     * downstairs
     */
    size_t pos=index, iter = (index<<1), min;
    while (iter<=num_items_) {
      if ( iter==num_items_ || Compare(heap_items_[iter], heap_items_[iter+1]) ) {
        min = iter;
      } else {
        min = iter+1;
      }

      if(Compare(node, heap_items_[min])) {
        break;
      } else {
        heap_items_[pos] = heap_items_[min];
        heap_items_[pos]->index = pos;
      }

      pos = min;
      iter = (pos<<1);
    }

    heap_items_[pos] = node;
    heap_items_[pos]->index = pos;
  }
}

template < typename Val, bool Order, typename FeatureF>
std::ostream& operator<<(
    std::ostream& oss,
    const HeapBase<Val, Order, FeatureF>& heap_base) {
  for (size_t i=1; i <= heap_base.num_items_; ++i) {
    oss << heap_base.feature_(heap_base.heap_items_[i]->val) << '\t';
  }
  oss << std::endl;
  return oss;
}

#ifndef WIN32
template <typename Key, typename Val, bool Order>
void SimpleKVHeap<Key, Val, Order>::Insert(IN const Key& key, IN const Val& val, OUT size_t& sign) { 
  std::pair<Key, Val> tmp(key, val);
  Father::Insert(tmp, sign); 
}
#endif

}

