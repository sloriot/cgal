#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

#define CGAL_COREFINEMENT_DEBUG

#include <CGAL/Polygon_mesh_processing/intersection.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/boost/graph/Face_filtered_graph.h>
#include <fstream>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Surface_mesh<K::Point_3>             Mesh;

namespace PMP = CGAL::Polygon_mesh_processing;

int main(int argc, char* argv[])
{
  const char* filename = (argc > 1) ? argv[1] : "data/blobby.off";
  std::ifstream input(filename);

  Mesh mesh;
  if (!input || !(input >> mesh))
  {
    std::cerr << "Input mesh is not a valid off file." << std::endl;
    return 1;
  }
  input.close();

  std::ofstream output;
  //~ std::cout << "Test surface_self_intersection\n";
  //~ std::vector< std::vector<K::Point_3> >polylines;

  //~ PMP::experimental::surface_self_intersection(mesh, std::back_inserter(polylines));

  //~ //dump polylines
  //~ std::ofstream output("intersection_polylines.cgal");
  //~ BOOST_FOREACH(const std::vector<K::Point_3>& polyline, polylines)
  //~ {
    //~ output << polyline.size() << " ";
    //~ std::copy(polyline.begin(), polyline.end(),std::ostream_iterator<K::Point_3>(output," "));
    //~ output << "\n";
  //~ }
  //~ output.close();

  std::cout << "Number of vertices before autorefinement " << num_vertices(mesh) << "\n";
  
  Mesh::Property_map<Mesh::Edge_index,bool> ecm =
    mesh.add_property_map<Mesh::Edge_index, bool>("e:is_constrained").first;
  PMP::experimental::autorefine(mesh, PMP::parameters::edge_is_constrained_map(ecm));
  std::cout << "Number of vertices after autorefinement " << num_vertices(mesh) << "\n";

  output.open("mesh_autorefined.off");
  output << mesh;
  output.close();

  Mesh::Property_map<Mesh::Face_index, int> cc_ids =
    mesh.add_property_map<Mesh::Face_index, int>("f:cc_ids").first;
  int nb_cc = PMP::connected_components(mesh,cc_ids, PMP::parameters::edge_is_constrained_map(ecm));

  std::cout << "nb_cc = " << nb_cc << "\n";

  for (int i=0; i!= nb_cc; ++i)
  {
    typedef CGAL::Face_filtered_graph<Mesh> Filtered_graph;
    Filtered_graph filtered_sm(mesh, i, cc_ids);
      CGAL_assertion(filtered_sm.is_selection_valid());
    Mesh tmp;
    CGAL::copy_face_graph(filtered_sm, tmp);
    std::stringstream ss;
    ss << "debug/part-" << i << ".off";
    std::ofstream out(ss.str().c_str());
    out << tmp;
  }



  mesh.remove_property_map(ecm);
  mesh.remove_property_map(cc_ids);




  input.open(filename);
  mesh.clear();
  input >> mesh;
  std::cout << "Number of vertices before self-intersection removal " << num_vertices(mesh) << "\n";
  if (!PMP::experimental::autorefine_and_remove_self_intersections(mesh))
    std::cout << "WARNING: Cannot remove all self-intersections\n";
  std::cout << "Number of vertices after self-intersection removal " << num_vertices(mesh) << "\n";

  output.open("mesh_fixed.off");
  output << std::setprecision(17) << mesh;
  output.close();

  return 0;
}
