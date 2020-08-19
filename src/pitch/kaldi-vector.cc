#include <iostream>
#include <algorithm>
#include <limits>
#include <string>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include "src/pitch/kaldi-vector.h"
//#include "src/kaldi-matrix.h"
#include "src/pitch/cblas-wrappers.h"
using namespace std;


template<typename Real>
Real VecVec(const VectorBase<Real> &a,
            const VectorBase<Real> &b) {
  MatrixIndexT adim = a.Dim();
  assert(adim == b.Dim());
  return cblas_Xdot(adim, a.Data(), 1, b.Data(), 1);
//  Real sum = 0;
//  for(MatrixIndexT i=0; i<a.Dim(); ++i)
//	  sum += a(i) * b (i);
//  return sum;
}

template
float VecVec<>(const VectorBase<float> &a,
               const VectorBase<float> &b);
template
double VecVec<>(const VectorBase<double> &a,
                const VectorBase<double> &b);

template<typename Real, typename OtherReal>
Real VecVec(const VectorBase<Real> &ra,
            const VectorBase<OtherReal> &rb) {
  MatrixIndexT adim = ra.Dim();
  assert(adim == rb.Dim());
  const Real *a_data = ra.Data();
  const OtherReal *b_data = rb.Data();
  Real sum = 0.0;
  for (MatrixIndexT i = 0; i < adim; i++)
    sum += a_data[i]*b_data[i];
  return sum;
}

// instantiate the template above.
template
float VecVec<>(const VectorBase<float> &ra,
               const VectorBase<double> &rb);
template
double VecVec<>(const VectorBase<double> &ra,
                const VectorBase<float> &rb);


template<>
template<>
void VectorBase<float>::AddVec(const float alpha,
                               const VectorBase<float> &v) {
  assert(dim_ == v.dim_);
  assert(&v != this);
  cblas_Xaxpy(dim_, alpha, v.Data(), 1, data_, 1);
}

template<>
template<>
void VectorBase<double>::AddVec(const double alpha,
                                const VectorBase<double> &v) {
  assert(dim_ == v.dim_);
  assert(&v != this);
  cblas_Xaxpy(dim_, alpha, v.Data(), 1, data_, 1);
}

template<typename Real>
void VectorBase<Real>::AddMatVec(const Real alpha,
                                  const MatrixBase<Real> &M,
                                  MatrixTransposeType trans,
                                  const VectorBase<Real> &v,
                                  const Real beta) {
  assert((trans == kNoTrans && M.NumCols() == v.dim_ && M.NumRows() == dim_)
               || (trans == kTrans && M.NumRows() == v.dim_ && M.NumCols() == dim_));
  assert(&v != this);
  cblas_Xgemv(trans, M.NumRows(), M.NumCols(), alpha, M.Data(), M.Stride(),
              v.Data(), 1, beta, data_, 1);
}

template<typename Real>
inline void Vector<Real>::Init(const MatrixIndexT dim) {
  assert(dim >= 0);
  if (dim == 0) {
    this->dim_ = 0;
	this->data_ = NULL;
	return;
  }
  MatrixIndexT size;
  void *data;
  void *free_data;

  size = dim * sizeof(Real);

  if ((data = KALDI_MEMALIGN(16, size, &free_data)) != NULL) {
    this->data_ = static_cast<Real*> (data);
	this->dim_ = dim;
  } else {
    throw std::bad_alloc();
  }
}


template<typename Real>
void Vector<Real>::Resize(const MatrixIndexT dim, MatrixResizeType resize_type) {

  // the next block uses recursion to handle what we have to do if
  // resize_type == kCopyData.
  if (resize_type == kCopyData) {
    if (this->data_ == NULL || dim == 0) resize_type = kSetZero;  // nothing to copy.
    else if (this->dim_ == dim) { return; } // nothing to do.
    else {
      // set tmp to a vector of the desired size.
      Vector<Real> tmp(dim, kUndefined);
      if (dim > this->dim_) {
        memcpy(tmp.data_, this->data_, sizeof(Real)*this->dim_);
        memset(tmp.data_+this->dim_, 0, sizeof(Real)*(dim-this->dim_));
      } else {
        memcpy(tmp.data_, this->data_, sizeof(Real)*dim);
      }
      tmp.Swap(this);
      // and now let tmp go out of scope, deleting what was in *this.
      return;
    }
  }
  // At this point, resize_type == kSetZero or kUndefined.

  if (this->data_ != NULL) {
    if (this->dim_ == dim) {
      if (resize_type == kSetZero) this->SetZero();
      return;
    } else {
      Destroy();
    }
  }
  Init(dim);
  if (resize_type == kSetZero) this->SetZero();
}


/// Copy data from another vector
template<typename Real>
void VectorBase<Real>::CopyFromVec(const VectorBase<Real> &v) {
  assert(Dim() == v.Dim());
  if (data_ != v.data_) {
    std::memcpy(this->data_, v.data_, dim_ * sizeof(Real));
  }
}


/// Load data into the vector
//template<typename Real>
//void VectorBase<Real>::CopyFromPtr(const Real *data, MatrixIndexT sz) {
//  assert(dim_ == sz);
//  std::memcpy(this->data_, data, Dim() * sizeof(Real));
//}

template<typename Real>
template<typename OtherReal>
void VectorBase<Real>::CopyFromVec(const VectorBase<OtherReal> &other) {
  assert(dim_ == other.Dim());
  Real * __restrict__  ptr = data_;
  const OtherReal * __restrict__ other_ptr = other.Data();
  for (MatrixIndexT i = 0; i < dim_; i++)
    ptr[i] = other_ptr[i];
}

template void VectorBase<float>::CopyFromVec(const VectorBase<double> &other);
template void VectorBase<double>::CopyFromVec(const VectorBase<float> &other);

// Remove element from the vector. The vector is non reallocated
template<typename Real>
void Vector<Real>::RemoveElement(MatrixIndexT i) {
  assert(i <  this->dim_ && "Access out of vector");
  for (MatrixIndexT j = i + 1; j <  this->dim_; j++)
    this->data_[j-1] =  this->data_[j];
  this->dim_--;
}


/// Deallocates memory and sets object to empty vector.
template<typename Real>
void Vector<Real>::Destroy() {
  /// we need to free the data block if it was defined
  if (this->data_ != NULL)
    KALDI_MEMALIGN_FREE(this->data_);
  this->data_ = NULL;
  this->dim_ = 0;
}

template<typename Real>
void VectorBase<Real>::SetZero() {
  std::memset(data_, 0, dim_ * sizeof(Real));
}

template<typename Real>
bool VectorBase<Real>::IsZero(Real cutoff) const {
  Real abs_max = 0.0;
  for (MatrixIndexT i = 0; i < Dim(); i++)
    abs_max = std::max(std::abs(data_[i]), abs_max);
  return (abs_max <= cutoff);
}

template<typename Real>
void VectorBase<Real>::Set(Real f) {
  // Why not use memset here?
  for (MatrixIndexT i = 0; i < dim_; i++) { data_[i] = f; }
}

// Computes the p-th norm. Throws exception if could not.
template<typename Real>
Real VectorBase<Real>::Norm(Real p) const {
  assert(p >= 0.0);
  Real sum = 0.0;
  if (p == 0.0) {
    for (MatrixIndexT i = 0; i < dim_; i++)
      if (data_[i] != 0.0) sum += 1.0;
    return sum;
  } else if (p == 1.0) {
    for (MatrixIndexT i = 0; i < dim_; i++)
      sum += std::abs(data_[i]);
    return sum;
  } else if (p == 2.0) {
    for (MatrixIndexT i = 0; i < dim_; i++)
      sum += data_[i] * data_[i];
    return std::sqrt(sum);
  } else if (p == std::numeric_limits<Real>::infinity()){
    for (MatrixIndexT i = 0; i < dim_; i++)
      sum = std::max(sum, std::abs(data_[i]));
    return sum;
  } else {
    Real tmp;
    bool ok = true;
    for (MatrixIndexT i = 0; i < dim_; i++) {
      tmp = pow(std::abs(data_[i]), p);
//      if (tmp == HUGE_VAL) // HUGE_VAL is what pow returns on error.
//        ok = false;
      sum += tmp;
    }
    tmp = pow(sum, static_cast<Real>(1.0/p));
//    assert(tmp != HUGE_VAL); // should not happen here.
    if (ok) {
      return tmp;
    } else {
      Real maximum = this->Max(), minimum = this->Min(),
          max_abs = std::max(maximum, -minimum);
      assert(max_abs > 0); // Or should not have reached here.
      Vector<Real> tmp(*this);
      tmp.Scale(1.0 / max_abs);
      return tmp.Norm(p) * max_abs;
    }
  }
}

template<typename Real>
bool VectorBase<Real>::ApproxEqual(const VectorBase<Real> &other, float tol) const {
  if (dim_ != other.dim_) std::cerr << "ApproxEqual: size mismatch "
                                    << dim_ << " vs. " << other.dim_ << std::endl;
  assert(tol >= 0.0);
  if (tol != 0.0) {
    Vector<Real> tmp(*this);
    tmp.AddVec(-1.0, other);
    return (tmp.Norm(2.0) <= static_cast<Real>(tol) * this->Norm(2.0));
  } else { // Test for exact equality.
    const Real *data = data_;
    const Real *other_data = other.data_;
    for (MatrixIndexT dim = dim_, i = 0; i < dim; i++)
      if (data[i] != other_data[i]) return false;
    return true;
  }
}

template<typename Real>
Real VectorBase<Real>::Max() const {
  Real ans = - std::numeric_limits<Real>::infinity();
  const Real *data = data_;
  MatrixIndexT i, dim = dim_;
  for (i = 0; i + 4 <= dim; i += 4) {
    Real a1 = data[i], a2 = data[i+1], a3 = data[i+2], a4 = data[i+3];
    if (a1 > ans || a2 > ans || a3 > ans || a4 > ans) {
      Real b1 = (a1 > a2 ? a1 : a2), b2 = (a3 > a4 ? a3 : a4);
      if (b1 > ans) ans = b1;
      if (b2 > ans) ans = b2;
    }
  }
  for (; i < dim; i++)
    if (data[i] > ans) ans = data[i];
  return ans;
}

template<typename Real>
Real VectorBase<Real>::Max(MatrixIndexT *index_out) const {
  if (dim_ == 0) std::cerr << "Empty vector" << std::endl;
  Real ans = - std::numeric_limits<Real>::infinity();
  MatrixIndexT index = 0;
  const Real *data = data_;
  MatrixIndexT i, dim = dim_;
  for (i = 0; i + 4 <= dim; i += 4) {
    Real a1 = data[i], a2 = data[i+1], a3 = data[i+2], a4 = data[i+3];
    if (a1 > ans || a2 > ans || a3 > ans || a4 > ans) {
      if (a1 > ans) { ans = a1; index = i; }
      if (a2 > ans) { ans = a2; index = i + 1; }
      if (a3 > ans) { ans = a3; index = i + 2; }
      if (a4 > ans) { ans = a4; index = i + 3; }
    }
  }
  for (; i < dim; i++)
    if (data[i] > ans) { ans = data[i]; index = i; }
  *index_out = index;
  return ans;
}

template<typename Real>
Real VectorBase<Real>::Min() const {
  Real ans = std::numeric_limits<Real>::infinity();
  const Real *data = data_;
  MatrixIndexT i, dim = dim_;
  for (i = 0; i + 4 <= dim; i += 4) {
    Real a1 = data[i], a2 = data[i+1], a3 = data[i+2], a4 = data[i+3];
    if (a1 < ans || a2 < ans || a3 < ans || a4 < ans) {
      Real b1 = (a1 < a2 ? a1 : a2), b2 = (a3 < a4 ? a3 : a4);
      if (b1 < ans) ans = b1;
      if (b2 < ans) ans = b2;
    }
  }
  for (; i < dim; i++)
    if (data[i] < ans) ans = data[i];
  return ans;
}

template<typename Real>
Real VectorBase<Real>::Min(MatrixIndexT *index_out) const {
  if (dim_ == 0) std::cerr << "Empty vector" << std::endl;
  Real ans = std::numeric_limits<Real>::infinity();
  MatrixIndexT index = 0;
  const Real *data = data_;
  MatrixIndexT i, dim = dim_;
  for (i = 0; i + 4 <= dim; i += 4) {
    Real a1 = data[i], a2 = data[i+1], a3 = data[i+2], a4 = data[i+3];
    if (a1 < ans || a2 < ans || a3 < ans || a4 < ans) {
      if (a1 < ans) { ans = a1; index = i; }
      if (a2 < ans) { ans = a2; index = i + 1; }
      if (a3 < ans) { ans = a3; index = i + 2; }
      if (a4 < ans) { ans = a4; index = i + 3; }
    }
  }
  for (; i < dim; i++)
    if (data[i] < ans) { ans = data[i]; index = i; }
  *index_out = index;
  return ans;
}

template<typename Real>
Real VectorBase<Real>::Sum() const {
  double sum = 0.0;
  for (MatrixIndexT i = 0; i < dim_; i++) { sum += data_[i]; }
  return sum;
}

template<typename Real>
void VectorBase<Real>::Add(Real c) {
  for (MatrixIndexT i = 0; i < dim_; i++) {
    data_[i] += c;
  }
}

template<typename Real>
void VectorBase<Real>::Scale(Real alpha) {
  cblas_Xscal(dim_, alpha, data_, 1);
}

template<typename Real>
void VectorBase<Real>::AddVecVec(Real alpha, const VectorBase<Real> &v,
                                 const VectorBase<Real> &r, Real beta) {
  assert(v.data_ != this->data_ && r.data_ != this->data_);
  // We pretend that v is a band-diagonal matrix.
  assert(dim_ == v.dim_ && dim_ == r.dim_);
  cblas_Xgbmv(kNoTrans, dim_, dim_, 0, 0, alpha, v.data_, 1,
              r.data_, 1, beta, this->data_, 1);
}

template<typename Real>
template<typename OtherReal>
void VectorBase<Real>::AddVec(const Real alpha, const VectorBase<OtherReal> &v) {
  assert(dim_ == v.dim_);
  // remove __restrict__ if it causes compilation problems.
  register Real *__restrict__ data = data_;
  register OtherReal *__restrict__ other_data = v.data_;
  MatrixIndexT dim = dim_;
  if (alpha != 1.0)
    for (MatrixIndexT i = 0; i < dim; i++)
      data[i] += alpha * other_data[i];
  else
    for (MatrixIndexT i = 0; i < dim; i++)
      data[i] += other_data[i];
}

template
void VectorBase<float>::AddVec(const float alpha, const VectorBase<double> &v);
template
void VectorBase<double>::AddVec(const double alpha, const VectorBase<float> &v);

template<typename Real>
template<typename OtherReal>
void VectorBase<Real>::AddVec2(const Real alpha, const VectorBase<OtherReal> &v) {
  assert(dim_ == v.dim_);
  // remove __restrict__ if it causes compilation problems.
  register Real *__restrict__ data = data_;
  register OtherReal *__restrict__ other_data = v.data_;
  MatrixIndexT dim = dim_;
  if (alpha != 1.0)
    for (MatrixIndexT i = 0; i < dim; i++)
      data[i] += alpha * other_data[i] * other_data[i];
  else
    for (MatrixIndexT i = 0; i < dim; i++)
      data[i] += other_data[i] * other_data[i];
}

template
void VectorBase<float>::AddVec2(const float alpha, const VectorBase<double> &v);
template
void VectorBase<double>::AddVec2(const double alpha, const VectorBase<float> &v);

template<typename Real>
void VectorBase<Real>::AddVec2(const Real alpha, const VectorBase<Real> &v) {
  assert(dim_ == v.dim_);
  for (MatrixIndexT i = 0; i < dim_; i++)
    data_[i] += alpha * v.data_[i] * v.data_[i];
}


template<typename Real>
Real VecMatVec(const VectorBase<Real> &v1, const MatrixBase<Real> &M,
               const VectorBase<Real> &v2) {
  assert(v1.Dim() == M.NumRows() && v2.Dim() == M.NumCols());
  Vector<Real> vtmp(M.NumRows());
  vtmp.AddMatVec(1.0, M, kNoTrans, v2, 0.0);
  return VecVec(v1, vtmp);
}

template
float VecMatVec(const VectorBase<float> &v1, const MatrixBase<float> &M,
                const VectorBase<float> &v2);
template
double VecMatVec(const VectorBase<double> &v1, const MatrixBase<double> &M,
                 const VectorBase<double> &v2);

template<typename Real>
void Vector<Real>::Swap(Vector<Real> *other) {
  std::swap(this->data_, other->data_);
  std::swap(this->dim_, other->dim_);
}


template class Vector<float>;
template class Vector<double>;
template class VectorBase<float>;
template class VectorBase<double>;

