#include <iostream>
#include <map>
#include <assert.h>
#include <errno.h>

#include "alloc.h"
#include "SimulatorConfig.h"

using namespace std;

#define MIN(a,b)  ((a)<=(b) ? (a):(b))
#define MAX(a,b)  ((a)>=(b) ? (a):(b))
#define P2ALIGN(x, align) ((x) & -(align))

template<typename T>
interval_set<T>::interval_set() {
  intervals.clear();
}

template<typename T>
void interval_set<T>::insert(T off, T len, T* poff, T* plen) {
  std::cout << "insert range " << off << "~" << len << std::endl;
  typename std::map<T, T>::iterator it = intervals.lower_bound(off);
  if (it != intervals.begin() && (it == intervals.end() || it->first > off)) {
    it--;
    if (it->first + it->second < off)
      it++;
  }
  if (it == intervals.end()) {
    intervals[off] = len;
    if(poff)
      *poff = off;
    if(plen)
      *plen = len;
    return;
  } else {
    cout << "insert range " << it->first << "->" << it->second << " with off " << off << "->" << len << endl; 
    if(it->first >= off) {
      if (off + len == it->first) {
        std::cout << " merge after" << std::endl;
        intervals[off] = len + it->second;
        if(poff)
          *poff = it->first;
        if(plen)
          *plen = it->second + len;
        intervals.erase(it);
      } else {
        intervals[off] = len;
        if(poff)
          *poff = off;
        if(plen)
          *plen = len;
      }
    } else {
      if (it->first + it->second == off) {
        it->second += len;
        typename map<uint64_t, uint64_t>::iterator n = it;
        n++;
        if (n != intervals.end() && off + len == n->first) {
          it->second += n->second;
          intervals.erase(n);
        }
        if(poff)
          *poff = it->first;
        if(plen)
          *plen = it->second;
      } else {
        intervals[off] = len;
        if(poff)
          *poff = off;
        if(plen)
          *plen = len;
      }
    }
  } 
  size -= len;
#ifdef USE_LOG
  std::cout << "new range " << *poff << "~" << *plen << " size " << (size-len) << "->" << size << std::endl;
#endif
}

template<typename T>
bool interval_set<T>::contains(T off, T* poff, T* plen) {
  typename std::map<T, T>::iterator p = intervals.lower_bound(off);
  if (p != intervals.begin() &&
    (p == intervals.end() || p->first > off)) {
    p--;   
    if (p->first + p->second <= off)
      p++; 
  }
  if (p == intervals.end()) return false;
  if (p->first > off) return false;
  if (p->first+p->second <= off) return false;
  assert(p->first <= off && p->first+p->second > off);
  if (poff)
    *poff = p->first;
  if (plen)
    *plen = p->second;
  return true;
}

template<typename T>
void interval_set<T>::erase(T& off, T& len) {
  typename std::map<T, T>::iterator it = intervals.lower_bound(off);
  if (it != intervals.begin() && (it == intervals.end() || it->first < off)) {
     --it;
    if (it->first + it->second < off)
      ++it;
  }
#ifdef USE_LOG
  std::cout << "intervals size " << intervals.size() << endl;
  for(typename std::map<T, T>::iterator ip=intervals.begin(); ip!=intervals.end(); ++ip)
    std::cout << "range " << it->first << "~" << it->second << endl;
  std::cout << __func__ << " " << off << "->" << len << " in range " << it->first << "->" << it->second << endl;
#endif
  assert(it->second >= len);
  T before = off - it->first;
  T after = it->second - len - (off - it->first);
  if(before)
    it->second = off - it->first;
  else
    intervals.erase(it);
  if(after)
    intervals[off+len] = after;
  size += len;
}

template<typename T>
void interval_set<T>::insert(interval_set<T>& a) {
  typename std::map<T, T>::iterator it = a.intervals.begin();
  while(it != a.intervals.end())
    insert(it->first, it->second, 0, 0);
}

template<typename T>
void interval_set<T>::subtract(interval_set<T>& a) {
  typename std::map<T, T>::iterator it = a.intervals.begin();
  while(it != a.intervals.end())
    erase(it->first, it->second);
}

template<typename T>
void interval_set<T>::intersection_of(interval_set<T>& a, interval_set<T>& b) {
  intervals.clear();
  typename std::map<T, T>::iterator ia = a.intervals.begin();
  typename std::map<T, T>::iterator ib = b.intervals.begin();
  while(ia != a.intervals.end() && ib != b.intervals.end()) {
    if(ia->first + ia->second <= ib->first) {
      ia++;
      continue;
    }
    if(ib->first + ib->second <= ia->first) {
      ib++;
      continue;
    }
    T start = (ia->first > ib->first ? ia->first : ib->first);
    T end = ((ia->first + ia->second) < (ib->first + ib->second) ? ia->first + ia->second : ib->first + ib->second);
    insert(start, end - start);
    if((ia->first + ia->second) < (ib->first + ib->second))
      ia++;
    else
      ib++;
  }
}

template<typename T>
void interval_set<T>::union_of(interval_set<T>& a, interval_set<T>& b) {
  intervals = a.intervals;
  interval_set<T> ab = intersection(a, b);
  subtract(ab);
  insert(b);
}

template<typename T>
void interval_set<T>::union_of(interval_set<T>& a) {
  interval_set<T> t;
  t.intervals.swap(a.intervals); 
  union_of(t, a);
}

template<typename T>
void interval_set<T>::clear() {
  intervals.clear();
}

template<typename T>
interval_set<T>::~interval_set() {
  clear();
}

stupidallocator::stupidallocator(uint64_t size, uint64_t block_size) 
  : size(size), free_size(size), block_size(block_size), free(20), last_alloc(0) {
  _insert_free((uint64_t)0, size);
}

stupidallocator::~stupidallocator() {
  free.clear();
}

uint64_t stupidallocator::_choose_bin(uint64_t len) {
  uint64_t bin = 0;
  uint64_t tmp = len / block_size;
  while(tmp > 0) {
    tmp = tmp / 2;
    bin++;
  }
  return bin;
}

void stupidallocator::_insert_free(uint64_t off, uint64_t len) {
  uint64_t bin = _choose_bin(len);
  uint64_t poff, plen;
  while(true) {
    free[bin].insert(off, len, &poff, &plen);
    uint64_t newbin = _choose_bin(plen);
#ifdef USE_LOG
  std::cout << "new " << poff << "~" << plen << " bin " << bin << " new bin " << newbin << endl;
#endif
    if (bin == newbin)
      break;
    std::cout << "break";
    bin = newbin;
    off = poff;
    len = plen;
  }
  std::cout << "end insert free" << endl;
}

void stupidallocator::release(interval_set<uint64_t>& freed) {
#ifdef USE_LOG
   std::cout << "release " << freed << endl;
#endif
  interval_set<uint64_t>::iterator it = freed.begin();
  while(it != freed.end()) {
    _insert_free(it.get_start(), it.get_len());
    free_size += it.get_len();
    it++;
  }
}

uint64_t stupidallocator::_aligned_len(interval_set<uint64_t>::iterator p, uint64_t& alloc_unit) {
  uint64_t skew = p.get_start() % alloc_unit;
  if (skew)
    skew = alloc_unit - skew;
  if (skew > p.get_len())
    return 0;
  else
    return p.get_len() - skew;
}

int stupidallocator::allocate(uint64_t len, uint64_t alloc_unit, interval_set<uint64_t>& alloc) {
  std::cout << "allocate " << len << " bytes " << std::endl;
  uint64_t left = len;
  uint64_t poff, plen;
  uint64_t hint = 0;
  while(left > 0) {
    int r = _allocate(len, alloc_unit, hint, &poff, &plen);
    if (r != 0) {
      std::cout << "allocate " << len << " bytes error" << endl;
      return r; 
    }
    alloc.insert(poff, plen, 0, 0);
    left -= plen;
    hint = poff + plen;
  }
  std::cout << "allocate " << len << " bytes successful"<< std::endl;
  return 0;
}
int stupidallocator::_allocate(uint64_t size, uint64_t alloc_unit, uint64_t hint, uint64_t* offset, uint64_t* length) {
  uint64_t alloc = size > alloc_unit ? size : alloc_unit;
  uint64_t bin = _choose_bin(alloc);
  uint64_t orig_bin = bin;
  interval_set<uint64_t>::iterator p = free[0].begin();
#ifdef USE_LOG
  std::cout << __func__ << " " << size << " bytes with hint " <<  hint << "  unit " << alloc_unit << " in bin " << bin << std::endl;
#endif
  if(!hint) 
    hint = last_alloc;

  if (hint) {
    for (bin = orig_bin; bin < free.size(); ++bin) {
      p = free[bin].lower_bound(hint);
      while (p != free[bin].end()) {
	if (_aligned_len(p, alloc_unit) >= size) {
	  goto found;
	}
	++p;
      }
    }
  }
  
  for (bin = orig_bin; bin < (int)free.size(); ++bin) {
    p = free[bin].begin();
    interval_set<uint64_t>::iterator end = hint ? free[bin].lower_bound(hint) : free[bin].end();
    while (p != end) {
      if (_aligned_len(p, alloc_unit) >= size) {
	goto found;
      }
      ++p;
    }
  }

  if (hint) {
    for (bin = orig_bin; bin >= 0; --bin) {
      p = free[bin].lower_bound(hint);
      while (p != free[bin].end()) {
	if (_aligned_len(p, alloc_unit) >= alloc_unit) {
	  goto found;
	}
	++p;
      }
    }
  }
  
  for (bin = orig_bin; bin >= 0; --bin) {
    p = free[bin].begin();
    interval_set<uint64_t>::iterator end = hint ? free[bin].lower_bound(hint) : free[bin].end();
    while (p != end) {
      if (_aligned_len(p, alloc_unit) >= alloc_unit) {
	goto found;
      }
      ++p;
    }
  }

  return -ENOSPC;

  found:
    int64_t skew = p.get_start() % alloc_unit;
    if (skew)
      skew = alloc_unit - skew;
    *offset = p.get_start() + skew;
    *length = MIN(MAX(alloc_unit, size), P2ALIGN((p.get_len() - skew), alloc_unit));
    free[bin].erase(*offset, *length);
#ifdef USE_LOG
  std::cout << "allocated range " << *offset << "~" << *length << std::endl;
#endif
    uint64_t off, len;
    uint64_t p1 = *offset - 1;
    if (*offset && free[bin].contains(p1, &off, &len)) {
#ifdef USE_LOG
    std::cout << "demote range " << off << "~" << len << std::endl;
#endif
      int newbin = _choose_bin(len);
      if (newbin != bin) {
	free[bin].erase(off, len);
	_insert_free(off, len);
      }
    }
    if (free[bin].contains(*offset + *length, &off, &len)) {
#ifdef USE_LOG
    std::cout << "demote range " << off << "~" << len << std::endl;
#endif
      int newbin = _choose_bin(len);
      if (newbin != bin) {
	free[bin].erase(off, len);
	_insert_free(off, len);
      }
    }
    free_size -= *length;
    last_alloc = *offset+*length;
    return 0;
}
  
std::ostream& operator<<(std::ostream& out, stupidallocator& ac) {
  out << "size " << ac.size << " free_size " << ac.free_size << " block_size " << ac.block_size << '\n';
  for(std::vector<interval_set<uint64_t> >::iterator it=ac.free.begin(); it!=ac.free.end();it++)
    if(!it->empty())
      out << "bin " << (it-ac.free.begin()) << " : "  << *it << '\n';
  return out;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, interval_set<T> &a) {
  out << "[";
  for(typename interval_set<T>::iterator it = a.begin(); it != a.end(); it++) {
    out << it.get_start() << "~" << it.get_len() << ",";
  }
  out << "]";
  return out;
}

