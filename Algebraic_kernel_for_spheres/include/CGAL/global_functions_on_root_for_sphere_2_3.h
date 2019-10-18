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
//             Julien Hazebrouck
//             Damien Leroy

#ifndef CGAL_ALGEBRAIC_KERNEL_GLOBAL_FUNCTIONS_ON_ROOT_FOR_SPHERE_2_3_H
#define CGAL_ALGEBRAIC_KERNEL_GLOBAL_FUNCTIONS_ON_ROOT_FOR_SPHERE_2_3_H

#include <CGAL/license/Circular_kernel_3.h>


#include <CGAL/enum.h>

namespace CGAL {

template < class AK >
inline 
Comparison_result 
compare_x(const typename AK::Root_for_spheres_2_3& r1,
	   const typename AK::Root_for_spheres_2_3& r2)
{ return AK().compare_x_object()(r1, r2); }

template < class AK >
inline 
Comparison_result 
compare_y(const typename AK::Root_for_spheres_2_3& r1,
	   const typename AK::Root_for_spheres_2_3& r2)
{ return AK().compare_y_object()(r1, r2); }

template < class AK >
inline 
Comparison_result 
compare_z(const typename AK::Root_for_spheres_2_3& r1,
	     const typename AK::Root_for_spheres_2_3& r2)
{ return AK().compare_z_object()(r1, r2); }

template < class AK >
inline 
Comparison_result 
compare_xy(const typename AK::Root_for_spheres_2_3& r1,
	     const typename AK::Root_for_spheres_2_3& r2)
{ return AK().compare_xy_object()(r1, r2); }

template < class AK >
inline 
Comparison_result 
compare_xyz(const typename AK::Root_for_spheres_2_3& r1,
	     const typename AK::Root_for_spheres_2_3& r2)
{ return AK().compare_xyz_object()(r1, r2); }

} //namespace CGAL

#endif //CGAL_ALGEBRAIC_KERNEL_GLOBAL_FUNCTIONS_ON_ROOT_FOR_SPHERE_2_3_H
