// Copyright (c) 1997-2010  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: LGPL-3.0-or-later
// 
//
// Author(s)     : Mariette Yvinec

#ifndef CGAL_PROJECTION_TRAITS_XY_3_H
#define CGAL_PROJECTION_TRAITS_XY_3_H

#include <CGAL/internal/Projection_traits_3.h>

namespace CGAL { 

template < class R >
class Projection_traits_xy_3
  : public internal::Projection_traits_3<R,2>
{};
  
} //namespace CGAL 

#endif // CGAL_PROJECTION_TRAITS_XY_3_H
