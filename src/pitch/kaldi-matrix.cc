#include <cassert>
#include <cstring>
#include "src/pitch/kaldi-matrix.h"
#include "src/pitch/kaldi-type.h"
#include "src/pitch/kaldi-common.h"
#include "src/pitch/kaldi-vector.h"
/// hubo

// template instantiations.
template
void MatrixBase<float>::CopyFromMat(const MatrixBase<double> & M,
                                    MatrixTransposeType Trans);
template
void MatrixBase<double>::CopyFromMat(const MatrixBase<float> & M,
                                     MatrixTransposeType Trans);
template
void MatrixBase<float>::CopyFromMat(const MatrixBase<float> & M,
                                    MatrixTransposeType Trans);
template
void MatrixBase<double>::CopyFromMat(const MatrixBase<double> & M,
                                     MatrixTransposeType Trans);

// Copy constructor.  Copies data to newly allocated memory.
template<typename Real>
Matrix<Real>::Matrix (const MatrixBase<Real> & M,
                      MatrixTransposeType trans/*=kNoTrans*/)
    : MatrixBase<Real>() {
  if (trans == kNoTrans) {
    Resize(M.num_rows_, M.num_cols_);
    this->CopyFromMat(M);
  } else {
    Resize(M.num_cols_, M.num_rows_);
    this->CopyFromMat(M, kTrans);
  }
}

// Copy constructor.  Copies data to newly allocated memory.
template<typename Real>
Matrix<Real>::Matrix (const Matrix<Real> & M):
    MatrixBase<Real>() {
  Resize(M.num_rows_, M.num_cols_);
  this->CopyFromMat(M);
}

/// Copy constructor from another type.
template<typename Real>
template<typename OtherReal>
Matrix<Real>::Matrix(const MatrixBase<OtherReal> & M,
                     MatrixTransposeType trans) : MatrixBase<Real>() {
  if (trans == kNoTrans) {
    Resize(M.NumRows(), M.NumCols());
    this->CopyFromMat(M);
  } else {
    Resize(M.NumCols(), M.NumRows());
    this->CopyFromMat(M, kTrans);
  }
}

// Instantiate this constructor for float->double and double->float.
template
Matrix<float>::Matrix(const MatrixBase<double> & M,
                      MatrixTransposeType trans);
template
Matrix<double>::Matrix(const MatrixBase<float> & M,
                       MatrixTransposeType trans);


template<typename Real>
template<typename OtherReal>
void MatrixBase<Real>::CopyFromMat(const MatrixBase<OtherReal> &M,
                                   MatrixTransposeType Trans) {
  if (sizeof(Real) == sizeof(OtherReal) &&
      static_cast<const void*>(M.Data()) ==
      static_cast<const void*>(this->Data())) {
    // CopyFromMat called on same data.  Nothing to do (except sanity checks).
    assert(Trans == kNoTrans && M.NumRows() == NumRows() &&
                 M.NumCols() == NumCols() && M.Stride() == Stride());
    return;
  }
  if (Trans == kNoTrans) {
    assert(num_rows_ == M.NumRows() && num_cols_ == M.NumCols());
    for (MatrixIndexT i = 0; i < num_rows_; i++)
      (*this).Row(i).CopyFromVec(M.Row(i));
  } else {
    assert(num_cols_ == M.NumRows() && num_rows_ == M.NumCols());
    int32 this_stride = stride_, other_stride = M.Stride();
    Real *this_data = data_;
    const OtherReal *other_data = M.Data();
    for (MatrixIndexT i = 0; i < num_rows_; i++)
      for (MatrixIndexT j = 0; j < num_cols_; j++)
        this_data[i * this_stride + j] = other_data[j * other_stride + i];
  }
}

template<typename Real>
void MatrixBase<Real>::SetZero() {
  if (num_cols_ == stride_)
    memset(data_, 0, sizeof(Real)*num_rows_*num_cols_);
  else
    for (MatrixIndexT row = 0; row < num_rows_; row++)
      memset(data_ + row*stride_, 0, sizeof(Real)*num_cols_);
}

template<typename Real>
void MatrixBase<Real>::Set(Real value) {
  for (MatrixIndexT row = 0; row < num_rows_; row++) {
    for (MatrixIndexT col = 0; col < num_cols_; col++) {
      (*this)(row, col) = value;
    }
  }
}

template<typename Real>
void MatrixBase<Real>::SetUnit() {
  SetZero();
  for (MatrixIndexT row = 0; row < std::min(num_rows_, num_cols_); row++)
    (*this)(row, row) = 1.0;
}
/*
template<typename Real>
void MatrixBase<Real>::SetRandn() {
  RandomState rstate;
  for (MatrixIndexT row = 0; row < num_rows_; row++) {
    Real *row_data = this->RowData(row);
    MatrixIndexT nc = (num_cols_ % 2 == 1) ? num_cols_ - 1 : num_cols_;
    for (MatrixIndexT col = 0; col < nc; col += 2) {
      kaldi::RandGauss2(row_data + col, row_data + col + 1, &rstate);
    }
    if (nc != num_cols_) row_data[nc] = static_cast<Real>(kaldi::RandGauss(&rstate));
  }
}
*/
/*
template<typename Real>
void MatrixBase<Real>::SetRandUniform() {
  kaldi::RandomState rstate;
  for (MatrixIndexT row = 0; row < num_rows_; row++) {
    Real *row_data = this->RowData(row);
    for (MatrixIndexT col = 0; col < num_cols_; col++, row_data++) {
      *row_data = static_cast<Real>(kaldi::RandUniform(&rstate));
    }
  }
}
*/
template<typename Real>
void Matrix<Real>::Resize(const MatrixIndexT rows,
		const MatrixIndexT cols,
		MatrixResizeType resize_type,
		MatrixStrideType stride_type) {
  // the next block uses recursion to handle what we have to do if
  // resize_type == kCopyData.
  if (resize_type == kCopyData) {
    if (this->data_ == NULL || rows == 0) resize_type = kSetZero;  // nothing to copy.
    else if (rows == this->num_rows_ && cols == this->num_cols_) { return; } // nothing to do.
    else {
      // set tmp to a matrix of the desired size; if new matrix
      // is bigger in some dimension, zero it.
      MatrixResizeType new_resize_type =
          (rows > this->num_rows_ || cols > this->num_cols_) ? kSetZero : kUndefined;
      Matrix<Real> tmp(rows, cols, new_resize_type);
      MatrixIndexT rows_min = std::min(rows, this->num_rows_),
          cols_min = std::min(cols, this->num_cols_);
      tmp.Range(0, rows_min, 0, cols_min).
          CopyFromMat(this->Range(0, rows_min, 0, cols_min));
      tmp.Swap(this);
      // and now let tmp go out of scope, deleting what was in *this.
      return;
    }
  }
  // At this point, resize_type == kSetZero or kUndefined.

  if (MatrixBase<Real>::data_ != NULL) {
    if (rows == MatrixBase<Real>::num_rows_
        && cols == MatrixBase<Real>::num_cols_) {
      if (resize_type == kSetZero)
        this->SetZero();
      return;
    }
    else
      Destroy();
  }
  Init(rows, cols, stride_type);
  if (resize_type == kSetZero) MatrixBase<Real>::SetZero();
}

template<typename Real>
void Matrix<Real>::Swap(Matrix<Real> *other) {
  std::swap(this->data_, other->data_);
  std::swap(this->num_cols_, other->num_cols_);
  std::swap(this->num_rows_, other->num_rows_);
  std::swap(this->stride_, other->stride_);
}

template<typename Real>
void MatrixBase<Real>::CopyColFromVec(const VectorBase<Real> &rv,
		const MatrixIndexT col) {
	assert(rv.Dim() == num_rows_ &&	
			static_cast<UnsignedMatrixIndexT>(col) <
			static_cast<UnsignedMatrixIndexT>(num_cols_));

	const Real *rv_data = rv.Data();
	Real *col_data = data_ + col;
	for (MatrixIndexT r = 0; r < num_rows_; r++)
		col_data[r * stride_] = rv_data[r];
}

// Constructor... note that this is not const-safe as it would
// be quite complicated to implement a "const SubMatrix" class that
// would not allow its contents to be changed.
template<typename Real>
SubMatrix<Real>::SubMatrix(const MatrixBase<Real> &M,
                           const MatrixIndexT ro,
                           const MatrixIndexT r,
                           const MatrixIndexT co,
                           const MatrixIndexT c) {
  if (r == 0 || c == 0) {
    // we support the empty sub-matrix as a special case.
    assert(c == 0 && r == 0);
    this->data_ = NULL;
    this->num_cols_ = 0;
    this->num_rows_ = 0;
    this->stride_ = 0;
    return;
  }
  assert(static_cast<UnsignedMatrixIndexT>(ro) <
               static_cast<UnsignedMatrixIndexT>(M.NumRows()) &&
               static_cast<UnsignedMatrixIndexT>(co) <
               static_cast<UnsignedMatrixIndexT>(M.NumCols()) &&
               static_cast<UnsignedMatrixIndexT>(r) <=
               static_cast<UnsignedMatrixIndexT>(M.NumRows() - ro) &&
               static_cast<UnsignedMatrixIndexT>(c) <=
               static_cast<UnsignedMatrixIndexT>(M.NumCols() - co));
  // point to the begining of window
  MatrixBase<Real>::num_rows_ = r;
  MatrixBase<Real>::num_cols_ = c;
  MatrixBase<Real>::stride_ = M.Stride();
  MatrixBase<Real>::data_ = M.Data_workaround() +
      static_cast<size_t>(co) +
      static_cast<size_t>(ro) * static_cast<size_t>(M.Stride());
}


template<typename Real>
SubMatrix<Real>::SubMatrix(Real *data,
                           MatrixIndexT num_rows,
                           MatrixIndexT num_cols,
                           MatrixIndexT stride):
    MatrixBase<Real>(data, num_cols, num_rows, stride) { // caution: reversed order!
  if (data == NULL) {
    assert(num_rows * num_cols == 0);
    this->num_rows_ = 0;
    this->num_cols_ = 0;
    this->stride_ = 0;
  } else {
    assert(this->stride_ >= this->num_cols_);
  }
}

//Explicit instantiation of the classes
//Apparently, it seems to be necessary that the instantiation
//happens at the end of the file. Otherwise, not all the member
//functions will get instantiated.

template class Matrix<float>;
template class Matrix<double>;
template class MatrixBase<float>;
template class MatrixBase<double>;
template class SubMatrix<float>;
template class SubMatrix<double>;
