#ifndef __READ_VECTOR_H__
#define __READ_VECTOR_H__

#include <istream>
#include <vector>


#ifdef NAMESPACE
namespace datemoon {
#endif

void ReadVector(std::istream &is , std::vector<float> &vec , bool binary);
#ifdef NAMESPACE
} // namespace datemoon
#endif
#endif
