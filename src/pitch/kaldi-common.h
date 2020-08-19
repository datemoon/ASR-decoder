#ifndef __KALDI_COMMON_H__
#define __KALDI_COMMON_H__ 1

#include <limits>
#include <cstdlib>
#include <cmath>
extern "C"
{
#include "src/lib/cblas.h"
}
template<typename Real> class VectorBase;
template<typename Real> class Vector;
template<typename Real> class SubVector;

template<typename Real> class MatrixBase;
template<typename Real> class SubMatrix;
template<typename Real> class Matrix;

typedef int MatrixIndexT;
typedef unsigned int UnsignedMatrixIndexT;

typedef enum {
	kSetZero,
	kUndefined,
	kCopyData
} MatrixResizeType;


typedef enum {
	kTrans   = CblasTrans,
	kNoTrans = CblasNoTrans
} MatrixTransposeType;

typedef enum {
	kDefaultStride,
	kStrideEqualNumCols,
} MatrixStrideType;

double Exp(double x);

float Exp(float x);

double Log(double x);

float Log(float x);

#ifdef __ANDROID_API__
#define KALDI_MEMALIGN(align, size, pp_orig) \
         malloc(size)
#else
#define KALDI_MEMALIGN(align, size, pp_orig) \
	     (!posix_memalign(pp_orig, align, size) ? *(pp_orig) : NULL)
#endif
#define KALDI_MEMALIGN_FREE(x) free(x)

/// return abs(a - b) <= relative_tolerance * (abs(a)+abs(b)).
static inline bool ApproxEqual(float a, float b,
		float relative_tolerance = 0.001) {
	// a==b handles infinities.
	if (a == b) return true;
	float diff = std::abs(a-b);
	if (diff == std::numeric_limits<float>::infinity()
			|| diff != diff) return false;  // diff is +inf or nan.
	return (diff <= relative_tolerance*(std::abs(a)+std::abs(b)));
}

// State for thread-safe random number generator
struct RandomState {
	RandomState();
	unsigned seed;
};

int Rand(struct RandomState* state = NULL);

inline float RandUniform(struct RandomState* state = NULL) {
	return static_cast<float>((Rand(state) + 1.0) / (RAND_MAX+2.0));
}

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#ifndef M_2PI
#define M_2PI 6.283185307179586476925286766559005
#endif
inline float RandGauss(struct RandomState* state = NULL) {
	return static_cast<float>(sqrtf (-2 * Log(RandUniform(state)))
			* cosf(2*M_PI*RandUniform(state)));
}


#endif
