#include <CGAL/Arr_point_location_result.h>
#include <CGAL/point_generators_2.h>
#include <CGAL/function_objects.h>
#include <CGAL/point_generators_2.h>
#include <CGAL/random_polygon_2.h>
#include <CGAL/Random.h>
#include <CGAL/algorithm.h>
//-----------------------------------------------------------------------------
// Print the result of a point-location query.
//
#if CGAL_ARR_POINT_LOCATION_VERSION < 2
template <class Arrangement_2>
void print_point_location(const typename Arrangement_2::Point_2& q,
                          CGAL::Object    obj)
{
  typename Arrangement_2::Vertex_const_handle    v;
  typename Arrangement_2::Halfedge_const_handle  e;
  typename Arrangement_2::Face_const_handle      f;

  std::cout << "The point (" << q << ") is located ";
  if (CGAL::assign(f, obj))
  {
    // q is located inside a face:
    if (f->is_unbounded())
      std::cout << "inside the unbounded face." << std::endl;
    else
      std::cout << "inside a bounded face." << std::endl;
  }
  else if (CGAL::assign(e, obj))
  {
    // q is located on an edge:
    std::cout << "on an edge: " << e->curve() << std::endl;
  }
  else if (CGAL::assign(v, obj))
  {
    // q is located on a vertex:
    if (v->is_isolated())
      std::cout << "on an isolated vertex: " << v->point() << std::endl;
    else
      std::cout << "on a vertex: " << v->point() << std::endl;
  }
  else
  {
    CGAL_error_msg( "Invalid object.");
  }
}
#else
template <typename Arrangement_2>
void
print_point_location
(const typename Arrangement_2::Point_2& q,
 typename CGAL::Arr_point_location_result<Arrangement_2>::Type obj)
{
  typedef typename Arrangement_2::Vertex_const_handle   Vertex_const_handle;
  typedef typename Arrangement_2::Halfedge_const_handle Halfedge_const_handle;
  typedef typename Arrangement_2::Face_const_handle     Face_const_handle;

  const Vertex_const_handle*   v;
  const Halfedge_const_handle* e;
  const Face_const_handle*     f;
  
  std::cout << "The point (" << q << ") is located ";
  if ( ( f = boost::get<Face_const_handle>(&obj) ) )          // located inside a face
    std::cout << "inside "
              << (((*f)->is_unbounded()) ? "the unbounded" : "a bounded")
              << " face." << std::endl;
  else if ( ( e = boost::get<Halfedge_const_handle>(&obj) ) ) // located on an edge
    std::cout << "on an edge: " << (*e)->curve() << std::endl;
  else if ( ( v = boost::get<Vertex_const_handle>(&obj) ) )   // located on a vertex
    std::cout << "on " << (((*v)->is_isolated()) ? "an isolated" : "a")
              << " vertex: " << (*v)->point() << std::endl;
  else CGAL_error_msg("Invalid object.");
}
#endif

//-----------------------------------------------------------------------------
// Perform a point-location query and print the result.
//
template <class PointLocation>
void point_location_query(const PointLocation& pl,
                          const typename
                          PointLocation::Arrangement_2::Point_2& q)
{
  // Perform the point-location query.
  typedef PointLocation                                 Point_location;
  typedef typename Point_location::Arrangement_2        Arrangement_2;
  typename CGAL::Arr_point_location_result<Arrangement_2>::Type obj =
    pl.locate(q);

  // Print the result.
  print_point_location<Arrangement_2>(q, obj);
}

//-----------------------------------------------------------------------------
// Perform a vertical ray-shooting query and print the result.
//
template <class VerticalRayShoot>
void vertical_ray_shooting_query(const VerticalRayShoot& vrs,
                                 const typename
                                 VerticalRayShoot::Arrangement_2::Point_2& q)
{
  typedef VerticalRayShoot                                      Vertical_ray_shooting;

  // Perform the point-location query.
  typename Vertical_ray_shooting::result_type obj = vrs.ray_shoot_up(q);
  
  // Print the result.
  typedef typename Vertical_ray_shooting::Arrangement_2 Arrangement_2;
  typedef typename Arrangement_2::Vertex_const_handle   Vertex_const_handle;
  typedef typename Arrangement_2::Halfedge_const_handle Halfedge_const_handle;
  typedef typename Arrangement_2::Face_const_handle     Face_const_handle;

  const Vertex_const_handle*   v;
  const Halfedge_const_handle* e;
  const Face_const_handle*     f;
  
  std::cout << "Shooting up from (" << q << ") : ";

  if ( (v = boost::get<Vertex_const_handle>(&obj)) )         // we hit a vertex
    std::cout << "hit " << (((*v)->is_isolated()) ? "an isolated" : "a")
              << " vertex: " << (*v)->point() << std::endl;
  else if ( (e = boost::get<Halfedge_const_handle>(&obj)) )  // we hit an edge
    std::cout << "hit an edge: " << (*e)->curve() << std::endl;
  else if ( (f = boost::get<Face_const_handle>(&obj)) ) {    // we hit nothing
    CGAL_assertion((*f)->is_unbounded());
    std::cout << "hit nothing." << std::endl;
  }
  else CGAL_error_msg("Invalid object.");
}

//-----------------------------------------------------------------------------
// Construct the arrangement of segments needed for the point-location and
// the vertical ray-shooting examples.
// The function assumes that the arrangement is of line segments with integer
// coordinates.
//			
template <class Arrangement_>
void construct_segments_arr(Arrangement_& arr)
{
  typedef Arrangement_                                Arrangement_2;
  typedef typename Arrangement_2::Point_2             Point_2;
  typedef typename Arrangement_2::X_monotone_curve_2  Segment_2;
  typedef typename Arrangement_2::Halfedge_handle     Halfedge_handle;  
  typedef CGAL::Creator_uniform_2<double, Point_2>             Creator;
  typedef CGAL::Random_points_in_square_2<Point_2, Creator> Point_generator;
  
  int N = 100;
  std::vector<Point_2> points;
  points.reserve(2*N);
  CGAL::copy_n_unique(Point_generator(10), 2*N,std::back_inserter(points));
  
  std::vector<Segment_2> segments;
  segments.reserve(N);
  for (int i=0;i<N; ++i)
    segments.push_back(Segment_2(points[2*i], points[2*i+1]));
  
  insert(arr, segments.begin(), segments.end());
}
