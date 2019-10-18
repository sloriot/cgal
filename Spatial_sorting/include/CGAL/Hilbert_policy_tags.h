// Copyright (c) 2011  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Author(s)     : Olivier Devillers

#ifndef CGAL_HILBERT_POLICY_H
#define CGAL_HILBERT_POLICY_H


namespace CGAL {

struct Middle {};
struct Median {};


// A policy to select the sorting strategy.

template < typename Tag >
struct Hilbert_policy {};

typedef Hilbert_policy<Middle>      Hilbert_sort_middle_policy;
typedef Hilbert_policy<Median>      Hilbert_sort_median_policy;

} // namespace CGAL

#endif // CGAL_HILBERT_POLICY_H
