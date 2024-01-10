
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Projection_traits_3.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Timer.h>
#include <fstream>
#include <iostream>
#include <string>

typedef CGAL::Exact_predicates_exact_constructions_kernel EK;
typedef EK::FT FT;
typedef EK::Point_3 Point_3;
typedef EK::Vector_3 Vector_3;
typedef CGAL::Projection_traits_3<EK> P_traits;
typedef CGAL::No_constraint_intersection_tag Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<P_traits ,CGAL::Default, Itag> CDT_2;
typedef CGAL::Timer Timer;

int main()
{
  std::vector<Point_3> points;
  std::vector<std::pair<std::size_t,size_t> > segments;

  std::ifstream pin("points_3.txt");
  std::string type;
  Point_3 p;

  while(pin){
    pin >> type;
    if(type == "double") {
      pin >> p;
      points.push_back(p);
    }else{
      FT x, y, z;
      pin >> x >> y >> z;
      points.push_back(Point_3(x,y,z));
    }
  }
  pin.close();

  std::ifstream sin("segments_3.txt");
  while(sin){
    std::size_t i, j;
    sin >> i >> j;
    segments.push_back(std::make_pair(i,j));
  }

  std::size_t size = points.size();

  assert(! collinear(points[size-3],
                            points[size-2],
                     points[size-1]));

  Vector_3 n = CGAL::normal(points[size-3],
                            points[size-2],
                            points[size-1]);
  P_traits ptraits(n);
  CDT_2 cdt(ptraits);
  cdt.insert_constraints(points.begin(), points.end(), segments.begin(), segments.end());
  return 0;
}
