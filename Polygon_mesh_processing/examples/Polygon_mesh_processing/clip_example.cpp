#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_mesh_processing/clip.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/IO/polygon_mesh_io.h>
#include <CGAL/boost/graph/generators.h>
#include <CGAL/Timer.h>
#include <boost/property_map/property_map.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

namespace PMP = CGAL::Polygon_mesh_processing;
namespace params = CGAL::parameters;

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Surface_mesh<K::Point_3> Surface_mesh;
typedef K::Point_3 Point_3;

int main(int argc, char* argv[])
{

    const std::string filename1 = (argc > 1) ? argv[1] : CGAL::data_file_path("meshes/blobby.off");
    const std::string filename2 = (argc > 2) ? argv[2] : CGAL::data_file_path("meshes/eight.off");
    Surface_mesh tet, tri;

    if (!PMP::IO::read_polygon_mesh(filename1, tet) || !PMP::IO::read_polygon_mesh(filename2, tri))
    {
        std::cerr << "Invalid input." << std::endl;
        return 1;
    }

  Surface_mesh tet_copy = tet;

  {
    CGAL::Timer t;
    t.start();
    PMP::clip(tet, tri);
    std::cout << "New: " << t.time() << " sec." << std::endl;
    std::ofstream out("new_out.off");
    out.precision(17);
    out << tet << std::endl;
  }


  {
    CGAL::Timer t;
    t.start();
    K::Plane_3 p(-0.990461, -0.0105343, 0.137388,169.738);
    PMP::clip(tet_copy, p);
    std::cout << "Old: " << t.time() << " sec." << std::endl;
    std::ofstream out("old_out.off");
    out.precision(17);
    out << tet_copy << std::endl;
  }


  return EXIT_SUCCESS;
}
