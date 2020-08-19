#ifndef __READ_VECTOR_H__
#define __READ_VECTOR_H__

#include <istream>
#include <vector>

#include "src/util/namespace-start.h"

void ReadVector(std::istream &is , std::vector<float> &vec , bool binary);

#include "src/util/namespace-end.h"

#endif
