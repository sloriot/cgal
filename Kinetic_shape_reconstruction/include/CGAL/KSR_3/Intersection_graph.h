// Copyright (c) 2019 GeometryFactory SARL (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Simon Giraudot, Dmitry Anisimov

#ifndef CGAL_KSR_3_INTERSECTION_GRAPH_H
#define CGAL_KSR_3_INTERSECTION_GRAPH_H

// #include <CGAL/license/Kinetic_shape_reconstruction.h>

// Boost includes.
#include <boost/graph/adjacency_list.hpp>

// CGAL includes.
#include <CGAL/Cartesian_converter.h>
#include <CGAL/Polygon_2.h>

// Internal includes.
#include <CGAL/KSR/utils.h>

namespace CGAL {
namespace KSR_3 {

template<typename GeomTraits>
class Intersection_graph {

public:
  using Kernel = GeomTraits;
  using EK = CGAL::Exact_predicates_exact_constructions_kernel;

  using FT        = typename Kernel::FT;
  using Point_3   = typename Kernel::Point_3;
  using Segment_3 = typename Kernel::Segment_3;
  using Line_3    = typename Kernel::Line_3;
  using Polygon_2 = typename CGAL::Polygon_2<Kernel>;

  struct Vertex_property {
    Point_3 point;
    bool active;
    Vertex_property() : active(true) {}
    Vertex_property(const Point_3& point) : point(point), active(true) {}
  };

  using Kinetic_interval = std::vector<std::pair<FT, FT> >;

  struct Edge_property {
    std::size_t line;
    std::map<std::size_t, std::pair<std::size_t, std::size_t> > faces; // For each intersecting support plane there is one pair of adjacent faces (or less if the edge is on the bbox)
    std::set<std::size_t> planes;
    std::set<std::size_t> crossed;
    std::map<std::size_t, Kinetic_interval> intervals; // Maps support plane index to the kinetic interval. std::pair<FT, FT> is the barycentric coordinate and intersection time.
    bool active;
    Edge_property() : line(KSR::no_element()), active(true) { }
  };

  using Kinetic_interval_iterator = typename std::map<std::size_t, Kinetic_interval>::const_iterator;

  using Graph = boost::adjacency_list<
    boost::setS, boost::vecS, boost::undirectedS,
    Vertex_property, Edge_property>;

  using Vertex_descriptor = typename boost::graph_traits<Graph>::vertex_descriptor;
  using Edge_descriptor = typename boost::graph_traits<Graph>::edge_descriptor;
  using Face_descriptor = std::size_t;

  struct Face_property {
    Face_property() : support_plane(-1), part_of_partition(false) {}
    Face_property(std::size_t support_plane_idx) : support_plane(support_plane_idx), part_of_partition(false) {}
    std::size_t support_plane;
    bool part_of_partition;
    CGAL::Polygon_2<EK> poly;
    std::vector<typename Kernel::Point_2> pts;
    std::vector<Edge_descriptor> edges;
    std::vector<Vertex_descriptor> vertices;
    bool is_part(Edge_descriptor a, Edge_descriptor b) {
      std::size_t aidx = std::size_t(-1);
      for (std::size_t i = 0; i < edges.size(); i++) {
        if (edges[i] == a) {
          aidx = i;
          break;
        }
      }

      if (aidx == std::size_t(-1))
        return false;

      if (edges[(aidx + 1) % edges.size()] == b || edges[(aidx + edges.size() - 1) % edges.size()] == b)
        return true;

      return false;
    }
  };

private:
  Graph m_graph;
  std::size_t m_nb_lines;
  std::size_t m_nb_lines_on_bbox;
  std::map<Point_3, Vertex_descriptor> m_map_points;
  std::map<std::vector<std::size_t>, Vertex_descriptor> m_map_vertices;
  std::map<Vertex_descriptor, Vertex_descriptor> m_vmap;
  std::map<Edge_descriptor, Edge_descriptor> m_emap;
  std::vector<Face_property> m_ifaces;

public:
  Intersection_graph() :
  m_nb_lines(0),
  m_nb_lines_on_bbox(0)
  { }

  void clear() {
    m_graph.clear();
    m_nb_lines = 0;
    m_map_points.clear();
    m_map_vertices.clear();
  }

  std::size_t number_of_vertices() const {
    return static_cast<std::size_t>(boost::num_vertices(m_graph));
  }

  std::size_t number_of_edges() const {
    return static_cast<std::size_t>(boost::num_edges(m_graph));
  }

  template<typename IG>
  void convert(IG& ig) {

    using CFT      = typename IG::Kernel::FT;
    using CPoint_3 = typename IG::Kernel::Point_3;

    // using Converter = CGAL::Cartesian_converter<Kernel, typename IG::Kernel>;
    // Converter converter;

    ig.set_nb_lines(m_nb_lines);
    const auto vpair = boost::vertices(m_graph);
    const auto vertex_range = CGAL::make_range(vpair);
    for (const auto vertex : vertex_range) {
      const auto vd = boost::add_vertex(ig.graph());
      // ig.graph()[vd].point = converter(m_graph[vertex].point);
      ig.graph()[vd].point = CPoint_3(
        static_cast<CFT>(CGAL::to_double(m_graph[vertex].point.x())),
        static_cast<CFT>(CGAL::to_double(m_graph[vertex].point.y())),
        static_cast<CFT>(CGAL::to_double(m_graph[vertex].point.z())));
      ig.graph()[vd].active = m_graph[vertex].active;
      CGAL_assertion(m_graph[vertex].active);
      m_vmap[vertex] = vd;
    }
    CGAL_assertion(boost::num_vertices(ig.graph()) == boost::num_vertices(m_graph));

    const auto epair = boost::edges(m_graph);
    const auto edge_range = CGAL::make_range(epair);
    for (const auto edge : edge_range) {
      const auto ed = boost::add_edge(
        boost::source(edge, m_graph), boost::target(edge, m_graph), ig.graph()).first;

      CGAL_assertion(m_graph[edge].line >= 0);
      ig.graph()[ed].line   = m_graph[edge].line;

      CGAL_assertion(m_graph[edge].planes.size() >= 1);
      ig.graph()[ed].planes = m_graph[edge].planes;

      CGAL_assertion(m_graph[edge].active);
      ig.graph()[ed].active = m_graph[edge].active;

      m_emap[edge] = ed;
    }
    CGAL_assertion(boost::num_edges(ig.graph()) == boost::num_edges(m_graph));

    // for (const auto& mp : m_map_points) {
    //   ig.mapped_points()[converter(mp.first)] = m_vmap.at(mp.second);
    // }
    // for (const auto& mv : m_map_vertices) {
    //   ig.mapped_vertices()[mv.first] = m_vmap.at(mv.second);
    // }
  }

  const std::map<Vertex_descriptor, Vertex_descriptor>& vmap() const {
    return m_vmap;
  }

  const std::map<Edge_descriptor, Edge_descriptor>& emap() const {
    return m_emap;
  }

  static Vertex_descriptor null_ivertex() {
    return boost::graph_traits<Graph>::null_vertex();
  }

  static Edge_descriptor null_iedge() {
    return Edge_descriptor(null_ivertex(), null_ivertex(), nullptr);
  }

  static Face_descriptor null_iface() {
    return std::size_t(-1);
  }

  std::size_t add_line() { return ( m_nb_lines++ ); }
  std::size_t nb_lines() const { return m_nb_lines; }
  void set_nb_lines(const std::size_t value) { m_nb_lines = value; }
  Graph& graph() { return m_graph; }

  const std::pair<Vertex_descriptor, bool> add_vertex(const Point_3& point) {

    const auto pair = m_map_points.insert(std::make_pair(point, Vertex_descriptor()));
    const auto is_inserted = pair.second;
    if (is_inserted) {
      pair.first->second = boost::add_vertex(m_graph);
      m_graph[pair.first->second].point = point;
    }
    return std::make_pair(pair.first->second, is_inserted);
  }

  const std::pair<Vertex_descriptor, bool> add_vertex(
    const Point_3& point, const std::vector<std::size_t>& intersected_planes) {

    const auto pair = m_map_vertices.insert(std::make_pair(intersected_planes, Vertex_descriptor()));
    const auto is_inserted = pair.second;
    if (is_inserted) {
      pair.first->second = boost::add_vertex(m_graph);
      m_graph[pair.first->second].point = point;
    }
    return std::make_pair(pair.first->second, is_inserted);
  }

  const std::pair<Edge_descriptor, bool> add_edge(
    const Vertex_descriptor& source, const Vertex_descriptor& target,
    const std::size_t support_plane_idx) {

    const auto out = boost::add_edge(source, target, m_graph);
    m_graph[out.first].planes.insert(support_plane_idx);
    return out;
  }

  template<typename IndexContainer>
  const std::pair<Edge_descriptor, bool> add_edge(
    const Vertex_descriptor& source, const Vertex_descriptor& target,
    const IndexContainer& support_planes_idx) {

    const auto out = boost::add_edge(source, target, m_graph);
    for (const auto support_plane_idx : support_planes_idx) {
      m_graph[out.first].planes.insert(support_plane_idx);
    }
    return out;
  }

  const std::pair<Edge_descriptor, bool> add_edge(
    const Point_3& source, const Point_3& target) {
    return add_edge(add_vertex(source).first, add_vertex(target).first);
  }

  std::size_t add_face(std::size_t support_plane_idx) {
    m_ifaces.push_back(Face_property(support_plane_idx));
    return std::size_t(m_ifaces.size() - 1);
  }

  bool add_face(std::size_t sp_idx, const Edge_descriptor& edge, const Face_descriptor& idx) {
    auto &pair = m_graph[edge].faces.insert(std::make_pair(sp_idx, std::pair<Face_descriptor, Face_descriptor>(-1, -1)));
    if (pair.first->second.first == -1) {
      pair.first->second.first = idx;
      return true;
    }
    else if (pair.first->second.second == -1) {
      pair.first->second.second = idx;
      return true;
    }
    return false;
  }

  void get_faces(std::size_t sp_idx, const Edge_descriptor& edge, std::pair<Face_descriptor, Face_descriptor> &pair) const {
    auto it = m_graph[edge].faces.find(sp_idx);
    if (it != m_graph[edge].faces.end())
      pair = it->second;
  }

  const Face_property& face(Face_descriptor idx) const {
    CGAL_assertion(idx < m_ifaces.size());
    return m_ifaces[idx];
  }

  Face_property& face(Face_descriptor idx) {
    CGAL_assertion(idx < m_ifaces.size());
    return m_ifaces[idx];
  }

  const Edge_property& edge(Edge_descriptor idx) const {
    return m_graph[idx];
  }

  void set_line(const Edge_descriptor& edge, const std::size_t line_idx) {
    m_graph[edge].line = line_idx;
  }

  std::size_t line(const Edge_descriptor& edge) const { return m_graph[edge].line; }

  bool line_is_on_bbox(std::size_t line_idx) const {
    return line_idx < m_nb_lines_on_bbox;
  }

  bool line_is_bbox_edge(std::size_t line_idx) const {
    return line_idx < 12;
  }

  bool iedge_is_on_bbox(Edge_descriptor e) {
    return line(e) < m_nb_lines_on_bbox;
  }

  void finished_bbox() {
    m_nb_lines_on_bbox = m_nb_lines;
  }

  const std::pair<Edge_descriptor, Edge_descriptor>
  split_edge(const Edge_descriptor& edge, const Vertex_descriptor& vertex) {

    const auto source = boost::source(edge, m_graph);
    const auto target = boost::target(edge, m_graph);
    const auto prop = m_graph[edge];
    boost::remove_edge(edge, m_graph);

    bool is_inserted;
    Edge_descriptor sedge;
    std::tie(sedge, is_inserted) = boost::add_edge(source, vertex, m_graph);
    if (!is_inserted) {
      std::cerr << "WARNING: " << segment_3(edge) << " " << point_3(vertex) << std::endl;
    }
    CGAL_assertion(is_inserted);
    m_graph[sedge] = prop;

    Edge_descriptor tedge;
    std::tie(tedge, is_inserted) = boost::add_edge(vertex, target, m_graph);
    if (!is_inserted) {
      std::cerr << "WARNING: " << segment_3(edge) << " " << point_3(vertex) << std::endl;
    }
    CGAL_assertion(is_inserted);
    m_graph[tedge] = prop;

    return std::make_pair(sedge, tedge);
  }

  decltype(auto) vertices() const { return CGAL::make_range(boost::vertices(m_graph)); }
  decltype(auto) edges() const { return CGAL::make_range(boost::edges(m_graph)); }

  std::vector<Face_descriptor>& faces() { return m_ifaces; }
  const std::vector<Face_descriptor>& faces() const { return m_ifaces; }

  const Vertex_descriptor source(const Edge_descriptor& edge) const { return boost::source(edge, m_graph); }
  const Vertex_descriptor target(const Edge_descriptor& edge) const { return boost::target(edge, m_graph); }

  bool is_edge(const Vertex_descriptor& source, const Vertex_descriptor& target) const {
    return boost::edge(source, target, m_graph).second;
  }

  const Edge_descriptor edge(const Vertex_descriptor& source, const Vertex_descriptor& target) const {
    return boost::edge(source, target, m_graph).first;
  }

  decltype(auto) incident_edges(const Vertex_descriptor& vertex) const {
    return CGAL::make_range(boost::out_edges(vertex, m_graph));
  }

  const std::set<std::size_t>& intersected_planes(const Edge_descriptor& edge) const { return m_graph[edge].planes; }
  std::set<std::size_t>& intersected_planes(const Edge_descriptor& edge) { return m_graph[edge].planes; }

  const std::pair<Kinetic_interval_iterator, Kinetic_interval_iterator> kinetic_intervals(const Edge_descriptor& edge) { return std::pair<Kinetic_interval_iterator, Kinetic_interval_iterator>(m_graph[edge].intervals.begin(), m_graph[edge].intervals.end()); }
  Kinetic_interval& kinetic_interval(const Edge_descriptor& edge, std::size_t sp_idx) { return m_graph[edge].intervals[sp_idx]; }

  const Point_3& point_3(const Vertex_descriptor& vertex) const {
    return m_graph[vertex].point;
  }

  const Segment_3 segment_3(const Edge_descriptor& edge) const {
    return Segment_3(
      m_graph[boost::source(edge, m_graph)].point,
      m_graph[boost::target(edge, m_graph)].point);
  }

  const Line_3 line_3(const Edge_descriptor& edge) const {
    return Line_3(
      m_graph[boost::source(edge, m_graph)].point,
      m_graph[boost::target(edge, m_graph)].point);
  }

  const bool& is_active(const Vertex_descriptor& vertex) const { return m_graph[vertex].active; }
  bool& is_active(const Vertex_descriptor& vertex) { return m_graph[vertex].active; }
  const bool& is_active(const Edge_descriptor& edge) const { return m_graph[edge].active; }
  bool& is_active(const Edge_descriptor& edge) { return m_graph[edge].active; }
  bool has_crossed(const Edge_descriptor& edge, std::size_t sp_idx) { return m_graph[edge].crossed.count(sp_idx) == 1; }
  void set_crossed(const Edge_descriptor& edge, std::size_t sp_idx) { m_graph[edge].crossed.insert(sp_idx); }
};

} // namespace KSR_3
} // namespace CGAL

#endif // CGAL_KSR_3_INTERSECTION_GRAPH_H
