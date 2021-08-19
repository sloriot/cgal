// STL.
#include <array>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>

// Boost.
#include <boost/type_index.hpp>

// Kernels.
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Simple_homogeneous.h>

// CGAL.
#include <CGAL/Surface_mesh.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/point_generators_2.h>
#include <CGAL/Join_input_iterator.h>
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>
#ifndef CGAL_DONT_USE_LAZY_KERNEL
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#endif
#include <CGAL/Surface_sweep_2_algorithms.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Counting_iterator.h>
#include <CGAL/function_objects.h>
#include <CGAL/Exact_rational.h>
#include <CGAL/Exact_integer.h>
#include <CGAL/Lazy_exact_nt.h>
#include <CGAL/Real_timer.h>

using Timer = CGAL::Real_timer;
using ET    = CGAL::Exact_rational;
using EI    = CGAL::Exact_integer;
using EPECK = CGAL::Exact_predicates_exact_constructions_kernel;
using SCKER = CGAL::Simple_cartesian<ET>;
using LAZY1 = CGAL::Simple_cartesian< CGAL::Lazy_exact_nt<ET> >;
using LAZY2 = CGAL::Filtered_kernel<LAZY1>; // basically the same as EPECK
using LAZY3 = CGAL::Lazy_kernel<SCKER>;     // basically the same as LAZY2
using LAZY4 = CGAL::Simple_cartesian< CGAL::Interval_nt<false> >; // pure interval
using HOMOG = CGAL::Simple_homogeneous<EI>; // works for nef, but only for the intersection part, not for IO

#ifndef CGAL_DONT_USE_LAZY_KERNEL
namespace PMP = CGAL::Polygon_mesh_processing;
namespace params = CGAL::Polygon_mesh_processing::parameters;
#endif

enum class BENCH_TYPE {
  ALL = 0,
  NEF = 1,
  PMP = 2,
  ARR = 3
};

template<typename Kernel>
void print_parameters(const std::size_t num_iters, const bool verbose) {

  if (verbose) {
    std::cout << "* Parameters:" << std::endl;
    std::cout << "- Number of iterations N: " << num_iters << std::endl;
    std::cout << "- Kernel: " << boost::typeindex::type_id<Kernel>() << std::endl;
    std::cout << std::endl;

    #if defined(CGAL_DONT_USE_LAZY_KERNEL)
      std::cout << "- CGAL_DONT_USE_LAZY_KERNEL: true" << std::endl;
    #else
      std::cout << "- CGAL_DONT_USE_LAZY_KERNEL: false" << std::endl;
    #endif

    #if defined(CGAL_DISABLE_GMP)
      std::cout << "- CGAL_DISABLE_GMP: true" << std::endl;
    #else
      std::cout << "- CGAL_DISABLE_GMP: false" << std::endl;
    #endif

    #if defined(CGAL_USE_GMP)
      std::cout << "- CGAL_USE_GMP: true" << std::endl;
    #else
      std::cout << "- CGAL_USE_GMP: false" << std::endl;
    #endif

    #if defined(CGAL_DISABLE_GMPXX)
      std::cout << "- CGAL_DISABLE_GMPXX: true" << std::endl;
    #else
      std::cout << "- CGAL_DISABLE_GMPXX: false" << std::endl;
    #endif

    #if defined(CGAL_USE_GMPXX)
      std::cout << "- CGAL_USE_GMPXX: true" << std::endl;
    #else
      std::cout << "- CGAL_USE_GMPXX: false" << std::endl;
    #endif
    std::cout << std::endl;

    #if defined(CGAL_USE_CORE)
      std::cout << "- CGAL_USE_CORE: true" << std::endl;
    #else
      std::cout << "- CGAL_USE_CORE: false" << std::endl;
    #endif

    #if defined(CGAL_USE_CPP_INT)
      std::cout << "- CGAL_USE_CPP_INT: true" << std::endl;
    #else
      std::cout << "- CGAL_USE_CPP_INT: false" << std::endl;
    #endif

    #if defined(CGAL_USE_CPP_RATIONAL)
      std::cout << "- CGAL_USE_CPP_RATIONAL: true" << std::endl;
    #else
      std::cout << "- CGAL_USE_CPP_RATIONAL: false" << std::endl;
    #endif

    #if defined(CGAL_USE_LEDA)
      std::cout << "- CGAL_USE_LEDA: true" << std::endl;
    #else
      std::cout << "- CGAL_USE_LEDA: false" << std::endl;
    #endif
    std::cout << std::endl;

    #if defined(CGAL_DO_NOT_USE_BOOST_MP)
      std::cout << "- CGAL_DO_NOT_USE_BOOST_MP: true" << std::endl;
    #else
      std::cout << "- CGAL_DO_NOT_USE_BOOST_MP: false" << std::endl;
    #endif

    #if defined(CGAL_USE_BOOST_MP)
      std::cout << "- CGAL_USE_BOOST_MP: true" << std::endl;
    #else
      std::cout << "- CGAL_USE_BOOST_MP: false" << std::endl;
    #endif
    std::cout << std::endl;
  }

  std::cout << "* CHOSEN EXACT RATIONAL TYPE:" << std::endl;
  std::cout << boost::typeindex::type_id<ET>() << std::endl;
  std::cout << std::endl;

  std::cout << "* CHOSEN EXACT INTEGER TYPE:" << std::endl;
  std::cout << boost::typeindex::type_id<EI>() << std::endl;
  std::cout << std::endl;
}

template<typename Segment_2>
void print_segments(
  const std::string name, const std::vector<Segment_2>& segments) {

  std::ofstream out(name + ".polylines.txt");
  for (const auto& segment : segments) {
    out << "2 " << segment.source() << " 0 " << segment.target() << " 0 " << std::endl;
  }
  out.close();
}

template<typename Polygon_mesh>
bool read_meshes(
  const std::string filename1, const std::string filename2, const bool verbose,
  Polygon_mesh& A, Polygon_mesh& B) {

  if (verbose) {
    std::cout << "- file #1: " << filename1 << std::endl;
    std::cout << "- file #2: " << filename2 << std::endl;
  }

  std::ifstream inA("data/" + filename1);
  std::ifstream inB("data/" + filename2);

  inA >> A;
  assert(A.number_of_faces() > 0);
  assert(A.number_of_vertices() > 0);
  if (A.number_of_faces() == 0) return false;

  inB >> B;
  assert(B.number_of_faces() > 0);
  assert(B.number_of_vertices() > 0);
  if (B.number_of_faces() == 0) return false;

  inA.close();
  inB.close();
  return true;
}

template<typename Kernel, typename Segment_2>
void generate_random_segments_1(
  const std::size_t num_segments, std::vector<Segment_2>& segments, const bool verbose) {

  using Point_2 = typename Kernel::Point_2;
  using C1 = CGAL::Creator_uniform_2<double, Point_2>;
  using C2 = CGAL::Creator_uniform_2<Point_2, Segment_2>;
  using P1 = CGAL::Random_points_on_segment_2<Point_2, C1>;
  using P2 = CGAL::Random_points_on_circle_2<Point_2, C1>;
  using JI = CGAL::Join_input_iterator_2<P1, P2, C2>;

  segments.clear();
  segments.reserve(num_segments);

  P1 p1(Point_2(-100, 0), Point_2(100, 0));
  P2 p2(250);
  JI generator(p1, p2);
  std::copy_n(generator, num_segments, std::back_inserter(segments));

  if (verbose) print_segments("rnd-segs-1", segments);
  assert(segments.size() == num_segments);
}

template<typename Kernel, typename Segment_2>
void generate_random_segments_2(
  const std::size_t num_segments, std::vector<Segment_2>& segments, const bool verbose) {

  using Point_2 = typename Kernel::Point_2;
  using PG = CGAL::Points_on_segment_2<Point_2>;
  using Creator = CGAL::Creator_uniform_2<Point_2, Segment_2>;
  using JI = CGAL::Join_input_iterator_2<PG, PG, Creator>;
  using CI = CGAL::Counting_iterator<JI, Segment_2>;

  segments.clear();
  segments.reserve(num_segments);

  PG p1(Point_2(-250,  -50), Point_2(-250,  50), num_segments / 2);
  PG p2(Point_2( 250, -250), Point_2( 250, 250), num_segments / 2);
  JI t1(p1, p2);
  CI t1_begin(t1);
  CI t1_end(t1, num_segments / 2);
  std::copy(t1_begin, t1_end, std::back_inserter(segments));

  PG p3(Point_2( -50, -250), Point_2(  50, -250), num_segments / 2);
  PG p4(Point_2(-250,  250), Point_2( 250,  250), num_segments / 2);
  JI t2(p3, p4);
  CI t2_begin(t2);
  CI t2_end(t2, num_segments / 2);
  std::copy(t2_begin, t2_end, std::back_inserter(segments));

  if (verbose) print_segments("rnd-segs-2", segments);
  assert(segments.size() == num_segments);
}

template<typename Kernel>
double run_nef_bench(
  const std::string filename1, const std::string filename2,
  const std::size_t num_iters, const bool verbose) {

  using Point_3        = typename Kernel::Point_3;
  using Polygon_mesh   = CGAL::Surface_mesh<Point_3>;
  using Nef_polyhedron = CGAL::Nef_polyhedron_3<Kernel>;

  Polygon_mesh A, B;
  if (!read_meshes(filename1, filename2, verbose, A, B)) return 0.0;

  Timer timer;
  double avg_time = 0.0;
  for (std::size_t k = 0; k < num_iters; ++k) {
    timer.start();

    // Running nef.
    Nef_polyhedron nefA(A), nefB(B);
    const Nef_polyhedron nefC = nefA.intersection(nefB);

    timer.stop();
    avg_time += timer.time();
    timer.reset();
  }
  avg_time /= static_cast<double>(num_iters);
  if (verbose) {
    std::cout << "- avg time: " << avg_time << " sec." << std::endl;
    std::cout << std::endl;
  }
  return avg_time;
}

template<typename Kernel>
double run_pmp_bench(
  const std::string filename1, const std::string filename2,
  const std::size_t num_iters, const bool verbose) {

  using Point_3      = typename Kernel::Point_3;
  using Polygon_mesh = CGAL::Surface_mesh<Point_3>;

  Polygon_mesh A, B, AA, BB;
  if (!read_meshes(filename1, filename2, verbose, A, B)) return 0.0;

  Timer timer;
  double avg_time = 0.0;
  for (std::size_t k = 0; k < num_iters; ++k) {
    AA = A; BB = B;
    timer.start();

    // Running pmp.
    #ifndef CGAL_DONT_USE_LAZY_KERNEL
    Polygon_mesh out_union, out_intersection;
    std::array<boost::optional<Polygon_mesh*>, 4> output;
    output[PMP::Corefinement::UNION] = &out_union;
    output[PMP::Corefinement::INTERSECTION] = &out_intersection;
    PMP::corefine_and_compute_boolean_operations(
      AA, BB,
      output,
      params::all_default(), // A named parameters
      params::all_default(), // B named parameters
      std::make_tuple(
        params::vertex_point_map(get(boost::vertex_point, out_union)),        // named parameters for out_union
        params::vertex_point_map(get(boost::vertex_point, out_intersection)), // named parameters for out_intersection
        params::all_default(), // named parameters for mesh1-mesh2 not used
        params::all_default()) // named parameters for mesh2-mesh1 not used)
    );
    #endif

    timer.stop();
    avg_time += timer.time();
    timer.reset();
  }
  avg_time /= static_cast<double>(num_iters);
  if (verbose) {
    std::cout << "- avg time: " << avg_time << " sec." << std::endl;
    std::cout << std::endl;
  }
  return avg_time;
}

template<typename Kernel>
double run_arr_bench(
  const std::string type, const std::size_t num_segments,
  const std::size_t num_iters, const bool verbose) {

  using Point_2 = typename Kernel::Point_2;
  using Arr_traits_2 = CGAL::Arr_segment_traits_2<Kernel>;
  using Segment_2 = typename Arr_traits_2::Curve_2;

  std::vector<Segment_2> segments;
  if (type == "rnd-segs-1") generate_random_segments_1<Kernel>(num_segments, segments, verbose);
  else if (type == "rnd-segs-2") generate_random_segments_2<Kernel>(num_segments, segments, verbose);
  else generate_random_segments_1<Kernel>(num_segments, segments, verbose);

  Timer timer;
  double avg_time = 0.0;
  std::vector<Point_2> result;
  for (std::size_t k = 0; k < num_iters; ++k) {
    timer.start();

    // Running arr.
    CGAL::compute_intersection_points(
      segments.begin(), segments.end(), std::back_inserter(result));

    timer.stop();
    avg_time += timer.time();
    timer.reset();
    assert(segments.size() == num_segments);
    result.clear();
  }
  avg_time /= static_cast<double>(num_iters);
  if (verbose) {
    std::cout << "- avg time: " << avg_time << " sec." << std::endl;
    std::cout << std::endl;
  }
  return avg_time;
}

// Works only with kernels indicated here:
// https://doc.cgal.org/latest/Nef_3/classCGAL_1_1Nef__polyhedron__3.html
template<typename Kernel>
void run_all_nef_benches(const std::size_t num_iters, const bool verbose) {

  std::vector<double> times;
  std::cout << "* benching NEF ..." << std::endl;

  // Use it to debug ET types.

  // These work for any type!
  // std::cout << "test1" << std::endl;
  // times.push_back(run_nef_bench<Kernel>("triangle-1.off", "triangle-1.off", num_iters, verbose));
  // std::cout << "test2" << std::endl;
  // times.push_back(run_nef_bench<Kernel>("triangle-1.off", "triangle-2.off", num_iters, verbose));
  // std::cout << "test3" << std::endl;
  // times.push_back(run_nef_bench<Kernel>("triangle-1.off", "triangle-3.off", num_iters, verbose));

  // These do not work for all types including gmp!
  // std::cout << "test4" << std::endl;
  // times.push_back(run_nef_bench<Kernel>("triangle-1.off", "triangle-4.off", num_iters, verbose));
  // std::cout << "test5" << std::endl;
  // times.push_back(run_nef_bench<Kernel>("triangle-1.off", "triangle-5.off", num_iters, verbose));

  // Always works.
  // std::cout << "test6" << std::endl;
  // times.push_back(run_nef_bench<Kernel>("tetrahedron-1.off", "tetrahedron-1.off", num_iters, verbose));

  // Works only with Simple_cartesian.
  // std::cout << "test7" << std::endl;
  // times.push_back(run_nef_bench<Kernel>("tetrahedron-1.off", "tetrahedron-2.off", num_iters, verbose));

  // Real use cases.
  // std::cout << "test-real" << std::endl;

  times.push_back(run_nef_bench<Kernel>("sphere.off", "spheregrid.off", num_iters, verbose));
  times.push_back(run_nef_bench<Kernel>("sphere.off", "rotated-spheregrid.off", num_iters, verbose));
  times.push_back(run_nef_bench<Kernel>("spheregrid.off", "shifted-spheregrid.off", num_iters, verbose));
  times.push_back(run_nef_bench<Kernel>("rotated-spheregrid.off", "rotated-shifted-spheregrid.off", num_iters, verbose));

  if (!verbose) {
    std::cout << "{|class=\"wikitable\" style=\"text-align:center;margin-right:1em;\" " << std::endl;
    std::cout << "! # !! ";
    std::cout << "N !! ";
    std::cout << "ET !! ";
    std::cout << "sphere -- spheregrid !! ";
    std::cout << "sphere -- rotated-spheregrid !! ";
    std::cout << "spheregrid -- shifted-spheregrid !! ";
    std::cout << "rotated-spheregrid -- rotated-shifted-spheregrid ";
    std::cout << std::endl;
    std::cout << "|-" << std::endl;
    std::cout << "| #";
    std::cout << " || " << num_iters;
    std::cout << " || " << boost::typeindex::type_id<ET>();
    for (std::size_t k = 0; k < times.size(); ++k) {
      std::cout << " || " << times[k];
    }
    std::cout << std::endl << "|}" << std::endl;
  }
}

// Not very representative because they do not really use EPECK internally!
template<typename Kernel>
void run_all_pmp_benches(const std::size_t num_iters, const bool verbose) {

  std::vector<double> times;
  std::cout << "* benching PMP ..." << std::endl;
  CGAL_assertion_msg(false, "WARNING: These benches are not representative!");

  times.push_back(run_pmp_bench<Kernel>("blobby.off", "eight.off", num_iters, verbose));
  times.push_back(run_pmp_bench<Kernel>("cheese.off", "cheese-rotated.off", num_iters, verbose));

  if (!verbose) {
    std::cout << "{|class=\"wikitable\" style=\"text-align:center;margin-right:1em;\" " << std::endl;
    std::cout << "! # !! ";
    std::cout << "N !! ";
    std::cout << "ET !! ";
    std::cout << "blobby -- eight !! ";
    std::cout << "cheese -- cheese-rotated ";
    std::cout << std::endl;
    std::cout << "|-" << std::endl;
    std::cout << "| #";
    std::cout << " || " << num_iters;
    std::cout << " || " << boost::typeindex::type_id<ET>();
    for (std::size_t k = 0; k < times.size(); ++k) {
      std::cout << " || " << times[k];
    }
    std::cout << std::endl << "|}" << std::endl;
  }
}

template<typename Kernel>
void run_all_arr_benches(const std::size_t num_iters, const bool verbose) {

  std::vector<double> times;
  std::cout << "* benching ARR ..." << std::endl;

  const std::size_t num_segments = 2000;
  times.push_back(run_arr_bench<Kernel>("rnd-segs-1", num_segments, num_iters, verbose));
  times.push_back(run_arr_bench<Kernel>("rnd-segs-2", num_segments, num_iters, verbose));

  if (!verbose) {
    std::cout << "{|class=\"wikitable\" style=\"text-align:center;margin-right:1em;\" " << std::endl;
    std::cout << "! # !! ";
    std::cout << "N !! ";
    std::cout << "ET !! ";
    std::cout << "random segments 1 !! ";
    std::cout << "random segments 2 ";
    std::cout << std::endl;
    std::cout << "|-" << std::endl;
    std::cout << "| #";
    std::cout << " || " << num_iters;
    std::cout << " || " << boost::typeindex::type_id<ET>();
    for (std::size_t k = 0; k < times.size(); ++k) {
      std::cout << " || " << times[k];
    }
    std::cout << std::endl << "|}" << std::endl;
  }
}

int main(int argc, char* argv[]) {

  std::cout.precision(4);
  std::cout.setf(std::ios::fixed, std::ios::floatfield);

  std::cout << std::endl;
  std::cout << " --- NT BENCH --- " << std::endl;
  std::cout << std::endl;

  // Parameters.
  const bool verbose = false; // do we print extra info
  const std::string btype = ( (argc > 1) ? std::string(argv[1]) : "all" ); // bench type
  const std::size_t num_iters = ( (argc > 2) ? std::atoi(argv[2]) : 1 ); // number of iterations to average the timing

  // Choose a kernel.
  // using Kernel = SCKER; // pure arithmetic, works for nef and arr
  using Kernel = EPECK; // full support, real use case, fails for nef and works for arr

  // using Kernel = LAZY1; // lazy evaluation 1, works for nef and arr
  // using Kernel = LAZY2; // = EPECK, lazy evaluation 2, fails for nef and works for arr
  // using Kernel = LAZY3; // = EPECK, lazy evaluation 3, fails for nef and works for arr
  // using Kernel = LAZY4; // lazy evaluation 4, fails for nef and arr

  // Print only for filtered kernels.
  // std::cout << "- EK: " << boost::typeindex::type_id<typename Kernel::Exact_kernel>() << std::endl;
  // std::cout << "- IK: " << boost::typeindex::type_id<typename Kernel::Approximate_kernel>() << std::endl;
  // std::cout << std::endl;

  print_parameters<Kernel>(num_iters, verbose);
  auto bench_type = BENCH_TYPE::ALL;
  if (btype == "nef") bench_type = BENCH_TYPE::NEF;
  else if (btype == "pmp") bench_type = BENCH_TYPE::PMP;
  else if (btype == "arr") bench_type = BENCH_TYPE::ARR;

  // Bench.
  if (bench_type == BENCH_TYPE::ALL || bench_type == BENCH_TYPE::NEF) {
    run_all_nef_benches<Kernel>(num_iters, verbose);
  }
  if (bench_type == BENCH_TYPE::ALL || bench_type == BENCH_TYPE::PMP) {
    run_all_pmp_benches<Kernel>(num_iters, verbose);
  }
  if (bench_type == BENCH_TYPE::ALL || bench_type == BENCH_TYPE::ARR) {
    run_all_arr_benches<Kernel>(num_iters, verbose);
  }

  return EXIT_SUCCESS;
}
