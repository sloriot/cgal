// Copyright (c) 2018  GeometryFactory Sarl (France).
// All rights reserved.
//
// $URL$
// $Id$
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Author(s)     : Maxime Gimeno
#if BOOST_VERSION >= 105600 && (! defined(BOOST_GCC) || BOOST_GCC >= 40500)

#ifndef CGAL_IO_TRAITS_LINESTRING_H
#define CGAL_IO_TRAITS_LINESTRING_H
#include <CGAL/internal/Geometry_container.h>
#include <boost/geometry/io/wkt/write.hpp>
#include <boost/geometry/io/wkt/read.hpp>



namespace boost{
namespace geometry{
namespace traits{
template< typename R> struct tag<CGAL::internal::Geometry_container<R, linestring_tag> >
{ typedef linestring_tag type; };

}}} //end namespaces

#endif // TRAITS_LINESTRING_H
#endif
