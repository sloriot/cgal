// Copyright (c) 1997  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Mariette Yvinec, Jean Daniel Boissonnat

#ifndef CGAL_CONSTRAINED_DELAUNAY_TRIANGULATION_2_H
#define CGAL_CONSTRAINED_DELAUNAY_TRIANGULATION_2_H

#include <CGAL/license/Triangulation_2.h>


#include <CGAL/assertions.h>
#include <CGAL/Constrained_triangulation_2.h>
#include <CGAL/Triangulation_2/insert_constraints.h>

#ifndef CGAL_TRIANGULATION_2_DONT_INSERT_RANGE_OF_POINTS_WITH_INFO
#include <CGAL/Spatial_sort_traits_adapter_2.h>
#include <CGAL/STL_Extension/internal/info_check.h>
#include <CGAL/type_traits/is_iterator.h>

#include <boost/container/flat_set.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/mpl/and.hpp>

namespace CGAL {

/// \ingroup PkgKvisibilityRefFunctions
/// place holder for a function in this package
template <class Segments>
void watch_tower_1_5(const Segments& segments);

} // end of CGAL namespace
