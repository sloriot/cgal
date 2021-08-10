// STL.
#include <fstream>
#include <iostream>
#include <vector>

// CGAL.
#include <CGAL/Real_timer.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

// PMP.
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/IO/polygon_mesh_io.h>

using Kernel       = CGAL::Exact_predicates_inexact_constructions_kernel;
using Polygon_mesh = CGAL::Surface_mesh<typename Kernel::Point_3>;
using Timer        = CGAL::Real_timer;

namespace PMP = CGAL::Polygon_mesh_processing;
namespace params = CGAL::Polygon_mesh_processing::parameters;

#define N 10
int main(int argc, char* argv[]) {

  const char* filename1 = (argc > 1) ? argv[1] : "data/blobby.off";
  const char* filename2 = (argc > 2) ? argv[2] : "data/eight.off";

  Polygon_mesh mesh1, mesh2, mesh11, mesh22;
  if (
    !PMP::IO::read_polygon_mesh(filename1, mesh1) ||
    !PMP::IO::read_polygon_mesh(filename2, mesh2)) {

    std::cerr << "ERROR: Invalid input!" << std::endl;
    return EXIT_FAILURE;
  }

  Timer timer;
  double total_time = 0.0;
  for (int i = 0; i < N; ++i) {

    mesh11 = mesh1;
    mesh22 = mesh2;
    timer.start();
    {
      Polygon_mesh out_union, out_intersection;
      std::array<boost::optional<Polygon_mesh*>, 4> output;
      output[PMP::Corefinement::UNION] = &out_union;
      output[PMP::Corefinement::INTERSECTION] = &out_intersection;

      // for the example, we explicit the named parameters, this is identical to
      // PMP::corefine_and_compute_boolean_operations(mesh1, mesh2, output)
      // std::array<bool, 4> res =
      PMP::corefine_and_compute_boolean_operations(
        mesh11, mesh22,
        output,
        params::all_default(), // mesh1 named parameters
        params::all_default(), // mesh2 named parameters
        std::make_tuple(
          params::vertex_point_map(get(boost::vertex_point, out_union)), // named parameters for out_union
          params::vertex_point_map(get(boost::vertex_point, out_intersection)), // named parameters for out_intersection
          params::all_default(), // named parameters for mesh1-mesh2 not used
          params::all_default()) // named parameters for mesh2-mesh1 not used)
        );
    }
    timer.stop();
    std::cout << timer.time() << "s, ";
    std::flush(std::cout);
    total_time += timer.time();
    timer.reset();
  }

  std::cout << "mean time on " << N << " runs: " <<
    total_time / static_cast<double>(N) << "sec" << std::endl;
  return EXIT_SUCCESS;
}
