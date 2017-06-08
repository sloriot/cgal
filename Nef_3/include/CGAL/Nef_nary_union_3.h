// Copyright (c) 1997-2007  Max-Planck-Institute Saarbruecken (Germany).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$ 
// 
//
// Author(s)     :     Peter Hachenberger  <hachenberger@mpi-sb.mpg.de>

#ifndef CGAL_NEF_NARY_UNION_3_H
#define CGAL_NEF_NARY_UNION_3_H

#include <CGAL/license/Nef_3.h>


#include <list>

namespace CGAL {

template<class Polyhedron>
class Nef_nary_union_3 {

  int inserted;
  std::list<Polyhedron> queue;
  typedef typename std::list<Polyhedron>::iterator pit;
  Polyhedron empty;
  bool simplify;

 public:
  Nef_nary_union_3(bool s=true) : inserted(0), simplify(s) {}
  
  void unite() {
    pit i1(queue.begin()), i2(i1);
    ++i2;

    Polyhedron tmp(*i1);
    tmp = tmp.join(*i2, simplify);

    queue.pop_front();
    queue.pop_front();
    queue.push_front(tmp);
  }

  void add_polyhedron(const Polyhedron& P) {
    queue.push_front(P);
    ++inserted;
    for(int i=2;(inserted%i) == 0; i*=2) {
      unite();
    }
  }

  Polyhedron get_union() {

    while(queue.size() > 1)
      unite();
    inserted = 0;
    return queue.front();
  }
};

} //namespace CGAL
#endif // CGAL_NEF_NARY_UNION_H
