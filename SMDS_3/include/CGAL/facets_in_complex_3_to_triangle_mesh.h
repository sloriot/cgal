// Copyright (c) 2009-2017 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Maxime Gimeno,
//                 Mael Rouxel-Labbé

#ifndef CGAL_FACETS_IN_COMPLEX_3_TO_TRIANGLE_MESH_H
#define CGAL_FACETS_IN_COMPLEX_3_TO_TRIANGLE_MESH_H

#include <CGAL/license/SMDS_3.h>

#include <CGAL/array.h>
#include <CGAL/boost/graph/Euler_operations.h>
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Named_function_parameters.h>
#include <CGAL/boost/graph/named_params_helper.h>
#include <CGAL/Time_stamper.h>
#include <CGAL/property_map.h>
#include <CGAL/Container_helper.h>

#include <boost/unordered_map.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>

namespace CGAL {

namespace SMDS_3 {

namespace internal {

template<class C3T3, class PointContainer, class FaceContainer, class PatchIndexContainer>
void facets_in_complex_3_to_triangle_soup(const C3T3& c3t3,
                                          const typename C3T3::Subdomain_index sd_index,
                                          PointContainer& points,
                                          FaceContainer& faces,
                                          PatchIndexContainer& patches,
                                          const bool normals_point_outside_of_the_subdomain = true,
                                          const bool export_all_facets = false)
{
  typedef typename PointContainer::value_type                            Point_3;
  typedef typename FaceContainer::value_type                             Face;

  typedef typename C3T3::Triangulation                                   Tr;
  typedef typename C3T3::Surface_patch_index                             Surface_patch_index;

  typedef typename Tr::Vertex_handle                                     Vertex_handle;
  typedef typename Tr::Cell_handle                                       Cell_handle;
  typedef typename Tr::Weighted_point                                    Weighted_point;
  typedef typename Tr::Facet                                             Facet;

  typedef CGAL::Hash_handles_with_or_without_timestamps                  Hash_fct;
  typedef boost::unordered_map<Vertex_handle, std::size_t, Hash_fct>     VHmap;

  typedef typename C3T3::size_type                                       size_type;

  size_type nf = c3t3.number_of_facets_in_complex();
  faces.reserve(faces.size() + nf);
  patches.reserve(faces.size() + nf);
  points.reserve(points.size() + nf/2); // approximating Euler

  VHmap vh_to_ids;
  std::size_t inum = 0;

  for(Facet fit : c3t3.facets_in_complex())
  {
    Cell_handle c = fit.first;
    int s = fit.second;
    const Surface_patch_index spi = c->surface_patch_index(s);
    Face f;
    CGAL::internal::resize(f, 3);

    typename C3T3::Subdomain_index cell_sdi = c3t3.subdomain_index(c);
    typename C3T3::Subdomain_index opp_sdi = c3t3.subdomain_index(c->neighbor(s));

    if(!export_all_facets && cell_sdi != sd_index && opp_sdi != sd_index)
      continue;

    for(std::size_t i=1; i<4; ++i)
    {
      typename VHmap::iterator map_entry;
      bool is_new;
      Vertex_handle v = c->vertex((s+i)&3);
      CGAL_assertion(v != Vertex_handle() && !c3t3.triangulation().is_infinite(v));

      boost::tie(map_entry, is_new) = vh_to_ids.insert(std::make_pair(v, inum));
      if(is_new)
      {
        const Weighted_point& p = c3t3.triangulation().point(c, (s+i)&3);
        const Point_3 bp = Point_3(CGAL::to_double(p.x()),
                                   CGAL::to_double(p.y()),
                                   CGAL::to_double(p.z()));
        points.push_back(bp);
        ++inum;
      }

      f[i-1] = map_entry->second;
    }

    if(export_all_facets)
    {
      if((cell_sdi > opp_sdi) == (s%2 == 1))
        std::swap(f[0], f[1]);
    }
    else
    {
      if(((cell_sdi == sd_index) == (s%2 == 1)) == normals_point_outside_of_the_subdomain)
        std::swap(f[0], f[1]);
    }

    faces.push_back(f);
    patches.push_back(spi);
  }
}

template<class C3T3, class PointContainer, class FaceContainer, class SurfacePatchContainer>
void facets_in_complex_3_to_triangle_soup(const C3T3& c3t3,
                                          PointContainer& points,
                                          FaceContainer& faces,
                                          SurfacePatchContainer& patches)
{
  typedef typename C3T3::Subdomain_index              Subdomain_index;
  Subdomain_index useless = Subdomain_index();
  facets_in_complex_3_to_triangle_soup(c3t3, useless, points, faces, patches,
                                       true/*point outward*/, true /*extract all facets*/);
}

template <typename Index2FaceMap, typename SurfacePatchRange>
void set_face_patches(const Index2FaceMap&,
                      const SurfacePatchRange&,
                      const internal_np::Param_not_found&)
{
  return;
}

template <typename Index2FaceMap,
          typename SurfacePatchRange,
          typename FacePatchMap>
void set_face_patches(const Index2FaceMap& i2f,
                      const SurfacePatchRange& patches,
                      const FacePatchMap& fpmap)
{
  for (auto index_and_face : i2f)
  {
    put(fpmap, index_and_face.second, patches[index_and_face.first]);
  }
}

} // end namespace internal

} // end namespace SMDS_3

  /**
   * @ingroup PkgSMDS3Functions
   *
   * @brief builds a `TriangleMesh` from the surface facets, with a consistent orientation at the interface of two subdomains.
   *
   * This function exports the surface as a `TriangleMesh` and appends it to `tm`, using `orient_polygon_soup()`.
   *
   * @tparam C3T3 a model of `MeshComplexWithFeatures_3InTriangulation_3`.
   * @tparam TriangleMesh a model of `MutableFaceGraph` with an internal point property map. The point type must be compatible with the one used in `C3T3`.
   * @tparam NamedParameters a sequence of \ref bgl_namedparameters "Named Parameters"
   *
   * @param c3t3 an instance of `C3T3`
   * @param tm an instance of `TriangleMesh`
   * @param np an optional sequence of \ref bgl_namedparameters "Named Parameters" among the ones listed below
   *
   * \cgalNamedParamsBegin
   *   \cgalParamNBegin{face_patch_map}
  *     \cgalParamDescription{a property map with the patch id's associated to the faces of `faces(tm)`}
  *     \cgalParamType{a class model of `ReadWritePropertyMap` with `boost::graph_traits<TriangleMesh>::%face_descriptor`
  *                    as key type and the desired property, model of `CopyConstructible` and `LessThanComparable` as value type.}
  *     \cgalParamDefault{If not provided, faces patch ids are ignored.}
  *     \cgalParamExtra{The map is updated during the remeshing process while new faces are created.}
  *   \cgalParamNEnd
  * \cgalNamedParamsEnd
  */
  template<class C3T3, class TriangleMesh, typename NamedParameters>
  void facets_in_complex_3_to_triangle_mesh(const C3T3& c3t3,
    TriangleMesh& tm,
    const NamedParameters& np)
  {
    namespace PMP = CGAL::Polygon_mesh_processing;

    typedef typename boost::property_map<TriangleMesh, boost::vertex_point_t>::type  VertexPointMap;
    typedef typename boost::property_traits<VertexPointMap>::value_type              Point_3;
    typedef typename boost::graph_traits<TriangleMesh>::face_descriptor              face_descriptor;
    typedef typename C3T3::Surface_patch_index                                       Surface_patch_index;

    typedef std::array<std::size_t, 3>                                       Face;

    std::vector<Face> faces;
    std::vector<Surface_patch_index> patches;
    std::vector<Point_3> points;

    SMDS_3::internal::facets_in_complex_3_to_triangle_soup(c3t3, points, faces, patches);

    if (!PMP::is_polygon_soup_a_polygon_mesh(faces))
      PMP::orient_polygon_soup(points, faces);
    CGAL_postcondition(PMP::is_polygon_soup_a_polygon_mesh(faces));

    boost::unordered_map<std::size_t, face_descriptor> i2f;
    PMP::polygon_soup_to_polygon_mesh(points, faces, tm,
      CGAL::parameters::polygon_to_face_output_iterator(std::inserter(i2f, i2f.end())));

    using parameters::choose_parameter;
    using parameters::get_parameter;

    SMDS_3::internal::set_face_patches(i2f,
                                      patches,
                                      get_parameter(np, internal_np::face_patch));
  }

  template<class C3T3, class TriangleMesh>
  void facets_in_complex_3_to_triangle_mesh(const C3T3& c3t3, TriangleMesh& tm)
  {
    facets_in_complex_3_to_triangle_mesh(c3t3, tm, parameters::all_default());
  }

} // namespace CGAL

#endif // CGAL_FACETS_IN_COMPLEX_3_TO_TRIANGLE_MESH_H