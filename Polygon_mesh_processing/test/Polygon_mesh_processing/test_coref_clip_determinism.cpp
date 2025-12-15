#define CGAL_FORCE_COREFINEMENT_DETERMINISM

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/IO/polygon_mesh_io.h>

#include <iostream>
#include <string>

typedef CGAL::Exact_predicates_inexact_constructions_kernel   K;
typedef CGAL::Surface_mesh<K::Point_3>                        Mesh;

namespace PMP = CGAL::Polygon_mesh_processing;

int main(int argc, char* argv[])
{
  const std::string filename1 = (argc > 1) ? argv[1] : CGAL::data_file_path("meshes/blobby.off");
  const std::string filename2 = (argc > 2) ? argv[2] : CGAL::data_file_path("meshes/eight.off");

  Mesh mesh1_ref, mesh2_ref;
  if(!PMP::IO::read_polygon_mesh(filename1, mesh1_ref) || !PMP::IO::read_polygon_mesh(filename2, mesh2_ref))
  {
    std::cerr << "Invalid input." << std::endl;
    return 1;
  }

  std::vector<Mesh> outputs(10);

  for (int i=0; i<10; ++i)
  {
    Mesh mesh1=mesh1_ref, mesh2=mesh2_ref;

    Mesh& out = outputs[i];
    bool valid_union = PMP::corefine_and_compute_union(mesh1,mesh2, out);

    assert(valid_union);
  }

  for (int i=0; i<9; ++i)
  {
    Mesh& mesh1=outputs[i];
    Mesh& mesh2=outputs[i+1];

    assert(vertices(mesh1).size()==vertices(mesh2).size());
    auto itv1=vertices(mesh1).begin(), itv2=vertices(mesh2).begin();
    for (; itv1!=vertices(mesh1).end(); ++itv1, ++itv2)
    {
      if( mesh1.point(*itv1)!=mesh2.point(*itv2) )
      {
        std::ofstream("m1.off") << std::setprecision(17) << mesh1;
        std::ofstream("m2.off") << std::setprecision(17) << mesh2;
        std::cout << *itv1 << " vs " << *itv2 << "\n";
        std::cout << mesh1.point(*itv1) << " vs " << mesh2.point(*itv2) << "\n";
      }
      assert( mesh1.point(*itv1)==mesh2.point(*itv2) );
    }

    auto itf1=faces(mesh1).begin(), itf2=faces(mesh2).begin();
    for (; itf1!=faces(mesh1).end(); ++itf1, ++itf2)
    {
      auto h1=halfedge(*itf1, mesh1), h2=halfedge(*itf2, mesh2);
      for (int k=0;k<3; ++k)
      {
        assert(target(h1, mesh1).idx() == target(h2, mesh2).idx());
        h1=next(h1,mesh1);
        h2=next(h2,mesh2);
      }
    }
  }


  std::cout << "Union could not be computed\n";

  return 0;
}
