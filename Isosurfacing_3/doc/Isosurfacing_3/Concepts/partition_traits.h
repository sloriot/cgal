namespace CGAL {
namespace Isosurfacing {

/*!
 * \ingroup PkgIsosurfacing3Concepts
 *
 * \cgalConcept
 *
 * \relates Partition_3
 *
 * The class `partition_traits` is the API compatibility layer between a model of `Partition_3`
 * and the isosurfacing domain classes `CGAL::Isosurfacing::Marching_cubes_domain_3` and
 * `CGAL::Isosurfacing::Dual_contouring_domain_3`.
 *
 * For each model of `Partition_3`, a partial specialization of `partition_traits` must be provided,
 * providing the types and functions listed below. Such a partial specialization is provided
 * for `CGAL::Isosurfacing::Cartesian_grid_3`.
 */
template <typename Partition_3>
class partition_traits
{
public:
  /*!
   * A vertex descriptor corresponds to a unique vertex in an abstract partition instance.
   */
  typedef unspecified_type Vertex_descriptor;

  /*!
   * An edge descriptor corresponds to a unique edge in an abstract partition instance.
   */
  typedef unspecified_type Edge_descriptor;

  /*!
   * A cell descriptor corresponds to a unique edge in an abstract partition instance.
   */
  typedef unspecified_type Cell_descriptor;

  /*!
   * A container for the two vertices of an edge.
   * Must be a model of `RandomAccessContainer` whose `value_type` must be `Vertex_descriptor`.
   */
  typedef unspecified_type Vertices_incident_to_edge;

  /*!
   * A container for the cells incident to an edge.
   * Must be a model of `ForwardRange` whose `value_type` must be `Cell_descriptor`.
   */
  typedef unspecified_type Cells_incident_to_edge;

  /*!
   * A container for the vertices of a cell.
   * Must be a model of `ForwardRange` whose `value_type` must be `Vertex_descriptor`.
   */
  typedef unspecified_type Cell_vertices;

  /*!
   * A container for the edges of a cell.
   * Must be a model of `ForwardRange` whose `value_type` must be `Edge_descriptor`.
   */
  typedef unspecified_type Cell_edges;

  /*!
   * \returns the 3D position of the vertex `v`.
   */
  static Point_3 point(const Vertex_descriptor& v, const Partition_3& partition);

  /*!
   * \returns the two vertices incident to the edge `e`.
   */
  static Vertices_incident_to_edge incident_vertices(const Edge_descriptor& e, const Partition_3& partition);

  /*!
   * \returns all the cells incident to the edge `e`, in a geometrically ordered manner around the edge.
   */
  static Cells_incident_to_edge incident_cells(const Edge_descriptor& e, const Partition_3& partition);

  /*!
   * \returns all the vertices of the cell `c`.
   */
  static Cell_vertices cell_vertices(const Cell_descriptor& c, const Partition_3& partition);

  /*!
   * \returns all the edges of the cell `c`.
   */
  static Cell_edges cell_edges(const Cell_descriptor& c, const Partition_3& partition);

  /*!
   * iterates over all vertices, and calls the functor `f` on each one.
   *
   * \tparam ConcurrencyTag decides if the vertices are iterated sequentially or in parallel.
   * Can be either `CGAL::Sequential_tag`, `CGAL::Parallel_if_available_tag`, or `CGAL::Parallel_tag`.
   * \tparam Functor must implement `void operator()(const Vertex_descriptor& vertex)`
   *
   * \param f the functor called on every vertex
   * \param partition the partition whose vertices are being iterated
  */
  template <typename ConcurrencyTag, typename Functor>
  static void for_each_vertex(Functor& f, const Partition_3& partition);

  /*!
   * iterates over all edges, and calls the functor `f` on each one.
   *
   * \tparam ConcurrencyTag decides if the edges are iterated sequentially or in parallel.
   * Can be either `CGAL::Sequential_tag`, `CGAL::Parallel_if_available_tag`, or `CGAL::Parallel_tag`.
   * \tparam Functor must implement `void operator()(const Edge_descriptor& edge)`.
   *
   * \param f the functor called on every edge
   * \param partition the partition whose edges are being iterated
   */
  template <typename ConcurrencyTag, typename Functor>
  static void for_each_edge(Functor& f, const Partition_3& partition);

  /*!
   * iterates over all cells, and calls the functor `f` on each one.
   *
   * \tparam ConcurrencyTag decides if the cells are iterated sequentially or in parallel.
   * Can be either `CGAL::Sequential_tag`, `CGAL::Parallel_if_available_tag`, or `CGAL::Parallel_tag`.
   * \tparam Functor must implement `void operator()(const Cell_descriptor& cell)`.
   *
   * \param f the functor called on every cell
   * \param partition the partition whose cells are being iterated
   */
  template <typename ConcurrencyTag, typename Functor>
  static void for_each_cell(Functor& f, const Partition_3& partition);
};

} // namespace Isosurfacing
} // namespace CGAL