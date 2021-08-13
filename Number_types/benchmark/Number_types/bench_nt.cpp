// STL.
#include <array>
#include <fstream>
#include <iostream>
#include <vector>

// Boost.
#include <boost/type_index.hpp>

// Kernels.
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

// CGAL.
#include <CGAL/Surface_mesh.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Exact_rational.h>
#include <CGAL/Real_timer.h>

using Timer = CGAL::Real_timer;
using EPECK = CGAL::Exact_predicates_exact_constructions_kernel;
using EPICK = CGAL::Exact_predicates_inexact_constructions_kernel;

namespace PMP = CGAL::Polygon_mesh_processing;
namespace params = CGAL::Polygon_mesh_processing::parameters;

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
  std::cout << boost::typeindex::type_id<CGAL::Exact_rational>() << std::endl;
  std::cout << std::endl;
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

  return true;
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

    avg_time += timer.time();
    timer.stop();
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

    avg_time += timer.time();
    timer.stop();
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
void run_all_nef_benches(const std::size_t num_iters, const bool verbose) {

  std::vector<double> times;
  std::cout << "* benching NEF ..." << std::endl;

  times.push_back(run_nef_bench<Kernel>("sphere.off", "shifted-spheregrid.off", num_iters, verbose));
  times.push_back(run_nef_bench<Kernel>("spheregrid.off", "shifted-spheregrid.off", num_iters, verbose));
  times.push_back(run_nef_bench<Kernel>("spheregrid.off", "sphere.off", num_iters, verbose));
  times.push_back(run_nef_bench<Kernel>("rotated-shifted-spheregrid.off", "rotated-spheregrid.off", num_iters, verbose));

  if (!verbose) {
    std::cout << "{|class=\"wikitable\" style=\"text-align:center;margin-right:1em;\" " << std::endl;
    std::cout << "! N !! ";
    std::cout << "ET !! ";
    std::cout << "sphere -- shifted-spheregrid !! ";
    std::cout << "spheregrid -- shifted-spheregrid !! ";
    std::cout << "spheregrid -- sphere !! ";
    std::cout << "rotated-shifted-spheregrid -- rotated-spheregrid ";
    std::cout << std::endl;
    std::cout << "|-" << std::endl;
    std::cout << "| " << num_iters;
    std::cout << " || " << boost::typeindex::type_id<CGAL::Exact_rational>();
    for (std::size_t k = 0; k < times.size(); ++k) {
      std::cout << " || " << times[k];
    }
    std::cout << std::endl << "|}" << std::endl;
  }
}

template<typename Kernel>
void run_all_pmp_benches(const std::size_t num_iters, const bool verbose) {

  std::vector<double> times;
  std::cout << "* benching PMP ..." << std::endl;
  CGAL_assertion_msg(false, "WARNING: These benches are not representative!");

  times.push_back(run_pmp_bench<Kernel>("blobby.off", "eight.off", num_iters, verbose));
  times.push_back(run_pmp_bench<Kernel>("cheese.off", "cheese-rotated.off", num_iters, verbose));

  if (!verbose) {
    std::cout << "{|class=\"wikitable\" style=\"text-align:center;margin-right:1em;\" " << std::endl;
    std::cout << "! N !! ";
    std::cout << "ET !! ";
    std::cout << "blobby -- eight !! ";
    std::cout << "cheese -- cheese-rotated ";
    std::cout << std::endl;
    std::cout << "|-" << std::endl;
    std::cout << "| " << num_iters;
    std::cout << " || " << boost::typeindex::type_id<CGAL::Exact_rational>();
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

  // todo ...

  // if (!verbose) {
  //   std::cout << "{|class=\"wikitable\" style=\"text-align:center;margin-right:1em;\" " << std::endl;
  //   std::cout << "! N !! ";
  //   std::cout << "ET !! ";
  //   std::cout << "sphere -- shifted-spheregrid !! ";
  //   std::cout << "spheregrid -- shifted-spheregrid !! ";
  //   std::cout << "spheregrid -- sphere !! ";
  //   std::cout << "rotated-shifted-spheregrid -- rotated-spheregrid ";
  //   std::cout << std::endl;
  //   std::cout << "|-" << std::endl;
  //   std::cout << "| " << num_iters;
  //   std::cout << " || " << boost::typeindex::type_id<CGAL::Exact_rational>();
  //   for (std::size_t k = 0; k < times.size(); ++k) {
  //     std::cout << " || " << times[k];
  //   }
  //   std::cout << std::endl << "|}" << std::endl;
  // }
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
  using Kernel = EPECK; // chosen kernel
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
