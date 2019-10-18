// Copyright (c) 2005-2006  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Author(s) : Monique Teillaud <Monique.Teillaud@sophia.inria.fr>
//             Sylvain Pion
//             Pedro Machado

#ifndef CGAL_ALGEBRAIC_KERNEL_COMPARISON_ROOT_FOR_SPHERES_2_3_H
#define CGAL_ALGEBRAIC_KERNEL_COMPARISON_ROOT_FOR_SPHERES_2_3_H

#include <CGAL/license/Circular_kernel_3.h>



namespace CGAL {
  namespace AlgebraicSphereFunctors{
  
    template <typename RT>
      Comparison_result 
      compare_x(const CGAL::Root_for_spheres_2_3<RT>& r1, const CGAL::Root_for_spheres_2_3<RT>& r2){
      return compare(r1.x(), r2.x());
    }
  
    template <typename RT>
      Comparison_result 
      compare_y(const CGAL::Root_for_spheres_2_3<RT>& r1, const CGAL::Root_for_spheres_2_3<RT>& r2){
      return compare(r1.y(), r2.y());
    }

    template <typename RT>
      Comparison_result 
      compare_z(const CGAL::Root_for_spheres_2_3<RT>& r1, const CGAL::Root_for_spheres_2_3<RT>& r2){
      return compare(r1.z(), r2.z());
    }
  
    template <typename RT>
      Comparison_result
      compare_xy(const CGAL::Root_for_spheres_2_3<RT>& r1, const CGAL::Root_for_spheres_2_3<RT>& r2){
      Comparison_result compx = compare_x(r1, r2);
      if(compx != 0)
	return compx;
      return compare_y(r1, r2);
    }

     template <typename RT>
      Comparison_result
      compare_xyz(const CGAL::Root_for_spheres_2_3<RT>& r1, const CGAL::Root_for_spheres_2_3<RT>& r2){
      Comparison_result compxy = compare_xy(r1, r2);
      if(compxy != 0)
	return compxy;
      return compare_z(r1, r2);
    }


  } // namespace AlgebraicSphereFunctors
} // namespace CGAL

#endif // CGAL_ALGEBRAIC_KERNEL_COMPARISON_ROOT_FOR_SPHERES_2_3_H
