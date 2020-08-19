#ifndef KALDI_MATRIX_KALDI_MATRIX_H_
#define KALDI_MATRIX_KALDI_MATRIX_H_ 1

#include <iostream>
#include <cassert>
#include "src/pitch/kaldi-common.h"

/// @{ \addtogroup matrix_funcs_scalar


/// \addtogroup matrix_group
/// @{

/// Base class which provides matrix operations not involving resizing
/// or allocation.   Classes Matrix and SubMatrix inherit from it and take care
/// of allocation and resizing.
template<typename Real>
class MatrixBase {
 public:
  // so this child can access protected members of other instances.
  friend class Matrix<Real>;
  /// Returns number of rows (or zero for emtpy matrix).
  inline MatrixIndexT  NumRows() const { return num_rows_; }

  /// Returns number of columns (or zero for emtpy matrix).
  inline MatrixIndexT NumCols() const { return num_cols_; }

  void Print()
  {
	  for(MatrixIndexT r=0;r<num_rows_;++r)
	  {
		  for(MatrixIndexT c=0;c<num_cols_;++c)
			  std::cout << *(data_ + r * stride_ + c) << " ";
		  std::cout << std::endl;
	  }
  }
  /// Stride (distance in memory between each row).  Will be >= NumCols.
  inline MatrixIndexT Stride() const {  return stride_; }

  /// Returns size in bytes of the data held by the matrix.
  size_t  SizeInBytes() const {
    return static_cast<size_t>(num_rows_) * static_cast<size_t>(stride_) *
        sizeof(Real);
  }

  /// Gives pointer to raw data (const).
  inline const Real* Data() const {
    return data_;
  }

  /// Gives pointer to raw data (non-const).
  inline Real* Data() { return data_; }

  inline SubMatrix<Real> Range(const MatrixIndexT row_offset,
		  const MatrixIndexT num_rows,
		  const MatrixIndexT col_offset,
		  const MatrixIndexT num_cols) const {
	      return SubMatrix<Real>(*this, row_offset, num_rows,
				  col_offset, num_cols);
  }

  inline SubMatrix<Real> RowRange(const MatrixIndexT row_offset,
		  const MatrixIndexT num_rows) const {
	  return SubMatrix<Real>(*this, row_offset, num_rows, 0, num_cols_);
  }


  /// Returns pointer to data for one row (non-const)
  inline  Real* RowData(MatrixIndexT i) {
    assert(static_cast<UnsignedMatrixIndexT>(i) <
                 static_cast<UnsignedMatrixIndexT>(num_rows_));
    return data_ + i * stride_;
  }

  /// Returns pointer to data for one row (const)
  inline const Real* RowData(MatrixIndexT i) const {
    assert(static_cast<UnsignedMatrixIndexT>(i) <
                 static_cast<UnsignedMatrixIndexT>(num_rows_));
    return data_ + i * stride_;
  }

  /// Indexing operator, non-const
  /// (only checks sizes if compiled with -DKALDI_PARANOID)
  inline Real&  operator() (MatrixIndexT r, MatrixIndexT c) {
    assert(static_cast<UnsignedMatrixIndexT>(r) <
                          static_cast<UnsignedMatrixIndexT>(num_rows_) &&
                          static_cast<UnsignedMatrixIndexT>(c) <
                          static_cast<UnsignedMatrixIndexT>(num_cols_));
    return *(data_ + r * stride_ + c);
  }
  /// Indexing operator, provided for ease of debugging (gdb doesn't work
  /// with parenthesis operator).
  Real &Index (MatrixIndexT r, MatrixIndexT c) {  return (*this)(r, c); }

  /// Indexing operator, const
  /// (only checks sizes if compiled with -DKALDI_PARANOID)
  inline const Real operator() (MatrixIndexT r, MatrixIndexT c) const {
    assert(static_cast<UnsignedMatrixIndexT>(r) <
                          static_cast<UnsignedMatrixIndexT>(num_rows_) &&
                          static_cast<UnsignedMatrixIndexT>(c) <
                          static_cast<UnsignedMatrixIndexT>(num_cols_));
    return *(data_ + r * stride_ + c);
  }

  inline Real*  Data_workaround() const {
	  return data_;
  }
  /*   Basic setting-to-special values functions. */

  /// Sets matrix to zero.
  void SetZero();
  /// Sets all elements to a specific value.
  void Set(Real);
  /// Sets to zero, except ones along diagonal [for non-square matrices too]
  void SetUnit();
  /// Sets to random values of a normal distribution
  void SetRandn();
  /// Sets to numbers uniformly distributed on (0, 1)
  void SetRandUniform();

  /*  Copying functions.  These do not resize the matrix! */


  /// Copy given matrix. (no resize is done).
  template<typename OtherReal>
  void CopyFromMat(const MatrixBase<OtherReal> & M,
                   MatrixTransposeType trans = kNoTrans);

  /// Return specific row of matrix [const].
  inline const SubVector<Real> Row(MatrixIndexT i) const {
    assert(static_cast<UnsignedMatrixIndexT>(i) <
                 static_cast<UnsignedMatrixIndexT>(num_rows_));
    return SubVector<Real>(data_ + (i * stride_), NumCols());
  }

  /// Return specific row of matrix.
  inline SubVector<Real> Row(MatrixIndexT i) {
    assert(static_cast<UnsignedMatrixIndexT>(i) <
                 static_cast<UnsignedMatrixIndexT>(num_rows_));
    return SubVector<Real>(data_ + (i * stride_), NumCols());
  }

  /// Copy vector into specific column of matrix.
  void CopyColFromVec(const VectorBase<Real> &v, const MatrixIndexT col);
  
 protected:

  ///  Initializer, callable only from child.
  explicit MatrixBase(Real *data, MatrixIndexT cols, MatrixIndexT rows, MatrixIndexT stride) :
    data_(data), num_cols_(cols), num_rows_(rows), stride_(stride) {
//    assert_IS_FLOATING_TYPE(Real);
  }

  ///  Initializer, callable only from child.
  /// Empty initializer, for un-initialized matrix.
  explicit MatrixBase(): data_(NULL),num_cols_(0),num_rows_(0),stride_(0) {
//    assert_IS_FLOATING_TYPE(Real);
  }

  // Make sure pointers to MatrixBase cannot be deleted.
  ~MatrixBase() { }


  /// data memory area
  Real*   data_;

  /// these atributes store the real matrix size as it is stored in memory
  /// including memalignment
  MatrixIndexT    num_cols_;   /// < Number of columns
  MatrixIndexT    num_rows_;   /// < Number of rows
  /** True number of columns for the internal matrix. This number may differ
   * from num_cols_ as memory alignment might be used. */
  MatrixIndexT    stride_;
 private:
//  KALDI_DISALLOW_COPY_AND_ASSIGN(MatrixBase);
};

/// A class for storing matrices.
template<typename Real>
class Matrix : public MatrixBase<Real> {
 public:

  /// Empty constructor.
  Matrix() { }

  /// Basic constructor.
  Matrix(const MatrixIndexT r, const MatrixIndexT c,
         MatrixResizeType resize_type = kSetZero,
         MatrixStrideType stride_type = kDefaultStride):
      MatrixBase<Real>() { Resize(r, c, resize_type, stride_type); }

  /// Swaps the contents of *this and *other.  Shallow swap.
  void Swap(Matrix<Real> *other);
  
  /// Constructor from any MatrixBase. Can also copy with transpose.
  /// Allocates new memory.
  explicit Matrix(const MatrixBase<Real> & M,
                  MatrixTransposeType trans = kNoTrans);
  
  /// Same as above, but need to avoid default copy constructor.
  Matrix(const Matrix<Real> & M);  //  (cannot make explicit)

  /// Copy constructor: as above, but from another type.
  template<typename OtherReal>
  explicit Matrix(const MatrixBase<OtherReal> & M,
                    MatrixTransposeType trans = kNoTrans);

  /// Remove a specified row.
//  void RemoveRow(MatrixIndexT i);
void RemoveRow(MatrixIndexT i) {
  assert(static_cast<UnsignedMatrixIndexT>(i) <
               static_cast<UnsignedMatrixIndexT>(MatrixBase<Real>::num_rows_)
               && "Access out of matrix");
  for (MatrixIndexT j = i + 1; j <  MatrixBase<Real>::num_rows_; j++)
    MatrixBase<Real>::Row(j-1).CopyFromVec( MatrixBase<Real>::Row(j));
  MatrixBase<Real>::num_rows_--;
}

  /// Distructor to free matrices.
  ~Matrix() { Destroy(); }

  /// Sets matrix to a specified size (zero is OK as long as both r and c are
  /// zero).  The value of the new data depends on resize_type:
  ///   -if kSetZero, the new data will be zero
  ///   -if kUndefined, the new data will be undefined
  ///   -if kCopyData, the new data will be the same as the old data in any
  ///      shared positions, and zero elsewhere.
  ///
  /// You can set stride_type to kStrideEqualNumCols to force the stride
  /// to equal the number of columns; by default it is set so that the stride
  /// in bytes is a multiple of 16.
  ///
  /// This function takes time proportional to the number of data elements.
  void Resize(const MatrixIndexT r,
              const MatrixIndexT c,
              MatrixResizeType resize_type = kSetZero,
              MatrixStrideType stride_type = kDefaultStride);

  /// Assignment operator that takes MatrixBase.
  Matrix<Real> &operator = (const MatrixBase<Real> &other) {
    if (MatrixBase<Real>::NumRows() != other.NumRows() ||
        MatrixBase<Real>::NumCols() != other.NumCols())
      Resize(other.NumRows(), other.NumCols(), kUndefined);
    MatrixBase<Real>::CopyFromMat(other);
    return *this;
  }

  /// Assignment operator. Needed for inclusion in std::vector.
  Matrix<Real> &operator = (const Matrix<Real> &other) {
    if (MatrixBase<Real>::NumRows() != other.NumRows() ||
        MatrixBase<Real>::NumCols() != other.NumCols())
      Resize(other.NumRows(), other.NumCols(), kUndefined);
    MatrixBase<Real>::CopyFromMat(other);
    return *this;
  }


 private:
  /// Deallocates memory and sets to empty matrix (dimension 0, 0).
//  void Destroy();
  void Destroy() {
    // we need to free the data block if it was defined
    if (NULL != MatrixBase<Real>::data_)
      KALDI_MEMALIGN_FREE( MatrixBase<Real>::data_);
    MatrixBase<Real>::data_ = NULL;
    MatrixBase<Real>::num_rows_ = MatrixBase<Real>::num_cols_
        = MatrixBase<Real>::stride_ = 0;
  }

  /// Init assumes the current class contents are invalid (i.e. junk or have
  /// already been freed), and it sets the matrix to newly allocated memory with
  /// the specified number of rows and columns.  r == c == 0 is acceptable.  The data
  /// memory contents will be undefined.
//  void Init(const MatrixIndexT r,
//            const MatrixIndexT c,
//            const MatrixStrideType stride_type);
    inline void Init(const MatrixIndexT rows,
			const MatrixIndexT cols,
			const MatrixStrideType stride_type) {
      if (rows * cols == 0) {
	    assert(rows == 0 && cols == 0);
		this->num_rows_ = 0;
		this->num_cols_ = 0;
		this->stride_ = 0;
		this->data_ = NULL;
		return;
	  }
      assert(rows > 0 && cols > 0);
	  MatrixIndexT skip, stride;
	  size_t size;
	  void *data;  // aligned memory block
	  void *temp;  // memory block to be really freed

	  // compute the size of skip and real cols
	  skip = ((16 / sizeof(Real)) - cols % (16 / sizeof(Real)))
		  % (16 / sizeof(Real));
	  stride = cols + skip;
	  size = static_cast<size_t>(rows) * static_cast<size_t>(stride)
		  * sizeof(Real);

	  // allocate the memory and set the right dimensions and parameters
	  if (NULL != (data = KALDI_MEMALIGN(16, size, &temp))) {
	  	  MatrixBase<Real>::data_        = static_cast<Real *> (data);
	  	  MatrixBase<Real>::num_rows_      = rows;
	  	  MatrixBase<Real>::num_cols_      = cols;
	  	  MatrixBase<Real>::stride_  = (stride_type == kDefaultStride ? stride : cols);
	  } else {
	  	  throw std::bad_alloc();
	  }
	}

};
/// @} end "addtogroup matrix_group"

/// \addtogroup matrix_funcs_io
/// @{

/**
  Sub-matrix representation.
  Can work with sub-parts of a matrix using this class.
  Note that SubMatrix is not very const-correct-- it allows you to
  change the contents of a const Matrix.  Be careful!
*/

template<typename Real>
class SubMatrix : public MatrixBase<Real> {
 public:
  // Initialize a SubMatrix from part of a matrix; this is
  // a bit like A(b:c, d:e) in Matlab.
  // This initializer is against the proper semantics of "const", since
  // SubMatrix can change its contents.  It would be hard to implement
  // a "const-safe" version of this class.
  SubMatrix(const MatrixBase<Real>& T,
            const MatrixIndexT ro,  // row offset, 0 < ro < NumRows()
            const MatrixIndexT r,   // number of rows, r > 0
            const MatrixIndexT co,  // column offset, 0 < co < NumCols()
            const MatrixIndexT c);   // number of columns, c > 0

  // This initializer is mostly intended for use in CuMatrix and related
  // classes.  Be careful!
  SubMatrix(Real *data,
            MatrixIndexT num_rows,
            MatrixIndexT num_cols,
            MatrixIndexT stride);

  ~SubMatrix<Real>() {}

  /// This type of constructor is needed for Range() to work [in Matrix base
  /// class]. Cannot make it explicit.
  SubMatrix<Real> (const SubMatrix &other):
  MatrixBase<Real> (other.data_, other.num_cols_, other.num_rows_,
                    other.stride_) {}

 private:
  /// Disallow assignment.
  SubMatrix<Real> &operator = (const SubMatrix<Real> &other);
};
/// @} End of "addtogroup matrix_funcs_io".

/// \addtogroup matrix_funcs_scalar
/// @{

// Some declarations.  These are traces of products.


template<typename Real>
bool ApproxEqual(const MatrixBase<Real> &A,
                 const MatrixBase<Real> &B, Real tol = 0.01) {
  return A.ApproxEqual(B, tol);
}

template<typename Real>
inline void AssertEqual(const MatrixBase<Real> &A, const MatrixBase<Real> &B,
                        float tol = 0.01) {
  assert(A.ApproxEqual(B, tol));
}

#endif  // KALDI_MATRIX_KALDI_MATRIX_H_
