// #define CGAL_DISABLE_GMP 1

// STL.
#include <fstream>
#include <iostream>
#include <vector>

// Kernels.
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>

// CGAL.
#include <CGAL/Surface_mesh.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>
#include <CGAL/OFF_to_nef_3.h>
#include <CGAL/Real_timer.h>

using Kernel         = CGAL::Exact_predicates_exact_constructions_kernel;
using Polygon_mesh   = CGAL::Surface_mesh<typename Kernel::Point_3>;
using Nef_polyhedron = CGAL::Nef_polyhedron_3<Kernel>;
using Timer          = CGAL::Real_timer;

#define N 10
int main() {

  double total_time = 0.0;
  Polygon_mesh A, B;

  {
    std::ifstream in("data/rotated-shifted-spheregrid.off");
    in >> A;
  }

  {
    std::ifstream in("data/rotated-spheregrid.off");
    in >> B;
  }

  Timer timer;
  for (int i = 0; i < N; ++i) {
    timer.start();

    {
      Nef_polyhedron nefA(A), nefB(B);
      Nef_polyhedron nefC = nefA.intersection(nefB);
      if (i == 0) {
        std::cout << nefC.number_of_vertices() << "    ";
      }
    }

    total_time += timer.time();
    timer.stop();
    std::cout << timer.time() <<"s, ";
    std::flush(std::cout);
    timer.reset();
  }

  std::cout << "mean time on " << N << " runs: " <<
    total_time / static_cast<double>(N) << "sec" << std::endl;
  return EXIT_SUCCESS;
}
