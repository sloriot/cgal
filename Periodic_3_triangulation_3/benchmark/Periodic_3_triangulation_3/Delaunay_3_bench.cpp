// Copyright (c) 1998  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later
// 
//
// Author(s)     : Manuel Caroli

#include "types.h"

#include<CGAL/Delaunay_triangulation_3.h>
typedef CGAL::Delaunay_triangulation_3<K> Triang;

int main(int argc, char* argv[]) {
  Triang T;
  return bench_triang<Triang>(argc,argv,T,false);
}
 
