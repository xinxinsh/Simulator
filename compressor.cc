#include <iostream>
#include <string>

#include <assert.h>
#include <stdint.h>

#include "c_snappy.h"

using namespace std;

compressor* compressor::create(string& type) {
  if (type == "snappy") {
    return new c_snappy(type);
  } else {
    assert("unknow compress type" == 0);
  }
}
