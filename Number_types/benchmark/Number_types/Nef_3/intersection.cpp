// STL.
#include <fstream>
#include <iostream>
#include <vector>

// Kernels.
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

// CGAL.
#include <CGAL/Surface_mesh.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>
#include <CGAL/OFF_to_nef_3.h>
#include <CGAL/Real_timer.h>

// Boost.
#include <boost/type_index.hpp>

using Timer = CGAL::Real_timer;
using EPECK = CGAL::Exact_predicates_exact_constructions_kernel;
using EPICK = CGAL::Exact_predicates_inexact_constructions_kernel;

void print_options() {

  std::cout << "* Number Type Options:" << std::endl;
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

template<typename Kernel>
double run_bench(
  const std::string filename1, const std::string filename2,
  const std::size_t num_iters, const bool verbose) {

  using Point_3        = typename Kernel::Point_3;
  using Polygon_mesh   = CGAL::Surface_mesh<Point_3>;
  using Nef_polyhedron = CGAL::Nef_polyhedron_3<Kernel>;

  if (verbose) {
    std::cout << "* testing kernel: " << boost::typeindex::type_id<Kernel>() << std::endl;
    std::cout << "- file #1: " << filename1 << std::endl;
    std::cout << "- file #2: " << filename2 << std::endl;
    std::cout << "- num iters: " << num_iters << std::endl;
  }

  Polygon_mesh A, B;
  std::ifstream inA("data/" + filename1);
  std::ifstream inB("data/" + filename2);

  inA >> A;
  assert(A.number_of_faces() > 0);
  assert(A.number_of_vertices() > 0);
  if (A.number_of_faces() == 0) return 0.0;

  inB >> B;
  assert(B.number_of_faces() > 0);
  assert(B.number_of_vertices() > 0);
  if (B.number_of_faces() == 0) return 0.0;

  Timer timer;
  double avg_time = 0.0;
  for (std::size_t k = 0; k < num_iters; ++k) {
    timer.start();
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
void run_all_benches(const std::size_t num_iters, const bool verbose) {

  print_options();
  std::vector<double> times;

  times.push_back(run_bench<Kernel>("sphere.off", "shifted-spheregrid.off", num_iters, verbose));
  times.push_back(run_bench<Kernel>("spheregrid.off", "shifted-spheregrid.off", num_iters, verbose));
  times.push_back(run_bench<Kernel>("spheregrid.off", "sphere.off", num_iters, verbose));
  times.push_back(run_bench<Kernel>("rotated-shifted-spheregrid.off", "rotated-spheregrid.off", num_iters, verbose));

  if (!verbose) {
    std::cout << "| N | ";
    std::cout << "sphere <-> shifted-spheregrid | ";
    std::cout << "spheregrid <-> shifted-spheregrid | ";
    std::cout << "spheregrid <-> sphere | ";
    std::cout << "rotated-shifted-spheregrid <-> rotated-spheregrid | ";
    std::cout << std::endl;
    std::cout << "| -- | -- | -- | -- | -- |" << std::endl;
    std::cout << "| " << num_iters;
    for (std::size_t k = 0; k < times.size(); ++k) {
      std::cout << " | " << times[k];
    }
    std::cout << " |" << std::endl;
    std::cout << std::endl;
  }
}

int main() {

  std::cout.precision(4);
  std::cout.setf(std::ios::fixed, std::ios::floatfield);

  std::cout << std::endl;
  std::cout << " --- NEF_3 BENCH --- " << std::endl;
  std::cout << std::endl;

  // Parameters.
  const std::size_t num_iters = 1; // number of iterations to average the timing
  const bool verbose = false; // do we print extra info

  // Bench.
  run_all_benches<EPECK>(num_iters, verbose);
  return EXIT_SUCCESS;
}
