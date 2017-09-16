#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <vector>
#include <iterator>
#include <set>
#include <map>
#include <istream>
#include <ostream>

#include <stdint.h>

using namespace std;

template<class T>
class interval_set {
public:
  class iterator : public std::iterator <std::forward_iterator_tag, T>
  {
    public:
      explicit iterator(typename std::map<T,T>::iterator iter)
        : _iter(iter)
      { }

      bool operator==(const iterator& rhs) const {
        return (_iter == rhs._iter);
      }

      bool operator!=(const iterator& rhs) const {
        return (_iter != rhs._iter);
      }
      std::pair < T, T > &operator*() {
        return *_iter;
      }
      T get_start() const {
        return _iter->first;
      }  
      T get_len() const {
        return _iter->second;
      }
      void set_len(T len) {
        _iter->second = len;
      }
      iterator &operator++()
      {
        ++_iter;
        return *this;
      }
      iterator operator++(int)
      {
        iterator prev(_iter);
        ++_iter;
        return prev;
      }
    protected:
      typename std::map<T,T>::iterator _iter;
  };
  
  interval_set();
  typename interval_set<T>::iterator begin() {
    return typename interval_set<T>::iterator(intervals.begin());
  }

  typename interval_set<T>::iterator lower_bound(T start) {
    typename std::map<T, T>::iterator p = intervals.lower_bound(start);
    if (p != intervals.begin() &&
      (p == intervals.end() || p->first > start)) {
      p--;   
      if (p->first + p->second <= start)
        p++; 
    }
    return typename interval_set<T>::iterator(p);
  }

  typename interval_set<T>::iterator end() {
    return typename interval_set<T>::iterator(intervals.end());
  }

  void insert(T off, T len, T* poff, T* plen);
  bool contains(T off, T* poff, T* plen);
  void erase(T& off, T& len);
  void insert(interval_set<T>& a);
  void subtract(interval_set<T>& a);
  void intersection_of(interval_set<T>& a, interval_set<T>& b);
  void union_of(interval_set<T>& a, interval_set<T>& b);
  void union_of(interval_set<T>& a);
  void clear();
  bool empty() {return intervals.empty();}
  virtual ~interval_set();

  std::map<T, T> intervals;
  T size;
};

template<typename T>
std::ostream& operator<<(std::ostream& out, interval_set<T> &a); 

class stupidallocator {
public:
  stupidallocator(uint64_t size, uint64_t block_size);
  uint64_t _choose_bin(uint64_t len);
  void _insert_free(uint64_t off, uint64_t len);
  uint64_t _aligned_len(interval_set<uint64_t>::iterator p, uint64_t& alloc_unit);
  int allocate(uint64_t len, uint64_t alloc_unit, interval_set<uint64_t>& alloc);
  int _allocate(uint64_t size, uint64_t alloc_unit, uint64_t hint, uint64_t* offset, uint64_t* length);
  void release(interval_set<uint64_t>& freed);
  virtual ~stupidallocator();
  uint64_t size;
  uint64_t free_size;
  uint64_t block_size;
  uint64_t last_alloc;
  std::vector<interval_set<uint64_t> > free;
};

std::ostream& operator<<(std::ostream& out, stupidallocator& ac);

#endif
