// Copyright (c) 2006-2009 Max-Planck-Institute Saarbruecken (Germany).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: LGPL-3.0-or-later
// 
//
// Author(s)     : Michael Kerber    <mkerber@mpi-inf.mpg.de>
//
// ============================================================================

#ifndef CGAL_ALGEBRAIC_KERNEL_D_2_H
#define CGAL_ALGEBRAIC_KERNEL_D_2_H

#include <CGAL/disable_warnings.h>

#include <CGAL/basic.h>

#include <CGAL/Algebraic_kernel_d_1.h>
#include <CGAL/Algebraic_kernel_d/Algebraic_curve_kernel_2.h>

namespace CGAL {

template<typename Coefficient> class Algebraic_kernel_d_2
  : public CGAL::Algebraic_curve_kernel_2
               < CGAL::Algebraic_kernel_d_1 <Coefficient> >
{};
 
} //namespace CGAL

#include <CGAL/enable_warnings.h>

#endif // CGAL_ALGEBRAIC_KERNEL_D_1_H
