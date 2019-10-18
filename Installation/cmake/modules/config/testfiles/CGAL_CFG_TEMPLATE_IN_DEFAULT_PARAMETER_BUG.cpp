// Copyright (c) 2011  INRIA Saclay (France).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Author(s)     : Marc Glisse

//| If a compiler wants us to remove the template keyword in default
//| parameters, CGAL_CFG_TEMPLATE_IN_DEFAULT_PARAMETER_BUG is set. 
//| Bug found in sunCC 5.11.

template < typename A, typename B, typename C =
  typename A :: template T < B > :: type >
//                here
struct S {};

int main(){}
