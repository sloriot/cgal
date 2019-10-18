// Copyright (c) 2018 GeometryFactory (France).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Author: Mael Rouxel-Labbé

// Some tests are explicitely used to check the sanity of deprecated code and should not
// give warnings/errors on plateforms that defined CGAL_NO_DEPRECATED_CODE CGAL-wide
// (or did not disable deprecation warnings).

#if !defined(CGAL_NO_DEPRECATION_WARNINGS)
  #define CGAL_NO_DEPRECATION_WARNINGS
#endif

#if defined(CGAL_NO_DEPRECATED_CODE)
  #undef CGAL_NO_DEPRECATED_CODE
#endif
