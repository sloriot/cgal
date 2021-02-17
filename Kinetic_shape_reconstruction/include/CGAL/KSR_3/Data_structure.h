// Copyright (c) 2019 GeometryFactory SARL (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0+
//
// Author(s)     : Simon Giraudot

#ifndef CGAL_KSR_3_DATA_STRUCTURE_H
#define CGAL_KSR_3_DATA_STRUCTURE_H

// #include <CGAL/license/Kinetic_shape_reconstruction.h>

// CGAL includes.
#include <CGAL/Delaunay_triangulation_2.h>

// Internal includes.
#include <CGAL/KSR/enum.h>
#include <CGAL/KSR/utils.h>
#include <CGAL/KSR/debug.h>

#include <CGAL/KSR_3/Support_plane.h>
#include <CGAL/KSR_3/Intersection_graph.h>

namespace CGAL {
namespace KSR_3 {

template<typename GeomTraits>
class Data_structure {

public:
  using Kernel = GeomTraits;

private:
  using FT          = typename Kernel::FT;
  using Point_2     = typename Kernel::Point_2;
  using Point_3     = typename Kernel::Point_3;
  using Segment_2   = typename Kernel::Segment_2;
  using Segment_3   = typename Kernel::Segment_3;
  using Vector_2    = typename Kernel::Vector_2;
  using Direction_2 = typename Kernel::Direction_2;
  using Triangle_2  = typename Kernel::Triangle_2;
  using Line_2      = typename Kernel::Line_2;
  using Plane_3     = typename Kernel::Plane_3;
  using Polygon_2   = CGAL::Polygon_2<Kernel>;

public:
  using Support_plane      = KSR_3::Support_plane<Kernel>;
  using Intersection_graph = KSR_3::Intersection_graph<Kernel>;

  using Mesh           = typename Support_plane::Mesh;
  using Vertex_index   = typename Mesh::Vertex_index;
  using Face_index     = typename Mesh::Face_index;
  using Edge_index     = typename Mesh::Edge_index;
  using Halfedge_index = typename Mesh::Halfedge_index;

  using PVertex = std::pair<std::size_t, Vertex_index>;
  using PFace   = std::pair<std::size_t, Face_index>;
  using PEdge   = std::pair<std::size_t, Edge_index>;

  template<typename PSimplex>
  struct Make_PSimplex {
    using argument_type = typename PSimplex::second_type;
    using result_type   = PSimplex;

    const std::size_t support_plane_idx;
    Make_PSimplex(const std::size_t sp_idx) :
    support_plane_idx(sp_idx)
    { }

    const result_type operator()(const argument_type& arg) const {
      return result_type(support_plane_idx, arg);
    }
  };

  using PVertex_iterator =
    boost::transform_iterator<Make_PSimplex<PVertex>, typename Mesh::Vertex_range::iterator>;
  using PVertices = CGAL::Iterator_range<PVertex_iterator>;

  using PFace_iterator =
    boost::transform_iterator<Make_PSimplex<PFace>, typename Mesh::Face_range::iterator>;
  using PFaces = CGAL::Iterator_range<PFace_iterator>;

  using PEdge_iterator =
    boost::transform_iterator<Make_PSimplex<PEdge>, typename Mesh::Edge_range::iterator>;
  using PEdges = CGAL::Iterator_range<PEdge_iterator>;

  struct Halfedge_to_pvertex {
    using argument_type = Halfedge_index;
    using result_type   = PVertex;

    const std::size_t support_plane_idx;
    const Mesh& mesh;

    Halfedge_to_pvertex(const std::size_t sp_idx, const Mesh& m) :
    support_plane_idx(sp_idx),
    mesh(m)
    { }

    const result_type operator()(const argument_type& arg) const {
      return result_type(support_plane_idx, mesh.target(arg));
    }
  };

  using PVertex_of_pface_iterator =
    boost::transform_iterator<Halfedge_to_pvertex, CGAL::Halfedge_around_face_iterator<Mesh> >;
  using PVertices_of_pface = CGAL::Iterator_range<PVertex_of_pface_iterator>;

  struct Halfedge_to_pedge {
    using argument_type = Halfedge_index;
    using result_type = PEdge;

    const std::size_t support_plane_idx;
    const Mesh& mesh;

    Halfedge_to_pedge(const std::size_t sp_idx, const Mesh& m) :
    support_plane_idx(sp_idx),
    mesh(m)
    { }

    const result_type operator()(const argument_type& arg) const {
      return result_type(support_plane_idx, mesh.edge(arg));
    }
  };

  struct Halfedge_to_pface {
    using argument_type = Halfedge_index;
    using result_type = PFace;

    const std::size_t support_plane_idx;
    const Mesh& mesh;

    Halfedge_to_pface(const std::size_t sp_idx, const Mesh& m) :
    support_plane_idx(sp_idx),
    mesh(m)
    { }

    const result_type operator()(const argument_type& arg) const {
      return result_type(support_plane_idx, mesh.face(arg));
    }
  };

  using PEdge_around_pvertex_iterator =
    boost::transform_iterator<Halfedge_to_pedge, CGAL::Halfedge_around_target_iterator<Mesh> >;
  using PEdges_around_pvertex = CGAL::Iterator_range<PEdge_around_pvertex_iterator>;

  using PEdge_of_pface_iterator =
    boost::transform_iterator<Halfedge_to_pedge, CGAL::Halfedge_around_face_iterator<Mesh> >;
  using PEdges_of_pface = CGAL::Iterator_range<PEdge_of_pface_iterator>;

  using PFace_around_pvertex_iterator =
    boost::transform_iterator<Halfedge_to_pface, CGAL::Halfedge_around_target_iterator<Mesh> >;
  using PFaces_around_pvertex = CGAL::Iterator_range<PFace_around_pvertex_iterator>;

  using IVertex = typename Intersection_graph::Vertex_descriptor;
  using IEdge   = typename Intersection_graph::Edge_descriptor;

  using Visibility_label = KSR::Visibility_label;

  struct Volume_cell {
    std::vector<PFace> pfaces;
    std::vector<int> neighbors;
    std::set<PVertex> pvertices;
    std::size_t index = std::size_t(-1);
    Point_3 centroid;

    Visibility_label visibility = Visibility_label::INSIDE;
    FT inside  = FT(1);
    FT outside = FT(0);
    FT weight  = FT(0);

    void add_pface(const PFace& pface, const int neighbor) {
      pfaces.push_back(pface);
      neighbors.push_back(neighbor);
    }
    void set_index(const std::size_t idx) {
      index = idx;
    }
    void set_centroid(const Point_3& point) {
      centroid = point;
    }
  };

  struct Reconstructed_model {
    std::vector<PFace> pfaces;
    void clear() {
      pfaces.clear();
    }
  };

private:
  std::map< std::pair<std::size_t, IEdge>, Point_2>  m_points;
  std::map< std::pair<std::size_t, IEdge>, Vector_2> m_directions;
  std::vector<Support_plane> m_support_planes;
  Intersection_graph m_intersection_graph;

  using Limit_line = std::vector< std::pair< std::pair<std::size_t, std::size_t>, bool> >;
  std::vector<Limit_line> m_limit_lines;

  FT m_previous_time;
  FT m_current_time;
  bool m_verbose;

  std::vector<Volume_cell> m_volumes;
  std::map<int, std::size_t> m_volume_level_map;
  std::map<PFace, std::pair<int, int> > m_map_volumes;
  std::map<std::size_t, std::size_t> m_input_polygon_map;
  Reconstructed_model m_reconstructed_model;

public:
  Data_structure(const bool verbose) :
  m_previous_time(FT(0)),
  m_current_time(FT(0)),
  m_verbose(verbose)
  { }

  void clear() {
    m_points.clear();
    m_directions.clear();
    m_support_planes.clear();
    m_intersection_graph.clear();

    m_previous_time = FT(0);
    m_current_time  = FT(0);

    m_volumes.clear();
    m_volume_level_map.clear();
  }

  std::map<PFace, std::pair<int, int> >& pface_neighbors() { return m_map_volumes; }
  const std::map<PFace, std::pair<int, int> >& pface_neighbors() const { return m_map_volumes; }

  std::map<int, std::size_t>& volume_level_map() { return m_volume_level_map; }
  const std::map<int, std::size_t>& volume_level_map() const { return m_volume_level_map; }

  void precompute_iedge_data() {

    for (std::size_t i = 0; i < number_of_support_planes(); ++i) {
      auto& unique_iedges = support_plane(i).unique_iedges();
      CGAL_assertion(unique_iedges.size() > 0);

      auto& iedges    = this->iedges(i);
      auto& ibboxes   = this->ibboxes(i);
      auto& isegments = this->isegments(i);

      iedges.clear();
      iedges.reserve(unique_iedges.size());
      std::copy(unique_iedges.begin(), unique_iedges.end(), std::back_inserter(iedges));
      unique_iedges.clear();

      ibboxes.clear();
      isegments.clear();

      ibboxes.reserve(iedges.size());
      isegments.reserve(iedges.size());

      for (const auto& iedge : iedges) {
        isegments.push_back(segment_2(i, iedge));
        ibboxes.push_back(isegments.back().bbox());
      }
    }
  }

  void set_limit_lines() {

    m_limit_lines.clear();
    m_limit_lines.resize(nb_intersection_lines());

    std::vector<std::size_t> sps;
    std::set<std::size_t> unique_sps;
    std::set<PEdge> unique_pedges;

    auto pvertex = null_pvertex();
    std::size_t num_1_intersected = 0;
    std::size_t num_2_intersected = 0;

    std::vector<IEdge> iedges;
    for (std::size_t i = 0; i < m_limit_lines.size(); ++i) {

      iedges.clear();
      for (const auto iedge : this->iedges()) {
        const auto line_idx = this->line_idx(iedge);
        CGAL_assertion(line_idx != KSR::no_element());
        CGAL_assertion(line_idx < m_limit_lines.size());
        if (line_idx == i) {
          iedges.push_back(iedge);
        }
      }
      CGAL_assertion(iedges.size() > 0);

      unique_pedges.clear();
      for (const auto& iedge : iedges) {
        get_occupied_pedges(pvertex, iedge, unique_pedges);
      }
      CGAL_assertion(unique_pedges.size() >= 0);
      if (unique_pedges.size() == 0) continue;

      unique_sps.clear();
      for (const auto& pedge : unique_pedges) {
        unique_sps.insert(pedge.first);
      }
      CGAL_assertion(unique_sps.size() >  0);
      CGAL_assertion_msg(unique_sps.size() <= 2,
        "TODO: CAN WE HAVE MORE THAN 2 INTERSECTIONS?");

      sps.clear();
      std::copy(unique_sps.begin(), unique_sps.end(), std::back_inserter(sps));
      CGAL_assertion(sps.size() == unique_sps.size());

      auto& pairs = m_limit_lines[i];
      CGAL_assertion(pairs.size() == 0);

      if (sps.size() == 0) {

        // do nothing

      } else if (sps.size() == 1) {

        const auto sp_idx_1 = sps[0];
        std::vector<std::size_t> potential_sps;
        const auto intersected_planes = this->intersected_planes(iedges[0]);
        for (const auto plane_idx : intersected_planes) {
          if (plane_idx == sp_idx_1) continue;
          CGAL_assertion(plane_idx >= 6);
          potential_sps.push_back(plane_idx);
        }
        CGAL_assertion_msg(potential_sps.size() == 1,
        "TODO: CAN WE HAVE MORE THAN 2 INTERSECTIONS?");
        const auto sp_idx_2 = potential_sps[0];

        CGAL_assertion(sp_idx_2 != sp_idx_1);
        CGAL_assertion(sp_idx_1 != KSR::no_element());
        CGAL_assertion(sp_idx_2 != KSR::no_element());

        pairs.push_back(std::make_pair(std::make_pair(sp_idx_1, sp_idx_2), false));

        // Makes results much better! ??
        // Probably because it gives more available intersections between planes
        // that is the same as increasing k. Is it good? No! Is it correct?
        // pairs.push_back(std::make_pair(std::make_pair(sp_idx_2, sp_idx_1), false));

        ++num_1_intersected;
        // if (m_verbose) {
        //   std::cout << "pair 1: " << std::to_string(sp_idx_1) << "/" << std::to_string(sp_idx_2) << std::endl;
        // }
        // CGAL_assertion_msg(false, "TODO: 1 POLYGON IS INTERSECTED!");

      } else if (sps.size() == 2) {

        const auto sp_idx_1 = sps[0];
        const auto sp_idx_2 = sps[1];

        CGAL_assertion(sp_idx_2 != sp_idx_1);
        CGAL_assertion(sp_idx_1 != KSR::no_element());
        CGAL_assertion(sp_idx_2 != KSR::no_element());

        pairs.push_back(std::make_pair(std::make_pair(sp_idx_1, sp_idx_2), false));
        pairs.push_back(std::make_pair(std::make_pair(sp_idx_2, sp_idx_1), false));
        ++num_2_intersected;
        // if (m_verbose) {
        //   std::cout << "pair 1: " << std::to_string(sp_idx_1) << "/" << std::to_string(sp_idx_2) << std::endl;
        //   std::cout << "pair 2: " << std::to_string(sp_idx_2) << "/" << std::to_string(sp_idx_1) << std::endl;
        // }
        // CGAL_assertion_msg(false, "TODO: 2 POLYGONS ARE INTERSECTED!");

      } else {

        CGAL_assertion(sps.size() > 2);
        CGAL_assertion_msg(false,
        "TODO: CAN WE HAVE MORE THAN 2 INTERSECTIONS?");
      }
    }

    if (m_verbose) {
      std::cout << "- num 1 intersected: " << num_1_intersected << std::endl;
      std::cout << "- num 2 intersected: " << num_2_intersected << std::endl;
    }
    // CGAL_assertion_msg(false, "TODO: SET LIMIT LINES!");
  }

  void set_input_polygon_map(
    const std::map<std::size_t, std::size_t>& input_polygon_map) {
    m_input_polygon_map = input_polygon_map;
  }

  const int support_plane_index(const std::size_t polygon_index) const {

    CGAL_assertion(m_input_polygon_map.find(polygon_index) != m_input_polygon_map.end());
    const std::size_t sp_idx = m_input_polygon_map.at(polygon_index);
    return static_cast<int>(sp_idx);
  }

  const int number_of_volume_levels() const {
    return static_cast<int>(m_volume_level_map.size());
  }

  const std::size_t number_of_volumes(const int volume_level) const {

    CGAL_assertion(volume_level < number_of_volume_levels());
    if (volume_level >= number_of_volume_levels()) return std::size_t(-1);
    if (volume_level < 0) {
      return m_volumes.size();
    }

    CGAL_assertion(volume_level >= 0);
    CGAL_assertion(m_volume_level_map.find(volume_level) != m_volume_level_map.end());
    return m_volume_level_map.at(volume_level);
  }

  template<typename DS>
  void convert(DS& ds) {

    ds.clear();
    ds.resize(number_of_support_planes());
    CGAL_assertion(ds.number_of_support_planes() == number_of_support_planes());

    m_intersection_graph.convert(ds.igraph());
    for (std::size_t i = 0; i < number_of_support_planes(); ++i) {
      m_support_planes[i].convert(m_intersection_graph, ds.support_planes()[i]);
    }
    ds.set_input_polygon_map(m_input_polygon_map);
  }

  /*******************************
  **          GENERAL           **
  ********************************/

  const std::vector<Support_plane>& support_planes() const { return m_support_planes; }
  std::vector<Support_plane>& support_planes() { return m_support_planes; }

  const Intersection_graph& igraph() const { return m_intersection_graph; }
  Intersection_graph& igraph() { return m_intersection_graph; }

  void resize(const std::size_t number_of_items) {
    m_support_planes.resize(number_of_items);
  }

  void reserve(const std::size_t number_of_polygons) {
    m_support_planes.reserve(number_of_polygons + 6);
  }

  const FT current_time() const { return m_current_time; }
  const FT previous_time() const { return m_previous_time; }

  void update_positions(const FT time) {
    m_previous_time = m_current_time;
    m_current_time = time;
  }

  void set_last_event_time(const PVertex& pvertex, const FT time) {
    support_plane(pvertex).set_last_event_time(pvertex.second, time);
  }

  const FT last_event_time(const PVertex& pvertex) {
    return support_plane(pvertex).last_event_time(pvertex.second);
  }

  std::vector<Volume_cell>& volumes() { return m_volumes; }
  const std::vector<Volume_cell>& volumes() const { return m_volumes; }

  Reconstructed_model& reconstructed_model() { return m_reconstructed_model; }
  const Reconstructed_model& reconstructed_model() const { return m_reconstructed_model; }

  /*******************************
  **      SUPPORT PLANES        **
  ********************************/

  template<typename PSimplex>
  const Support_plane& support_plane(const PSimplex& psimplex) const { return support_plane(psimplex.first); }
  const Support_plane& support_plane(const std::size_t idx) const { return m_support_planes[idx]; }

  template<typename PSimplex>
  Support_plane& support_plane(const PSimplex& psimplex) { return support_plane(psimplex.first); }
  Support_plane& support_plane(const std::size_t idx) { return m_support_planes[idx]; }

  template<typename PSimplex>
  const Mesh& mesh(const PSimplex& psimplex) const { return mesh(psimplex.first); }
  const Mesh& mesh(const std::size_t support_plane_idx) const { return support_plane(support_plane_idx).mesh(); }

  template<typename PSimplex>
  Mesh& mesh(const PSimplex& psimplex) { return mesh(psimplex.first); }
  Mesh& mesh(const std::size_t support_plane_idx) { return support_plane(support_plane_idx).mesh(); }

  const std::size_t number_of_support_planes() const {
    return m_support_planes.size();
  }

  const bool is_bbox_support_plane(const std::size_t support_plane_idx) const {
    return (support_plane_idx < 6);
  }

  template<typename PointRange>
  const std::size_t add_support_plane(const PointRange& polygon) {

    const Support_plane new_support_plane(polygon);
    std::size_t support_plane_idx = KSR::no_element();
    bool found_coplanar_polygons = false;
    for (std::size_t i = 0; i < number_of_support_planes(); ++i) {
      if (new_support_plane == support_plane(i)) {
        found_coplanar_polygons = true;
        support_plane_idx = i;
        return support_plane_idx;
      }
    }
    CGAL_assertion_msg(!found_coplanar_polygons,
    "ERROR: NO COPLANAR POLYGONS HERE!");

    if (support_plane_idx == KSR::no_element()) {
      support_plane_idx = number_of_support_planes();
      m_support_planes.push_back(new_support_plane);
    }

    intersect_with_bbox(support_plane_idx);
    return support_plane_idx;
  }

  void intersect_with_bbox(const std::size_t support_plane_idx) {
    if (support_plane_idx < 6) return;

    Point_3 point;
    Point_3 centroid_3 = CGAL::ORIGIN;
    std::vector< std::pair<IEdge, Point_3> > intersections;

    for (const IEdge iedge : m_intersection_graph.edges()) {
      if (!KSR::intersection(
        support_plane(support_plane_idx).plane(), segment_3(iedge), point)) {
        continue;
      }

      centroid_3 = CGAL::barycenter(
        centroid_3, static_cast<FT>(intersections.size()), point, FT(1));
      intersections.push_back(std::make_pair(iedge, point));
    }

    Point_2 centroid_2 = support_plane(support_plane_idx).to_2d(centroid_3);
    std::sort(intersections.begin(), intersections.end(),
    [&] (const std::pair<IEdge, Point_3>& a, const std::pair<IEdge, Point_3>& b) -> bool {
      const auto a2 = support_plane(support_plane_idx).to_2d(a.second);
      const auto b2 = support_plane(support_plane_idx).to_2d(b.second);
      const Segment_2 sega(centroid_2, a2);
      const Segment_2 segb(centroid_2, b2);
      return ( Direction_2(sega) < Direction_2(segb) );
    });

    std::vector<std::size_t> common_planes_idx;
    std::map<std::size_t, std::size_t> map_lines_idx;
    std::vector<IVertex> vertices;

    const std::size_t n = intersections.size();
    vertices.reserve(n);

    for (std::size_t i = 0; i < n; ++i) {
      const auto& iedge0 = intersections[i].first;
      const auto& iedge1 = intersections[(i + 1) % n].first;

      std::size_t common_plane_idx = KSR::no_element();
      std::set_intersection(
        m_intersection_graph.intersected_planes(iedge0).begin(),
        m_intersection_graph.intersected_planes(iedge0).end(),
        m_intersection_graph.intersected_planes(iedge1).begin(),
        m_intersection_graph.intersected_planes(iedge1).end(),
        boost::make_function_output_iterator(
          [&](const std::size_t& idx) -> void {
            if (idx < 6) {
              CGAL_assertion(common_plane_idx == KSR::no_element());
              common_plane_idx = idx;
            }
          }
        )
      );
      CGAL_assertion(common_plane_idx != KSR::no_element());
      common_planes_idx.push_back(common_plane_idx);

      typename std::map<std::size_t, std::size_t>::iterator iter;
      const auto pair = map_lines_idx.insert(std::make_pair(common_plane_idx, KSR::no_element()));
      const bool is_inserted = pair.second;
      if (is_inserted) {
        pair.first->second = m_intersection_graph.add_line();
      }
      vertices.push_back(m_intersection_graph.add_vertex(
        intersections[i].second).first);
    }
    CGAL_assertion(vertices.size() == n);

    for (std::size_t i = 0; i < n; ++i) {
      const auto& iplanes = m_intersection_graph.intersected_planes(intersections[i].first);
      for (const std::size_t sp_idx : iplanes) {
        support_plane(sp_idx).unique_iedges().erase(intersections[i].first);
      }
      const auto edges = m_intersection_graph.split_edge(
        intersections[i].first, vertices[i]);

      const auto& iplanes_1 = m_intersection_graph.intersected_planes(edges.first);
      for (const std::size_t sp_idx : iplanes_1) {
        support_plane(sp_idx).unique_iedges().insert(edges.first);
      }

      const auto& iplanes_2 = m_intersection_graph.intersected_planes(edges.second);
      for (const std::size_t sp_idx : iplanes_2) {
        support_plane(sp_idx).unique_iedges().insert(edges.second);
      }

      const auto new_edge = m_intersection_graph.add_edge(
        vertices[i], vertices[(i + 1) % n], support_plane_idx).first;
      m_intersection_graph.intersected_planes(new_edge).insert(common_planes_idx[i]);
      m_intersection_graph.set_line(new_edge, map_lines_idx[common_planes_idx[i]]);

      support_plane(support_plane_idx).unique_iedges().insert(new_edge);
      support_plane(common_planes_idx[i]).unique_iedges().insert(new_edge);
    }
  }

  template<typename PointRange>
  void add_bbox_polygon(const PointRange& polygon) {

    const std::size_t support_plane_idx = add_support_plane(polygon);

    std::array<IVertex, 4> ivertices;
    std::array<Point_2, 4> points;
    for (std::size_t i = 0; i < 4; ++i) {
      points[i] = support_plane(support_plane_idx).to_2d(polygon[i]);
      ivertices[i] = m_intersection_graph.add_vertex(polygon[i]).first;
    }

    const auto vertices =
      support_plane(support_plane_idx).add_bbox_polygon(points, ivertices);

    for (std::size_t i = 0; i < 4; ++i) {
      const auto pair = m_intersection_graph.add_edge(ivertices[i], ivertices[(i+1)%4], support_plane_idx);
      const auto& iedge = pair.first;
      const bool is_inserted = pair.second;
      if (is_inserted) {
        m_intersection_graph.set_line(iedge, m_intersection_graph.add_line());
      }

      support_plane(support_plane_idx).set_iedge(vertices[i], vertices[(i + 1) % 4], iedge);
      support_plane(support_plane_idx).unique_iedges().insert(iedge);
    }
  }

  template<typename PointRange>
  void add_input_polygon(
    const PointRange& polygon, const std::size_t input_index) {

    const std::size_t support_plane_idx = add_support_plane(polygon);
    std::vector<Point_2> points;
    points.reserve(polygon.size());
    for (const auto& point : polygon) {
      const Point_3 converted(
        static_cast<FT>(point.x()),
        static_cast<FT>(point.y()),
        static_cast<FT>(point.z()));
      points.push_back(support_plane(support_plane_idx).to_2d(converted));
    }
    const auto centroid = sort_points_by_direction(points);
    std::vector<std::size_t> input_indices;
    input_indices.push_back(input_index);
    support_plane(support_plane_idx).
      add_input_polygon(points, centroid, input_indices);
    m_input_polygon_map[input_index] = support_plane_idx;
  }

  const Point_2 sort_points_by_direction(
    std::vector<Point_2>& points) const {

    // Naive version.
    // const auto centroid = CGAL::centroid(points.begin(), points.end());

    // Better version.
    using TRI = CGAL::Delaunay_triangulation_2<Kernel>;
    TRI tri(points.begin(), points.end());
    std::vector<Triangle_2> triangles;
    triangles.reserve(tri.number_of_faces());
    for (auto fit = tri.finite_faces_begin(); fit != tri.finite_faces_end(); ++fit) {
      triangles.push_back(Triangle_2(
          fit->vertex(0)->point(), fit->vertex(1)->point(), fit->vertex(2)->point()));
    }
    const auto centroid = CGAL::centroid(triangles.begin(), triangles.end());

    std::sort(points.begin(), points.end(),
    [&](const Point_2& a, const Point_2& b) -> bool {
      const Segment_2 sega(centroid, a);
      const Segment_2 segb(centroid, b);
      return ( Direction_2(sega) < Direction_2(segb) );
    });
    return centroid;
  }

  void add_input_polygon(
    const std::size_t support_plane_idx,
    const std::vector<std::size_t>& input_indices,
    std::vector<Point_2>& points) {

    const auto centroid = sort_points_by_direction(points);
    support_plane(support_plane_idx).
      add_input_polygon(points, centroid, input_indices);
    for (const std::size_t input_index : input_indices) {
      m_input_polygon_map[input_index] = support_plane_idx;
    }
  }

  /*******************************
  **        PSimplices          **
  ********************************/

  static PVertex null_pvertex() { return PVertex(KSR::no_element(), Vertex_index()); }
  static PEdge   null_pedge()   { return   PEdge(KSR::no_element(),   Edge_index()); }
  static PFace   null_pface()   { return   PFace(KSR::no_element(),   Face_index()); }

  const PVertices pvertices(const std::size_t support_plane_idx) const {
    return PVertices(
      boost::make_transform_iterator(
        mesh(support_plane_idx).vertices().begin(),
        Make_PSimplex<PVertex>(support_plane_idx)),
      boost::make_transform_iterator(
        mesh(support_plane_idx).vertices().end(),
        Make_PSimplex<PVertex>(support_plane_idx)));
  }

  const PEdges pedges(const std::size_t support_plane_idx) const {
    return PEdges(
      boost::make_transform_iterator(
        mesh(support_plane_idx).edges().begin(),
        Make_PSimplex<PEdge>(support_plane_idx)),
      boost::make_transform_iterator(
        mesh(support_plane_idx).edges().end(),
        Make_PSimplex<PEdge>(support_plane_idx)));
  }

  const PFaces pfaces(const std::size_t support_plane_idx) const {
    return PFaces(
      boost::make_transform_iterator(
        mesh(support_plane_idx).faces().begin(),
        Make_PSimplex<PFace>(support_plane_idx)),
      boost::make_transform_iterator(
        mesh(support_plane_idx).faces().end(),
        Make_PSimplex<PFace>(support_plane_idx)));
  }

  // Get prev and next pvertices of the free pvertex.
  const PVertex prev(const PVertex& pvertex) const {
    return PVertex(pvertex.first, support_plane(pvertex).prev(pvertex.second));
  }
  const PVertex next(const PVertex& pvertex) const {
    return PVertex(pvertex.first, support_plane(pvertex).next(pvertex.second));
  }

  // Get prev and next pvertices of the constrained pvertex.
  const std::pair<PVertex, PVertex> prev_and_next(const PVertex& pvertex) const {

    std::pair<PVertex, PVertex> out(null_pvertex(), null_pvertex());
    for (const auto& he : halfedges_around_target(
      halfedge(pvertex.second, mesh(pvertex)), mesh(pvertex))) {

      const auto iedge = support_plane(pvertex).iedge(mesh(pvertex).edge(he));
      if (iedge == this->iedge(pvertex)) {
        continue;
      }
      if (out.first == null_pvertex()) {
        out.first  = PVertex(pvertex.first, mesh(pvertex).source(he));
      } else {
        out.second = PVertex(pvertex.first, mesh(pvertex).source(he));
        return out;
      }
    }
    return out;
  }

  const std::pair<PVertex, PVertex> border_prev_and_next(const PVertex& pvertex) const {

    // std::cout << point_3(pvertex) << std::endl;
    auto he = mesh(pvertex).halfedge(pvertex.second);
    const auto end = he;

    // std::cout << point_3(PVertex(pvertex.first, mesh(pvertex).source(he))) << std::endl;
    // std::cout << point_3(PVertex(pvertex.first, mesh(pvertex).target(he))) << std::endl;

    // If the assertion below fails, it probably means that we need to circulate
    // longer until we hit the border edge!

    std::size_t count = 0;
    while (true) {
      if (mesh(pvertex).face(he) != Face_index()) {
        he = mesh(pvertex).prev(mesh(pvertex).opposite(he));

        // std::cout << point_3(PVertex(pvertex.first, mesh(pvertex).source(he))) << std::endl;
        // std::cout << point_3(PVertex(pvertex.first, mesh(pvertex).target(he))) << std::endl;

        ++count;
      } else { break; }

      // std::cout << "count: " << count << std::endl;
      CGAL_assertion(count <= 2);
      if (he == end) {
        CGAL_assertion_msg(false, "ERROR: BORDER HALFEDGE IS NOT FOUND, FULL CIRCLE!");
        break;
      }
      if (count == 100) {
        CGAL_assertion_msg(false, "ERROR: BORDER HALFEDGE IS NOT FOUND, LIMIT ITERATIONS!");
        break;
      }
    }

    CGAL_assertion(mesh(pvertex).face(he) == Face_index());
    return std::make_pair(
      PVertex(pvertex.first, mesh(pvertex).source(he)),
      PVertex(pvertex.first, mesh(pvertex).target(mesh(pvertex).next(he))));
  }

  const PVertex add_pvertex(const std::size_t support_plane_idx, const Point_2& point) {

    CGAL_assertion(support_plane_idx != KSR::uninitialized());
    CGAL_assertion(support_plane_idx != KSR::no_element());

    auto& m = mesh(support_plane_idx);
    const auto vi = m.add_vertex(point);
    CGAL_assertion(vi != typename Support_plane::Mesh::Vertex_index());
    return PVertex(support_plane_idx, vi);
  }

  template<typename VertexRange>
  const PFace add_pface(const VertexRange& pvertices) {

    const auto support_plane_idx = pvertices.front().first;
    CGAL_assertion(support_plane_idx != KSR::uninitialized());
    CGAL_assertion(support_plane_idx != KSR::no_element());

    auto& m = mesh(support_plane_idx);
    const auto range = CGAL::make_range(
      boost::make_transform_iterator(pvertices.begin(),
      CGAL::Property_map_to_unary_function<CGAL::Second_of_pair_property_map<PVertex> >()),
      boost::make_transform_iterator(pvertices.end(),
      CGAL::Property_map_to_unary_function<CGAL::Second_of_pair_property_map<PVertex> >()));
    const auto fi = m.add_face(range);
    CGAL_assertion(fi != Support_plane::Mesh::null_face());
    return PFace(support_plane_idx, fi);
  }

  void clear_polygon_faces(const std::size_t support_plane_idx) {
    Mesh& m = mesh(support_plane_idx);
    for (const auto& fi : m.faces()) {
      m.remove_face(fi);
    }
    for (const auto& ei : m.edges()) {
      m.remove_edge(ei);
    }
    for (const auto& vi : m.vertices()) {
      m.set_halfedge(vi, Halfedge_index());
    }
  }

  const PVertex source(const PEdge& pedge) const {
    return PVertex(pedge.first, mesh(pedge).source(mesh(pedge).halfedge(pedge.second)));
  }
  const PVertex target(const PEdge& pedge) const {
    return PVertex(pedge.first, mesh(pedge).target(mesh(pedge).halfedge(pedge.second)));
  }
  const PVertex opposite(const PEdge& pedge, const PVertex& pvertex) const {

    if (mesh(pedge).target(mesh(pedge).halfedge(pedge.second)) == pvertex.second) {
      return PVertex(pedge.first, mesh(pedge).source(mesh(pedge).halfedge(pedge.second)));
    }
    CGAL_assertion(mesh(pedge).source(mesh(pedge).halfedge(pedge.second)) == pvertex.second);
    return PVertex(pedge.first, mesh(pedge).target(mesh(pedge).halfedge(pedge.second)));
  }

  const Point_3 centroid_of_pface(const PFace& pface) const {

    const std::function<Point_3(PVertex)> unary_f =
    [&](const PVertex& pvertex) -> Point_3 {
      return point_3(pvertex);
    };
    const std::vector<Point_3> polygon(
      boost::make_transform_iterator(pvertices_of_pface(pface).begin(), unary_f),
      boost::make_transform_iterator(pvertices_of_pface(pface).end()  , unary_f));
    CGAL_assertion(polygon.size() >= 3);
    return CGAL::centroid(polygon.begin(), polygon.end());
  }

  const Plane_3 plane_of_pface(const PFace& pface) const {

    const std::function<Point_3(PVertex)> unary_f =
    [&](const PVertex& pvertex) -> Point_3 {
      return point_3(pvertex);
    };
    const std::vector<Point_3> polygon(
      boost::make_transform_iterator(pvertices_of_pface(pface).begin(), unary_f),
      boost::make_transform_iterator(pvertices_of_pface(pface).end()  , unary_f));
    CGAL_assertion(polygon.size() >= 3);
    return Plane_3(polygon[0], polygon[1], polygon[2]);
  }

  const PFace pface_of_pvertex(const PVertex& pvertex) const {
    return PFace(pvertex.first, support_plane(pvertex).face(pvertex.second));
  }

  const std::pair<PFace, PFace> pfaces_of_pvertex(const PVertex& pvertex) const {

    std::pair<PFace, PFace> out(null_pface(), null_pface());
    std::tie(out.first.second, out.second.second) =
      support_plane(pvertex).faces(pvertex.second);
    if (out.first.second != Face_index()) {
      out.first.first = pvertex.first;
    }
    if (out.second.second != Face_index()) {
      out.second.first = pvertex.first;
    }
    return out;
  }

  const PFaces_around_pvertex pfaces_around_pvertex(const PVertex& pvertex) const {

    return PFaces_around_pvertex(
      boost::make_transform_iterator(
        halfedges_around_target(halfedge(pvertex.second, mesh(pvertex)), mesh(pvertex)).begin(),
        Halfedge_to_pface(pvertex.first, mesh(pvertex))),
      boost::make_transform_iterator(
        halfedges_around_target(halfedge(pvertex.second, mesh(pvertex)), mesh(pvertex)).end(),
        Halfedge_to_pface(pvertex.first, mesh(pvertex))));
  }

  void non_null_pfaces_around_pvertex(
    const PVertex& pvertex, std::vector<PFace>& pfaces) const {

    pfaces.clear();
    const auto nfaces = pfaces_around_pvertex(pvertex);
    for (const auto pface : nfaces) {
      if (pface.second == Support_plane::Mesh::null_face()) continue;
      pfaces.push_back(pface);
    }
  }

  const PVertices_of_pface pvertices_of_pface(const PFace& pface) const {

    return PVertices_of_pface(
      boost::make_transform_iterator(
        halfedges_around_face(halfedge(pface.second, mesh(pface)), mesh(pface)).begin(),
        Halfedge_to_pvertex(pface.first, mesh(pface))),
      boost::make_transform_iterator(
        halfedges_around_face(halfedge(pface.second, mesh(pface)), mesh(pface)).end(),
        Halfedge_to_pvertex(pface.first, mesh(pface))));
  }

  const PEdges_of_pface pedges_of_pface(const PFace& pface) const {

    return PEdges_of_pface(
      boost::make_transform_iterator(
        halfedges_around_face(halfedge(pface.second, mesh(pface)), mesh(pface)).begin(),
        Halfedge_to_pedge(pface.first, mesh(pface))),
      boost::make_transform_iterator(
        halfedges_around_face(halfedge(pface.second, mesh(pface)), mesh(pface)).end(),
        Halfedge_to_pedge(pface.first, mesh(pface))));
  }

  const PEdges_around_pvertex pedges_around_pvertex(const PVertex& pvertex) const {
    return PEdges_around_pvertex(
      boost::make_transform_iterator(
        halfedges_around_target(halfedge(pvertex.second, mesh(pvertex)), mesh(pvertex)).begin(),
        Halfedge_to_pedge(pvertex.first, mesh(pvertex))),
      boost::make_transform_iterator(
        halfedges_around_target(halfedge(pvertex.second, mesh(pvertex)), mesh(pvertex)).end(),
        Halfedge_to_pedge(pvertex.first, mesh(pvertex))));
  }

  const std::vector<Volume_cell> incident_volumes(const PFace& query_pface) const {
    std::vector<Volume_cell> nvolumes;
    for (const auto& volume : m_volumes) {
      for (const auto& pface : volume.pfaces) {
        if (pface == query_pface) nvolumes.push_back(volume);
      }
    }
    return nvolumes;
  }

  void incident_faces(const IEdge& query_iedge, std::vector<PFace>& nfaces) const {

    nfaces.clear();
    for (const auto plane_idx : intersected_planes(query_iedge)) {
      for (const auto pedge : pedges(plane_idx)) {
        if (iedge(pedge) == query_iedge) {
          const auto& m = mesh(plane_idx);
          const auto he = m.halfedge(pedge.second);
          const auto op = m.opposite(he);
          const auto face1 = m.face(he);
          const auto face2 = m.face(op);
          if (face1 != Support_plane::Mesh::null_face()) {
            nfaces.push_back(PFace(plane_idx, face1));
          }
          if (face2 != Support_plane::Mesh::null_face()) {
            nfaces.push_back(PFace(plane_idx, face2));
          }
        }
      }
    }
  }

  const std::vector<std::size_t>& input(const PFace& pface) const{ return support_plane(pface).input(pface.second); }
  std::vector<std::size_t>& input(const PFace& pface) { return support_plane(pface).input(pface.second); }

  const unsigned int& k(const std::size_t support_plane_idx) const { return support_plane(support_plane_idx).k(); }
  unsigned int& k(const std::size_t support_plane_idx) { return support_plane(support_plane_idx).k(); }

  const unsigned int& k(const PFace& pface) const { return support_plane(pface).k(pface.second); }
  unsigned int& k(const PFace& pface) { return support_plane(pface).k(pface.second); }

  const bool is_frozen(const PVertex& pvertex) const { return support_plane(pvertex).is_frozen(pvertex.second); }

  const Vector_2& direction(const PVertex& pvertex) const { return support_plane(pvertex).direction(pvertex.second); }
  Vector_2& direction(const PVertex& pvertex) { return support_plane(pvertex).direction(pvertex.second); }

  const FT speed(const PVertex& pvertex) { return support_plane(pvertex).speed(pvertex.second); }

  const bool is_active(const PVertex& pvertex) const { return support_plane(pvertex).is_active(pvertex.second); }

  void deactivate(const PVertex& pvertex) {

    support_plane(pvertex).set_active(pvertex.second, false);
    if (iedge(pvertex) != null_iedge()) {
      m_intersection_graph.is_active(iedge(pvertex)) = false;
    }
    // std::cout << str(pvertex) << " ";
    if (ivertex(pvertex) != null_ivertex()) {
      // std::cout << " ivertex: " << point_3(ivertex(pvertex));
      m_intersection_graph.is_active(ivertex(pvertex)) = false;
    }
    // std::cout << std::endl;
  }

  void activate(const PVertex& pvertex) {

    support_plane(pvertex).set_active(pvertex.second, true);
    if (iedge(pvertex) != null_iedge()) {
      m_intersection_graph.is_active(iedge(pvertex)) = true;
    }
    if (ivertex(pvertex) != null_ivertex()) {
      m_intersection_graph.is_active(ivertex(pvertex)) = true;
    }
  }

  /*******************************
  **          ISimplices        **
  ********************************/

  static IVertex null_ivertex() { return Intersection_graph::null_ivertex(); }
  static IEdge null_iedge() { return Intersection_graph::null_iedge(); }

  decltype(auto) ivertices() const { return m_intersection_graph.vertices(); }
  decltype(auto) iedges() const { return m_intersection_graph.edges(); }

  const std::size_t nb_intersection_lines() const { return m_intersection_graph.nb_lines(); }
  const std::size_t line_idx(const IEdge& iedge) const { return m_intersection_graph.line(iedge); }
  const std::size_t line_idx(const PVertex& pvertex) const { return line_idx(iedge(pvertex)); }

  const IVertex add_ivertex(const Point_3& point, const std::set<std::size_t>& support_planes_idx) {

    std::vector<std::size_t> vec_planes;
    std::copy(
      support_planes_idx.begin(),
      support_planes_idx.end(),
      std::back_inserter(vec_planes));
    const auto pair = m_intersection_graph.add_vertex(point, vec_planes);
    const auto ivertex = pair.first;
    return ivertex;
  }

  void add_iedge(const std::set<std::size_t>& support_planes_idx, std::vector<IVertex>& vertices) {

    const auto source = m_intersection_graph.point_3(vertices.front());
    std::sort(vertices.begin(), vertices.end(),
      [&](const IVertex& a, const IVertex& b) -> bool {
        const auto ap = m_intersection_graph.point_3(a);
        const auto bp = m_intersection_graph.point_3(b);
        const auto sq_dist_a = CGAL::squared_distance(source, ap);
        const auto sq_dist_b = CGAL::squared_distance(source, bp);
        return (sq_dist_a < sq_dist_b);
      }
    );

    std::size_t line_idx = m_intersection_graph.add_line();
    for (std::size_t i = 0; i < vertices.size() - 1; ++i) {

      const auto pair = m_intersection_graph.add_edge(
        vertices[i], vertices[i + 1], support_planes_idx);
      const auto iedge = pair.first;
      const auto is_inserted = pair.second;
      CGAL_assertion(is_inserted);
      m_intersection_graph.set_line(iedge, line_idx);

      for (const auto support_plane_idx : support_planes_idx) {
        support_plane(support_plane_idx).unique_iedges().insert(iedge);
      }
    }
  }

  const IVertex source(const IEdge& edge) const { return m_intersection_graph.source(edge); }
  const IVertex target(const IEdge& edge) const { return m_intersection_graph.target(edge); }

  const IVertex opposite(const IEdge& edge, const IVertex& ivertex) const {
    const auto out = source(edge);
    if (out == ivertex) {
      return target(edge);
    }
    CGAL_assertion(target(edge) == ivertex);
    return out;
  }

  decltype(auto) incident_iedges(const IVertex& ivertex) const {
    return m_intersection_graph.incident_edges(ivertex);
  }

  const std::vector<IEdge>& iedges(const std::size_t support_plane_idx) const {
    return support_plane(support_plane_idx).iedges();
  }
  std::vector<IEdge>& iedges(const std::size_t support_plane_idx) {
    return support_plane(support_plane_idx).iedges();
  }

  const std::vector<Segment_2>& isegments(const std::size_t support_plane_idx) const {
    return support_plane(support_plane_idx).isegments();
  }
  std::vector<Segment_2>& isegments(const std::size_t support_plane_idx) {
    return support_plane(support_plane_idx).isegments();
  }

  const std::vector<Bbox_2>& ibboxes(const std::size_t support_plane_idx) const {
    return support_plane(support_plane_idx).ibboxes();
  }
  std::vector<Bbox_2>& ibboxes(const std::size_t support_plane_idx) {
    return support_plane(support_plane_idx).ibboxes();
  }

  const std::set<std::size_t>& intersected_planes(const IEdge& iedge) const {
    return m_intersection_graph.intersected_planes(iedge);
  }

  const std::set<std::size_t> intersected_planes(
    const IVertex& ivertex, const bool keep_bbox = true) const {

    std::set<std::size_t> out;
    for (const auto incident_iedge : incident_iedges(ivertex)) {
      for (const auto support_plane_idx : intersected_planes(incident_iedge)) {
        if (!keep_bbox && support_plane_idx < 6) {
          continue;
        }
        out.insert(support_plane_idx);
      }
    }
    return out;
  }

  const bool is_iedge(const IVertex& source, const IVertex& target) const {
    return m_intersection_graph.is_edge(source, target);
  }

  const bool is_active(const IEdge& iedge) const {
    return m_intersection_graph.is_active(iedge);
  }
  const bool is_active(const IVertex& ivertex) const {
    return m_intersection_graph.is_active(ivertex);
  }

  const bool is_bbox_iedge(const IEdge& edge) const {

    for (const auto support_plane_idx : m_intersection_graph.intersected_planes(edge)) {
      if (support_plane_idx < 6) {
        return true;
      }
    }
    return false;
  }

  /*******************************
  **          STRINGS           **
  ********************************/

  inline const std::string str(const PVertex& pvertex) const {
    return "PVertex(" + std::to_string(pvertex.first) + ":v" + std::to_string(pvertex.second) + ")";
  }
  inline const std::string str(const PEdge& pedge) const {
    return "PEdge(" + std::to_string(pedge.first) + ":e" + std::to_string(pedge.second) + ")";
  }
  inline const std::string str(const PFace& pface) const {
    return "PFace(" + std::to_string(pface.first) + ":f" + std::to_string(pface.second) + ")";
  }
  inline const std::string str(const IVertex& ivertex) const {
    return "IVertex(" + std::to_string(ivertex) + ")";
  }
  inline const std::string str(const IEdge& iedge) const {
    std::ostringstream oss; oss << "IEdge" << iedge; return oss.str();
  }

  inline const std::string lstr(const PFace& pface) const {

    if (pface == null_pface()) {
      return "PFace(null)";
    }
    std::string out = "PFace(" + std::to_string(pface.first) + ":f" + std::to_string(pface.second) + ")[";
    for (const auto pvertex : pvertices_of_pface(pface)) {
      out += "v" + std::to_string(pvertex.second);
    }
    out += "]";
    return out;
  }

  inline const std::string lstr(const PEdge& pedge) const {
    return "PEdge(" + std::to_string(pedge.first) + ":e" + std::to_string(pedge.second)
      + ")[v" + std::to_string(source(pedge).second) + "->v" + std::to_string(target(pedge).second) + "]";
  }

  /*******************************
  **        CONNECTIVITY        **
  ********************************/

  const bool has_complete_graph(const PVertex& pvertex) const {
    if (!has_ivertex(pvertex)) {
      std::cout << "- disconnected pvertex: " << point_3(pvertex) << std::endl;
      CGAL_assertion(has_ivertex(pvertex));
      return false;
    }

    const auto pedges = pedges_around_pvertex(pvertex);
    for (const auto pedge : pedges) {
      if (!has_iedge(pedge)) {
        std::cout << "- disconnected pedge: " << segment_3(pedge) << std::endl;
        CGAL_assertion(has_iedge(pedge));
        return false;
      }
    }
    return true;
  }

  const bool has_one_pface(const PVertex& pvertex) const {
    std::vector<PFace> nfaces;
    const auto pface = pface_of_pvertex(pvertex);
    non_null_pfaces_around_pvertex(pvertex, nfaces);
    CGAL_assertion(nfaces.size() == 1);
    CGAL_assertion(nfaces[0] == pface);
    return (nfaces.size() == 1 && nfaces[0] == pface);
  }

  const bool is_sneaking_pedge(
    const PVertex& pvertex, const PVertex& pother, const IEdge& iedge) const {

    // Here, pvertex and pother must cross the same iedge.
    // Otherwise, this check does not make any sense!
    if (
      is_occupied(pvertex, iedge).first ||
      is_occupied(pother , iedge).first) {
      CGAL_assertion_msg(false,
      "ERROR: TWO PVERTICES SNEAK TO THE OTHER SIDE EVEN WHEN WE HAVE A POLYGON!");
      return true;
    }
    return false;
  }

  const bool must_be_swapped(
    const Point_2& source_p, const Point_2& target_p,
    const PVertex& pextra, const PVertex& pvertex, const PVertex& pother) const {

    const Vector_2 current_direction = compute_future_direction(
      source_p, target_p, pextra, pvertex, pother);
    const Vector_2 iedge_direction(source_p, target_p);
    const FT dot_product = current_direction * iedge_direction;
    CGAL_assertion(dot_product < FT(0));
    return (dot_product < FT(0));
  }

  const bool has_ivertex(const PVertex& pvertex) const { return support_plane(pvertex).has_ivertex(pvertex.second); }
  const IVertex ivertex(const PVertex& pvertex) const { return support_plane(pvertex).ivertex(pvertex.second); }

  const bool has_iedge(const PVertex& pvertex) const { return support_plane(pvertex).has_iedge(pvertex.second); }
  const IEdge iedge(const PVertex& pvertex) const { return support_plane(pvertex).iedge(pvertex.second); }

  const bool has_iedge(const PEdge& pedge) const { return support_plane(pedge).has_iedge(pedge.second); }
  const IEdge iedge(const PEdge& pedge) const { return support_plane(pedge).iedge(pedge.second); }

  const bool has_pedge(
    const std::size_t sp_idx, const IEdge& iedge) const {

    for (const auto pedge : this->pedges(sp_idx)) {
      if (this->iedge(pedge) == iedge) {
        return true;
      }
    }
    return false;
  }

  void connect(const PVertex& pvertex, const IVertex& ivertex) { support_plane(pvertex).set_ivertex(pvertex.second, ivertex); }
  void connect(const PVertex& pvertex, const IEdge& iedge) { support_plane(pvertex).set_iedge(pvertex.second, iedge); }
  void connect(const PVertex& pvertex, const PVertex& pother, const IEdge& iedge) { support_plane(pvertex).set_iedge(pvertex.second, pother.second, iedge); }
  void connect(const PEdge& pedge, const IEdge& iedge) { support_plane(pedge).set_iedge(pedge.second, iedge); }

  const IVertex disconnect_ivertex(const PVertex& pvertex) {
    const auto ivertex = this->ivertex(pvertex);
    support_plane(pvertex).set_ivertex(pvertex.second, null_ivertex());
    return ivertex;
  }

  const IEdge disconnect_iedge(const PVertex& pvertex) {
    const auto iedge = this->iedge(pvertex);
    support_plane(pvertex).set_iedge(pvertex.second, null_iedge());
    return iedge;
  }

  struct Queue_element {
    PVertex previous;
    PVertex pvertex;
    bool front;
    bool previous_was_free;

    Queue_element(
      const PVertex& previous, const PVertex& pvertex,
      const bool front, const bool previous_was_free) :
    previous(previous), pvertex(pvertex),
    front(front), previous_was_free(previous_was_free)
    { }
  };

  const std::vector<PVertex> pvertices_around_ivertex(
    const PVertex& pvertex, const IVertex& ivertex) const {

    if (m_verbose) {
      std::cout.precision(20);
      std::cout << "** searching pvertices around " << str(pvertex) << " wrt " << str(ivertex) << std::endl;
      std::cout << "- pvertex: " << point_3(pvertex) << std::endl;
      std::cout << "- ivertex: " << point_3(ivertex) << std::endl;
    }

    std::deque<PVertex> pvertices;
    pvertices.push_back(pvertex);

    if (m_verbose) {
      const auto iedge = this->iedge(pvertex);
      if (iedge != null_iedge()) {
        std::cout << "- came from: " << str(iedge) << " " << segment_3(iedge) << std::endl;
      } else {
        std::cout << "- came from: unconstrained setting" << std::endl;
      }
    }

    PVertex prev, next;
    std::queue<Queue_element> todo;
    std::tie(prev, next) = border_prev_and_next(pvertex);
    // std::cout << "prev in: " << str(prev)    << " " << point_3(prev)    << std::endl;
    // std::cout << "next in: " << str(next)    << " " << point_3(next)    << std::endl;
    // std::cout << "curr in: " << str(pvertex) << " " << point_3(pvertex) << std::endl;

    todo.push(Queue_element(pvertex, prev, true, false));
    todo.push(Queue_element(pvertex, next, false, false));
    while (!todo.empty()) {

      // std::cout << std::endl;
      auto previous = todo.front().previous;
      auto current  = todo.front().pvertex;
      bool front    = todo.front().front;
      bool previous_was_free = todo.front().previous_was_free;
      todo.pop();

      const auto iedge = this->iedge(current);
      bool is_free = (iedge == null_iedge());
      // std::cout << "is free 1: " << is_free << std::endl;

      // if (!is_free) std::cout << "iedge: " << segment_3(iedge) << std::endl;
      if (!is_free && source(iedge) != ivertex && target(iedge) != ivertex) {
        // std::cout << "is free 2: " << is_free << std::endl;
        is_free = true;
      }

      if (!is_free) {

        auto other = source(iedge);
        if (other == ivertex) { other = target(iedge); }
        else { CGAL_assertion(target(iedge) == ivertex); }

        // Filter backwards vertex.
        const Vector_2 dir1 = direction(current);
        // std::cout << "dir1: " << dir1 << std::endl;
        const Vector_2 dir2(
          point_2(current.first, other), point_2(current.first, ivertex));
        // std::cout << "dir2: " << dir2 << std::endl;
        const FT dot_product = dir1 * dir2;
        // std::cout << "dot: "  << dot_product << std::endl;

        if (dot_product < FT(0)) {
          if (m_verbose) {
            std::cout << "- " << str(current) << " is backwards" << std::endl;
            // std::cout << point_3(current) << std::endl;
          }
          is_free = true;
        }

        if (is_frozen(current)) {
          if (m_verbose) {
            std::cout << "- " << str(current) << " is frozen" << std::endl;
            // std::cout << point_3(current) << std::endl;
          }
          is_free = true;
        }
        // std::cout << "is free 3: " << is_free << std::endl;
      }

      if (previous_was_free && is_free) {
        if (m_verbose) {
          std::cout << "- " << str(current) << " has no iedge, stopping there" << std::endl;
          // std::cout << point_3(current) << std::endl;
        }
        continue;
      }

      if (is_free) {
        if (m_verbose) {
          std::cout << "- " << str(current) << " has no iedge" << std::endl;
          // std::cout << point_3(current) << std::endl;
        }
      } else {
        if (m_verbose) {
          std::cout << "- " << str(current) << " has iedge " << str(iedge)
          << " from " << str(source(iedge)) << " to " << str(target(iedge)) << std::endl;
          // std::cout << segment_3(iedge) << std::endl;
          // std::cout << point_3(current) << std::endl;
        }
      }

      if (front) {
        pvertices.push_front(current);
        // std::cout << "pushed front" << std::endl;
      } else {
        pvertices.push_back(current);
        // std::cout << "pushed back" << std::endl;
      }

      std::tie(prev, next) = border_prev_and_next(current);
      if (prev == previous) {
        CGAL_assertion(next != previous);
        todo.push(Queue_element(current, next, front, is_free));
        // std::cout << "pushed next" << std::endl;
      } else {
        todo.push(Queue_element(current, prev, front, is_free));
        // std::cout << "pushed prev" << std::endl;
      }
    }
    CGAL_assertion(todo.empty());

    std::vector<PVertex> crossed_pvertices;
    crossed_pvertices.reserve(pvertices.size());
    std::copy(pvertices.begin(), pvertices.end(),
    std::back_inserter(crossed_pvertices));

    if (m_verbose) {
      std::cout << "- found " << crossed_pvertices.size() <<
      " pvertices ready to be merged: " << std::endl;
      for (const auto& crossed_pvertex : crossed_pvertices) {
        std::cout << str(crossed_pvertex) << ": " << point_3(crossed_pvertex) << std::endl;
      }
    }
    CGAL_assertion(crossed_pvertices.size() >= 3);
    return crossed_pvertices;
  }

  /*******************************
  **        CONVERSIONS         **
  ********************************/

  const Point_2 to_2d(const std::size_t support_plane_idx, const IVertex& ivertex) const {
    return support_plane(support_plane_idx).to_2d(point_3(ivertex));
  }
  const Segment_2 to_2d(const std::size_t support_plane_idx, const Segment_3& segment_3) const {
    return support_plane(support_plane_idx).to_2d(segment_3);
  }
  const Point_2 to_2d(const std::size_t support_plane_idx, const Point_3& point_3) const {
    return support_plane(support_plane_idx).to_2d(point_3);
  }

  const Point_2 point_2(const PVertex& pvertex, const FT time) const {
    return support_plane(pvertex).point_2(pvertex.second, time);
  }
  const Point_2 point_2(const PVertex& pvertex) const {
    return point_2(pvertex, m_current_time);
  }
  const Point_2 point_2(const std::size_t support_plane_idx, const IVertex& ivertex) const {
    return support_plane(support_plane_idx).to_2d(point_3(ivertex));
  }

  const Segment_2 segment_2(const std::size_t support_plane_idx, const IEdge& iedge) const {
    return support_plane(support_plane_idx).to_2d(segment_3(iedge));
  }

  const Point_3 to_3d(const std::size_t support_plane_idx, const Point_2& point_2) const {
    return support_plane(support_plane_idx).to_3d(point_2);
  }

  const Point_3 point_3(const PVertex& pvertex, const FT time) const {
    return support_plane(pvertex).point_3(pvertex.second, time);
  }
  const Point_3 point_3(const PVertex& pvertex) const {
    return point_3(pvertex, m_current_time);
  }
  const Point_3 point_3(const IVertex& vertex) const {
    return m_intersection_graph.point_3(vertex);
  }

  const Segment_3 segment_3(const PEdge& pedge, const FT time) const {
    return support_plane(pedge).segment_3(pedge.second, time);
  }
  const Segment_3 segment_3(const PEdge& pedge) const {
    return segment_3 (pedge, m_current_time);
  }
  const Segment_3 segment_3(const IEdge& edge) const {
    return m_intersection_graph.segment_3(edge);
  }

  /*******************************
  **          PREDICATES        **
  ********************************/

  // TODO: ADD FUNCTION HAS_PEDGES() OR NUM_PEDGES() THAT RETURNS THE NUMBER OF PEDGES
  // CONNECTED TO THE IEDGE. THAT WILL BE FASTER THAN CURRENT COMPUTATIONS!

  // Check if there is a collision with another polygon.
  const std::pair<bool, bool> collision_occured (
    const PVertex& pvertex, const IEdge& iedge) const {

    bool collision = false;
    for (const auto support_plane_idx : intersected_planes(iedge)) {
      if (support_plane_idx < 6) {
        return std::make_pair(true, true); // bbox plane
      }

      for (const auto pedge : pedges(support_plane_idx)) {
        if (this->iedge(pedge) == iedge) {
          const auto pedge_segment = Segment_3(point_3(source(pedge)), point_3(target(pedge)));

          const Segment_3 source_to_pvertex(pedge_segment.source(), point_3(pvertex));
          const FT dot_product = pedge_segment.to_vector() * source_to_pvertex.to_vector();
          if (dot_product < FT(0)) {
            continue;
          }
          CGAL_assertion(pedge_segment.squared_length() != FT(0));
          if (source_to_pvertex.squared_length() <= pedge_segment.squared_length()) {
            collision = true; break;
          }
        }
      }
    }
    return std::make_pair(collision, false);
  }

  const std::pair<bool, bool> is_occupied(
    const PVertex& pvertex, const IVertex& ivertex, const IEdge& query_iedge) const {

    const auto pair = is_occupied(pvertex, query_iedge);
    const bool has_polygon = pair.first;
    const bool is_bbox_reached = pair.second;

    if (is_bbox_reached) return std::make_pair(true, true);
    CGAL_assertion(!is_bbox_reached);
    if (!has_polygon) {
      // std::cout << "NO POLYGON DETECTED" << std::endl;
      return std::make_pair(false, false);
    }
    CGAL_assertion(has_polygon);

    CGAL_assertion(ivertex != null_ivertex());
    std::set<PEdge> pedges;
    get_occupied_pedges(pvertex, query_iedge, pedges);
    for (const auto& pedge : pedges) {
      CGAL_assertion(pedge != null_pedge());
      // std::cout << "PEDGE: " << segment_3(pedge) << std::endl;

      const auto source = this->source(pedge);
      const auto target = this->target(pedge);
      if (this->ivertex(source) == ivertex || this->ivertex(target) == ivertex) {
        return std::make_pair(true, false);
      }
    }
    return std::make_pair(false, false);
  }

  void get_occupied_pedges(
    const PVertex& pvertex, const IEdge& query_iedge, std::set<PEdge>& pedges) const {

    for (const auto plane_idx : intersected_planes(query_iedge)) {
      if (plane_idx == pvertex.first) continue; // current plane
      if (plane_idx < 6) continue; // bbox plane

      for (const auto pedge : this->pedges(plane_idx)) {
        if (iedge(pedge) == query_iedge) {
          pedges.insert(pedge);
        }
      }
    }
  }

  const std::pair<bool, bool> is_occupied(
    const PVertex& pvertex, const IEdge& query_iedge) const {

    CGAL_assertion(query_iedge != null_iedge());
    // std::cout << str(query_iedge) << ": " << segment_3(query_iedge) << std::endl;
    std::size_t num_adjacent_faces = 0;
    for (const auto plane_idx : intersected_planes(query_iedge)) {
      if (plane_idx == pvertex.first) continue; // current plane
      if (plane_idx < 6) return std::make_pair(true, true); // bbox plane

      for (const auto pedge : pedges(plane_idx)) {
        if (!has_iedge(pedge)) continue;

        // std::cout << str(iedge(pedge)) << std::endl;
        if (iedge(pedge) == query_iedge) {
          const auto& m = mesh(plane_idx);
          const auto he = m.halfedge(pedge.second);
          const auto op = m.opposite(he);
          const auto face1 = m.face(he);
          const auto face2 = m.face(op);
          if (face1 != Support_plane::Mesh::null_face()) ++num_adjacent_faces;
          if (face2 != Support_plane::Mesh::null_face()) ++num_adjacent_faces;
        }
      }
    }

    // std::cout << "num adjacent faces: " << num_adjacent_faces << std::endl;
    if (num_adjacent_faces <= 1)
      return std::make_pair(false, false);
    return std::make_pair(true, false);
  }

  const bool update_limit_lines_and_k(
    const PVertex& pvertex, const IEdge& iedge, const bool is_occupied_iedge) {

    const std::size_t sp_idx_1 = pvertex.first;
    std::size_t sp_idx_2 = KSR::no_element();
    const auto intersected_planes = this->intersected_planes(iedge);
    for (const auto plane_idx : intersected_planes) {
      if (plane_idx == sp_idx_1) continue; // current plane
      if (plane_idx < 6) return true;
      sp_idx_2 = plane_idx;
      break;
    }
    CGAL_assertion(sp_idx_2 != KSR::no_element());
    CGAL_assertion(sp_idx_1 >= 6 && sp_idx_2 >= 6);
    CGAL_assertion(m_limit_lines.size() == nb_intersection_lines());

    bool is_limit_line = false;
    const std::size_t line_idx = this->line_idx(iedge);
    CGAL_assertion(line_idx != KSR::no_element());
    CGAL_assertion(line_idx < m_limit_lines.size());

    auto& pairs = m_limit_lines[line_idx];
    CGAL_assertion_msg(pairs.size() <= 2,
    "TODO: CAN WE HAVE MORE THAN TWO PLANES INTERSECTED ALONG THE SAME LINE?");

    for (const auto& item : pairs) {
      const auto& pair = item.first;

      const bool is_ok_1 = (pair.first  == sp_idx_1);
      const bool is_ok_2 = (pair.second == sp_idx_2);

      if (is_ok_1 && is_ok_2) {
        is_limit_line = item.second;
        if (m_verbose) std::cout << "- found intersection " << std::endl;
        return is_limit_line;
      }
    }

    if (m_verbose) {
      std::cout << "- first time intersection" << std::endl;
      std::cout << "- adding pair: " << std::to_string(sp_idx_1) << "-" << std::to_string(sp_idx_2);
    }

    CGAL_assertion(pairs.size() < 2);
    if (is_occupied_iedge) {
      if (this->k(pvertex.first) == 1) {
        if (m_verbose) std::cout << ", occupied, TRUE" << std::endl;
        is_limit_line = true;
        pairs.push_back(std::make_pair(std::make_pair(sp_idx_1, sp_idx_2), is_limit_line));
      } else {
        if (m_verbose) std::cout << ", occupied, FALSE" << std::endl;
        is_limit_line = false;
        pairs.push_back(std::make_pair(std::make_pair(sp_idx_1, sp_idx_2), is_limit_line));
        this->k(pvertex.first)--;
      }
    } else {
      if (m_verbose) std::cout << ", free, FALSE" << std::endl;
      is_limit_line = false;
      pairs.push_back(std::make_pair(std::make_pair(sp_idx_1, sp_idx_2), is_limit_line));
    }
    CGAL_assertion(pairs.size() <= 2);

    // CGAL_assertion_msg(false, "TODO: IS LIMIT LINE!");
    return is_limit_line;
  }

  /*******************************
  **    OPERATIONS ON POLYGONS  **
  ********************************/

  const PVertex crop_pvertex_along_iedge(
    const PVertex& pvertex, const IEdge& iedge) {

    if (m_verbose) {
      std::cout.precision(20);
      std::cout << "** cropping " << str(pvertex) << " along " << str(iedge) << std::endl;
      std::cout << "- pvertex: " << point_3(pvertex) << std::endl;
      std::cout << "- iedge: "   << segment_3(iedge) << std::endl;
    }

    CGAL_assertion_msg(
      point_2(pvertex.first, source(iedge)) != point_2(pvertex.first, target(iedge)),
    "TODO: PVERTEX -> IEDGE, HANDLE ZERO-LENGTH IEDGE!");

    const PVertex prev(pvertex.first, support_plane(pvertex).prev(pvertex.second));
    const PVertex next(pvertex.first, support_plane(pvertex).next(pvertex.second));

    Point_2 future_point_a, future_point_b;
    Vector_2 future_direction_a, future_direction_b;
    bool is_parallel_a = false, is_parallel_b = false;
    std::tie(is_parallel_a, is_parallel_b) =
    compute_future_points_and_directions(
      pvertex, iedge,
      future_point_a, future_point_b,
      future_direction_a, future_direction_b);
    CGAL_assertion(future_direction_a != Vector_2());
    CGAL_assertion(future_direction_b != Vector_2());
    if (is_parallel_a || is_parallel_b) {
      if (m_verbose) std::cout << "- pvertex to iedge, parallel case" << std::endl;
      // CGAL_assertion_msg(!is_parallel_a && !is_parallel_b,
      // "TODO: PVERTEX -> IEDGE, HANDLE CASE WITH PARALLEL LINES!");
    }

    const PEdge pedge(pvertex.first, support_plane(pvertex).split_vertex(pvertex.second));
    CGAL_assertion(source(pedge) == pvertex || target(pedge) == pvertex);
    const PVertex pother = opposite(pedge, pvertex);
    if (m_verbose) {
      std::cout << "- new pedge: " << str(pedge)  << " between "
        << str(pvertex) << " and " << str(pother) << std::endl;
    }

    connect(pedge, iedge);
    connect(pvertex, iedge);
    connect(pother, iedge);

    support_plane(pvertex).set_point(pvertex.second, future_point_a);
    support_plane(pother).set_point(pother.second, future_point_b);
    direction(pvertex) = future_direction_a;
    direction(pother) = future_direction_b;

    if (m_verbose) std::cout << "- new pvertices: " <<
    str(pother) << ": " << point_3(pother) << std::endl;

    // CGAL_assertion_msg(false, "TODO: CROP PVERTEX ALONG IEDGE!");
    return pother;
  }

  const std::array<PVertex, 3> propagate_pvertex_beyond_iedge(
    const PVertex& pvertex, const IEdge& iedge) {

    if (m_verbose) {
      std::cout.precision(20);
      std::cout << "** propagating " << str(pvertex) << " beyond " << str(iedge) << std::endl;
      std::cout << "- pvertex: " << point_3(pvertex) << std::endl;
      std::cout << "- iedge: "   << segment_3(iedge) << std::endl;
    }

    const Point_2 original_point = point_2(pvertex, FT(0));
    const Vector_2 original_direction = direction(pvertex);
    const PVertex pother = crop_pvertex_along_iedge(pvertex, iedge);

    const PVertex propagated = add_pvertex(pvertex.first, original_point);
    direction(propagated) = original_direction;

    if (m_verbose) {
      std::cout << "- propagated: " << str(propagated) << ": " << point_3(propagated) << std::endl;
    }

    std::array<PVertex, 3> pvertices;
    pvertices[0] = pvertex;
    pvertices[1] = pother;
    pvertices[2] = propagated;

    const PFace new_pface = add_pface(pvertices);
    CGAL_assertion(new_pface != null_pface());
    CGAL_assertion(new_pface.second != Face_index());
    if (m_verbose) {
      std::cout << "- new pface " << str(new_pface) << ": " << centroid_of_pface(new_pface) << std::endl;
    }

    // CGAL_assertion_msg(false, "TODO: PROPAGATE PVERTEX BEYOND IEDGE!");
    return pvertices;
  }

  void crop_pedge_along_iedge(
    const PVertex& pvertex, const PVertex& pother, const IEdge& iedge) {

    if (m_verbose) {
      std::cout.precision(20);
      std::cout << "** cropping pedge [" << str(pvertex) << "-" << str(pother)
      << "] along " << str(iedge) << std::endl;
      std::cout << "- pvertex: " << point_3(pvertex) << std::endl;
      std::cout << "- pother: "  << point_3(pother)  << std::endl;
      std::cout << "- iedge: "   << segment_3(iedge) << std::endl;
    }

    CGAL_assertion(pvertex.first == pother.first);
    CGAL_assertion_msg(
      point_2(pvertex.first, source(iedge)) != point_2(pvertex.first, target(iedge)),
    "TODO: PEDGE -> IEDGE, HANDLE ZERO-LENGTH IEDGE!");
    Point_2 future_point; Vector_2 future_direction;

    { // cropping pvertex ...
      const PVertex prev(pvertex.first, support_plane(pvertex).prev(pvertex.second));
      const PVertex next(pvertex.first, support_plane(pvertex).next(pvertex.second));

      if (m_verbose) {
        std::cout << "- prev pv: " << point_3(prev) << std::endl;
        std::cout << "- next pv: " << point_3(next) << std::endl;
      }

      PVertex pthird = null_pvertex();
      if (pother == prev) {
        pthird = next;
      } else {
        CGAL_assertion(pother == next);
        pthird = prev;
      }
      CGAL_assertion(pthird != null_pvertex());

      if (m_verbose) {
        std::cout << "- pthird pv: " << point_3(pthird) << std::endl;
      }

      const bool is_parallel =
      compute_future_point_and_direction(0, pvertex, pthird, iedge, future_point, future_direction);
      CGAL_assertion(future_direction != Vector_2());
      if (is_parallel) {
        if (m_verbose) std::cout << "- pedge to iedge 1, parallel case" << std::endl;
        // CGAL_assertion_msg(!is_parallel,
        // "TODO: PEDGE -> IEDGE 1, HANDLE CASE WITH PARALLEL LINES!");
      }

      direction(pvertex) = future_direction;
      support_plane(pvertex).set_point(pvertex.second, future_point);
      connect(pvertex, iedge);
    }

    { // cropping pother ...
      const PVertex prev(pother.first, support_plane(pother).prev(pother.second));
      const PVertex next(pother.first, support_plane(pother).next(pother.second));

      if (m_verbose) {
        std::cout << "- prev po: " << point_3(prev) << std::endl;
        std::cout << "- next po: " << point_3(next) << std::endl;
      }

      PVertex pthird = null_pvertex();
      if (pvertex == prev) {
        pthird = next;
      } else {
        CGAL_assertion(pvertex == next);
        pthird = prev;
      }
      CGAL_assertion(pthird != null_pvertex());

      if (m_verbose) {
        std::cout << "- pthird po: " << point_3(pthird) << std::endl;
      }

      const bool is_parallel =
      compute_future_point_and_direction(0, pother, pthird, iedge, future_point, future_direction);
      CGAL_assertion(future_direction != Vector_2());
      if (is_parallel) {
        if (m_verbose) std::cout << "- pedge to iedge 2, parallel case" << std::endl;
        // CGAL_assertion_msg(!is_parallel,
        // "TODO: PEDGE -> IEDGE 2, HANDLE CASE WITH PARALLEL LINES!");
      }

      direction(pother) = future_direction;
      support_plane(pother).set_point(pother.second, future_point);
      connect(pother, iedge);
    }

    const PEdge pedge(pvertex.first, support_plane(pvertex).edge(pvertex.second, pother.second));
    connect(pedge, iedge);

    // CGAL_assertion_msg(false, "TODO: CROP PEDGE ALONG IEDGE!");
  }

  const std::pair<PVertex, PVertex> propagate_pedge_beyond_iedge(
    const PVertex& pvertex, const PVertex& pother, const IEdge& iedge) {

    if (m_verbose) {
      std::cout.precision(20);
      std::cout << "** propagating pedge [" << str(pvertex) << "-" << str(pother)
      << "] beyond " << str(iedge) << std::endl;
      std::cout << "- pvertex: " << point_3(pvertex) << std::endl;
      std::cout << "- pother: "  << point_3(pother)  << std::endl;
      std::cout << "- iedge: "   << segment_3(iedge) << std::endl;
    }

    const Point_2 original_point_1 = point_2(pvertex, FT(0));
    const Point_2 original_point_2 = point_2(pother, FT(0));

    const Vector_2 original_direction_1 = direction(pvertex);
    const Vector_2 original_direction_2 = direction(pother);

    crop_pedge_along_iedge(pvertex, pother, iedge);

    const PVertex propagated_1 = add_pvertex(pvertex.first, original_point_1);
    direction(propagated_1) = original_direction_1;

    const PVertex propagated_2 = add_pvertex(pother.first, original_point_2);
    direction(propagated_2) = original_direction_2;

    if (m_verbose) {
      std::cout << "- propagated 1: " << str(propagated_1) << ": " << point_3(propagated_1) << std::endl;
      std::cout << "- propagated 2: " << str(propagated_2) << ": " << point_3(propagated_2) << std::endl;
    }

    std::array<PVertex, 4> pvertices;
    pvertices[0] = pvertex;
    pvertices[1] = pother;
    pvertices[2] = propagated_2;
    pvertices[3] = propagated_1;

    const PFace new_pface = add_pface(pvertices);
    CGAL_assertion(new_pface != null_pface());
    CGAL_assertion(new_pface.second != Face_index());
    if (m_verbose) {
      std::cout << "- new pface " << str(new_pface) << ": " << centroid_of_pface(new_pface) << std::endl;
    }

    // CGAL_assertion_msg(false, "TODO: PROPAGATE PEDGE BEYOND IEDGE!");
    return std::make_pair(propagated_2, propagated_1);
  }

  const bool transfer_pvertex_via_iedge(
    const PVertex& pvertex, const PVertex& pother) {

    if (m_verbose) {
      std::cout.precision(20);
      CGAL_assertion(has_iedge(pvertex));
      std::cout << "** transfering " << str(pother) << " through " << str(pvertex) << " via "
        << str(iedge(pvertex)) << std::endl;
      std::cout << "- pvertex: " << point_3(pvertex) << std::endl;
      std::cout << "- pother: "  << point_3(pother)  << std::endl;
    }
    CGAL_assertion(pvertex.first == pother.first);

    // Is pvertex adjacent to one or two pfaces?
    PFace source_pface, target_pface;
    std::tie(source_pface, target_pface) = pfaces_of_pvertex(pvertex);
    const auto common_pface = pface_of_pvertex(pother);
    if (common_pface == target_pface) {
      if (m_verbose) std::cout << "- swap pfaces" << std::endl;
      std::swap(source_pface, target_pface);
    }
    CGAL_assertion(common_pface == source_pface);

    if (m_verbose) {
      std::cout << "- initial pfaces: " << std::endl;
      if (source_pface != null_pface()) {
        std::cout << "source " << str(source_pface) << ": " << centroid_of_pface(source_pface) << std::endl;
      }
      if (target_pface != null_pface()) {
        std::cout << "target " << str(target_pface) << ": " << centroid_of_pface(target_pface) << std::endl;
      }
    }

    // Get pthird.
    PVertex pthird = next(pother);
    if (pthird == pvertex) pthird = prev(pother);
    if (m_verbose) std::cout << "- pthird: " << point_3(pthird) << std::endl;

    // Get future point and direction.
    CGAL_assertion(has_iedge(pvertex));
    const auto iedge = this->iedge(pvertex);
    const auto source_p = point_2(pvertex.first, source(iedge));
    const auto target_p = point_2(pvertex.first, target(iedge));
    CGAL_assertion_msg(source_p != target_p,
    "TODO: TRANSFER PVERTEX, HANDLE ZERO-LENGTH IEDGE!");
    const Line_2 iedge_line(source_p, target_p);

    Point_2 future_point;
    Vector_2 future_direction;
    const bool is_parallel =
    compute_future_point_and_direction(0, pother, pthird, iedge, future_point, future_direction);
    CGAL_assertion(future_direction != Vector_2());
    if (is_parallel) {
      if (m_verbose) std::cout << "- transfer pvertex, parallel case" << std::endl;
      // CGAL_assertion_msg(!is_parallel,
      // "TODO: TRANSFER PVERTEX, HANDLE CASE WITH PARALLEL LINES!");
    }

    if (target_pface == null_pface()) { // in case we have 1 pface

      support_plane(pvertex).set_point(pvertex.second, future_point);
      direction(pvertex) = future_direction;
      const auto he = mesh(pvertex).halfedge(pother.second, pvertex.second);
      CGAL::Euler::join_vertex(he, mesh(pvertex));

      // CGAL_assertion_msg(false,
      // "TODO: TRANSFER PVERTEX 1, ADD NEW FUTURE POINTS AND DIRECTIONS!");

    } else { // in case we have both pfaces

      disconnect_iedge(pvertex);
      PEdge pedge = null_pedge();
      for (const auto edge : pedges_around_pvertex(pvertex)) {
        if (this->iedge(edge) == iedge) {
          pedge = edge; break;
        }
      }
      CGAL_assertion(pedge != null_pedge());

      auto he = mesh(pedge).halfedge(pedge.second);
      if (mesh(pedge).face(he) != common_pface.second) {
        he = mesh(pedge).opposite(he);
      }
      CGAL_assertion(mesh(pedge).face(he) == common_pface.second);

      if (mesh(pedge).target(he) == pvertex.second) {
        // if (m_verbose) std::cout << "- shifting target" << std::endl;
        CGAL::Euler::shift_target(he, mesh(pedge));
      } else {
        CGAL_assertion(mesh(pedge).source(he) == pvertex.second);
        // if (m_verbose) std::cout << "- shifting source" << std::endl;
        CGAL::Euler::shift_source(he, mesh(pedge));
      }

      const auto pother_p = point_2(pother);
      const Point_2 pinit = iedge_line.projection(pother_p);
      direction(pvertex) = direction(pother);
      const auto fp = pinit - direction(pother) * m_current_time;
      support_plane(pvertex).set_point(pvertex.second, fp);

      support_plane(pother).set_point(pother.second, future_point);
      direction(pother) = future_direction;
      connect(pother, iedge);

      // CGAL_assertion_msg(false,
      // "TODO: TRANSFER PVERTEX 2, ADD NEW FUTURE POINTS AND DIRECTIONS!");
    }

    if (m_verbose) {
      std::cout << "- new pfaces: " << std::endl;
      if (source_pface != null_pface()) {
        std::cout << "source " << str(source_pface) << ": " << centroid_of_pface(source_pface) << std::endl;
      }
      if (target_pface != null_pface()) {
        std::cout << "target " << str(target_pface) << ": " << centroid_of_pface(target_pface) << std::endl;
      }
    }

    // CGAL_assertion_msg(false, "TODO: TRANSFER PVERTEX VIA IEDGE!");
    return (target_pface != null_pface());
  }

  const std::vector<PVertex> merge_pvertices_on_ivertex(
    const FT min_time, const FT max_time, const IVertex& ivertex,
    const std::vector<PVertex>& pvertices,
    std::vector< std::pair<IEdge, bool> >& crossed_iedges) {

    if (m_verbose) {
      std::cout.precision(20);
      std::cout << "** merging " << str(pvertices[1]) << " on " << str(ivertex) << std::endl;
      std::cout << "- pvertex: " << point_3(pvertices[1]) << std::endl;
      std::cout << "- ivertex: " << point_3(ivertex) << std::endl;
    }

    CGAL_assertion(pvertices.size() >= 3);
    const std::size_t support_plane_idx = pvertices.front().first;
    const PVertex prev = pvertices.front();
    const PVertex next = pvertices.back();
    const PVertex pvertex = pvertices[1];

    if (m_verbose) {
      const auto iedge = this->iedge(pvertex);
      if (iedge != null_iedge()) {
        std::cout << "- start from: " << str(iedge) << " " << segment_3(iedge) << std::endl;
      } else {
        std::cout << "- start from: unconstrained setting" << std::endl;
      }
    }

    // Copy front/back to remember position/direction.
    PVertex front, back;
    if (pvertices.size() < 3) {
      CGAL_assertion_msg(false, "ERROR: INVALID CONNECTIVITY CASE!");
    } else if (pvertices.size() == 3 || pvertices.size() == 4) {

      const auto& initial = pvertex;
      front = PVertex(support_plane_idx, support_plane(support_plane_idx).duplicate_vertex(initial.second));
      support_plane(support_plane_idx).set_point(
        front.second, support_plane(support_plane_idx).get_point(initial.second));
      back  = PVertex(support_plane_idx, support_plane(support_plane_idx).duplicate_vertex(front.second));
      support_plane(support_plane_idx).set_point(
        back.second, support_plane(support_plane_idx).get_point(front.second));

    } else if (pvertices.size() >= 5) {

      const auto& initial1 = pvertices[1];
      front = PVertex(support_plane_idx, support_plane(support_plane_idx).duplicate_vertex(initial1.second));
      support_plane(support_plane_idx).set_point(
        front.second, support_plane(support_plane_idx).get_point(initial1.second));

      const auto& initial2 = pvertices[pvertices.size() - 2];
      back  = PVertex(support_plane_idx, support_plane(support_plane_idx).duplicate_vertex(initial2.second));
      support_plane(support_plane_idx).set_point(
        back.second, support_plane(support_plane_idx).get_point(initial2.second));

    } else {
      CGAL_assertion_msg(false, "ERROR: INVALID CONNECTIVITY CASE!");
    }

    if (m_verbose) {
      std::cout << "- found neighbors: " << std::endl <<
      "prev = " << point_3(prev)  << std::endl <<
      "fron = " << point_3(front) << std::endl <<
      "back = " << point_3(back)  << std::endl <<
      "next = " << point_3(next)  << std::endl;
    }

    // Freeze pvertices.
    const Point_2 ipoint = point_2(support_plane_idx, ivertex);
    for (std::size_t i = 1; i < pvertices.size() - 1; ++i) {
      const PVertex& curr = pvertices[i];
      support_plane(curr).direction(curr.second) = CGAL::NULL_VECTOR;
      support_plane(curr).set_point(curr.second, ipoint);
    }
    connect(pvertex, ivertex);
    if (m_verbose) {
      std::cout << "- frozen pvertex: " << str(pvertex) << " : " << point_3(pvertex) << std::endl;
    }

    // Join pvertices.
    for (std::size_t i = 2; i < pvertices.size() - 1; ++i) {
      const auto he = mesh(support_plane_idx).halfedge(pvertices[i].second, pvertex.second);
      disconnect_ivertex(pvertices[i]);
      CGAL::Euler::join_vertex(he, mesh(support_plane_idx));
    }

    // Get all connected iedges.
    auto inc_iedges = this->incident_iedges(ivertex);
    std::vector< std::pair<IEdge, Direction_2> > iedges;
    std::copy(inc_iedges.begin(), inc_iedges.end(),
      boost::make_function_output_iterator(
        [&](const IEdge& inc_iedge) -> void {
          const auto iplanes = this->intersected_planes(inc_iedge);
          if (iplanes.find(support_plane_idx) == iplanes.end()) {
            return;
          }
          const Direction_2 direction(
            point_2(support_plane_idx, opposite(inc_iedge, ivertex)) -
            point_2(support_plane_idx, ivertex));
          iedges.push_back(std::make_pair(inc_iedge, direction));
        }
      )
    );

    std::sort(iedges.begin(), iedges.end(),
      [&](const std::pair<IEdge, Direction_2>& a,
          const std::pair<IEdge, Direction_2>& b) -> bool {
        return a.second < b.second;
      }
    );
    CGAL_assertion(iedges.size() > 0);

    // Get sub-event type.
    bool back_constrained = false;
    if (
      (iedge(next) != null_iedge() && (source(iedge(next)) == ivertex || target(iedge(next)) == ivertex)) ||
      (this->ivertex(next) != null_ivertex() && is_iedge(this->ivertex(next), ivertex))) {
      back_constrained = true;
    }

    bool front_constrained = false;
    if (
      (iedge(prev) != null_iedge() && (source(iedge(prev)) == ivertex || target(iedge(prev)) == ivertex)) ||
      (this->ivertex(prev) != null_ivertex() && is_iedge(this->ivertex(prev), ivertex))) {
      front_constrained = true;
    }

    if (back_constrained && !front_constrained) {
      if (m_verbose) std::cout << "- reverse iedges" << std::endl;
      std::reverse(iedges.begin(), iedges.end());
    }

    if (m_verbose) {
      std::cout << "- initial iedges: " << std::endl;
      for (const auto& iedge : iedges) {
        std::cout << str(iedge.first) << ": " << segment_3(iedge.first) << std::endl;
      }
    }

    // Handle sub-events.
    crossed_iedges.clear();
    std::vector<PVertex> new_pvertices;

    if (back_constrained && front_constrained) {
      apply_closing_case(pvertex);
    } else if (back_constrained) {
      apply_back_border_case(
        min_time, max_time,
        pvertex, ivertex, back, prev,
        iedges, crossed_iedges, new_pvertices);
    } else if (front_constrained) {
      apply_front_border_case(
        min_time, max_time,
        pvertex, ivertex, front, next,
        iedges, crossed_iedges, new_pvertices);
    } else {
      apply_open_case(
        min_time, max_time,
        pvertex, ivertex, front, back, prev, next,
        iedges, crossed_iedges, new_pvertices);
    }

    support_plane(support_plane_idx).remove_vertex(front.second);
    support_plane(support_plane_idx).remove_vertex(back.second);

    // Push also the remaining pvertex so that its events are recomputed.
    new_pvertices.push_back(pvertex);
    if (iedge(pvertex) != null_iedge()) {
      crossed_iedges.push_back(std::make_pair(iedge(pvertex), true));
    }
    // TODO: I THINK, I SHOULD RETURN ONLY THOSE IEDGES, WHICH HAVE BEEN HANDLED
    // AND THEY SHOULD BE EQUAL TO THE NUMBER OF NEW PVERTICES!

    if (m_verbose) {
      std::size_t num_new_pvertices = 0;
      for (const auto& new_pvertex : new_pvertices) {
        if (new_pvertex != null_pvertex()) ++num_new_pvertices;
      }
      std::cout << "- number of new pvertices: " << num_new_pvertices << std::endl;
      std::cout << "- number of crossed iedges: " << crossed_iedges.size() << std::endl;
    }

    // CGAL_assertion_msg(false, "TODO: MERGE PVERTICES ON IVERTEX!");
    return new_pvertices;
  }

  void apply_closing_case(const PVertex& pvertex) const {

    if (m_verbose) {
      std::cout.precision(20);
      std::cout << "*** CLOSING CASE" << std::endl;
    }
    CGAL_assertion(has_complete_graph(pvertex));

    // CGAL_assertion_msg(false, "TODO: CLOSING CASE!");
  }

  void apply_back_border_case(
    const FT min_time, const FT max_time,
    const PVertex& pvertex, const IVertex& ivertex,
    const PVertex& back, const PVertex& prev,
    const std::vector< std::pair<IEdge, Direction_2> >& iedges,
    std::vector< std::pair<IEdge, bool> >& crossed_iedges,
    std::vector<PVertex>& new_pvertices) {

    if (m_verbose) {
      std::cout.precision(20);
      std::cout << "*** BACK BORDER CASE" << std::endl;
    }

    // We use this modification in order to avoid collinear directions.
    CGAL_assertion(has_iedge(pvertex));
    const std::size_t other_side_limit = line_idx(pvertex);
    const FT prev_time = last_event_time(prev);
    CGAL_assertion(prev_time < m_current_time);
    CGAL_assertion(prev_time >= FT(0));

    const auto pp_last = point_2(prev, prev_time);
    const auto pp_curr = point_2(prev, m_current_time);
    const auto dirp = Vector_2(pp_last, pp_curr);
    const auto shifted_prev = pp_curr - dirp / FT(10);

    if (m_verbose) {
      std::cout << "- shifting prev: " << to_3d(pvertex.first, shifted_prev) << std::endl;
    }

    const auto ipoint = point_2(pvertex.first, ivertex);
    const Direction_2 ref_direction_prev(shifted_prev - ipoint);

    // Find the first iedge.
    std::size_t first_idx = std::size_t(-1);
    const std::size_t n = iedges.size();
    for (std::size_t i = 0; i < n; ++i) {
      const std::size_t ip = (i + 1) % n;

      const auto& i_dir  = iedges[i].second;
      const auto& ip_dir = iedges[ip].second;
      if (ref_direction_prev.counterclockwise_in_between(ip_dir, i_dir)) {
        first_idx = ip; break;
      }
    }
    CGAL_assertion(first_idx != std::size_t(-1));
    // std::cout << "- curr: " << segment_3(iedges[first_idx].first) << std::endl;

    // Find all crossed iedges.
    crossed_iedges.clear();
    CGAL_assertion(crossed_iedges.size() == 0);
    std::size_t iedge_idx = first_idx;
    std::size_t iteration = 0;
    while (true) {
      const auto& iedge = iedges[iedge_idx].first;
      // std::cout << "- next: " << segment_3(iedge) << std::endl;

      const bool is_bbox_reached  = ( collision_occured(pvertex, iedge)   ).second;
      const bool is_limit_reached = ( line_idx(iedge) == other_side_limit );
      if (m_verbose) {
        std::cout << "- bbox: " << is_bbox_reached << "; limit: " << is_limit_reached << std::endl;
      }

      crossed_iedges.push_back(std::make_pair(iedge, false));
      if (is_bbox_reached || is_limit_reached) {
        break;
      }

      iedge_idx = (iedge_idx + 1) % n;
      if (iteration >= iedges.size()) {
        CGAL_assertion_msg(false, "ERROR: BACK, WHY SO MANY ITERATIONS?");
      } ++iteration;
    }

    CGAL_assertion(crossed_iedges.size() > 0);
    if (m_verbose) {
      std::cout << "- crossed " << crossed_iedges.size() << " iedges: " << std::endl;
      for (const auto& crossed_iedge : crossed_iedges) {
        std::cout << str(crossed_iedge.first) << ": " << segment_3(crossed_iedge.first) << std::endl;
      }
    }

    // Compute future points and directions.
    Point_2 future_point; Vector_2 future_direction;
    IEdge prev_iedge = null_iedge();
    const auto iedge_0 = crossed_iedges[0].first;
    CGAL_assertion_msg(
      point_2(pvertex.first, source(iedge_0)) !=
      point_2(pvertex.first, target(iedge_0)),
    "TODO: BACK, HANDLE ZERO-LENGTH IEDGE!");

    { // future point and direction
      const bool is_parallel = compute_future_point_and_direction(
        0, back, prev, iedge_0, future_point, future_direction);
      if (is_parallel) {
        if (is_intersecting_iedge(min_time, max_time, prev, iedge_0)) {
          prev_iedge = iedge_0;
        }
      }
    }

    // Crop the pvertex.
    new_pvertices.clear();
    new_pvertices.resize(crossed_iedges.size(), null_pvertex());

    { // crop
      PVertex cropped = null_pvertex();
      if (prev_iedge == iedge_0) {
        if (m_verbose) std::cout << "- back, prev, parallel case" << std::endl;

        // In case, we are parallel, we update the future point and direction.
        cropped = prev;
        const auto pprev = ( border_prev_and_next(prev) ).first;
        compute_future_point_and_direction(
          0, prev, pprev, prev_iedge, future_point, future_direction);

      } else {
        if (m_verbose) std::cout << "- back, prev, standard case" << std::endl;
        cropped = PVertex(pvertex.first, support_plane(pvertex).split_edge(pvertex.second, prev.second));
      }
      CGAL_assertion(cropped != null_pvertex());

      const PEdge pedge(pvertex.first, support_plane(pvertex).edge(pvertex.second, cropped.second));
      CGAL_assertion(cropped != pvertex);
      new_pvertices[0] = cropped;

      connect(pedge, iedge_0);
      connect(cropped, iedge_0);

      CGAL_assertion(future_direction != Vector_2());
      support_plane(cropped).set_point(cropped.second, future_point);
      direction(cropped) = future_direction;
      if (m_verbose) std::cout << "- cropped: " << point_3(cropped) << std::endl;
    }

    // Create new pfaces if any.
    add_new_pfaces(
      pvertex, ivertex, back, prev, false, true,
      crossed_iedges, new_pvertices);

    // CGAL_assertion_msg(false, "TODO: BACK BORDER CASE!");
  }

  void apply_front_border_case(
    const FT min_time, const FT max_time,
    const PVertex& pvertex, const IVertex& ivertex,
    const PVertex& front, const PVertex& next,
    const std::vector< std::pair<IEdge, Direction_2> >& iedges,
    std::vector< std::pair<IEdge, bool> >& crossed_iedges,
    std::vector<PVertex>& new_pvertices) {

    if (m_verbose) {
      std::cout.precision(20);
      std::cout << "*** FRONT BORDER CASE" << std::endl;
    }

    // We use this modification in order to avoid collinear directions.
    CGAL_assertion(has_iedge(pvertex));
    const std::size_t other_side_limit = line_idx(pvertex);
    const FT next_time = last_event_time(next);
    CGAL_assertion(next_time < m_current_time);
    CGAL_assertion(next_time >= FT(0));

    const auto pn_last = point_2(next, next_time);
    const auto pn_curr = point_2(next, m_current_time);
    const auto dirn = Vector_2(pn_last, pn_curr);
    const auto shifted_next = pn_curr - dirn / FT(10);

    if (m_verbose) {
      std::cout << "- shifting next: " << to_3d(pvertex.first, shifted_next) << std::endl;
    }

    const auto ipoint = point_2(pvertex.first, ivertex);
    const Direction_2 ref_direction_next(shifted_next - ipoint);

    // Find the first iedge.
    std::size_t first_idx = std::size_t(-1);
    const std::size_t n = iedges.size();
    for (std::size_t i = 0; i < n; ++i) {
      const std::size_t ip = (i + 1) % n;

      const auto& i_dir  = iedges[i].second;
      const auto& ip_dir = iedges[ip].second;
      if (ref_direction_next.counterclockwise_in_between(i_dir, ip_dir)) {
        first_idx = ip; break;
      }
    }
    CGAL_assertion(first_idx != std::size_t(-1));
    // std::cout << "- curr: " << segment_3(iedges[first_idx].first) << std::endl;

    // Find all crossed iedges.
    crossed_iedges.clear();
    CGAL_assertion(crossed_iedges.size() == 0);
    std::size_t iedge_idx = first_idx;
    std::size_t iteration = 0;
    while (true) {
      const auto& iedge = iedges[iedge_idx].first;
      // std::cout << "- next: " << segment_3(iedge) << std::endl;

      const bool is_bbox_reached  = ( collision_occured(pvertex, iedge)   ).second;
      const bool is_limit_reached = ( line_idx(iedge) == other_side_limit );
      if (m_verbose) {
        std::cout << "- bbox: " << is_bbox_reached << "; limit: " << is_limit_reached << std::endl;
      }

      crossed_iedges.push_back(std::make_pair(iedge, false));
      if (is_bbox_reached || is_limit_reached) {
        break;
      }

      iedge_idx = (iedge_idx + 1) % n;
      if (iteration >= iedges.size()) {
        CGAL_assertion_msg(false, "ERROR: FRONT, WHY SO MANY ITERATIONS?");
      } ++iteration;
    }

    CGAL_assertion(crossed_iedges.size() > 0);
    if (m_verbose) {
      std::cout << "- crossed " << crossed_iedges.size() << " iedges: " << std::endl;
      for (const auto& crossed_iedge : crossed_iedges) {
        std::cout << str(crossed_iedge.first) << ": " << segment_3(crossed_iedge.first) << std::endl;
      }
    }

    // Compute future points and directions.
    Point_2 future_point; Vector_2 future_direction;
    IEdge next_iedge = null_iedge();
    const auto iedge_0 = crossed_iedges[0].first;
    CGAL_assertion_msg(
      point_2(pvertex.first, source(iedge_0)) !=
      point_2(pvertex.first, target(iedge_0)),
    "TODO: FRONT, HANDLE ZERO-LENGTH IEDGE!");

    { // future point and direction
      const bool is_parallel = compute_future_point_and_direction(
        0, front, next, iedge_0, future_point, future_direction);
      if (is_parallel) {
        if (is_intersecting_iedge(min_time, max_time, next, iedge_0)) {
          next_iedge = iedge_0;
        }
      }
    }

    // Crop the pvertex.
    new_pvertices.clear();
    new_pvertices.resize(crossed_iedges.size(), null_pvertex());

    { // crop
      PVertex cropped = null_pvertex();
      if (next_iedge == iedge_0) {
        if (m_verbose) std::cout << "- front, next, parallel case" << std::endl;

        // In case, we are parallel, we update the future point and direction.
        cropped = next;
        const auto nnext = ( border_prev_and_next(next) ).second;
        compute_future_point_and_direction(
          0, next, nnext, next_iedge, future_point, future_direction);

      } else {
        if (m_verbose) std::cout << "- front, next, standard case" << std::endl;
        cropped = PVertex(pvertex.first, support_plane(pvertex).split_edge(pvertex.second, next.second));
      }
      CGAL_assertion(cropped != null_pvertex());

      const PEdge pedge(pvertex.first, support_plane(pvertex).edge(pvertex.second, cropped.second));
      CGAL_assertion(cropped != pvertex);
      new_pvertices[0] = cropped;

      connect(pedge, iedge_0);
      connect(cropped, iedge_0);

      CGAL_assertion(future_direction != Vector_2());
      support_plane(cropped).set_point(cropped.second, future_point);
      direction(cropped) = future_direction;
      if (m_verbose) std::cout << "- cropped: " << point_3(cropped) << std::endl;
    }

    // Create new pfaces if any.
    add_new_pfaces(
      pvertex, ivertex, front, next, false, false,
      crossed_iedges, new_pvertices);

    // CGAL_assertion_msg(false, "TODO: FRONT BORDER CASE!");
  }

  void apply_open_case(
    const FT min_time, const FT max_time,
    const PVertex& pvertex, const IVertex& ivertex,
    const PVertex& front, const PVertex& back,
    const PVertex& prev , const PVertex& next,
    const std::vector< std::pair<IEdge, Direction_2> >& iedges,
    std::vector< std::pair<IEdge, bool> >& crossed_iedges,
    std::vector<PVertex>& new_pvertices) {

    if (m_verbose) {
      std::cout.precision(20);
      std::cout << "*** OPEN CASE" << std::endl;
    }

    // We use this modification in order to avoid collinear directions.
    const FT prev_time = last_event_time(prev);
    const FT next_time = last_event_time(next);
    CGAL_assertion(prev_time < m_current_time);
    CGAL_assertion(next_time < m_current_time);
    CGAL_assertion(prev_time >= FT(0));
    CGAL_assertion(next_time >= FT(0));

    const auto pp_last = point_2(prev, prev_time);
    const auto pp_curr = point_2(prev, m_current_time);
    const auto dirp = Vector_2(pp_last, pp_curr);
    const auto shifted_prev = pp_curr - dirp / FT(10);

    const auto pn_last = point_2(next, next_time);
    const auto pn_curr = point_2(next, m_current_time);
    const auto dirn = Vector_2(pn_last, pn_curr);
    const auto shifted_next = pn_curr - dirn / FT(10);

    if (m_verbose) {
      std::cout << "- shifting prev: " << to_3d(pvertex.first, shifted_prev) << std::endl;
      std::cout << "- shifting next: " << to_3d(pvertex.first, shifted_next) << std::endl;
    }

    const auto ipoint = point_2(pvertex.first, ivertex);
    const Direction_2 ref_direction_prev(shifted_prev - ipoint);
    const Direction_2 ref_direction_next(shifted_next - ipoint);

    // Find the first iedge.
    std::size_t first_idx = std::size_t(-1);
    const std::size_t n = iedges.size();
    for (std::size_t i = 0; i < n; ++i) {
      const std::size_t ip = (i + 1) % n;

      const auto& i_dir  = iedges[i].second;
      const auto& ip_dir = iedges[ip].second;
      if (ref_direction_next.counterclockwise_in_between(i_dir, ip_dir)) {
        first_idx = ip; break;
      }
    }
    CGAL_assertion(first_idx != std::size_t(-1));
    // std::cout << "- curr: " << segment_3(iedges[first_idx].first) << std::endl;

    // Find all crossed iedges.
    crossed_iedges.clear();
    CGAL_assertion(crossed_iedges.size() == 0);
    std::size_t iedge_idx = first_idx;
    std::size_t iteration = 0;
    while (true) {
      const auto& iedge = iedges[iedge_idx].first;
      // std::cout << "- next: " << segment_3(iedge) << std::endl;

      if (iteration == iedges.size()) {
        CGAL_assertion_msg(iedges.size() == 2,
        "ERROR: CAN WE HAVE THIS CASE IN THE CONSTRAINED SETTING?");
        break;
      }

      const auto& ref_direction = iedges[iedge_idx].second;
      if (!ref_direction.counterclockwise_in_between(
        ref_direction_next, ref_direction_prev)) {
        break;
      }

      crossed_iedges.push_back(std::make_pair(iedge, false));
      iedge_idx = (iedge_idx + 1) % n;
      if (iteration >= iedges.size()) {
        CGAL_assertion_msg(false, "ERROR: OPEN, WHY SO MANY ITERATIONS?");
      } ++iteration;
    }

    CGAL_assertion(crossed_iedges.size() > 0);
    if (m_verbose) {
      std::cout << "- crossed " << crossed_iedges.size() << " iedges: " << std::endl;
      for (const auto& crossed_iedge : crossed_iedges) {
        std::cout << str(crossed_iedge.first) << ": " << segment_3(crossed_iedge.first) << std::endl;
      }
    }

    // Compute future points and directions.
    std::vector<Point_2> future_points(2);
    std::vector<Vector_2> future_directions(2);
    IEdge prev_iedge = null_iedge(), next_iedge = null_iedge();

    CGAL_assertion_msg(
      point_2(pvertex.first, source(crossed_iedges.front().first)) !=
      point_2(pvertex.first, target(crossed_iedges.front().first)),
    "TODO: OPEN, FRONT, HANDLE ZERO-LENGTH IEDGE!");

    { // first future point and direction
      const bool is_parallel = compute_future_point_and_direction(
        pvertex, prev, next, crossed_iedges.front().first, future_points.front(), future_directions.front());
      if (is_parallel) {
        if (is_intersecting_iedge(min_time, max_time, prev, crossed_iedges.front().first)) {
          prev_iedge = crossed_iedges.front().first;
        }
        if (is_intersecting_iedge(min_time, max_time, next, crossed_iedges.front().first)) {
          next_iedge = crossed_iedges.front().first;
        }
      }
    }

    CGAL_assertion_msg(
      point_2(pvertex.first, source(crossed_iedges.back().first)) !=
      point_2(pvertex.first, target(crossed_iedges.back().first)),
    "TODO: OPEN, BACK, HANDLE ZERO-LENGTH IEDGE!");

    { // second future point and direction
      const bool is_parallel = compute_future_point_and_direction(
        pvertex, prev, next, crossed_iedges.back().first, future_points.back(), future_directions.back());
      if (is_parallel) {
        if (is_intersecting_iedge(min_time, max_time, prev, crossed_iedges.back().first)) {
          prev_iedge = crossed_iedges.back().first;
        }
        if (is_intersecting_iedge(min_time, max_time, next, crossed_iedges.back().first)) {
          next_iedge = crossed_iedges.back().first;
        }
      }
    }

    // Crop the pvertex.
    new_pvertices.clear();
    new_pvertices.resize(crossed_iedges.size(), null_pvertex());

    { // first crop
      PVertex cropped = null_pvertex();
      if (next_iedge == crossed_iedges.front().first) {
        if (m_verbose) std::cout << "- open, next, parallel case" << std::endl;

        // In case, we are parallel, we update the future point and direction.
        cropped = next;
        const auto nnext = ( border_prev_and_next(next) ).second;
        compute_future_point_and_direction(
          0, next, nnext, next_iedge, future_points.front(), future_directions.front());

      } else {
        if (m_verbose) std::cout << "- open, next, standard case" << std::endl;
        cropped = PVertex(pvertex.first, support_plane(pvertex).split_edge(pvertex.second, next.second));
      }
      CGAL_assertion(cropped != null_pvertex());

      const PEdge pedge(pvertex.first, support_plane(pvertex).edge(pvertex.second, cropped.second));
      CGAL_assertion(cropped != pvertex);
      new_pvertices.front() = cropped;

      connect(pedge, crossed_iedges.front().first);
      connect(cropped, crossed_iedges.front().first);

      CGAL_assertion(future_directions.front() != Vector_2());
      support_plane(cropped).set_point(cropped.second, future_points.front());
      direction(cropped) = future_directions.front();
      if (m_verbose) std::cout << "- cropped 1: " << point_3(cropped) << std::endl;
    }

    { // second crop
      PVertex cropped = null_pvertex();
      if (prev_iedge == crossed_iedges.back().first) {
        if (m_verbose) std::cout << "- open, prev, parallel case" << std::endl;

        // In case, we are parallel, we update the future point and direction.
        cropped = prev;
        const auto pprev = ( border_prev_and_next(prev) ).first;
        compute_future_point_and_direction(
          0, prev, pprev, prev_iedge, future_points.back(), future_directions.back());

      } else {
        if (m_verbose) std::cout << "- open, prev, standard case" << std::endl;
        cropped = PVertex(pvertex.first, support_plane(pvertex).split_edge(pvertex.second, prev.second));
      }
      CGAL_assertion(cropped != null_pvertex());

      const PEdge pedge(pvertex.first, support_plane(pvertex).edge(pvertex.second, cropped.second));
      CGAL_assertion(cropped != pvertex);
      new_pvertices.back() = cropped;

      connect(pedge, crossed_iedges.back().first);
      connect(cropped, crossed_iedges.back().first);

      CGAL_assertion(future_directions.back() != Vector_2());
      support_plane(cropped).set_point(cropped.second, future_points.back());
      direction(cropped) = future_directions.back();
      if (m_verbose) std::cout << "- cropped 2: " << point_3(cropped) << std::endl;
    }

    // Create new pfaces if any.
    add_new_pfaces(
      pvertex, ivertex, prev, next, true, false,
      crossed_iedges, new_pvertices);

    // CGAL_assertion_msg(false, "TODO: OPEN CASE!");
  }

  void add_new_pfaces(
    const PVertex& pvertex, const IVertex& ivertex,
    const PVertex& pv_prev, const PVertex& pv_next,
    const bool is_open, const bool reverse,
    std::vector< std::pair<IEdge, bool> >& crossed_iedges,
    std::vector<PVertex>& new_pvertices) {

    if (crossed_iedges.size() < 2) return;
    CGAL_assertion(crossed_iedges.size() >= 2);
    CGAL_assertion(crossed_iedges.size() == new_pvertices.size());
    CGAL_assertion(crossed_iedges.front().first != crossed_iedges.back().first);

    add_new_pfaces_global(
      pvertex, ivertex, pv_prev, pv_next, is_open, reverse,
      crossed_iedges, new_pvertices);

    // CGAL_assertion_msg(false, "TODO: ADD NEW PFACES!");
  }

  void add_new_pfaces_global(
    const PVertex& pvertex, const IVertex& ivertex,
    const PVertex& pv_prev, const PVertex& pv_next,
    const bool is_open, bool reverse,
    std::vector< std::pair<IEdge, bool> >& crossed_iedges,
    std::vector<PVertex>& new_pvertices) {

    traverse_iedges_global(
      pvertex, ivertex, pv_prev, pv_next, is_open, reverse,
      crossed_iedges, new_pvertices);

    if (is_open) {
      reverse = !reverse;
      std::reverse(new_pvertices.begin(), new_pvertices.end());
      std::reverse(crossed_iedges.begin(), crossed_iedges.end());

      traverse_iedges_global(
        pvertex, ivertex, pv_prev, pv_next, is_open, reverse,
        crossed_iedges, new_pvertices);

      reverse = !reverse;
      std::reverse(new_pvertices.begin(), new_pvertices.end());
      std::reverse(crossed_iedges.begin(), crossed_iedges.end());
    }

    // CGAL_assertion_msg(false, "TODO: ADD NEW PFACES GLOBAL!");
  }

  void traverse_iedges_global(
    const PVertex& pvertex, const IVertex& ivertex,
    const PVertex& pv_prev, const PVertex& pv_next,
    const bool is_open, const bool reverse,
    std::vector< std::pair<IEdge, bool> >& iedges,
    std::vector<PVertex>& pvertices) {

    if (m_verbose) {
      std::cout << "**** traversing iedges global" << std::endl;
      std::cout << "- k intersections before: " << this->k(pvertex.first) << std::endl;
    }

    std::size_t num_added_pfaces = 0;
    CGAL_assertion(iedges.size() >= 2);
    CGAL_assertion(iedges.size() == pvertices.size());
    CGAL_assertion(pvertices.front() != null_pvertex());
    for (std::size_t i = 0; i < iedges.size() - 1; ++i) {

      if (iedges[i].second) {
        if (m_verbose) {
          std::cout << "- break iedge " << std::to_string(i) << std::endl;
        } break;
      } else {
        if (m_verbose) {
          std::cout << "- handle iedge " << std::to_string(i) << std::endl;
        }
      }

      iedges[i].second = true;
      const auto& iedge_i = iedges[i].first;
      CGAL_assertion_msg(
        point_2(pvertex.first, ivertex) != point_2(pvertex.first, opposite(iedge_i, ivertex)),
      "TODO: TRAVERSE IEDGES GLOBAL, HANDLE ZERO LENGTH IEDGE I!");

      bool is_occupied_iedge, is_bbox_reached;
      std::tie(is_occupied_iedge, is_bbox_reached) = this->is_occupied(pvertex, ivertex, iedge_i);
      const bool is_limit_line = update_limit_lines_and_k(pvertex, iedge_i, is_occupied_iedge);

      if (m_verbose) {
        std::cout << "- bbox: " << is_bbox_reached  << "; " <<
        " limit: "    << is_limit_line << "; " <<
        " occupied: " << is_occupied_iedge << std::endl;
      }

      if (is_bbox_reached) {
        if (m_verbose) std::cout << "- bbox, stop" << std::endl;
        break;
      } else if (is_limit_line) {
        if (m_verbose) std::cout << "- limit, stop" << std::endl;
        break;
      } else {
        if (m_verbose) std::cout << "- free, any k, continue" << std::endl;
        CGAL_assertion(this->k(pvertex.first) >= 1);

        const std::size_t ip = i + 1;
        const auto& iedge_ip = iedges[ip].first;
        CGAL_assertion_msg(
          point_2(pvertex.first, ivertex) != point_2(pvertex.first, opposite(iedge_ip, ivertex)),
        "TODO: TRAVERSE IEDGES GLOBAL, HANDLE ZERO LENGTH IEDGE IP!");

        add_new_pface(pvertex, pv_prev, pv_next, is_open, reverse, i, iedge_ip, pvertices);
        ++num_added_pfaces;
        continue;
      }
    }

    CGAL_assertion(this->k(pvertex.first) >= 1);
    if (num_added_pfaces == iedges.size() - 1) {
      iedges.back().second = true;
    }

    if (m_verbose) {
      std::cout << "- num added pfaces: " << num_added_pfaces << std::endl;
      std::cout << "- k intersections after: " << this->k(pvertex.first) << std::endl;
    }

    // CGAL_assertion_msg(false, "TODO: TRAVERSE IEDGES GLOBAL!");
  }

  void add_new_pface(
    const PVertex& pvertex, const PVertex& pv_prev, const PVertex& pv_next,
    const bool is_open, const bool reverse, const std::size_t idx, const IEdge& iedge,
    std::vector<PVertex>& pvertices) {

    if (m_verbose) {
      std::cout << "- adding new pface: " << std::endl;
    }

    // The first pvertex of the new triangle.
    const auto& pv1 = pvertices[idx];
    CGAL_assertion(pv1 != null_pvertex());
    if (m_verbose) {
      std::cout << "- pv1 " << str(pv1) << ": " << point_3(pv1) << std::endl;
    }

    // The second pvertex of the new triangle.
    PVertex pv2 = null_pvertex();
    const bool pv2_exists = (pvertices[idx + 1] != null_pvertex());
    if (pv2_exists) {
      CGAL_assertion((pvertices.size() - 1) == (idx + 1));
      pv2 = pvertices[idx + 1];
    } else {
      create_new_pvertex(
        pvertex, pv_prev, pv_next, is_open, idx + 1, iedge, pvertices);
      pv2 = pvertices[idx + 1];
    }
    CGAL_assertion(pv2 != null_pvertex());
    if (m_verbose) {
      std::cout << "- pv2 " << str(pv2) << ": " << point_3(pv2) << std::endl;
    }

    // Adding new triangle.
    if (reverse) add_pface(std::array<PVertex, 3>{pvertex, pv2, pv1});
    else add_pface(std::array<PVertex, 3>{pvertex, pv1, pv2});
    if (!pv2_exists) connect_pedge(pvertex, pv2, iedge);

    // CGAL_assertion_msg(false, "TODO: ADD NEW PFACE!");
  }

  void create_new_pvertex(
    const PVertex& pvertex, const PVertex& pv_prev, const PVertex& pv_next,
    const bool is_open, const std::size_t idx, const IEdge& iedge,
    std::vector<PVertex>& pvertices) {

    if (m_verbose) std::cout << "- creating new pvertex" << std::endl;

    bool is_parallel = false;
    Point_2 future_point; Vector_2 future_direction;

    if (!is_open) {
      is_parallel = compute_future_point_and_direction(
        0, pv_prev, pv_next, iedge, future_point, future_direction);
      if (is_parallel) {
        if (m_verbose) std::cout << "- new pvertex, back/front, parallel case" << std::endl;
        CGAL_assertion_msg(!is_parallel,
        "TODO: CREATE PVERTEX, BACK/FRONT, ADD PARALLEL CASE!");
      }
    } else {
      is_parallel = compute_future_point_and_direction(
        pvertex, pv_prev, pv_next, iedge, future_point, future_direction);
      if (is_parallel) {
        if (m_verbose) std::cout << "- new pvertex, open, parallel case" << std::endl;
        CGAL_assertion_msg(!is_parallel,
        "TODO: CREATE_PVERTEX, OPEN, ADD PARALLEL CASE!");
      }
    }

    CGAL_assertion(future_direction != Vector_2());
    const auto propagated = add_pvertex(pvertex.first, future_point);
    direction(propagated) = future_direction;
    CGAL_assertion(propagated != pvertex);

    CGAL_assertion(idx < pvertices.size());
    CGAL_assertion(pvertices[idx] == null_pvertex());
    pvertices[idx] = propagated;

    // CGAL_assertion_msg(false, "TODO: CREATE NEW PVERTEX!");
  }

  void connect_pedge(
    const PVertex& pvertex, const PVertex& pother, const IEdge& iedge) {

    const PEdge pedge(pvertex.first,
      support_plane(pvertex).edge(pvertex.second, pother.second));
    connect(pedge, iedge);
    connect(pother, iedge);
  }

  /*******************************
  **    CHECKING PROPERTIES     **
  ********************************/

  const bool check_bbox() const {

    for (std::size_t i = 0; i < 6; ++i) {
      const auto pfaces = this->pfaces(i);
      for (const auto pface : pfaces) {
        for (const auto pedge : pedges_of_pface(pface)) {
          if (!has_iedge(pedge)) {
            std::cout << "debug pedge: " << segment_3(pedge) << std::endl;
            CGAL_assertion_msg(has_iedge(pedge), "ERROR: BBOX EDGE IS MISSING AN IEDGE!");
            return false;
          }
        }
        for (const auto pvertex : pvertices_of_pface(pface)) {
          if (!has_ivertex(pvertex)) {
            std::cout << "debug pvertex: " << point_3(pvertex) << std::endl;
            CGAL_assertion_msg(has_ivertex(pvertex), "ERROR: BBOX VERTEX IS MISSING AN IVERTEX!");
            return false;
          }
        }
      }
    }
    return true;
  }

  const bool check_interior() const {

    for (std::size_t i = 6; i < number_of_support_planes(); ++i) {
      const auto pfaces = this->pfaces(i);
      for (const auto pface : pfaces) {
        for (const auto pedge : pedges_of_pface(pface)) {
          if (!has_iedge(pedge)) {
            std::cout << "debug pedge: " << segment_3(pedge) << std::endl;
            CGAL_assertion_msg(has_iedge(pedge), "ERROR: INTERIOR EDGE IS MISSING AN IEDGE!");
            return false;
          }
        }
        for (const auto pvertex : pvertices_of_pface(pface)) {
          if (!has_ivertex(pvertex)) {
            std::cout << "debug pvertex: " << point_3(pvertex) << std::endl;
            CGAL_assertion_msg(has_ivertex(pvertex), "ERROR: INTERIOR VERTEX IS MISSING AN IVERTEX!");
            return false;
          }
        }
      }
    }
    return true;
  }

  const bool check_vertices() const {

    for (const auto vertex : m_intersection_graph.vertices()) {
      const auto nedges = m_intersection_graph.incident_edges(vertex);
      if (nedges.size() <= 2) {
        std::cout << "ERROR: CURRENT NUMBER OF EDGES = " << nedges.size() << std::endl;
        CGAL_assertion_msg(nedges.size() > 2,
        "ERROR: VERTEX MUST HAVE AT LEAST 3 NEIGHBORS!");
        return false;
      }
    }
    return true;
  }

  const bool check_edges() const {

    std::vector<PFace> nfaces;
    for (const auto edge : m_intersection_graph.edges()) {
      incident_faces(edge, nfaces);
      if (nfaces.size() == 1) {
        std::cout << "ERROR: CURRENT NUMBER OF FACES = " << nfaces.size() << std::endl;
        CGAL_assertion_msg(nfaces.size() != 1,
        "ERROR: EDGE MUST HAVE 0 OR AT LEAST 2 NEIGHBORS!");
        return false;
      }
    }
    return true;
  }

  const bool check_faces() const {

    for (std::size_t i = 0; i < number_of_support_planes(); ++i) {
      const auto pfaces = this->pfaces(i);
      for (const auto pface : pfaces) {
        const auto nvolumes = incident_volumes(pface);
        if (nvolumes.size() == 0 || nvolumes.size() > 2) {
          std::cout << "ERROR: CURRENT NUMBER OF VOLUMES = " << nvolumes.size() << std::endl;
          CGAL_assertion_msg(nvolumes.size() == 1 || nvolumes.size() == 2,
          "ERROR: FACE MUST HAVE 1 OR 2 NEIGHBORS!");
          return false;
        }
      }
    }
    return true;
  }

  const bool is_mesh_valid(
    const bool check_simplicity,
    const bool check_convexity,
    const std::size_t support_plane_idx) const {

    const bool is_valid = mesh(support_plane_idx).is_valid();
    if (!is_valid) {
      return false;
    }

    // Note: bbox faces may have multiple equal points after converting from exact to inexact!
    if (support_plane_idx < 6) {
      return true;
    }

    const auto pfaces = this->pfaces(support_plane_idx);
    for (const auto pface : pfaces) {
      std::function<Point_2(PVertex)> unary_f =
      [&](const PVertex& pvertex) -> Point_2 {
        return point_2(pvertex);
      };

      const auto pvertices = pvertices_of_pface(pface);
      const Polygon_2 polygon(
        boost::make_transform_iterator(pvertices.begin(), unary_f),
        boost::make_transform_iterator(pvertices.end(), unary_f));

      // Use only with an exact kernel!
      if (check_simplicity && !polygon.is_simple()) {
        dump_polygon(*this, support_plane_idx, polygon, "non-simple-polygon");
        const std::string msg = "ERROR: PFACE " + str(pface) + " IS NOT SIMPLE!";
        CGAL_assertion_msg(false, msg.c_str());
        return false;
      }

      // Use only with an exact kernel!
      if (check_convexity && !polygon.is_convex()) {
        dump_polygon(*this, support_plane_idx, polygon, "non-convex-polygon");
        const std::string msg = "ERROR: PFACE " + str(pface) + " IS NOT CONVEX!";
        CGAL_assertion_msg(false, msg.c_str());
        return false;
      }

      auto prev = null_pvertex();
      for (const auto pvertex : pvertices) {
        if (prev == null_pvertex()) {
          prev = pvertex;
          continue;
        }

        if (point_2(prev) == point_2(pvertex) &&
          direction(prev) == direction(pvertex)) {

          const std::string msg = "ERROR: PFACE " + str(pface) +
          " HAS TWO CONSEQUENT IDENTICAL VERTICES "
          + str(prev) + " AND " + str(pvertex) + "!";
          CGAL_assertion_msg(false, msg.c_str());
          return false;
        }
        prev = pvertex;
      }
    }
    return true;
  }

  const bool check_integrity(
    const bool is_initialized   = true,
    const bool check_simplicity = false,
    const bool check_convexity  = false) const {

    for (std::size_t i = 0; i < number_of_support_planes(); ++i) {
      if (!is_mesh_valid(check_simplicity, check_convexity, i)) {
        const std::string msg = "ERROR: MESH " + std::to_string(i) + " IS NOT VALID!";
        CGAL_assertion_msg(false, msg.c_str());
        return false;
      }

      if (is_initialized) {
        const auto& iedges = this->iedges(i);
        CGAL_assertion(iedges.size() > 0);
        for (const auto& iedge : iedges) {
          const auto& iplanes = this->intersected_planes(iedge);
          if (iplanes.find(i) == iplanes.end()) {

            const std::string msg = "ERROR: SUPPORT PLANE " + std::to_string(i) +
            " IS INTERSECTED BY " + str(iedge) +
            " BUT IT CLAIMS IT DOES NOT INTERSECT IT!";
            CGAL_assertion_msg(false, msg.c_str());
            return false;
          }
        }
      } else {
        const auto& iedges = support_plane(i).unique_iedges();
        CGAL_assertion(iedges.size() > 0);
        for (const auto& iedge : iedges) {
          const auto& iplanes = this->intersected_planes(iedge);
          if (iplanes.find(i) == iplanes.end()) {

            const std::string msg = "ERROR: SUPPORT PLANE " + std::to_string(i) +
            " IS INTERSECTED BY " + str(iedge) +
            " BUT IT CLAIMS IT DOES NOT INTERSECT IT!";
            CGAL_assertion_msg(false, msg.c_str());
            return false;
          }
        }
      }
    }

    for (const auto iedge : this->iedges()) {
      const auto& iplanes = this->intersected_planes(iedge);
      for (const auto support_plane_idx : iplanes) {

        if (is_initialized) {
          const auto& sp_iedges = this->iedges(support_plane_idx);
          CGAL_assertion(sp_iedges.size() > 0);
          if (std::find(sp_iedges.begin(), sp_iedges.end(), iedge) == sp_iedges.end()) {

            const std::string msg = "ERROR: IEDGE " + str(iedge) +
            " INTERSECTS SUPPORT PLANE " + std::to_string(support_plane_idx) +
            " BUT IT CLAIMS IT IS NOT INTERSECTED BY IT!";
            CGAL_assertion_msg(false, msg.c_str());
            return false;
          }
        } else {
          const auto& sp_iedges = support_plane(support_plane_idx).unique_iedges();
          CGAL_assertion(sp_iedges.size() > 0);
          if (sp_iedges.find(iedge) == sp_iedges.end()) {

            const std::string msg = "ERROR: IEDGE " + str(iedge) +
            " INTERSECTS SUPPORT PLANE " + std::to_string(support_plane_idx) +
            " BUT IT CLAIMS IT IS NOT INTERSECTED BY IT!";
            CGAL_assertion_msg(false, msg.c_str());
            return false;
          }
        }
      }
    }
    return true;
  }

  const bool check_volume(
    const int volume_index,
    const std::size_t volume_size,
    const std::map<PFace, std::pair<int, int> >& map_volumes) const {

    std::vector<PFace> pfaces;
    for (const auto& item : map_volumes) {
      const auto& pface = item.first;
      const auto& pair  = item.second;
      if (pair.first == volume_index || pair.second == volume_index) {
        pfaces.push_back(pface);
      }
    }

    const bool is_broken_volume = is_volume_degenerate(pfaces);
    if (is_broken_volume) {
      dump_volume(*this, pfaces, "volumes/degenerate");
    }
    CGAL_assertion(!is_broken_volume);
    if (is_broken_volume) return false;
    CGAL_assertion(pfaces.size() == volume_size);
    if (pfaces.size() != volume_size) return false;
    return true;
  }

  const bool is_volume_degenerate(
    const std::vector<PFace>& pfaces) const {

    for (const auto& pface : pfaces) {
      const auto pedges = pedges_of_pface(pface);
      const std::size_t n = pedges.size();

      std::size_t count = 0;
      for (const auto pedge : pedges) {
        CGAL_assertion(has_iedge(pedge));
        const auto iedge = this->iedge(pedge);
        const std::size_t num_found = find_adjacent_pfaces(pface, iedge, pfaces);
        if (num_found == 1) ++count;
      }
      if (count != n) {
        std::cout << "- current number of neighbors " << count << " != " << n << std::endl;
        dump_info(*this, pface, *pedges.begin(), pfaces);
        return true;
      }
    }
    return false;
  }

  const std::size_t find_adjacent_pfaces(
    const PFace& current,
    const IEdge& query,
    const std::vector<PFace>& pfaces) const {

    std::size_t num_found = 0;
    for (const auto& pface : pfaces) {
      if (pface == current) continue;
      const auto pedges = pedges_of_pface(pface);
      for (const auto pedge : pedges) {
        CGAL_assertion(has_iedge(pedge));
        const auto iedge = this->iedge(pedge);
        if (iedge == query) ++num_found;
      }
    }
    return num_found;
  }

private:

  /*************************************
  **   FUTURE POINTS AND DIRECTIONS   **
  *************************************/

  const std::pair<bool, bool> compute_future_points_and_directions(
    const PVertex& pvertex, const IEdge& iedge,
    Point_2& future_point_a, Point_2& future_point_b,
    Vector_2& future_direction_a, Vector_2& future_direction_b) const {

    bool is_parallel_prev = false;
    bool is_parallel_next = false;

    const auto source_p = point_2(pvertex.first, source(iedge));
    const auto target_p = point_2(pvertex.first, target(iedge));
    CGAL_assertion_msg(source_p != target_p,
    "TODO: COMPUTE FUTURE POINTS AND DIRECTIONS, HANDLE ZERO-LENGTH IEDGE!");

    const Vector_2 iedge_vec(source_p, target_p);
    const Line_2  iedge_line(source_p, target_p);
    // std::cout << "iedge segment: " << segment_3(iedge) << std::endl;

    const auto& curr = pvertex;
    const auto curr_p = point_2(curr);
    const Point_2 pinit = iedge_line.projection(curr_p);

    const PVertex prev(curr.first, support_plane(curr).prev(curr.second));
    const PVertex next(curr.first, support_plane(curr).next(curr.second));

    const auto prev_p = point_2(prev);
    const auto next_p = point_2(next);

    // std::cout << "prev: " << point_3(prev) << std::endl;
    // std::cout << "next: " << point_3(next) << std::endl;
    // std::cout << "curr: " << point_3(curr) << std::endl;

    const Line_2 future_line_prev(
      point_2(prev, m_current_time + FT(1)),
      point_2(curr, m_current_time + FT(1)));
    const Line_2 future_line_next(
      point_2(next, m_current_time + FT(1)),
      point_2(curr, m_current_time + FT(1)));

    const Vector_2 current_vec_prev(prev_p, curr_p);
    const Vector_2 current_vec_next(next_p, curr_p);

    // TODO: CAN WE AVOID THIS VALUE?
    const FT tol = KSR::tolerance<FT>();
    FT m1 = FT(100000), m2 = FT(100000), m3 = FT(100000);
    // std::cout << "tol: " << tol << std::endl;

    const FT prev_d = (curr_p.x() - prev_p.x());
    const FT next_d = (curr_p.x() - next_p.x());
    const FT edge_d = (target_p.x() - source_p.x());

    if (CGAL::abs(prev_d) > tol)
      m1 = (curr_p.y() - prev_p.y()) / prev_d;
    if (CGAL::abs(next_d) > tol)
      m2 = (curr_p.y() - next_p.y()) / next_d;
    if (CGAL::abs(edge_d) > tol)
      m3 = (target_p.y() - source_p.y()) / edge_d;

    // std::cout << "m1: " << m1 << std::endl;
    // std::cout << "m2: " << m2 << std::endl;
    // std::cout << "m3: " << m3 << std::endl;

    // std::cout << "m1 - m3 a: " << CGAL::abs(m1 - m3) << std::endl;
    // std::cout << "m2 - m3 b: " << CGAL::abs(m2 - m3) << std::endl;

    if (CGAL::abs(m1 - m3) < tol) {
      if (m_verbose) std::cout << "- prev parallel lines" << std::endl;

      is_parallel_prev = true;
      const FT prev_dot = current_vec_prev * iedge_vec;
      if (prev_dot < FT(0)) {
        if (m_verbose) std::cout << "- prev moves backwards" << std::endl;
        future_point_a = target_p;
        // std::cout << point_3(target(iedge)) << std::endl;
      } else {
        if (m_verbose) std::cout << "- prev moves forwards" << std::endl;
        future_point_a = source_p;
        // std::cout << point_3(source(iedge)) << std::endl;
      }
    } else {
      if (m_verbose) std::cout << "- prev intersected lines" << std::endl;

      const bool is_a_found = KSR::intersection(future_line_prev, iedge_line, future_point_a);
      if (!is_a_found) {
        std::cout << "WARNING: A IS NOT FOUND!" << std::endl;
        future_point_b = pinit + (pinit - future_point_a);
      }
    }

    CGAL_assertion(pinit != future_point_a);
    future_direction_a = Vector_2(pinit, future_point_a);
    CGAL_assertion(future_direction_a != Vector_2());
    future_point_a = pinit - m_current_time * future_direction_a;

    if (m_verbose) {
      auto tmp_a = future_direction_a;
      tmp_a = KSR::normalize(tmp_a);
      std::cout << "- prev future point a: " <<
      to_3d(curr.first, pinit + m_current_time * tmp_a) << std::endl;
      std::cout << "- prev future direction a: " << future_direction_a << std::endl;
    }

    if (CGAL::abs(m2 - m3) < tol) {
      if (m_verbose) std::cout << "- next parallel lines" << std::endl;

      is_parallel_next = true;
      const FT next_dot = current_vec_next * iedge_vec;
      if (next_dot < FT(0)) {
        if (m_verbose) std::cout << "- next moves backwards" << std::endl;
        future_point_b = target_p;
        // std::cout << point_3(target(iedge)) << std::endl;
      } else {
        if (m_verbose) std::cout << "- next moves forwards" << std::endl;
        future_point_b = source_p;
        // std::cout << point_3(source(iedge)) << std::endl;
      }
    } else {
      if (m_verbose) std::cout << "- next intersected lines" << std::endl;

      const bool is_b_found = KSR::intersection(future_line_next, iedge_line, future_point_b);
      if (!is_b_found) {
        std::cout << "WARNING: B IS NOT FOUND!" << std::endl;
        future_point_a = pinit + (pinit - future_point_b);
      }
    }

    CGAL_assertion(pinit != future_point_b);
    future_direction_b = Vector_2(pinit, future_point_b);
    CGAL_assertion(future_direction_b != Vector_2());
    future_point_b = pinit - m_current_time * future_direction_b;

    if (m_verbose) {
      auto tmp_b = future_direction_b;
      tmp_b = KSR::normalize(tmp_b);
      std::cout << "- next future point b: " <<
      to_3d(curr.first, pinit + m_current_time * tmp_b) << std::endl;
      std::cout << "- next future direction b: " << future_direction_b << std::endl;
    }
    return std::make_pair(is_parallel_prev, is_parallel_next);
  }

  const bool compute_future_point_and_direction(
    const std::size_t idx, const PVertex& pvertex, const PVertex& pother, // back prev // front next
    const IEdge& iedge, Point_2& future_point, Vector_2& future_direction) const {

    bool is_parallel = false;
    const auto source_p = point_2(pvertex.first, source(iedge));
    const auto target_p = point_2(pvertex.first, target(iedge));
    CGAL_assertion_msg(source_p != target_p,
    "TODO: COMPUTE FUTURE POINT AND DIRECTION 1, HANDLE ZERO-LENGTH IEDGE!");

    const Vector_2 iedge_vec(source_p, target_p);
    const Line_2  iedge_line(source_p, target_p);
    // std::cout << "iedge segment: " << segment_3(iedge) << std::endl;

    const auto& next = pother;
    const auto& curr = pvertex;

    // std::cout << "next: " << point_3(next) << std::endl;
    // std::cout << "curr: " << point_3(curr) << std::endl;

    const auto next_p = point_2(next);
    const auto curr_p = point_2(curr);

    const Point_2 pinit = iedge_line.projection(curr_p);

    const Line_2 future_line_next(
      point_2(next, m_current_time + FT(1)),
      point_2(curr, m_current_time + FT(1)));
    const Vector_2 current_vec_next(next_p, curr_p);

    // TODO: CAN WE AVOID THIS VALUE?
    const FT tol = KSR::tolerance<FT>();
    FT m2 = FT(100000), m3 = FT(100000);
    // std::cout << "tol: " << tol << std::endl;

    const FT next_d = (curr_p.x() - next_p.x());
    const FT edge_d = (target_p.x() - source_p.x());

    if (CGAL::abs(next_d) > tol)
      m2 = (curr_p.y() - next_p.y()) / next_d;
    if (CGAL::abs(edge_d) > tol)
      m3 = (target_p.y() - source_p.y()) / edge_d;

    // std::cout << "m2: " << m2 << std::endl;
    // std::cout << "m3: " << m3 << std::endl;

    // std::cout << "m2 - m3: " << CGAL::abs(m2 - m3) << std::endl;

    if (CGAL::abs(m2 - m3) < tol) {
      if (m_verbose) std::cout << "- back/front parallel lines" << std::endl;

      is_parallel = true;
      const FT next_dot = current_vec_next * iedge_vec;
      if (next_dot < FT(0)) {
        if (m_verbose) std::cout << "- back/front moves backwards" << std::endl;
        future_point = target_p;
        // std::cout << point_3(target(iedge)) << std::endl;
      } else {
        if (m_verbose) std::cout << "- back/front moves forwards" << std::endl;
        future_point = source_p;
        // std::cout << point_3(source(iedge)) << std::endl;
      }

    } else {
      if (m_verbose) std::cout << "- back/front intersected lines" << std::endl;
      future_point = KSR::intersection<Point_2>(future_line_next, iedge_line);
    }

    CGAL_assertion(pinit != future_point);
    future_direction = Vector_2(pinit, future_point);
    CGAL_assertion(future_direction != Vector_2());
    future_point = pinit - m_current_time * future_direction;

    if (m_verbose) {
      auto tmp = future_direction;
      tmp = KSR::normalize(tmp);
      std::cout << "- back/front future point: " <<
      to_3d(curr.first, pinit + m_current_time * tmp) << std::endl;
      std::cout << "- back/front future direction: " << future_direction << std::endl;
    }
    return is_parallel;
  }

  const bool compute_future_point_and_direction(
    const PVertex& pvertex, const PVertex& prev, const PVertex& next, // prev next
    const IEdge& iedge, Point_2& future_point, Vector_2& future_direction) const {

    bool is_parallel = false;
    const auto source_p = point_2(pvertex.first, source(iedge));
    const auto target_p = point_2(pvertex.first, target(iedge));
    CGAL_assertion_msg(source_p != target_p,
    "TODO: COMPUTE FUTURE POINT AND DIRECTION 2, HANDLE ZERO-LENGTH IEDGE!");

    const Line_2 iedge_line(source_p, target_p);
    // std::cout << "iedge segment: " << segment_3(iedge) << std::endl;

    const auto pv_point = point_2(pvertex);
    const Point_2 pinit = iedge_line.projection(pv_point);

    const auto& curr = prev;
    // std::cout << "next: " << point_3(next) << std::endl;
    // std::cout << "curr: " << point_3(curr) << std::endl;

    const auto next_p = point_2(next);
    const auto curr_p = point_2(curr);

    const Line_2 future_line_next(
      point_2(next, m_current_time + FT(1)),
      point_2(curr, m_current_time + FT(1)));

    // TODO: CAN WE AVOID THIS VALUE?
    const FT tol = KSR::tolerance<FT>();
    FT m2 = FT(100000), m3 = FT(100000);
    // std::cout << "tol: " << tol << std::endl;

    const FT next_d = (curr_p.x() - next_p.x());
    const FT edge_d = (target_p.x() - source_p.x());

    if (CGAL::abs(next_d) > tol)
      m2 = (curr_p.y() - next_p.y()) / next_d;
    if (CGAL::abs(edge_d) > tol)
      m3 = (target_p.y() - source_p.y()) / edge_d;

    // std::cout << "m2: " << m2 << std::endl;
    // std::cout << "m3: " << m3 << std::endl;

    // std::cout << "m2 - m3: " << CGAL::abs(m2 - m3) << std::endl;

    if (CGAL::abs(m2 - m3) < tol) {
      if (m_verbose) std::cout << "- open parallel lines" << std::endl;

      is_parallel = true;
      if (source_p == pv_point) {
        future_point = target_p;
        // std::cout << point_3(target(iedge)) << std::endl;
      } else {
        future_point = source_p;
        // std::cout << point_3(source(iedge)) << std::endl;
      }

    } else {
      if (m_verbose) std::cout << "- open intersected lines" << std::endl;
      future_point = KSR::intersection<Point_2>(future_line_next, iedge_line);
    }

    CGAL_assertion(pinit != future_point);
    future_direction = Vector_2(pinit, future_point);
    CGAL_assertion(future_direction != Vector_2());
    future_point = pinit - m_current_time * future_direction;

    if (m_verbose) {
      auto tmp = future_direction;
      tmp = KSR::normalize(tmp);
      std::cout << "- open future point: " <<
      to_3d(pvertex.first, pinit + m_current_time * tmp) << std::endl;
      std::cout << "- open future direction: " << future_direction << std::endl;
    }
    return is_parallel;
  }

  const bool is_intersecting_iedge(
    const FT min_time, const FT max_time,
    const PVertex& pvertex, const IEdge& iedge) const {

    const FT time_step = (max_time - min_time) / FT(100);
    const FT time_1 = m_current_time - time_step;
    const FT time_2 = m_current_time + time_step;
    CGAL_assertion(time_1 != time_2);

    const Segment_2 psegment(
      point_2(pvertex, time_1), point_2(pvertex, time_2));
    const auto pbbox = psegment.bbox();

    const auto isegment = segment_2(pvertex.first, iedge);
    const auto ibbox = isegment.bbox();

    if (has_iedge(pvertex)) {
      if (m_verbose) std::cout << "- constrained pvertex case" << std::endl;
      return false;
    }

    if (!is_active(pvertex)) {
      if (m_verbose) std::cout << "- pvertex no active case" << std::endl;
      return false;
    }

    if (!is_active(iedge)) {
      if (m_verbose) std::cout << "- iedge no active case" << std::endl;
      return false;
    }

    if (!CGAL::do_overlap(pbbox, ibbox)) {
      if (m_verbose) std::cout << "- no overlap case" << std::endl;
      return false;
    }

    Point_2 point;
    if (!KSR::intersection(psegment, isegment, point)) {
      if (m_verbose) std::cout << "- no intersection case" << std::endl;
      return false;
    }

    if (m_verbose) std::cout << "- found intersection" << std::endl;
    return true;
  }
};

} // namespace KSR_3
} // namespace CGAL

#endif // CGAL_KSR_3_DATA_STRUCTURE_H