#include <iostream>
#include <string>

#include "FS.h"
#include <assert.h> 
#include <cstdlib>
#include <ctime>
#include "SimulatorConfig.h"

using namespace std;

ostream& operator<<(ostream& out, extent& e) {
  out << "extent length " << e.len << " compression length " << e.c_len << " ";
  out << "compression type " << e.c_type << " ";
  out << e.exs;
  return out;
}

extent::extent(uint64_t len, interval_set<uint64_t>& exs, string c_type)
  : len(len), exs(exs), c_type(c_type) {
  for(interval_set<uint64_t>::iterator it = exs.begin(); it!=exs.end(); ++it)
    c_len = it.get_len();
}

void extent::release(uint64_t toff, uint64_t tlen, interval_set<uint64_t>& erased) {
  assert(toff+tlen <= len);
  uint64_t ps, pl;
  interval_set<uint64_t>::iterator it = exs.begin();
  while(it!=exs.end() && it.get_len() <= toff) {
    toff -= it.get_len();
    ++it;
  }
  ps = it.get_start()+toff;
  pl = it.get_len()-toff;
  erased.insert(ps, pl, 0, 0);
  exs.erase(ps, pl);
  tlen = tlen - pl; 
  it++;
  while(it != exs.end() && it.get_len() <= tlen) {
    ps = it.get_start();
    pl = it.get_len();
    erased.insert(ps, pl, 0, 0);
    exs.erase(ps, pl);
    ++it;
    tlen -= pl;
  }
  if (it != exs.end()) {
    ps = it.get_start();
    exs.erase(ps, tlen);
    erased.insert(ps, tlen, 0, 0);
  }
}

FS::FS(char* path, uint64_t& size, uint64_t& alloc_unit)
  : path(path), size(size), alloc_unit(alloc_unit) {
#ifdef USE_LOG
  std::cout << "initialize FS " << size << " allocation unit " << alloc_unit << std::endl;
#endif
  buf = new buffer(path, size);
  alloc = new stupidallocator(size, alloc_unit);
}

FS::~FS() {
  delete buf;
  delete alloc;
  extents.clear();
}

void FS::_update_extents(uint64_t off, uint64_t len, interval_set<uint64_t>& e) {
  std::cout << __func__ << " " << off << "~" << len << endl;
  map<uint64_t, extent>::iterator it = extents.lower_bound(off);
  if (it != extents.end())
  if (it != extents.begin() && (it == extents.end() || it->first > off)) {
    it--;
    if ((it->first + it->second.len) <= off) {
      it++;
    }
  }
  set<uint64_t> erased_keys;
  while((it != extents.end()) && (it->first + it->second.len <= off + len)) {
#ifdef USE_LOG
  if (it != extents.end()) {
    std::cout << "erase range " << off << "~" << len << " in [" << it->first << "~" << it->second.len << " @" << it->second << "]" << endl;
  }
  for(map<uint64_t, extent>::iterator ip=extents.begin(); ip!=extents.end(); ++ip)
    std::cout << "range " << ip->first << "~" << ip->second.len << " @" << ip->second << endl;
#endif
    interval_set<uint64_t> erased;
    //update free list
    if (it->first < off) {
      it->second.release(off-it->first, it->second.len-(off-it->first), erased);
    } else {
      erased = it->second.exs;
      erased_keys.insert(it->first);
      // we cannot use erase because after erase this iteraotr, the iterator behavior is not defined
      // extents.erase(it) 
    }
    ++it;
    alloc->release(erased);
#ifdef USE_LOG
  std::cout << "current allocator " << *alloc << endl;
#endif
  }
  for(set<uint64_t>::iterator is = erased_keys.begin(); is!=erased_keys.end(); ++is)
    extents.erase(*is);
#ifdef USE_LOG
  if (it != extents.end())
    std::cout << "erase last range " << off << "~" << len << " in [" << it->first << "~" << it->second.len << " @" << it->second << "]" << endl;
#endif
  if (it != extents.end()) {
    interval_set<uint64_t> erased;
    if (off > it->first) {
      it->second.release(off-it->first, len, erased);
      alloc->release(erased);
    } else if (off + len > it->first)  {
      uint64_t tlen = off+len-it->first;
      uint64_t toff = it->first;
      it->second.release(0, tlen, erased);
      alloc->release(erased);
    }
  }
  extent ae(len, e, "snappy");
  extents.insert(std::make_pair(off, ae));
#ifdef USE_LOG
  std::cout << "insert [" << off << "~" << len << "] -> " << ae << endl;
  for(map<uint64_t, extent>::iterator ip=extents.begin(); ip!=extents.end(); ++ip)
    std::cout << "range [" << ip->first << "~" << ip->second.len << "] -> " << ip->second << endl;
#endif
}

int FS::_write(interval_set<uint64_t>& e, string& write) {
  interval_set<uint64_t>::iterator it = e.begin();
  uint64_t off = 0;
  while(it != e.end()) {
#ifdef USE_LOG
  std::cout << "_write range " << it.get_start() << "~" << it.get_len() << endl;
#endif
    std::string substr = write.substr(off, it.get_len());
    buf->write(it.get_start(), it.get_len(), substr);
    off += it.get_len();
    it++;
  }
#ifdef USE_LOG
/*
  it = e.begin();
  std::cout << "_write range [";
  while (it != e.end()) {
    std::cout << it.get_start() << "~" << it.get_len() << "," << endl;
    it++;
  }
  std::cout << "] successful";
*/
#endif
  return 0;
}

void FS::write(uint64_t off, uint64_t len, std::string& write) {
  std::cout << "write " << off << "~" << len << endl;
  uint64_t head = off % alloc_unit;
  uint64_t tail = (off+len) % alloc_unit;
  tail = alloc_unit - tail;
  string h_buf(head, '-');
  string t_buf(tail, '-');
  bool r = read(off - head, head, h_buf);
  if (!r) {
    std::cout << "read head error" << endl;
    assert(r);
  }
  r = read(off+len, tail, t_buf);
  if (!r) {
    std::cout << "read tail error" << endl;
    assert(r);
  }
  string write_buf;
  write_buf.append(h_buf);
  write_buf.append(write);
  write_buf.append(t_buf);
  interval_set<uint64_t> aes;
  int rt = alloc->allocate(write_buf.size(), alloc_unit, aes);
#ifdef USE_LOG
  std::cout << "allocated extent " << aes << std::endl;
#endif
  if (rt < 0) {
    std::cerr << "can not allocate " << write_buf.size() << endl;
  }
  rt = _write(aes, write_buf);
  if (rt < 0) {
    std::cerr << "can not write " << write_buf.size() << " to extents " << aes << endl;
  }
  _update_extents(off-head, len+head+tail, aes);
  std::cout << "write range " << off << "~" << len << " successful"<< endl;
}

bool FS::_copy(std::string& dst, uint64_t d_off, const char* src, uint64_t off, uint64_t len)
{
  uint64_t i=0;
  while(i<len) {
    dst[d_off+i] = *(src+off+i);
    i++;
  }
}

int FS::_read(uint64_t off, uint64_t len, extent& e, std::string& read) {
#ifdef USE_LOG
  std::cout << "read " << off << "~" << len << " in " << e << endl;
#endif
  string tmp;
  interval_set<uint64_t>::iterator it = e.exs.begin();
  while(it != e.exs.end()) {
    std::string t(it.get_len(), '-');
#ifdef USE_LOG
    std::cout << __func__ << " it " << it.get_start() << "~" << it.get_len() << endl;
#endif
    int r = buf->read(it.get_start(), it.get_len(), t);
    if (r < 0) {
      std::cout << "read iterator error" << endl;
      assert(r>=0);
    }
    tmp.append(t);
    it++;
  }
  assert(tmp.size() == e.len);
  _copy(read, 0, tmp.c_str(), off, len);
}

bool FS::read(uint64_t off, uint64_t len, std::string& read) {
  std::cout << "read " << off << "~" << len << endl;
  map<uint64_t, uint64_t> res;
  _rw_extents(off, len, res);
  if (res.empty()) {
#ifdef USE_LOG
  std::cout << "read " << off << "~" << len << " is hole" <<  endl;
#endif
    return true;
  }
  map<uint64_t, uint64_t>::iterator it = res.begin();
  map<uint64_t, extent>::iterator ip = extents.lower_bound(it->first);
  if (ip != extents.begin() && (ip == extents.end() || ip->first > off)) {
    ip--;
    if (ip->first + ip->second.len < off)
      ip++;
  }
#ifdef USE_LOG
  std::cout << "read " << it->first << " @ " << it->second << " from extent " << ip->first << endl;
#endif
  while(it != res.end() && ip != extents.end()) {
    assert(ip->first <= it->first);
    std::string rt(it->second, '-');
    uint64_t toff = it->first-ip->first;
    int r = _read(toff, it->second, ip->second, rt);
    if (r < 0) {
      std::cerr << "read range " << it->first << "~" << it->second << " error " << endl;
      assert(r >= 0);
    }
    _copy(read, it->first-off, rt.c_str(), 0, rt.size());
    it++;
    ip++;
  } 
  std::cout << "read " << off << "~" << len << " successful" << endl;
  return true;
}

void FS::_rw_extents(uint64_t off, uint64_t len, std::map<uint64_t, uint64_t>& rws) {
  std::cout << __func__ << " " << off << "~" << len << std::endl;
  std::map<uint64_t,extent>::iterator it = extents.lower_bound(off);
  if (it != extents.begin() && (it == extents.end() || it->first > off)) {
    it--;
    if (it->first + it->second.len <= off) 
      it++;
  }
  if (it == extents.end()) {
    return;
  }
#ifdef USE_LOG
  std::cout << "current " << it->first << " -> " << it->second << std::endl;
#endif
  while(it != extents.end() && ((it->first + it->second.len) <= off + len)) {
    if (it->first < off) {
      // partial range of first extent
      std::cout << "insert after partial range " << off << "~" << (it->first+it->second.len-off) << " extent " << it->second << endl;
      rws.insert(std::make_pair(off, it->first+it->second.len-off));
    } else {
      // full range
      std::cout << "insert full range " << it->first << "~" << it->second.len << " extent " << it->second << endl;
      rws.insert(std::make_pair(it->first, it->second.len));
    }
    it++;
  }
  if (it != extents.end()) {
    if ((off + len > it->first) && (it->first >= off)) {
      // partial range of last extent
      std::cout << "insert before partial range " << it->first << "~" << (off+len-it->first) << " extent " << it->second << endl;
      rws.insert(std::make_pair(it->first, off+len-it->first));
    } else if (it->first < off) {
      // the first iterator include [off, len] range
      std::cout << "include range " << off << "~" << len << " extent " << it->second << endl;
      rws.insert(std::make_pair(off, len)); 
    }
  }
}
 
void FS::start(int counts) {
  std::srand(std::time(0));
  for(int i=0; i<counts; ++i) {
    std::cout << "=======================  " << i << "  =======================" << endl;
    int s = std::rand() % 100;
    std::string wr(s, 0);
    for(int j=0; j<s; ++j) {
      wr[j] = (std::rand() % 26) + 'a';
    }
    uint64_t off = std::rand() % size;
    uint64_t len = (size - off > wr.size() ? wr.size() : size - off);
    std::string sub = wr.substr(0, len);
    write(off, len, sub);
    std::string rd(len, 0);
    read(off, len, rd);
    std::cout << "EQUALS : " << sub.compare(rd) << std::endl;
  }
}

ostream& operator<<(ostream& out, FS& fs) {
  out << "allocator " << *(fs.alloc) ;
  out << "size " << fs.size << " unit " << fs.alloc_unit << '\n';
  out << fs.extents.size() << " extent in extents " << '\n';
  map<uint64_t, extent>::iterator ip = fs.extents.begin();
  while(ip!=fs.extents.end()) {
    out << "[" << ip->first << "~" << ip->second.len << "] -> " << ip->second << '\n';
    ip++;
  }
  out << '\n';
}
