/*!
  \ingroup PkgSolverConcepts
  \cgalConcept

  The concept `SvdTraits` describes the linear algebra types and algorithms needed 
  to solve in the least square sense a linear system with a singular value decomposition

  \cgalHasModel `CGAL::Eigen_svd`

*/
class SvdTraits {
public:

  /// \name Types 
  /// @{

  /*!
    The scalar type. 
  */ 
  typedef unspecified_type FT; 

  /*!
    The vector type, model of the concept `SvdTraits::Vector`.
  */ 
  typedef unspecified_type Vector;

  /*!
    The matrix type,  model of the concept `SvdTraits::Matrix`. 
  */ 
  typedef unspecified_type Matrix;
  
  /// @} 

  /// \name Operations 
  /// The concept `SvdTraits` has a linear solver using a
  /// singular value decomposition algorithm.
  /// @{

  /*!
    Solves the system \f$ MX=B\f$ (in the least square sense if \f$ M\f$ is not 
    square) using a singular value decomposition and returns the condition 
    number of \f$ M\f$. The solution is stored in \f$ B\f$. 
  */ 
  FT solve(const Matrix& M, Vector& B); 

  /// @}

}; /* end SvdTraits */

/*!
\cgalConcept
Concept of vector type used by the concept `SvdTraits`.

\cgalHasModel `CGAL::Eigen_vector<T>`
*/
class SvdTraits::Vector {
public:
  /*!
    initializes all the elements of the vector to zero. 
  */ 
  Vector(size_t n); 
  /*!
    returns the size of the vector
   */ 
  size_t size(); 

  /*!
    returns the `i`th entry, `i` from `0` to `size()-1`. 
  */ 
  FT operator()(size_t i); 

  /*!
    sets the `i`'th entry to `value`. 
  */ 
  void set(size_t i, const FT value); 

  /*!
    returns the vector as an array. 
  */ 
  FT* vector(); 
};


/*!
\cgalConcept
Concept of matrix type used by the concept `SvdTraits`.

\cgalHasModel `CGAL::Eigen_matrix<T>`
*/
class SvdTraits::Matrix {
public:
  /*!
    initializes all the entries of the matrix to zero. 
  */ 
  Matrix(size_t n1, size_t n2); 

  /*!
    returns the number of rows
   */ 
  size_t number_of_rows(); 

  /*!
     returns the number of columns
   */ 
  size_t number_of_columns(); 

  /*!
    returns the entry at row `i` and column `j`, `i` from `0` to `number_of_rows - 1`, 
    `j` from `0` to `number_of_columns - 1`. 
  */ 
  FT operator()(size_t i, size_t j); 

  /*!
    sets the entry at row `i` and column `j` to `value`. 
  */ 
  void set(size_t i, size_t j, const FT value); 
};

