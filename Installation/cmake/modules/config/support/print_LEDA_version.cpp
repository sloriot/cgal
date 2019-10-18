// Copyright (c) 2005  
// Utrecht University (The Netherlands),
// ETH Zurich (Switzerland),
// INRIA Sophia-Antipolis (France),
// Max-Planck-Institute Saarbruecken (Germany),
// and Tel-Aviv University (Israel).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: LGPL-3.0-or-later


#include <iostream>
#include <LEDA/system/basic.h>


int main()
{
  std::cout << "version=" << (  __LEDA__  / 100 ) << "." << ( __LEDA__ % 100 ) << std::endl;
  return 0;
}
