#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>
#include <fstream>


typedef CGAL::Exact_predicates_exact_constructions_kernel kernal;
typedef CGAL::Nef_polyhedron_3<kernal> Nef_polyhedron_3;
typedef CGAL::Polyhedron_3<kernal> Polyhedron_3;



void test0(std::string cube1_name,
           std::size_t nb_input_vertices,
           std::size_t nb_output_vertices)
{
  Polyhedron_3 poly;
  std::fstream file;
  file.open(cube1_name.c_str());
  file >> poly;
  Nef_polyhedron_3 nef1(poly, false);
  assert( nef1.number_of_vertices()==nb_input_vertices );
  
  file.close();
  file.open("data/no_simpl/cube2.off");
  poly.clear();
  file >> poly;
  Nef_polyhedron_3 nef2(poly, false);
  assert( nef2.number_of_vertices()==9 );

  file.close();
  file.open("data/no_simpl/cube3.off");
  poly.clear();
  file >> poly;
  Nef_polyhedron_3 nef3(poly, false);
  assert( nef3.number_of_vertices()==9 );

  file.close();
  file.open("data/no_simpl/bbox.off");
  poly.clear();
  file >> poly;
  Nef_polyhedron_3 bbox_nef(poly, false);

  Nef_polyhedron_3 nef_union = nef1.join(nef2, false);
  nef_union=nef_union.join(nef3, false);
  nef_union=nef_union.intersection(bbox_nef, false);

  std::ofstream output("out0.nef3");
  output << nef_union;
  output.close();

  assert(nef_union.number_of_vertices() == nb_output_vertices);
}

void test1()
{
  Polyhedron_3 poly;
  std::fstream file("data/no_simpl/tet1.off");
  file>>poly;
  Nef_polyhedron_3 nef1(poly, true);

  assert( nef1.number_of_vertices()==4 );

  file.close();
  file.open("data/no_simpl/tet2.off");
  poly.clear();
  file >> poly;
  Nef_polyhedron_3 nef2(poly, true);
  assert( nef2.number_of_vertices()==4 );
  Nef_polyhedron_3 nef_union = nef1.join(nef2, false);

  std::ofstream output("data/no_simpl/out1.nef3");
  output << nef_union;
  output.close();
  
  assert(nef_union.number_of_vertices() == 11);
}

void test2()
{
  Polyhedron_3 poly;
  std::fstream file("data/no_simpl/cube_meshed_1.off");
  file>>poly;

  assert(poly.size_of_vertices() == 1538);

  Nef_polyhedron_3 nef1(poly, false);

  assert( nef1.number_of_vertices()==1538 );
  
  file.close();
  file.open("data/no_simpl/cube_meshed_2.off");
  poly.clear();
  file >> poly;
  Nef_polyhedron_3 nef2(poly, false);
  assert( nef2.number_of_vertices()==1538 );

  file.close();
  file.open("data/no_simpl/cube_meshed_3.off");
  poly.clear();
  file >> poly;
  Nef_polyhedron_3 nef3(poly, false);
  assert( nef3.number_of_vertices()==1538 );

  file.close();
  file.open("data/no_simpl/bbox.off");
  poly.clear();
  file >> poly;
  Nef_polyhedron_3 bbox_nef(poly, false);

  Nef_polyhedron_3 nef_union = nef1.join(nef2, false);
  nef_union=nef_union.join(nef3, false);
  nef_union=nef_union.intersection(bbox_nef, false);

  std::ofstream output("out2.nef3");
  output << nef_union;
  output.close();
  
  assert(nef_union.number_of_vertices() == 5532);
}


int main()
{
  // CGAL_NEF_SETDTHREAD(0);
  std::cout << "Running test0(data/no_simpl/cube1.off)\n";
  test0("data/no_simpl/cube1.off", 9,44);
  std::cout << "Running test0(data/no_simpl/cube1_bis.off)\n";
  test0("data/no_simpl/cube1_bis.off", 11, 46);
  std::cout << "Running test0(data/no_simpl/cube1_ter.off)\n";
  test0("data/no_simpl/cube1_ter.off", 12, 47);
  std::cout << "Running test0(data/no_simpl/cube1_quat.off)\n";
  test0("data/no_simpl/cube1_quat.off", 9,56);
  std::cout << "Running test0(data/no_simpl/cube1_bis_bak.off)\n";
  test0("data/no_simpl/cube1_bis_bak.off", 11, 45);
  std::cout << "Running test1()\n";
  test1();
  std::cout << "Running test2()\n";
  test2();
  return 0;  
}
