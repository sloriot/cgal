
namespace CGAL {

/*!
\ingroup PkgSolver

The class `Eigen_svd` provides an algorithm to solve in the least 
square sense a linear system with a singular value decomposition using 
\ref thirdpartyEigen. 

\cgalModels `SvdTraits`

*/

class Eigen_svd {
public:
  /// Field number type
  typedef double FT;
  /// Vector type
  typedef Eigen_vector<FT> Vector;
  /// Matrix type
  typedef Eigen_matrix<FT> Matrix;

}; /* end Eigen_svd */
} /* end namespace CGAL */
