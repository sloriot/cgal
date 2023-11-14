// Copyright (c) 2023 GeometryFactory Sarl (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Sven Oesau, Florent Lafarge, Dmitry Anisimov, Simon Giraudot

#ifndef CGAL_KINETIC_SHAPE_PARTITION_3_H
#define CGAL_KINETIC_SHAPE_PARTITION_3_H

//#include <CGAL/license/Kinetic_shape_partition_3.h>

// Boost includes.
#include <CGAL/boost/graph/named_params_helper.h>
#include <CGAL/Named_function_parameters.h>

#include <algorithm>
#include <numeric>

// CGAL includes.
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/convex_hull_2.h>
#include <CGAL/linear_least_squares_fitting_3.h>
#include <CGAL/Real_timer.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Constrained_triangulation_plus_2.h>
#include <CGAL/Base_with_time_stamp.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>

#include <CGAL/Linear_cell_complex_for_combinatorial_map.h>
#include <CGAL/Linear_cell_complex_incremental_builder_3.h>
#include <CGAL/draw_linear_cell_complex.h>

// Internal includes.
#include <CGAL/KSR/utils.h>
#include <CGAL/KSR/debug.h>
#include <CGAL/KSR/parameters.h>

#include <CGAL/KSR_3/Data_structure.h>
#include <CGAL/KSR_3/Initializer.h>
#include <CGAL/KSR_3/FacePropagation.h>
#include <CGAL/KSR_3/Finalizer.h>

#include <CGAL/Octree.h>
#include <CGAL/Orthtree_traits_polygons.h>

//#define OVERLAY_2_DEBUG
#define OVERLAY_2_CHECK

namespace CGAL {

/*!
* \ingroup PkgKineticShapePartitionRef
  \brief creates the kinetic partition of the bounding box of the polygons given as input data. Use `Kinetic_shape_partition_3::Kinetic_shape_partition_3` to create an empty object, `Kinetic_shape_partition_3::insert` to provide input data and `Kinetic_shape_partition_3::initialize` to prepare the partition or use `Kinetic_shape_partition_3::Kinetic_shape_partition_3(
    const InputRange& input_range, const PolygonRange polygon_range, const NamedParameters &np)`.

  \tparam GeomTraits
    must be a model of `KineticShapePartitionTraits_3`.

  \tparam IntersectionTraits
    must be a model of `Kernel` using exact computations. Defaults to `CGAL::Exact_predicates_exact_constructions_kernel`.
*/
template<typename GeomTraits, typename IntersectionTraits = CGAL::Exact_predicates_exact_constructions_kernel>
class Kinetic_shape_partition_3 {

public:
  using Kernel = GeomTraits;
  using Intersection_kernel = IntersectionTraits;

  using Point_3 = typename Kernel::Point_3;

  using Index = std::pair<std::size_t, std::size_t>;

  struct LCC_min_items {
    typedef CGAL::Tag_true Use_index;
    typedef std::uint32_t Index_type;

    struct Face_property {
      int input_polygon_index; // -1 till -6 correspond to bbox faces, -7 to faces from octree
      typename Intersection_kernel::Plane_3 plane;
      bool part_of_initial_polygon;
    };

    struct Volume_property {
      typename Intersection_kernel::Point_3 barycenter;
      std::size_t volume_id;
    };

    template<class LCC>
    struct Dart_wrapper {
      typedef CGAL::Cell_attribute_with_point< LCC, void > Vertex_attribute;
      typedef CGAL::Cell_attribute< LCC, Face_property > Face_attribute;
      typedef CGAL::Cell_attribute< LCC, Volume_property > Volume_attribute;

      int this_is_a_test;

      typedef std::tuple<Vertex_attribute, void, Face_attribute, Volume_attribute> Attributes;
    };
  };

private:
  using FT = typename Kernel::FT;
  using Point_2 = typename Kernel::Point_2;
  using Vector_2 = typename Kernel::Vector_2;
  using Plane_3 = typename Kernel::Plane_3;
  using Line_3 = typename Kernel::Line_3;
  using Line_2 = typename Kernel::Line_2;
  using Triangle_2 = typename Kernel::Triangle_2;
  using Transform_3 = CGAL::Aff_transformation_3<Kernel>;

  using Data_structure = KSR_3::Data_structure<Kernel, Intersection_kernel>;

  using IVertex = typename Data_structure::IVertex;
  using IEdge   = typename Data_structure::IEdge;

  using From_exact = typename CGAL::Cartesian_converter<Intersection_kernel, Kernel>;
  using To_exact = typename CGAL::Cartesian_converter<Kernel, Intersection_kernel>;

  using Initializer = KSR_3::Initializer<Kernel, Intersection_kernel>;
  using Propagation = KSR_3::FacePropagation<Kernel, Intersection_kernel>;
  using Finalizer   = KSR_3::Finalizer<Kernel, Intersection_kernel>;

  using Polygon_mesh = CGAL::Surface_mesh<Point_3>;
  using Timer        = CGAL::Real_timer;
  using Parameters = KSR::Parameters_3<FT>;

  using Octree = CGAL::Orthtree<CGAL::Orthtree_traits_polygons<Kernel> >;
  using Octree_node = typename Octree::Node_index;

  struct VI
  {
    VI()
      : input(false), idA2(-1, -1), idB2(-1, -1)
    {}

    void set_index(std::size_t i) {
      idx = i;
    }

    void set_point(const typename Intersection_kernel::Point_3& p)
    {
      point_3 = p;
      input = true;
    }

    typename Intersection_kernel::Point_3 point_3;
    //std::size_t idx;  // ivertex?
    std::set<Index> adjacent;
    //std::set<Index> ids;
    Index idA2, idB2;
    bool input;
  };

  // Each face gets as id
  // The overlay face gets also the id from A and B
  struct ID {
    ID()
      : id(-1), idA(-1), idB(-1), id2(std::size_t(-1), std::size_t(-1)), idA2(std::size_t(-1), std::size_t(-1)), idB2(std::size_t(-1), std::size_t(-1))
    {}

    ID(const ID& other)
      :id(other.id), idA(other.idA), idB(other.idB), id2(other.id2), idA2(other.idA2), idB2(other.idB2)
    {}

    ID& operator=(const ID& other)
    {
      id = other.id;
      idA = other.idA;
      idB = other.idB;
      id2 = other.id2;
      idA2 = other.idA2;
      idB2 = other.idB2;
      volA = other.volA;
      volB = other.volB;
      return *this;
    }

    int volA, volB;
    Index id2, idA2, idB2;
    int id, idA, idB;
  };

  typedef CGAL::Triangulation_vertex_base_with_info_2<VI, Intersection_kernel> Vbi;
  typedef CGAL::Triangulation_face_base_with_info_2<ID, Intersection_kernel> Fbi;
  typedef CGAL::Constrained_triangulation_face_base_2<Intersection_kernel, Fbi>    Fb;
  typedef CGAL::Triangulation_data_structure_2<Vbi, Fb>       TDS;
  typedef CGAL::Exact_intersections_tag                     Itag;
  typedef CGAL::Constrained_Delaunay_triangulation_2<Intersection_kernel, TDS, Itag> CDT;
  typedef CGAL::Constrained_triangulation_plus_2<CDT>       CDTplus;
  typedef typename CDTplus::Vertex_handle            Vertex_handle;
  typedef typename CDTplus::Face_handle              Face_handle;
  typedef typename CDTplus::Finite_vertices_iterator Finite_vertices_iterator;
  typedef typename CDTplus::Finite_faces_iterator    Finite_faces_iterator;

private:
  struct Sub_partition {
    Sub_partition() : parent(-1) {}
    std::shared_ptr<Data_structure> m_data;
    std::array<typename Intersection_kernel::Point_3, 8> bbox;
    std::vector<typename Intersection_kernel::Plane_3> m_bbox_planes;
    std::vector<std::size_t> input_polygons;
    std::vector<std::vector<Point_3> > clipped_polygons;
    std::vector<typename Intersection_kernel::Plane_3> m_input_planes;
    std::size_t parent;
    std::vector<std::size_t> children;
    std::size_t split_plane;
    std::size_t index;

    std::vector<std::pair<Index, Index> > face_neighbors;
    std::vector<std::vector<Index> > face2vertices;

    std::vector<typename Data_structure::Volume_cell> volumes;
    //std::vector<std::vector<std::size_t> > face2vertices;
    //std::vector<Point_3> exact_vertices;

    typename Octree::Node_index node;
  };

  Parameters m_parameters;
  std::array<Point_3, 8> m_bbox;
  std::vector<Sub_partition> m_partition_nodes; // Tree of partitions.
  std::vector<std::size_t> m_partitions; // Contains the indices of the leaf nodes, the actual partitions to be calculated.
  std::size_t m_num_events;
  std::vector<Point_3> m_points;
  std::vector<std::vector<std::size_t> > m_polygons;
  std::vector<std::vector<Point_3> > m_input_polygons;
  std::vector<typename Intersection_kernel::Plane_3> m_input_planes;
  std::vector<Point_2> m_input_centroids;
  std::vector<std::size_t> m_input2regularized; // Mapping from actual input planes to regularized input planes.
  std::vector<std::vector<std::size_t> > m_regularized2input; // Mapping from partitioning planes to original input polygons.
  std::unique_ptr<Octree> m_octree;
  std::vector<std::size_t> m_node2partition;

  std::vector<Index> m_volumes;
  std::map<Index, std::size_t> m_index2volume;

  std::set<Index> duplicates;

public:
  /// \name Initialization
  /// @{
  /*!
  \brief constructs an empty kinetic shape partition object. Use `insert` afterwards to insert polygons into the partition and `initialize` to initialize the partition.

  \tparam NamedParameters
  a sequence of \ref bgl_namedparameters "Named Parameters"

  \param np
  a sequence of \ref bgl_namedparameters "Named Parameters"
  among the ones listed below

  \cgalNamedParamsBegin
    \cgalParamNBegin{verbose}
      \cgalParamDescription{Write timing and internal information to std::cout.}
      \cgalParamType{bool}
      \cgalParamDefault{false}
    \cgalParamNEnd
    \cgalParamNBegin{debug}
      \cgalParamDescription{Export of intermediate results.}
      \cgalParamType{bool}
      \cgalParamDefault{false}
    \cgalParamNEnd
  \cgalNamedParamsEnd
  */
  template<typename NamedParameters = parameters::Default_named_parameters>
  Kinetic_shape_partition_3(
    const NamedParameters& np = CGAL::parameters::default_values()) :
    m_parameters(
      parameters::choose_parameter(parameters::get_parameter(np, internal_np::verbose), false),
      parameters::choose_parameter(parameters::get_parameter(np, internal_np::debug), false)), // use true here to export all steps
    m_num_events(0),
    m_input2regularized()
  {
    m_parameters.angle_tolerance = parameters::choose_parameter(parameters::get_parameter(np, internal_np::angle_tolerance), 0);
    m_parameters.distance_tolerance = parameters::choose_parameter(parameters::get_parameter(np, internal_np::distance_tolerance), 0);
  }

  /*!
  \brief constructs a kinetic shape partition object and initializes it.

  \tparam InputRange
  must be a model of `ConstRange` whose iterator type is `RandomAccessIterator` and whose value type is Point_3.

  \tparam PolygonRange
  contains index ranges to form polygons by providing indices into InputRange

  \tparam NamedParameters
  a sequence of \ref bgl_namedparameters "Named Parameters"

  \param input_range
  an instance of `InputRange` with 3D points and corresponding 3D normal vectors

  \param polygon_range
  a range of non-coplanar polygons defined by a range of indices into `input_range`

  \param np
  a sequence of \ref bgl_namedparameters "Named Parameters" among the ones listed below

  \pre !input_range.empty() and !polygon_map.empty()

  \cgalNamedParamsBegin
     \cgalParamNBegin{point_map}
       \cgalParamDescription{a property map associating points to the elements of the `input_range`}
       \cgalParamType{a model of `ReadablePropertyMap` whose key type is the value type of the iterator of `InputRange` and whose value type is `GeomTraits::Point_3`}
       \cgalParamDefault{`CGAL::Identity_property_map<GeomTraits::Point_3>`}
     \cgalParamNEnd
    \cgalParamNBegin{verbose}
      \cgalParamDescription{Write timing and internal information to std::cout.}
      \cgalParamType{bool}
      \cgalParamDefault{false}
    \cgalParamNEnd
    \cgalParamNBegin{reorient_bbox}
      \cgalParamDescription{Use the oriented bounding box instead of the axis-aligned bounding box. While the z direction is maintained, the x axis is aligned with the largest variation in the horizontal plane.}
      \cgalParamType{bool}
      \cgalParamDefault{false}
    \cgalParamNEnd
    \cgalParamNBegin{bbox_dilation_ratio}
      \cgalParamDescription{Factor for extension of the bounding box of the input data to be used for the partition.}
      \cgalParamType{FT}
      \cgalParamDefault{1.1}
    \cgalParamNEnd
    \cgalParamNBegin{angle_tolerance}
      \cgalParamDescription{The tolerance angle to snap the planes of two input polygons into one plane.}
      \cgalParamType{FT}
      \cgalParamDefault{5}
    \cgalParamNEnd
    \cgalParamNBegin{distance_tolerance}
      \cgalParamDescription{The tolerance distance to snap the planes of two input polygons into one plane.}
      \cgalParamType{FT}
      \cgalParamDefault{1% of the bounding box diagonal}
    \cgalParamNEnd
  \cgalNamedParamsEnd
  */
  template<
    typename InputRange,
    typename PolygonRange,
    typename NamedParameters = parameters::Default_named_parameters>
  Kinetic_shape_partition_3(
    const InputRange& input_range,
    const PolygonRange &polygon_range,
    const NamedParameters & np = CGAL::parameters::default_values()) :
    m_parameters(
      parameters::choose_parameter(parameters::get_parameter(np, internal_np::verbose), false),
      parameters::choose_parameter(parameters::get_parameter(np, internal_np::debug), false)), // use true here to export all steps
    m_data(m_parameters),
    m_num_events(0),
    m_input2regularized()
  {
    m_parameters.angle_tolerance = parameters::choose_parameter(parameters::get_parameter(np, internal_np::angle_tolerance), 0);
    m_parameters.distance_tolerance = parameters::choose_parameter(parameters::get_parameter(np, internal_np::distance_tolerance), 0);
    insert(input_range, polygon_range, np);
    initialize(np);
  }

  /*!
  \brief inserts non-coplanar polygons, requires initialize() afterwards to have effect.

  \tparam InputRange
  must be a model of `ConstRange` whose iterator type is `RandomAccessIterator` and whose value type is Point_3.

  \tparam PolygonRange
  contains index ranges to form polygons by providing indices into InputRange

  \tparam NamedParameters
  a sequence of \ref bgl_namedparameters "Named Parameters"

  \param input_range
  an instance of `InputRange` with 3D points

  \param polygon_range
  a range of non-coplanar polygons defined by a range of indices into `input_range`

  \param np
  a sequence of \ref bgl_namedparameters "Named Parameters" among the ones listed below

  \cgalNamedParamsBegin
     \cgalParamNBegin{point_map}
       \cgalParamDescription{a property map associating points to the elements of the `input_range`}
       \cgalParamType{a model of `ReadablePropertyMap` whose key type is the value type of the iterator of `InputRange` and whose value type is `GeomTraits::Point_3`}
       \cgalParamDefault{`CGAL::Identity_property_map<GeomTraits::Point_3>`}
     \cgalParamNEnd
  \cgalNamedParamsEnd
  */
  template<typename InputRange, typename PolygonRange, typename NamedParameters = parameters::Default_named_parameters>
  void insert(
    const InputRange& input_range,
    const PolygonRange &polygon_range,
    const NamedParameters& np = CGAL::parameters::default_values()) {
    To_exact to_exact;
    From_exact from_exact;
    std::size_t offset = m_input2regularized.size();

    for (std::size_t p = 0; p < polygon_range.size();p++) {
      auto& poly = polygon_range[p];

      std::vector<Point_3> pts;
      pts.reserve(poly.size());
      for (auto it : poly)
        pts.push_back(*(input_range.begin() + it));
      Plane_3 pl;
      Point_2 c;
      std::vector<Point_2> ch;
      process_input_polygon(pts, pl, c, ch);
      typename Intersection_kernel::Plane_3 exact_pl = to_exact(pl);


      // Check if there is already a coplanar shape inserted
      bool skip = false;
      for (std::size_t i = 0; i < m_input_planes.size(); i++) {
        if (m_input_planes[i] == exact_pl) {
          std::cout << i << ". input polygon is coplanar to " << (p + offset) << ". input polygon" << std::endl;
          skip = true;
          break;
        }
      }
      if (skip)
        continue;


      m_input2regularized.push_back(m_input_planes.size());
      m_regularized2input.push_back(std::vector<std::size_t>());
      m_regularized2input.back().push_back(p);
      m_input_planes.push_back(to_exact(pl));
      m_input_centroids.push_back(c);
      m_input_polygons.push_back(std::vector<Point_3>(ch.size()));

      for (std::size_t i = 0; i < ch.size(); i++)
        m_input_polygons.back()[i] = pl.to_3d(ch[i]);
    }
  }

  /*!
  \brief initializes the kinetic partition of the bounding box.

  \tparam NamedParameters
  a sequence of \ref bgl_namedparameters "Named Parameters"

  \param np
  a sequence of \ref bgl_namedparameters "Named Parameters"
  among the ones listed below

  \pre Input data has been provided via `insert()`.

  \cgalNamedParamsBegin
    \cgalParamNBegin{reorient_bbox}
      \cgalParamDescription{Use the oriented bounding box instead of the axis-aligned bounding box.}
      \cgalParamType{bool}
      \cgalParamDefault{false}
    \cgalParamNEnd
    \cgalParamNBegin{bbox_dilation_ratio}
      \cgalParamDescription{Factor for extension of the bounding box of the input data to be used for the partition.}
      \cgalParamType{FT}
      \cgalParamDefault{1.1}
    \cgalParamNEnd
    \cgalParamNBegin{angle_tolerance}
      \cgalParamDescription{The tolerance angle to snap the planes of two input polygons into one plane.}
      \cgalParamType{FT}
      \cgalParamDefault{5}
    \cgalParamNEnd
    \cgalParamNBegin{distance_tolerance}
      \cgalParamDescription{The tolerance distance to snap the planes of two input polygons into one plane.}
      \cgalParamType{FT}
      \cgalParamDefault{0.5}
    \cgalParamNEnd
  \cgalNamedParamsEnd
  */

  template<
    typename NamedParameters = parameters::Default_named_parameters>
  void initialize(
    const NamedParameters& np = CGAL::parameters::default_values()) {

    Timer timer;
    m_parameters.bbox_dilation_ratio = parameters::choose_parameter(
      parameters::get_parameter(np, internal_np::bbox_dilation_ratio), FT(11) / FT(10));
    m_parameters.angle_tolerance = parameters::choose_parameter(
      parameters::get_parameter(np, internal_np::angle_tolerance), FT(0) / FT(10));
    m_parameters.distance_tolerance = parameters::choose_parameter(
      parameters::get_parameter(np, internal_np::distance_tolerance), FT(0) / FT(10));
    m_parameters.reorient_bbox = parameters::choose_parameter(
      parameters::get_parameter(np, internal_np::reorient_bbox), false);
    m_parameters.max_octree_depth = parameters::choose_parameter(
      parameters::get_parameter(np, internal_np::max_octree_depth), 3);
    m_parameters.max_octree_node_size = parameters::choose_parameter(
      parameters::get_parameter(np, internal_np::max_octree_node_size), 40);


    //CGAL_add_named_parameter(max_octree_depth_t, max_octree_depth, max_octree_depth)
    //CGAL_add_named_parameter(max_octree_node_size_t, max_octree_node_size, max_octree_node_size)

    std::cout.precision(20);
    if (m_input_polygons.size() == 0) {
      std::cout << "Warning: Your input is empty!";
      return;
    }

    std::set<std::size_t> n;
    for (auto p : m_input2regularized)
      n.insert(p);

    assert(m_regularized2input.size() == m_input_polygons.size());
    assert(m_regularized2input.size() == n.size());

    if (m_parameters.bbox_dilation_ratio < FT(1)) {
      CGAL_warning_msg(m_parameters.bbox_dilation_ratio >= FT(1),
        "Warning: You set enlarge_bbox_ratio < 1.0! The valid range is [1.0, +inf). Setting to 1.0!");
      m_parameters.bbox_dilation_ratio = FT(1);
    }

    if (m_parameters.verbose) {
      //const std::string is_reorient = (m_parameters.reorient ? "true" : "false");

      std::cout << std::endl << "--- PARTITION OPTIONS: " << std::endl;
      std::cout << "* enlarge bbox ratio: " << m_parameters.bbox_dilation_ratio << std::endl;
    }

    if (m_parameters.verbose) {
      std::cout << std::endl << "--- INITIALIZING PARTITION:" << std::endl;

      // Initialization.
      timer.reset();
      timer.start();
    }

    if (m_parameters.debug) {
      for (std::size_t i = 0; i < m_input_polygons.size(); i++)
        KSR_3::dump_polygon(m_input_polygons[i], std::to_string(i) + "-input_polygon");
    }

    split_octree();
    m_partitions.resize(m_partition_nodes.size());
    std::iota(m_partitions.begin(), m_partitions.end(), 0);

    for (std::size_t idx : m_partitions) {
      Sub_partition& partition = m_partition_nodes[idx];
      std::cout << idx << ". " << partition.input_polygons.size() << " polygons " << std::flush;
      partition.index = idx;

      partition.m_data = std::make_shared<Data_structure>(m_parameters, std::to_string(idx) + "-");

/*
      std::vector<std::vector<Point_3> > input_polygons(partition.input_polygons.size());
      for (std::size_t i = 0; i < partition.input_polygons.size(); i++)
        input_polygons[i] = m_input_polygons[partition.input_polygons[i]];*/

      Initializer initializer(partition.clipped_polygons, partition.m_input_planes, *partition.m_data, m_parameters);
      initializer.initialize(partition.bbox, partition.input_polygons);
      std::cout << std::endl;
    }

    // Timing.
    if (m_parameters.verbose) {
      const double time_to_initialize = timer.time();
      std::cout << "* initialization time: " << time_to_initialize << std::endl;
    }
  }

  /*!
  \brief propagates the kinetic polygons in the initialized partition.

  \param k
   maximum number of allowed intersections for each input polygon before its expansion stops.

  \pre initialized partition and k != 0
  */
  void partition(std::size_t k) {
    FT a, b, c;
    partition(k, a, b, c);
  }

#ifndef DOXYGEN_RUNNING
  void partition(std::size_t k, FT& partition_time, FT& finalization_time, FT& conformal_time) {
    m_volumes.clear();
    Timer timer;
    timer.start();
    partition_time = 0;
    finalization_time = 0;
    conformal_time = 0;

    /*
        if (m_parameters.debug)
          if (boost::filesystem::is_directory("volumes/"))
            for (boost::filesystem::directory_iterator end_dir_it, it("volumes/"); it != end_dir_it; ++it)
              boost::filesystem::remove_all(it->path());*/

    for (std::size_t idx : m_partitions) {
      Sub_partition& partition = m_partition_nodes[idx];
      timer.reset();
      std::cout.precision(20);

      // Already initialized?
      if (partition.m_data->number_of_support_planes() < 6) {
        std::cout << "Kinetic partition not initialized or empty. Number of support planes: " << partition.m_data->number_of_support_planes() << std::endl;

        return;
      }

      if (k == 0) { // for k = 0, we skip propagation
        std::cout << "k needs to be a positive number" << std::endl;

        return;
      }

      if (m_parameters.verbose) {
        std::cout << std::endl << "--- RUNNING THE QUEUE:" << std::endl;
        std::cout << "* propagation started" << std::endl;
      }

      // Propagation.
      std::size_t num_queue_calls = 0;

      Propagation propagation(*partition.m_data, m_parameters);
      std::tie(num_queue_calls, m_num_events) = propagation.propagate(k);

      partition_time += timer.time();

      if (m_parameters.verbose) {
        std::cout << "* propagation finished" << std::endl;
        std::cout << "* number of queue calls: " << num_queue_calls << std::endl;
        std::cout << "* number of events handled: " << m_num_events << std::endl;
      }

      if (m_parameters.verbose) {
        std::cout << std::endl << "--- FINALIZING PARTITION:" << std::endl;
      }

      // Finalization.

      for (std::size_t i = 0; i < partition.m_data->number_of_support_planes(); i++)
        if (!partition.m_data->support_plane(i).mesh().is_valid(true))
          std::cout << i << ". support has an invalid mesh!" << std::endl;

      for (std::size_t i = 6; i < partition.m_data->number_of_support_planes(); i++) {
        bool initial = false;
        typename Data_structure::Support_plane& sp = partition.m_data->support_plane(i);

        for (const auto& f : sp.mesh().faces())
          if (sp.is_initial(f)) {
            initial = true;
            break;
          }

        if (!initial)
          std::cout << i << " sp has no initial face before" << std::endl;
      }

      Finalizer finalizer(*partition.m_data, m_parameters);

      if (m_parameters.verbose)
        std::cout << "* getting volumes ..." << std::endl;

      finalizer.create_polyhedra();
      finalization_time += timer.time();

      for (std::size_t i = 6; i < partition.m_data->number_of_support_planes(); i++) {
        bool initial = false;
        typename Data_structure::Support_plane& sp = partition.m_data->support_plane(i);

        for (const auto& f : sp.mesh().faces())
          if (sp.is_initial(f)) {
            initial = true;
            break;
          }

        if (!initial)
          std::cout << i << " sp has no initial face" << std::endl;
      }

      if (m_parameters.verbose)
        std::cout << idx << ". partition with " << partition.input_polygons.size() << " input polygons split into " << partition.m_data->number_of_volumes() << " volumes" << std::endl;
    }

    // Convert face_neighbors to pair<Index, Index>
    for (std::size_t i = 0; i < m_partitions.size(); i++) {
      Sub_partition& partition = m_partition_nodes[m_partitions[i]];

      for (std::size_t j = 0; j < partition.m_data->number_of_volumes(); j++) {
        m_volumes.push_back(std::make_pair(m_partitions[i], j));
      }

      partition.face_neighbors.resize(partition.m_data->face_to_volumes().size());
      for (std::size_t j = 0; j < partition.m_data->face_to_volumes().size(); j++) {
        auto& p = partition.m_data->face_to_volumes()[j];
        partition.face_neighbors[j] = std::make_pair(Index(m_partitions[i], p.first), Index(m_partitions[i], p.second));
      }

      partition.face2vertices.resize(partition.m_data->face_to_vertices().size());

      for (std::size_t j = 0; j < partition.m_data->face_to_vertices().size(); j++) {
        partition.face2vertices[j].resize(partition.m_data->face_to_vertices()[j].size());
        for (std::size_t k = 0; k < partition.m_data->face_to_vertices()[j].size(); k++)
          partition.face2vertices[j][k] = std::make_pair(m_partitions[i], partition.m_data->face_to_vertices()[j][k]);
      }
    }

    for (std::size_t i = 0; i < m_volumes.size(); i++)
      m_index2volume[m_volumes[i]] = i;

    std::map<typename Intersection_kernel::Point_3, Index> pts2idx;

    for (std::size_t i = 0; i < number_of_volumes(); i++) {
      std::vector<Index> f1;
      faces(i, std::back_inserter(f1));
      for (const Index& f : f1) {
        std::vector<Index>& face = m_partition_nodes[f.first].face2vertices[f.second];
        for (std::size_t j = 0; j < face.size(); j++) {
          auto it = pts2idx.emplace(m_partition_nodes[face[j].first].m_data->exact_vertices()[face[j].second], face[j]);
          if (!it.second)
            face[j] = it.first->second;
        }
      }
    }

    timer.stop();

    /*
        if (m_parameters.debug) {
          if (boost::filesystem::is_directory("volumes/"))
            for (boost::filesystem::directory_iterator end_dir_it, it("volumes/"); it != end_dir_it; ++it)
              boost::filesystem::remove_all(it->path());

          KSR_3::dump_volumes_ksp(*this, "volumes/");
          for (std::size_t i = 1; i < m_volumes.size(); i++)
            if (m_volumes[i].first != m_volumes[i - 1].first)
              std::cout << i << " " << m_volumes[i - 1].first << std::endl;
          std::cout << m_volumes.size() << " " << m_volumes.back().first << std::endl;
        }*/

    timer.reset();
    timer.start();
    make_conformal(0);
    conformal_time = timer.time();

    /*
        if (m_parameters.debug) {
          if (boost::filesystem::is_directory("volumes_after/"))
          for (boost::filesystem::directory_iterator end_dir_it, it("volumes_after/"); it != end_dir_it; ++it)
            boost::filesystem::remove_all(it->path());
          KSR_3::dump_volumes_ksp(*this, "volumes_after/");
          for (std::size_t i = 1; i < m_volumes.size(); i++)
            if (m_volumes[i].first != m_volumes[i - 1].first)
              std::cout << i << " " << m_volumes[i - 1].first << std::endl;
          std::cout << m_volumes.size() << " " << m_volumes.back().first << std::endl;
        }*/

        //make it specific to some subnodes?
    if (m_parameters.verbose)
      check_tjunctions();

    // Clear unused data structures
    for (std::size_t i = 0; i < m_partitions.size(); i++) {
      m_partition_nodes[i].m_data->pface_neighbors().clear();
      m_partition_nodes[i].m_data->face_to_vertices().clear();
      m_partition_nodes[i].m_data->face_to_index().clear();
      m_partition_nodes[i].m_data->face_to_volumes().clear();
    }

    return;
  }
#endif
  /// @}

  /*******************************
  **         Access         **
  ********************************/

  /// \name Access
  /// @{
  /*!
  \brief returns the number of vertices in the kinetic partition.

  \pre created partition
  */
  std::size_t number_of_vertices() const {
    return m_data.vertices().size();
  }

  /*!
  \brief returns the number of faces in the kinetic partition.

  \pre created partition
  */
  std::size_t number_of_faces() const {
    return m_data.face_to_vertices().size();
  }

  /*!
  \brief returns the number of volumes created by the kinetic partition.

  \pre created partition
  */
  std::size_t number_of_volumes() const {
    return m_volumes.size();
  }

  /*!
  \brief Provides the support planes of the partition derived from the input polygons

  @return
   vector of planes.

  \pre inserted polygons
  */
  const std::vector<typename Intersection_kernel::Plane_3> &input_planes() const {
    return m_input_planes;
  }

  /*!
   \brief Exports the kinetic partition into a `Linear_cell_complex_for_combinatorial_map<3, 3>` using a model of `KineticLCCItems` as items, e.g., `Lcc_min_items`.

   Volume and face attributes defined in the model `KineticLCCItems` are filled. The volume index is in the range [0, number of volumes -1]

   \param lcc instance of `Linear_cell_complex_for_combinatorial_map<3, 3>` to be filled with the kinetic partition.

   \pre created partition
  */
  template<class LCC>
  void get_linear_cell_complex(LCC &lcc) const {
    lcc.clear();

    std::map<Index, std::size_t> mapped_vertices;
    std::map<typename Intersection_kernel::Point_3, std::size_t> mapped_points;
    std::vector<typename Intersection_kernel::Point_3> vtx;
    std::vector<Index> vtx_index;

    From_exact to_inexact;
    To_exact to_exact;

    std::vector<Index> faces_of_volume, vtx_of_face;
    std::vector<typename Intersection_kernel::Point_3> pts_of_face;

    for (std::size_t i = 0; i < number_of_volumes(); i++) {
      faces(i, std::back_inserter(faces_of_volume));

      for (const Index& f : faces_of_volume) {
        exact_vertices(f, std::back_inserter(pts_of_face), std::back_inserter(vtx_of_face));

        for (std::size_t j = 0; j < pts_of_face.size(); j++) {
          auto pit = mapped_points.emplace(pts_of_face[j], vtx.size());
          if (pit.second) {
            mapped_vertices[vtx_of_face[j]] = vtx.size();
            vtx.push_back(pts_of_face[j]);
            vtx_index.push_back(vtx_of_face[j]);
          }
          else mapped_vertices[vtx_of_face[j]] = pit.first->second;
        }

        pts_of_face.clear();
        vtx_of_face.clear();
      }
      faces_of_volume.clear();
    }

    CGAL::Linear_cell_complex_incremental_builder_3<LCC> ib(lcc);
    for (std::size_t i = 0; i < vtx.size(); i++)
      ib.add_vertex(vtx[i]);

    std::size_t num_faces = 0;
    std::size_t num_vols = 0;
    std::size_t num_vtx = 0;

    typename LCC::Dart_descriptor d;

    std::vector<bool> used_vertices(mapped_vertices.size(), false);
    std::vector<bool> added_volumes(number_of_volumes(), false);
    std::deque<std::size_t> queue;
    queue.push_back(0);
    while (!queue.empty()) {
      std::size_t v = queue.front();
      queue.pop_front();

      if (added_volumes[v])
        continue;

      if (!can_add_volume_to_lcc(v, added_volumes, mapped_vertices, used_vertices)) {
        queue.push_back(v);
        continue;
      }

      added_volumes[v] = true;

      ib.begin_surface();
      //std::cout << v << " inserting:";
      num_vols++;
      faces(v, std::back_inserter(faces_of_volume));

      typename Intersection_kernel::Point_3 centroid = to_exact(m_partition_nodes[m_volumes[v].first].m_data->volumes()[m_volumes[v].second].centroid);

      /*
            std::ofstream vout3(std::to_string(v) + ".xyz");
            vout3.precision(20);
            vout3 << " " << m_partition_nodes[m_volumes[v].first].m_data->volumes()[m_volumes[v].second].centroid << std::endl;
            vout3 << std::endl;
            vout3.close();*/

            // How to order faces accordingly?
            // First take faces of adjacent volumes and collect all added edges
            // Then pick from the remaining faces and take those which have already inserted edges
            // Repeat the last step until all are done.
      //       std::set<std::pair<std::size_t, std::size_t> > edges;
      //       for (std::size_t j=0;)
            // Try easy way and remove cells, I did not add after every loop?

      for (std::size_t j = 0; j < faces_of_volume.size(); j++) {
        vertex_indices(faces_of_volume[j], std::back_inserter(vtx_of_face));

        auto pair = neighbors(faces_of_volume[j]);

        if (pair.first != v && !added_volumes[pair.first])
          queue.push_back(pair.first);
        if (pair.second != v && pair.second >= 0 && !added_volumes[pair.second])
          queue.push_back(pair.second);

        //auto vertex_range = m_data.pvertices_of_pface(vol.pfaces[i]);
        ib.begin_facet();
        num_faces++;

        //std::cout << "(";

        //Sub_partition& p = m_partition_nodes[faces_of_volume[j].first];

        typename Intersection_kernel::Vector_3 norm;
        std::size_t i = 0;
        do {
          std::size_t n = (i + 1) % vtx_of_face.size();
          std::size_t nn = (n + 1) % vtx_of_face.size();
          norm = CGAL::cross_product(vtx[mapped_vertices[vtx_of_face[n]]] - vtx[mapped_vertices[vtx_of_face[i]]], vtx[mapped_vertices[vtx_of_face[nn]]] - vtx[mapped_vertices[vtx_of_face[n]]]);
          i++;
        } while (to_inexact(norm.squared_length()) == 0 && i < vtx_of_face.size());

        FT len = sqrt(to_inexact(norm.squared_length()));
        if (len != 0)
          len = 1.0 / len;
        norm = norm * to_exact(len);
        typename Kernel::Vector_3 n1 = to_inexact(norm);

        bool outwards_oriented = (vtx[mapped_vertices[vtx_of_face[0]]] - centroid) * norm < 0;
        //outward[std::make_pair(v, j)] = outwards_oriented;

        if (!outwards_oriented)
          std::reverse(vtx_of_face.begin(), vtx_of_face.end());

        /*
                auto p1 = edge_to_volface.emplace(std::make_pair(std::make_pair(mapped_vertices[vtx_of_face[0]], mapped_vertices[vtx_of_face[1]]), std::make_pair(v, j)));
                if (!p1.second) {
                  std::size_t first = mapped_vertices[vtx_of_face[0]];
                  std::size_t second = mapped_vertices[vtx_of_face[1]];
                  auto p = edge_to_volface[std::make_pair(first, second)];
                  auto o1 = outward[p];
                  auto o2 = outward[std::make_pair(v, j)];
                }

                for (std::size_t k = 1; k < vtx_of_face.size() - 1; k++) {
                  auto p = edge_to_volface.emplace(std::make_pair(std::make_pair(mapped_vertices[vtx_of_face[k]], mapped_vertices[vtx_of_face[k + 1]]), std::make_pair(v, j)));
                  if (!p.second) {
                    std::size_t first = mapped_vertices[vtx_of_face[k]];
                    std::size_t second = mapped_vertices[vtx_of_face[k + 1]];
                    auto p = edge_to_volface[std::make_pair(first, second)];
                    auto o1 = outward[p];
                    auto o2 = outward[std::make_pair(v, j)];
                  }
                }

                auto p2 = edge_to_volface.emplace(std::make_pair(std::make_pair(mapped_vertices[vtx_of_face.back()], mapped_vertices[vtx_of_face[0]]), std::make_pair(v, j)));
                if (!p2.second) {
                  std::size_t first = mapped_vertices[vtx_of_face.back()];
                  std::size_t second = mapped_vertices[vtx_of_face[0]];
                  auto p = edge_to_volface[std::make_pair(first, second)];
                  auto o1 = outward[p];
                  auto o2 = outward[std::make_pair(v, j)];
                }*/

        for (const auto& v : vtx_of_face) {
          ib.add_vertex_to_facet(static_cast<std::size_t>(mapped_vertices[v]));
          //std::cout << " " << mapped_vertices[v];
          if (!used_vertices[mapped_vertices[v]]) {
            used_vertices[mapped_vertices[v]] = true;
            num_vtx++;
          }
        }

        //std::cout << ")";
        auto face_dart = ib.end_facet(); // returns a dart to the face
        if (lcc.attribute<2>(face_dart) == lcc.null_descriptor) {
          lcc.set_attribute<2>(face_dart, lcc.create_attribute<2>());
          // How to handle bbox planes that coincide with input polygons? Check support plane
          std::size_t sp = m_partition_nodes[faces_of_volume[j].first].m_data->face_to_support_plane()[faces_of_volume[j].second];

          // There are three different cases:
          // 1. face belongs to a plane from an input polygon
          // 2. face originates from octree splitting (and does not have an input plane)
          // 3. face lies on the bbox
          int ip = m_partition_nodes[faces_of_volume[j].first].m_data->support_plane(sp).data().actual_input_polygon;
          if (ip != -1)
            lcc.info<2>(face_dart).input_polygon_index = m_partition_nodes[faces_of_volume[j].first].input_polygons[ip];
          else {
            // If there is no input polygon, I need to check whether it has two neighbors
            auto n = neighbors(faces_of_volume[j]);
            if (n.second >= 0)
              lcc.info<2>(face_dart).input_polygon_index = -7;
            else
              lcc.info<2>(face_dart).input_polygon_index = n.second;
          }
          lcc.info<2>(face_dart).part_of_initial_polygon = m_partition_nodes[faces_of_volume[j].first].m_data->face_is_part_of_input_polygon()[faces_of_volume[j].second];
          lcc.info<2>(face_dart).plane = m_partition_nodes[faces_of_volume[j].first].m_data->support_plane(m_partition_nodes[faces_of_volume[j].first].m_data->face_to_support_plane()[faces_of_volume[j].second]).exact_plane();
        }
        else {
          assert(lcc.info<2>(face_dart).part_of_initial_polygon == m_partition_nodes[faces_of_volume[j].first].m_data->face_is_part_of_input_polygon()[faces_of_volume[j].second]);
        }

        vtx_of_face.clear();
      }

      d = ib.end_surface();

      lcc.set_attribute<3>(d, lcc.create_attribute<3>());
      lcc.info<3>(d).barycenter = centroid;
      lcc.info<3>(d).volume_id = v;

      std::size_t unused = 0;

      faces_of_volume.clear();
    }

    // Todo: Remove check if all volumes were added
    for (std::size_t i = 0; i < added_volumes.size(); i++)
      if (!added_volumes[i])
        std::cout << "volume " << i << " has not been added" << std::endl;

    std::cout << "lcc #volumes: " << lcc.one_dart_per_cell<3>().size() << " ksp #volumes: " << number_of_volumes() << std::endl;
    std::cout << "lcc #faces: " << lcc.one_dart_per_cell<2>().size() << " ksp #faces: " << num_faces << std::endl;
    std::cout << "lcc #n-edges: " << lcc.one_dart_per_cell<1>().size() << std::endl;
    std::cout << "lcc #vtx: " << lcc.one_dart_per_cell<0>().size() << " ksp #vtx: " << vtx.size() << std::endl;

    // Verification
    // Check attributes in each dart
    for (auto& d : lcc.one_dart_per_cell<0>()) {
      if (!lcc.is_dart_used(lcc.dart_descriptor(d))) {
        std::cout << "unused dart in 0" << std::endl;
      }
    }
    for (auto& d : lcc.one_dart_per_cell<1>()) {
      if (!lcc.is_dart_used(lcc.dart_descriptor(d))) {
        std::cout << "unused dart in 1" << std::endl;
      }
    }
    for (auto& d : lcc.one_dart_per_cell<2>()) {
      if (!lcc.is_dart_used(lcc.dart_descriptor(d))) {
        std::cout << "unused dart in 2" << std::endl;
      }
    }
    for (auto& d : lcc.one_dart_per_cell<3>()) {
      if (!lcc.is_dart_used(lcc.dart_descriptor(d))) {
        std::cout << "unused dart in 3" << std::endl;
      }
    }

    lcc.display_characteristics(std::cout) << std::endl;

    if (!lcc.is_valid())
      std::cout << "LCC is not valid" << std::endl;
  }

  /// @}

  /*******************************
  **           MEMORY           **
  ********************************/
  /*!
   \brief clears all input data and the kinetic partition.
   */
  void clear() {
    m_data.clear();
    m_num_events = 0;
  }

private:
  struct Constraint_info {
    typename CDTplus::Constraint_id id_single, id_merged, id_overlay;
    std::size_t volume;
    Index vA, vB;
  };

  const Point_3& volume_centroid(std::size_t volume_index) const {
    assert(volume_index < m_volumes.size());
    auto p = m_volumes[volume_index];
    return m_partition_nodes[p.first].m_data->volumes()[p.second].centroid;
  }


  template<class OutputIterator>
  void faces_of_input_polygon(const std::size_t polygon_index, OutputIterator it) const {
    if (polygon_index >= m_input_planes.size()) {
      assert(false);
    }

    //std::cout << "switch to Data_structure::m_face2sp" << std::endl;

    for (std::size_t idx : m_partitions) {
      const Sub_partition& p = m_partition_nodes[idx];
      // Check if it contains this input polygon and get support plane index
      int sp_idx = -1;
      for (std::size_t i = 0; i < p.input_polygons.size(); i++) {
        if (p.input_polygons[i] == polygon_index) {
          sp_idx = p.m_data->support_plane_index(i);
          break;
        }
      }

      // Continue if the partition does not contain this input polygon.
      if (sp_idx == -1)
        continue;

      auto pfaces = p.m_data->pfaces(sp_idx);
      auto f2i = p.m_data->face_to_index();
      const auto& f2sp = p.m_data->face_to_support_plane();

      for (std::size_t i = 0; i < f2sp.size(); i++) {
        if (f2sp[i] == sp_idx)
          *it++ = std::make_pair(idx, i);
      }
    }
  }

  const std::vector<std::vector<std::size_t> >& input_mapping() const {
    return m_regularized2input;
  }

  /*!
  \brief Face indices of the volume.

  \param volume_index
   index of the query volume.

  @return
   vector of face indices.

  \pre created partition
  */
  template<class OutputIterator>
  void faces(std::size_t volume_index, OutputIterator it) const {
    CGAL_assertion(m_volumes.size() > volume_index);
    auto p = m_volumes[volume_index];

    for (std::size_t i : m_partition_nodes[p.first].m_data->volumes()[p.second].faces)
      *it++ = std::make_pair(p.first, i);
  }


  /*!
  \brief Mapping of a vertex index to its position.

  @return
   vector of points.

    \pre created partition
  */
  const Point_3& vertex(const Index& vertex_index) const {
    return m_partition_nodes[vertex_index.first].m_data->vertices()[vertex_index.second];
  }

  /*!
  \brief Mapping of a vertex index to its exact position.

  @return
   vector of points.

    \pre created partition
  */
  const typename Intersection_kernel::Point_3& exact_vertex(const Index& vertex_index) const {
    return m_partition_nodes[vertex_index.first].m_data->exact_vertices()[vertex_index.second];
  }

  /*!
  \brief Vertices of a face.

  \param volume_index
   index of the query volume.

  @return
   vector of face indices.

  \pre successful partition
  */
  template<class OutputIterator>
  void vertices(const Index& face_index, OutputIterator it) const {
    for (auto& p : m_partition_nodes[face_index.first].face2vertices[face_index.second])
      *it++ = m_partition_nodes[p.first].m_data->vertices()[p.second];
  }

  template<class OutputIterator>
  void vertex_indices(const Index& face_index, OutputIterator it) const {
    for (auto& p : m_partition_nodes[face_index.first].face2vertices[face_index.second])
      *it++ = p;
  }

  /*!
  \brief Vertices of a face.

  \param volume_index
   index of the query volume.

  @return
   vector of face indices.

  \pre successful partition
  */
  template<class OutputIterator>
  void exact_vertices(const Index& face_index, OutputIterator it) const {

    for (auto& p : m_partition_nodes[face_index.first].face2vertices[face_index.second])
      *it++ = m_partition_nodes[p.first].m_data->exact_vertices()[p.second];
  }

  template<class OutputIterator, class IndexOutputIterator>
  void exact_vertices(const Index& face_index, OutputIterator pit, IndexOutputIterator iit) const {
    for (auto& p : m_partition_nodes[face_index.first].face2vertices[face_index.second]) {
      *iit++ = p;
      *pit++ = m_partition_nodes[p.first].m_data->exact_vertices()[p.second];
    }
  }


  /*!
  \brief Indices of adjacent volumes. Negative indices correspond to the empty spaces behind the sides of the bounding box.

    \param face_index
    index of the query face.

    @return
    pair of adjacent volumes.

    -1 zmin
    -2 ymin
    -3 xmax
    -4 ymax
    -5 xmin
    -6 zmax

    \pre successful partition
  */
  std::pair<int, int> neighbors(const Index &face_index) const {
    const auto &p = m_partition_nodes[face_index.first].face_neighbors[face_index.second];
    if (p.second.second >= std::size_t(-6)) { // Faces on the boundary box are neighbors with an infinite outside volume
      auto it = m_index2volume.find(p.first);
      assert(it != m_index2volume.end());
      return std::pair<int, int>(static_cast<int>(it->second), static_cast<int>(p.second.second));
    }
    else {
      auto it1 = m_index2volume.find(p.first);
      assert(it1 != m_index2volume.end());
      auto it2 = m_index2volume.find(p.second);
      assert(it2 != m_index2volume.end());
      return std::pair<int, int>(static_cast<int>(it1->second), static_cast<int>(it2->second));
    }
    //const auto& p = m_partition_nodes[face_index.first].m_data->face_to_volumes()[face_index.second];
    //return std::pair<Index, Index>(std::make_pair(face_index.first, p.first), std::make_pair(face_index.first, p.second));// m_data.face_to_volumes()[face_index];
  }


  void create_bounding_box(
    const FT enlarge_bbox_ratio,
    const bool reorient,
    std::array<Point_3, 8>& bbox) const {

    if (reorient) {
      initialize_optimal_box(bbox);
    }
    else {
      initialize_axis_aligned_box(bbox);
    }

    CGAL_assertion(bbox.size() == 8);

    enlarge_bounding_box(enlarge_bbox_ratio, bbox);

    const auto& minp = bbox.front();
    const auto& maxp = bbox.back();
    if (m_parameters.verbose) {
      std::cout.precision(20);
      std::cout << "* bounding box minp: " << std::fixed <<
        minp.x() << "\t, " << minp.y() << "\t, " << minp.z() << std::endl;
    }
    if (m_parameters.verbose) {
      std::cout.precision(20);
      std::cout << "* bounding box maxp: " << std::fixed <<
        maxp.x() << "\t, " << maxp.y() << "\t, " << maxp.z() << std::endl;
    }
  }

  void initialize_optimal_box(
    std::array<Point_3, 8>& bbox) const {
    /*

    // Number of input points.
    std::size_t num_points = 0;
    for (const auto& poly : m_input_polygons) {
      num_points += poly.size();
    }

    // Set points.
    std::vector<Point_3> points;
    points.reserve(num_points);
    for (const auto& poly : m_input_polygons) {
      for (const auto& point : poly) {
        const Point_3 ipoint(
          static_cast<FT>(CGAL::to_double(point.x())),
          static_cast<FT>(CGAL::to_double(point.y())),
          static_cast<FT>(CGAL::to_double(point.z())));
        points.push_back(ipoint);
      }
    }

    // Compute optimal bbox.
    // The order of faces corresponds to the standard order from here:
    // https://doc.cgal.org/latest/BGL/group__PkgBGLHelperFct.html#gad9df350e98780f0c213046d8a257358e
    const OBB_traits obb_traits;
    std::array<Point_3, 8> ibbox;
    CGAL::oriented_bounding_box(
      points, ibbox,
      CGAL::parameters::use_convex_hull(true).
      geom_traits(obb_traits));

    for (std::size_t i = 0; i < 8; ++i) {
      const auto& ipoint = ibbox[i];
      const Point_3 point(
        static_cast<FT>(ipoint.x()),
        static_cast<FT>(ipoint.y()),
        static_cast<FT>(ipoint.z()));
      bbox[i] = point;
    }

    const FT bbox_length_1 = KSR::distance(bbox[0], bbox[1]);
    const FT bbox_length_2 = KSR::distance(bbox[0], bbox[3]);
    const FT bbox_length_3 = KSR::distance(bbox[0], bbox[5]);
    CGAL_assertion(bbox_length_1 >= FT(0));
    CGAL_assertion(bbox_length_2 >= FT(0));
    CGAL_assertion(bbox_length_3 >= FT(0));
    const FT tol = KSR::tolerance<FT>();
    if (bbox_length_1 < tol || bbox_length_2 < tol || bbox_length_3 < tol) {
      if (m_parameters.verbose) {
        std::cout << "* warning: optimal bounding box is flat, reverting ..." << std::endl;
      }
      initialize_axis_aligned_box(bbox);
    }
    else {
      if (m_parameters.verbose) {
        std::cout << "* using optimal bounding box" << std::endl;
      }
    }*/
  }

  void initialize_axis_aligned_box(
    std::array<Point_3, 8>& bbox) const {


    Bbox_3 box;
    for (const auto& poly : m_input_polygons) {
      box += CGAL::bbox_3(poly.begin(), poly.end());
    }

    // The order of faces corresponds to the standard order from here:
    // https://doc.cgal.org/latest/BGL/group__PkgBGLHelperFct.html#gad9df350e98780f0c213046d8a257358e
    bbox = {
      Point_3(box.xmin(), box.ymin(), box.zmin()),
      Point_3(box.xmax(), box.ymin(), box.zmin()),
      Point_3(box.xmax(), box.ymax(), box.zmin()),
      Point_3(box.xmin(), box.ymax(), box.zmin()),
      Point_3(box.xmin(), box.ymax(), box.zmax()),
      Point_3(box.xmin(), box.ymin(), box.zmax()),
      Point_3(box.xmax(), box.ymin(), box.zmax()),
      Point_3(box.xmax(), box.ymax(), box.zmax()) };

    const FT bbox_length_1 = KSR::distance(bbox[0], bbox[1]);
    const FT bbox_length_2 = KSR::distance(bbox[0], bbox[3]);
    const FT bbox_length_3 = KSR::distance(bbox[0], bbox[5]);
    CGAL_assertion(bbox_length_1 >= FT(0));
    CGAL_assertion(bbox_length_2 >= FT(0));
    CGAL_assertion(bbox_length_3 >= FT(0));
    const FT tol = KSR::tolerance<FT>();
    if (bbox_length_1 < tol || bbox_length_2 < tol || bbox_length_3 < tol) {
      const FT d = 0.1;

      if (bbox_length_1 < tol) { // yz case
        CGAL_assertion_msg(bbox_length_2 >= tol, "ERROR: DEGENERATED INPUT POLYGONS!");
        CGAL_assertion_msg(bbox_length_3 >= tol, "ERROR: DEGENERATED INPUT POLYGONS!");

        bbox[0] = Point_3(bbox[0].x() - d, bbox[0].y() - d, bbox[0].z() - d);
        bbox[3] = Point_3(bbox[3].x() - d, bbox[3].y() + d, bbox[3].z() - d);
        bbox[4] = Point_3(bbox[4].x() - d, bbox[4].y() + d, bbox[4].z() + d);
        bbox[5] = Point_3(bbox[5].x() - d, bbox[5].y() - d, bbox[5].z() + d);

        bbox[1] = Point_3(bbox[1].x() + d, bbox[1].y() - d, bbox[1].z() - d);
        bbox[2] = Point_3(bbox[2].x() + d, bbox[2].y() + d, bbox[2].z() - d);
        bbox[7] = Point_3(bbox[7].x() + d, bbox[7].y() + d, bbox[7].z() + d);
        bbox[6] = Point_3(bbox[6].x() + d, bbox[6].y() - d, bbox[6].z() + d);
        if (m_parameters.verbose) {
          std::cout << "* setting x-based flat axis-aligned bounding box" << std::endl;
        }

      }
      else if (bbox_length_2 < tol) { // xz case
        CGAL_assertion_msg(bbox_length_1 >= tol, "ERROR: DEGENERATED INPUT POLYGONS!");
        CGAL_assertion_msg(bbox_length_3 >= tol, "ERROR: DEGENERATED INPUT POLYGONS!");

        bbox[0] = Point_3(bbox[0].x() - d, bbox[0].y() - d, bbox[0].z() - d);
        bbox[1] = Point_3(bbox[1].x() + d, bbox[1].y() - d, bbox[1].z() - d);
        bbox[6] = Point_3(bbox[6].x() + d, bbox[6].y() - d, bbox[6].z() + d);
        bbox[5] = Point_3(bbox[5].x() - d, bbox[5].y() - d, bbox[5].z() + d);

        bbox[3] = Point_3(bbox[3].x() - d, bbox[3].y() + d, bbox[3].z() - d);
        bbox[2] = Point_3(bbox[2].x() + d, bbox[2].y() + d, bbox[2].z() - d);
        bbox[7] = Point_3(bbox[7].x() + d, bbox[7].y() + d, bbox[7].z() + d);
        bbox[4] = Point_3(bbox[4].x() - d, bbox[4].y() + d, bbox[4].z() + d);
        if (m_parameters.verbose) {
          std::cout << "* setting y-based flat axis-aligned bounding box" << std::endl;
        }

      }
      else if (bbox_length_3 < tol) { // xy case
        CGAL_assertion_msg(bbox_length_1 >= tol, "ERROR: DEGENERATED INPUT POLYGONS!");
        CGAL_assertion_msg(bbox_length_2 >= tol, "ERROR: DEGENERATED INPUT POLYGONS!");

        bbox[0] = Point_3(bbox[0].x() - d, bbox[0].y() - d, bbox[0].z() - d);
        bbox[1] = Point_3(bbox[1].x() + d, bbox[1].y() - d, bbox[1].z() - d);
        bbox[2] = Point_3(bbox[2].x() + d, bbox[2].y() + d, bbox[2].z() - d);
        bbox[3] = Point_3(bbox[3].x() - d, bbox[3].y() + d, bbox[3].z() - d);

        bbox[5] = Point_3(bbox[5].x() - d, bbox[5].y() - d, bbox[5].z() + d);
        bbox[6] = Point_3(bbox[6].x() + d, bbox[6].y() - d, bbox[6].z() + d);
        bbox[7] = Point_3(bbox[7].x() + d, bbox[7].y() + d, bbox[7].z() + d);
        bbox[4] = Point_3(bbox[4].x() - d, bbox[4].y() + d, bbox[4].z() + d);
        if (m_parameters.verbose) {
          std::cout << "* setting z-based flat axis-aligned bounding box" << std::endl;
        }

      }
      else {
        CGAL_assertion_msg(false, "ERROR: WRONG CASE!");
      }
    }
    else {
      if (m_parameters.verbose) {
        std::cout << "* using axis-aligned bounding box" << std::endl;
      }
    }
  }

  void enlarge_bounding_box(
    const FT enlarge_bbox_ratio,
    std::array<Point_3, 8>& bbox) const {

    FT enlarge_ratio = enlarge_bbox_ratio;
    const FT tol = KSR::tolerance<FT>();
    if (enlarge_bbox_ratio == FT(1)) {
      enlarge_ratio += FT(2) * tol;
    }

    const auto a = CGAL::centroid(bbox.begin(), bbox.end());
    Transform_3 scale(CGAL::Scaling(), enlarge_ratio);
    for (auto& point : bbox)
      point = scale.transform(point);

    const auto b = CGAL::centroid(bbox.begin(), bbox.end());
    Transform_3 translate(CGAL::Translation(), a - b);
    for (auto& point : bbox)
      point = translate.transform(point);
  }

  void process_input_polygon(const std::vector<Point_3> poly, Plane_3& pl, Point_2& c, std::vector<Point_2>& ch) const {
    CGAL::linear_least_squares_fitting_3(poly.begin(), poly.end(), pl, CGAL::Dimension_tag<0>());

    std::vector<Point_2> pts2d(poly.size());
    for (std::size_t i = 0; i < poly.size(); i++)
      pts2d[i] = pl.to_2d(poly[i]);

    ch.clear();
    CGAL::convex_hull_2(pts2d.begin(), pts2d.end(), std::back_inserter(ch));

    // Centroid
    FT x = 0, y = 0, w = 0;
    for (std::size_t i = 2; i < ch.size(); i++) {
      Triangle_2 tri(ch[0], ch[i - 1], ch[i]);
      w += CGAL::area(ch[0], ch[i - 1], ch[i]);
      Point_2 c = CGAL::centroid(ch[0], ch[i - 1], ch[i]);
      x += c.x() * w;
      y += c.y() * w;
    }

    c = Point_2(x / w, y / w);
  }

  std::pair<int, int> make_canonical_pair(int i, int j)
  {
    if (i > j) return std::make_pair(j, i);
    return std::make_pair(i, j);
  }

  double build_cdt(CDTplus& cdt, std::vector<Index>& faces, std::vector<std::vector<Constraint_info> >&constraints, const typename Intersection_kernel::Plane_3& plane) {
    double area = 0;
    From_exact from_exact;
    To_exact to_exact;

    cdt.clear();
    //keep track of constraints when inserting to iterate later
    constraints.resize(faces.size());

    //check orientation of faces so that they are ccw oriented
    std::vector<std::vector<Index> > pts_idx(faces.size());
    std::vector<std::vector<typename Intersection_kernel::Point_3> > pts(faces.size());
    for (std::size_t i = 0; i < faces.size(); ++i) {
      exact_vertices(faces[i], std::back_inserter(pts[i]), std::back_inserter(pts_idx[i]));
      constraints[i].resize(pts[i].size());
      //auto& v = faces[i];

      std::size_t j = 0;

      CGAL::Orientation res = CGAL::COLLINEAR;
      bool pos = false;
      bool neg = false;

      for (std::size_t j = 0; j < pts[i].size(); j++) {
        std::size_t k = (j + 1) % pts[i].size();
        std::size_t l = (k + 1) % pts[i].size();

        Point_2 pj = from_exact(plane.to_2d(pts[i][j]));
        Point_2 pk = from_exact(plane.to_2d(pts[i][k]));
        Point_2 pl = from_exact(plane.to_2d(pts[i][l]));

        res = orientation(plane.to_2d(pts[i][j]), plane.to_2d(pts[i][k]), plane.to_2d(pts[i][l]));
        if (res == CGAL::LEFT_TURN)
          pos = true;
        if (res == CGAL::RIGHT_TURN)
          neg = true;
      }

      if (pos && neg) {
        std::cout << "face is not convex" << std::endl;
        exit(1);
      }

      if (!pos && !neg) {
        std::cout << "face is degenerated" << std::endl;
        exit(1);
      }

      if (neg) {
        std::reverse(pts[i].begin(), pts[i].end());
        std::reverse(pts_idx[i].begin(), pts_idx[i].end());
      }
    }

    std::map<Index, std::size_t> face2vtx;
    std::map<std::size_t, Index> vtx2face;
    std::vector<Vertex_handle> vertices;
    for (std::size_t f = 0; f < faces.size(); f++)
      for (std::size_t v = 0; v < pts_idx[f].size(); v++) {
        //vertices.push_back(cdt.insert(to_exact(from_exact(plane.to_2d(pts[f][v])))));
        vertices.push_back(cdt.insert(plane.to_2d(pts[f][v])));

        if (vertices.back()->info().idA2.first != -1 && vertices.back()->info().idA2 != pts_idx[f][v]) {
          std::cout << "build_cdt faces has non-unique vertices" << std::endl;
        }

        vertices.back()->info().idA2 = pts_idx[f][v];
        assert(pts_idx[f][v].first != -1);
        assert(pts_idx[f][v].second != -1);
        vertices.back()->info().adjacent.insert(faces[f]);
        vertices.back()->info().set_point(pts[f][v]);
        face2vtx[pts_idx[f][v]] = vertices.size() - 1;
        vtx2face[vertices.size() - 1] = pts_idx[f][v];
      }

    typedef std::set<std::pair<int, int> > Edges;
    Edges edges;

    // Iterating over each face and inserting each edge as a constraint.
    for (std::size_t i = 0; i < pts_idx.size(); ++i) {
      auto& v = pts_idx[i];
      for (std::size_t j = 0; j < v.size(); ++j) {
        int vj = face2vtx[v[j]];
        int vjj = face2vtx[v[(j + 1) % v.size()]];
        std::pair<Edges::iterator, bool> res = edges.insert(make_canonical_pair(vj, vjj));
#ifdef OVERLAY_2_DEBUG
        int vjjj = face2vtx[v[(j + 2) % v.size()]];
        if (orientation(vertices[vj]->point(), vertices[vjj]->point(), vertices[vjjj]->point()) != CGAL::LEFT_TURN) {
          std::cerr << "orientation( " << vertices[vj]->point() << ", " << vertices[vjj]->point() << ", " << vertices[vjjj]->point() << std::endl;
          std::cerr << orientation(vertices[vj]->point(), vertices[vjj]->point(), vertices[vjjj]->point()) << std::endl;
        }
#endif
        if (res.second) {
          constraints[i][j].id_single = cdt.insert_constraint(vertices[vj], vertices[vjj]);
          auto p = neighbors(faces[i]);
          if (p.second >= 0)
            std::cout << "p.second is positive" << std::endl;
          if (p.first < 0)
            std::cout << "p.first is negative" << std::endl;
          constraints[i][j].volume = p.first;
          constraints[i][j].vA = v[j];
          constraints[i][j].vB = v[(j + 1) % v.size()];
        }
      }
    }

    for (CDTplus::Finite_faces_iterator fit = cdt.finite_faces_begin(); fit != cdt.finite_faces_end(); ++fit) {
#ifdef OVERLAY_2_CHECK
      Point_2 p = from_exact(fit->vertex(0)->point());
      Point_2 q = from_exact(fit->vertex(1)->point());
      Point_2 r = from_exact(fit->vertex(2)->point());
      area += CGAL::area(p, q, r);
#endif

      std::set<Index>& a(fit->vertex(0)->info().adjacent), & b(fit->vertex(1)->info().adjacent), & c(fit->vertex(2)->info().adjacent);

      std::set<Index> res, res2;
      Index common(std::size_t(-1), std::size_t(-1));
      std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(res, res.begin()));
      std::set_intersection(res.begin(), res.end(), c.begin(), c.end(), std::inserter(res2, res2.begin()));

      if (res2.size() != 1) {
        std::cout << "build_cdt: face assignment not unique!" << std::endl;
        const std::string vfilename = "no-face.polylines.txt";
        std::ofstream vout(vfilename);
        vout.precision(20);
        vout << 4;
        vout << " " << from_exact(plane.to_3d(fit->vertex(0)->point()));
        vout << " " << from_exact(plane.to_3d(fit->vertex(1)->point()));
        vout << " " << from_exact(plane.to_3d(fit->vertex(2)->point()));
        vout << " " << from_exact(plane.to_3d(fit->vertex(0)->point()));
        vout << std::endl;
        vout.close();
      }
      else fit->info().id2 = *res2.begin();
    }

    return area;
  }

  bool check_index(const Index& a) const {
    if (a == Index(0, 16) || a == Index(1, 16)
      || a == Index(0, 17) || a == Index(1, 17)
      || a == Index(4, 17) || a == Index(5, 12)
      || a == Index(4, 33) || a == Index(5, 37))
      return true;
    return false;
  }

  void check_tjunctions() {
    std::map<Index, std::vector<Index> > vertex2neighbors;

    for (std::size_t v = 0; v < m_volumes.size(); v++) {
      auto &vp = m_volumes[v];
      for (const std::size_t f : m_partition_nodes[vp.first].m_data->volumes()[vp.second].faces) {
        auto& vtx = m_partition_nodes[vp.first].face2vertices[f];
        for (std::size_t i = 0; i < vtx.size(); i++) {
          vertex2neighbors[vtx[i]].push_back(vtx[(i + 1) % vtx.size()]);
          vertex2neighbors[vtx[i]].push_back(vtx[(i - 1 + vtx.size()) % vtx.size()]);
        }
      }
    }

    for (auto& p : vertex2neighbors) {
      typename Intersection_kernel::Point_3 a = m_partition_nodes[p.first.first].m_data->exact_vertices()[p.first.second];
      //Check pairwise collinear
      for (std::size_t i = 0; i < p.second.size(); i++) {
        typename Intersection_kernel::Point_3 b = m_partition_nodes[p.second[i].first].m_data->exact_vertices()[p.second[i].second];
        for (std::size_t j = i + 1; j < p.second.size(); j++) {
          if (p.second[i] == p.second[j])
            continue;
          typename Intersection_kernel::Point_3 c = m_partition_nodes[p.second[j].first].m_data->exact_vertices()[p.second[j].second];
          if (CGAL::collinear(a, b, c) && ((b - a) * (c - a) > 0)) {
            std::cout << "non-manifold v (" << p.first.first << ", " << p.first.second << ")" << std::endl;
            std::cout << " v (" << p.second[i].first << ", " << p.second[i].second << ")" << std::endl;
            std::cout << " v (" << p.second[j].first << ", " << p.second[j].second << ")" << std::endl;

            From_exact from_exact;

            std::ofstream vout2("a.xyz");
            vout2.precision(20);
            vout2 << from_exact(a) << std::endl;
            vout2.close();
            std::ofstream vout3("b.xyz");
            vout3.precision(20);
            vout3 << from_exact(b) << std::endl;
            vout3.close();
            std::ofstream vout4("c.xyz");
            vout4.precision(20);
            vout4 << from_exact(c) << std::endl;
            vout4.close();

            for (std::size_t v = 0; v < m_volumes.size(); v++) {
              auto& vp = m_volumes[v];
              for (const std::size_t f : m_partition_nodes[vp.first].m_data->volumes()[vp.second].faces) {
                auto& vtx = m_partition_nodes[vp.first].face2vertices[f];
                bool hasa = false, hasb = false, hasc = false;
                for (std::size_t k = 0; k < vtx.size(); k++) {
                  if (vtx[k] == p.first)
                    hasa = true;
                  if (vtx[k] == p.second[i])
                    hasb = true;
                  if (vtx[k] == p.second[j])
                    hasc = true;
                }

                if (hasa && (hasb || hasc)) {
                  const std::string vfilename = std::to_string(v) + " " + std::to_string(f) + "-non_manifold.polylines.txt";
                  std::ofstream vout(vfilename);
                  vout.precision(20);
                  vout << vtx.size() + 1;
                  for (const auto& v : vtx) {
                    vout << " " << from_exact(m_partition_nodes[v.first].m_data->exact_vertices()[v.second]);
                  }

                  vout << " " << from_exact(m_partition_nodes[vtx[0].first].m_data->exact_vertices()[vtx[0].second]);

                  vout << std::endl;
                  vout.close();
                }
              }
            }
            std::cout << std::endl;
          }
        }
      }
    }
  }

  void insert_map(const Index& a, const Index& b, std::map<Index, Index>& pm) const {
    if (a == b)
      return;

    Index target = b;
    auto it = pm.find(b);
    if (it != pm.end())
      target = it->second;
    pm[a] = target;
  }

  double build_cdt(CDTplus& cdt, std::vector<CDTplus>& partitions,
    std::vector<std::vector<std::vector<Constraint_info> > >& constraints,
    const typename Intersection_kernel::Plane_3& plane) {
    if (partitions.size() == 0)
      return 0;

    double area = 0;

    From_exact from_exact;
    //To_exact to_exact;
    //cdt = partitions[0];

    for (std::size_t i = 0; i < partitions.size(); i++) {
      std::vector<Vertex_handle> vertices;
      vertices.reserve(6);

      for (std::size_t j = 0; j < constraints[i].size(); j++)
        for (std::size_t k = 0; k < constraints[i][j].size(); k++) {
          if (constraints[i][j][k].id_single == 0)
            continue;
          for (typename CDTplus::Vertices_in_constraint_iterator vi = partitions[i].vertices_in_constraint_begin(constraints[i][j][k].id_single); vi != partitions[i].vertices_in_constraint_end(constraints[i][j][k].id_single); vi++) {
            vertices.push_back(*vi);
          }
          /*

                for (typename CDTplus::Constraint_iterator ci = partitions[i].constraints_begin(); ci != partitions[i].constraints_end(); ++ci) {
                  for (typename CDTplus::Vertices_in_constraint_iterator vi = partitions[i].vertices_in_constraint_begin(*ci); vi != partitions[i].vertices_in_constraint_end(*ci); vi++) {
                    vertices.push_back(*vi);
                  }*/

          // Insert constraints and replacing vertex handles in vector while copying data.
          VI tmp = vertices[0]->info();
          vertices[0] = cdt.insert(vertices[0]->point());
          vertices[0]->info() = tmp;

          tmp = vertices.back()->info();
          vertices.back() = cdt.insert(vertices.back()->point());
          vertices.back()->info() = tmp;

          constraints[i][j][k].id_merged = cdt.insert_constraint(vertices[0], vertices.back());

          vertices.clear();
        }
    }

    // Generate 3D points corresponding to the intersections
    std::size_t newpts = 0;
    for (typename CDTplus::Finite_vertices_iterator vit = cdt.finite_vertices_begin(); vit != cdt.finite_vertices_end(); ++vit) {
      if (!vit->info().input) {
        vit->info().point_3 = plane.to_3d(vit->point());
        vit->info().idA2 = vit->info().idB2 = Index(-1, -1);
        newpts++;
      }
    }

    //std::cout << newpts << " new vertices added in build_cdt from cdts" << std::endl;

    for (CDTplus::Finite_faces_iterator fit = cdt.finite_faces_begin(); fit != cdt.finite_faces_end(); ++fit) {
#ifdef OVERLAY_2_CHECK
      Point_2 p = from_exact(fit->vertex(0)->point());
      Point_2 q = from_exact(fit->vertex(1)->point());
      Point_2 r = from_exact(fit->vertex(2)->point());
      area += CGAL::area(p, q, r);
#endif

      Index idx(std::size_t(-1), std::size_t(-1));

      typename Intersection_kernel::Point_2 pt = CGAL::centroid(fit->vertex(0)->point(), fit->vertex(1)->point(), fit->vertex(2)->point());
      for (std::size_t i = 0; i < partitions.size(); i++) {
        typename CDTplus::Face_handle fh = partitions[i].locate(pt);

        if (!partitions[i].is_infinite(fh)) {
          if (fh->info().id2 != std::make_pair(std::size_t(-1), std::size_t(-1)))
            idx = fit->info().id2 = fh->info().id2;
          else
            std::cout << "Face id is missing " << std::endl;
        }
      }

      if (fit->info().id2.first == std::size_t(-1))
        std::cout << "cdt fusion: no id found" << std::endl;
    }

    return area;
  }

  std::pair<double, double> overlay(CDTplus& cdtC, const CDTplus& cdtA, std::vector<std::vector<std::vector<Constraint_info> > >& constraints_a, const CDTplus& cdtB, std::vector<std::vector<std::vector<Constraint_info> > >& constraints_b, const typename Intersection_kernel::Plane_3& plane) {
    From_exact from_exact;
    //To_exact to_exact;
    std::pair<double, double> result;
    cdtC = cdtA;

    std::vector<Vertex_handle> vertices;
    vertices.reserve(2);

    std::size_t idx = 0;
    for (typename CDTplus::Constraint_iterator ci = cdtC.constraints_begin(); ci != cdtC.constraints_end(); ++ci) {
      for (typename CDTplus::Vertices_in_constraint_iterator vi = cdtC.vertices_in_constraint_begin(*ci); vi != cdtC.vertices_in_constraint_end(*ci); vi++) {
        vertices.push_back(*vi);
      }

      if (vertices.size() >= 2) {
        const std::string vfilename = "cdt/A" + std::to_string(idx) + "-constraint.polylines.txt";
        std::ofstream vout(vfilename);
        vout.precision(20);
        vout << vertices.size();
        for (std::size_t i = 0; i < vertices.size(); i++)
          vout << " " << from_exact(plane.to_3d(vertices[i]->point()));
        vout << std::endl;
        vout.close();
      }

      vertices.clear();
      idx++;
    }

    // Todo: remove?
    idx = 0;
    for (std::size_t i = 0; i < constraints_a.size(); i++)
    for (std::size_t j = 0; j < constraints_a[i].size(); j++)
        for (std::size_t k = 0; k < constraints_a[i][j].size(); k++) {
          if (constraints_a[i][j][k].id_merged == 0) {
            if (constraints_a[i][j][k].id_single != 0)
              constraints_a[i][j][k].id_merged = constraints_a[i][j][k].id_single;
            else
              continue;
          }
          for (typename CDTplus::Vertices_in_constraint_iterator vi = cdtA.vertices_in_constraint_begin(constraints_a[i][j][k].id_merged); vi != cdtA.vertices_in_constraint_end(constraints_a[i][j][k].id_merged); vi++) {
            vertices.push_back(*vi);
          }

          // Insert constraints and replacing vertex handles in vector while copying data.
          VI tmp = vertices[0]->info();
          vertices[0] = cdtC.insert(vertices[0]->point());
          vertices[0]->info() = tmp;

          tmp = vertices.back()->info();
          vertices.back() = cdtC.insert(vertices.back()->point());
          vertices.back()->info() = tmp;

          constraints_a[i][j][k].id_overlay = cdtC.insert_constraint(vertices[0], vertices.back());

          vertices.clear();
          idx++;
        }
    idx = 0;
    //std::size_t idx = 0;
    for (std::size_t i = 0; i < constraints_b.size(); i++)
      for (std::size_t j = 0; j < constraints_b[i].size(); j++)
        for (std::size_t k = 0; k < constraints_b[i][j].size(); k++) {
          if (constraints_b[i][j][k].id_merged == 0) {
            if (constraints_b[i][j][k].id_single != 0)
              constraints_b[i][j][k].id_merged = constraints_b[i][j][k].id_single;
            else
              continue;
          }
          for (typename CDTplus::Vertices_in_constraint_iterator vi = cdtB.vertices_in_constraint_begin(constraints_b[i][j][k].id_merged); vi != cdtB.vertices_in_constraint_end(constraints_b[i][j][k].id_merged); vi++) {
            vertices.push_back(*vi);
          }

          if (vertices.size() >= 2) {
            const std::string vfilename = "cdt/B" + std::to_string(idx) + "-constraint.polylines.txt";
            std::ofstream vout(vfilename);
            vout.precision(20);
            vout << vertices.size();
            for (std::size_t i = 0; i < vertices.size(); i++)
              vout << " " << from_exact(plane.to_3d(vertices[i]->point()));
            vout << std::endl;
            vout.close();
          }

          // Insert constraints and replacing vertex handles in vector while copying data.
          VI tmp = vertices[0]->info();
          vertices[0] = cdtC.insert(vertices[0]->point());
          vertices[0]->info() = tmp;

          tmp = vertices.back()->info();
          vertices.back() = cdtC.insert(vertices.back()->point());
          vertices.back()->info() = tmp;
/*
          for (std::size_t i = 0; i < vertices.size(); i++) {
            VI tmp = vertices[i]->info();
            vertices[i] = cdtC.insert(vertices[i]->point());

            check_index(vertices[i]->info().idA2);
            check_index(tmp.idA2);
            if (vertices[i]->info().idA2.first == -1)
              std::cout << "overlay: inserting vertex without vertex index" << std::endl;
            std::copy(tmp.adjacent.begin(), tmp.adjacent.end(), std::inserter(vertices[i]->info().adjacent, vertices[i]->info().adjacent.begin()));
            vertices[i]->info().idB2 = tmp.idA2;
            vertices[i]->info().input |= tmp.input;
          }*/

/*
          if (vertices.size() > 2) {
            for (std::size_t j = 2; j < vertices.size(); j++)
              if (!CGAL::collinear(vertices[j - 2]->point(), vertices[j - 1]->point(), vertices[j]->point())) {
                Point_2 a = from_exact(vertices[j - 2]->point());
                Point_2 b = from_exact(vertices[j - 1]->point());
                Point_2 c = from_exact(vertices[j]->point());
                std::cout << from_exact(vertices[j - 2]->point()) << std::endl;
                std::cout << from_exact(vertices[j - 1]->point()) << std::endl;
                std::cout << from_exact(vertices[j]->point()) << std::endl;
                std::cout << ((a.x() - b.x()) / (a.x() - c.x())) << " " << ((a.y() - b.y()) / (a.y() - c.y())) << std::endl;
                std::cout << "constraints not linear" << std::endl;
              }
          }*/

          constraints_b[i][j][k].id_overlay = cdtC.insert_constraint(vertices[0], vertices.back());

          vertices.clear();
          idx++;
        }

    idx = 0;
    for (typename CDTplus::Constraint_iterator ci = cdtC.constraints_begin(); ci != cdtC.constraints_end(); ++ci) {
      for (typename CDTplus::Vertices_in_constraint_iterator vi = cdtC.vertices_in_constraint_begin(*ci); vi != cdtC.vertices_in_constraint_end(*ci); vi++) {
        vertices.push_back(*vi);
      }

      if (vertices.size() >= 2) {
        const std::string vfilename = "cdt/C" + std::to_string(idx) + "-constraint.polylines.txt";
        std::ofstream vout(vfilename);
        vout.precision(20);
        vout << vertices.size();
        for (std::size_t i = 0; i < vertices.size(); i++)
          vout << " " << from_exact(plane.to_3d(vertices[i]->point()));
        vout << std::endl;
        vout.close();
      }
      vertices.clear();
      idx++;
    }

    std::size_t newpts = 0;
    // Generate 3D points corresponding to the intersections
    //std::ofstream vout3("newpts.xyz");
    //vout3.precision(20);
    for (typename CDTplus::Finite_vertices_iterator vit = cdtC.finite_vertices_begin(); vit != cdtC.finite_vertices_end(); ++vit) {
      if (!vit->info().input) {
        vit->info().point_3 = plane.to_3d(vit->point());
        vit->info().idA2 = vit->info().idB2 = Index(-1, -1);
        //vout3 << " " << from_exact(vit->info().point_3) << std::endl;
        newpts++;
      }
    }

    for (typename CDTplus::Finite_faces_iterator cit = cdtC.finite_faces_begin(); cit != cdtC.finite_faces_end(); ++cit) {
      double a = 0;
      cit->info().id2 = std::make_pair(-1, -1);
#ifdef OVERLAY_2_CHECK
      Point_2 ap = from_exact(cit->vertex(0)->point());
      Point_2 aq = from_exact(cit->vertex(1)->point());
      Point_2 ar = from_exact(cit->vertex(2)->point());
      a = CGAL::area(ap, aq, ar);
#endif
      typename Intersection_kernel::Point_2 p = CGAL::centroid(cit->vertex(0)->point(), cit->vertex(1)->point(), cit->vertex(2)->point());
      typename CDTplus::Face_handle fhA = cdtA.locate(p);

      if (cdtA.is_infinite(fhA)) {
        std::cout << "No face located in A: " << from_exact(plane.to_3d(p)) << std::endl;
        //vout << " " << from_exact(plane.to_3d(p)) << std::endl;
      }
      if (fhA->info().id2 != std::make_pair(std::size_t(-1), std::size_t(-1))) {
        cit->info().idA2 = fhA->info().id2;
        // std::cerr << "A: " << fhA->info().id << std::endl;
        result.first += a;
      }
      else {
        std::cout << "Face in A is missing ID " << from_exact(plane.to_3d(p)) << std::endl;
        //vout << " " << from_exact(plane.to_3d(p)) << std::endl;
      }
      typename CDTplus::Face_handle fhB = cdtB.locate(p);
      if (cdtB.is_infinite(fhB)) {
        std::cout << "No face located in B: " << from_exact(plane.to_3d(p)) << std::endl;
        //vout << " " << from_exact(plane.to_3d(p)) << std::endl;
      }
      if (fhB->info().id2 != std::make_pair(std::size_t(-1), std::size_t(-1))) {
        cit->info().idB2 = fhB->info().id2;
        // std::cerr << "B: " << fhB->info().id << std::endl;
        result.second += a;
      }
      else {
        std::cout << "Face in B is missing ID " << from_exact(plane.to_3d(p)) << std::endl;
        //vout << " " << from_exact(plane.to_3d(p)) << std::endl;
      }
    }

    return result;
  }

  std::size_t check_cdt(CDTplus& cdt, const typename Intersection_kernel::Plane_3& plane) {
    std::size_t missing = 0;
    for (typename CDTplus::Finite_faces_iterator cit = cdt.finite_faces_begin(); cit != cdt.finite_faces_end(); ++cit) {
      if (cit->info().id2 == std::make_pair(std::size_t(-1), std::size_t(-1))) {
        std::cout << missing << ":";
        const std::string vfilename = std::to_string(missing) + "-missing-id.polylines.txt";
        std::ofstream vout(vfilename);
        vout.precision(20);
        vout << 4;
        for (std::size_t i = 0; i < 3; i++) {
          //std::cout << " v(" << cit->vertex(i)->info().idx2.first << ", " << cit->vertex(i)->info().idx2.second << ")";
          vout << " " << plane.to_3d(cit->vertex(i)->point());
        }
        vout << " " << plane.to_3d(cit->vertex(0)->point()) << std::endl;
        std::cout << std::endl;
        vout << std::endl;
        vout.close();
        missing++;
      }
    }

    return missing;
  }

  std::size_t check_fusioned_cdt(CDTplus& cdt, const typename Intersection_kernel::Plane_3& plane) {
    std::size_t missing = 0;
    for (typename CDTplus::Finite_faces_iterator cit = cdt.finite_faces_begin(); cit != cdt.finite_faces_end(); ++cit) {
      if (cit->info().idA2 == std::make_pair(std::size_t(-1), std::size_t(-1)) || cit->info().idB2 == std::make_pair(std::size_t(-1), std::size_t(-1))) {
        std::cout << missing << ":";
        const std::string vfilename = std::to_string(missing) + "-missing-id.polylines.txt";
        std::ofstream vout(vfilename);
        vout.precision(20);
        vout << 4;
        for (std::size_t i = 0; i < 3; i++) {
          std::cout << " v(" << cit->vertex(i)->info().idx2.first << ", " << cit->vertex(i)->info().idx2.second << ")";
          vout << " " << plane.to_3d(cit->vertex(i)->point());
        }
        vout << " " << plane.to_3d(cit->vertex(0)->point()) << std::endl;
        std::cout << std::endl;
        vout << std::endl;
        vout.close();
        missing++;
      }
    }

    return missing;
  }

  void check_constraints(CDTplus& cdt, std::vector<std::vector<Constraint_info> >& c, std::size_t depth = 1) {
    for (std::size_t i = 0;i<c.size();i++)
      for (std::size_t j = 0; j < c[i].size(); j++) {
        auto id = c[i][j].id_single;
        if (depth > 1)
          id = (c[i][j].id_merged != 0) ? c[i][j].id_merged : id;

        if (depth > 2)
          id = (c[i][j].id_overlay != 0) ? c[i][j].id_overlay : id;

        if (id == 0)
          continue;

        std::vector<Vertex_handle> vertices;

        for (typename CDTplus::Vertices_in_constraint_iterator vi = cdt.vertices_in_constraint_begin(id); vi != cdt.vertices_in_constraint_end(id); vi++) {
          vertices.push_back(*vi);
        }

        if (vertices.size() == 0)
          std::cout << "constraint without vertices" << std::endl;

        if (vertices.size() == 1)
          std::cout << "constraint with single vertex" << std::endl;

        if (vertices.size() > 2)
          std::cout << "constraint with multiple vertices" << std::endl;
      }
  }

  void check_constraints_linear(CDTplus& cdt, std::vector<std::vector<Constraint_info> >& c, std::size_t depth = 1) {
    std::size_t multi = 0;
    for (std::size_t i = 0; i < c.size(); i++)
      for (std::size_t j = 0; j < c[i].size(); j++) {
        auto id = c[i][j].id_single;
        if (depth > 1)
          id = (c[i][j].id_merged != 0) ? c[i][j].id_merged : id;

        if (depth > 2)
          id = (c[i][j].id_overlay != 0) ? c[i][j].id_overlay : id;

        if (id == 0)
          continue;

        std::vector<Vertex_handle> vertices;

        for (typename CDTplus::Vertices_in_constraint_iterator vi = cdt.vertices_in_constraint_begin(id); vi != cdt.vertices_in_constraint_end(id); vi++) {
          vertices.push_back(*vi);
        }

        assert(vertices.size() >= 2);

        if (vertices.size() > 2) {
          multi++;

          for (std::size_t k = 2; k < vertices.size(); k++) {
            assert(CGAL::collinear(vertices[k - 2]->point(), vertices[k - 1]->point(), vertices[k]->point()));
          }
        }
      }
    std::cout << "multi: " << multi << std::endl;
  }

  void collect_faces(std::size_t partition_idx, std::size_t sp_idx, std::vector<std::pair<std::size_t, std::size_t> >& face_idx, std::vector<std::vector<size_t> >& faces) {
    Sub_partition& p = m_partition_nodes[partition_idx];

    for (std::size_t i = 0; i < p.m_data->volumes().size(); i++) {
      typename Data_structure::Volume_cell& v = p.m_data->volumes()[i];
      for (std::size_t j = 0; j < v.faces.size(); j++) {
        if (v.pfaces[j].first == sp_idx) {
          face_idx.push_back(std::make_pair(i, j));
          faces.push_back(p.m_data->face_to_vertices()[v.faces[j]]);
        }
      }
    }
  }

  void collect_faces(std::size_t partition_idx, std::size_t sp_idx, std::vector<Index>& faces, typename Intersection_kernel::Plane_3& plane) {
    Sub_partition& p = m_partition_nodes[partition_idx];

    plane = p.m_data->support_plane(sp_idx).data().exact_plane;

    const std::vector<std::size_t>& f2sp = p.m_data->face_to_support_plane();

    for (std::size_t i = 0; i < f2sp.size(); i++)
      if (f2sp[i] == sp_idx)
        faces.push_back(std::make_pair(partition_idx, i));

/*
    for (std::size_t i = 0; i < p.m_data->volumes().size(); i++) {
      typename Data_structure::Volume_cell& v = p.m_data->volumes()[i];
      for (std::size_t j = 0; j < v.faces.size(); j++) {
        if (v.pfaces[j].first == sp_idx) {
          faces.push_back(std::make_pair(partition_idx, v.faces[j]));
        }
      }
    }*/
  }

  void check_faces(std::size_t partition_idx, std::size_t sp_idx) {
    Sub_partition& p = m_partition_nodes[partition_idx];

    for (std::size_t i = 0; i < p.m_data->volumes().size(); i++) {
      typename Data_structure::Volume_cell& v = p.m_data->volumes()[i];
      for (std::size_t j = 0; j < v.faces.size(); j++) {
        if (v.pfaces[j].first == sp_idx) {
          if (v.neighbors[j] == -1)
            std::cout << "neighbor not set partition: " << partition_idx << " volume: " << i << " face: " << j << std::endl;
          else
            std::cout << "neighbor is set partition: " << partition_idx << " volume: " << i << " face: " << j <<  " " << v.neighbors[j] << std::endl;
        }
      }
    }
  }

  void collect_planes(std::size_t partition_idx, std::size_t sp_idx, std::vector<std::size_t> &planes) {
    Sub_partition& part = m_partition_nodes[partition_idx];
    typename Data_structure::Support_plane& sp = part.m_data->support_plane(sp_idx);
    auto pedges = sp.mesh().edges();

    std::set<std::size_t> pl;

    for (auto edge : pedges) {
      if (sp.has_iedge(edge)) {
        for (std::size_t p : part.m_data->intersected_planes(sp.iedge(edge)))
          if (!part.m_data->is_bbox_support_plane(p))
            pl.insert(p);
      }
    }

    std::copy(pl.begin(), pl.end(), std::back_inserter(planes));
  }

  void collect_faces(Octree_node node, std::size_t dimension, bool lower, std::vector<Index>& faces, typename Intersection_kernel::Plane_3 &plane) {
    // Collects boundary faces of node from its children.
    // dimension specifies the axis of the boundary face and lower determines if it is the lower of upper face of the cube on the axis.

    // Support plane indices:
    // xmin 4, xmax 2
    // ymin 1, ymax 3
    // zmin 0, zmax 5

    if (m_octree->is_leaf(node)) {
      // Mapping to partition is needed.
      std::size_t idx = m_node2partition[node];
      Sub_partition& partition = m_partition_nodes[m_node2partition[node]];
      From_exact from_exact;

      if (lower)
        switch (dimension) {
        case 0:
          collect_faces(idx, 4, faces, plane);
          break;
        case 1:
          collect_faces(idx, 1, faces, plane);
          break;
        case 2:
          collect_faces(idx, 0, faces, plane);
          break;
        }
      else
        switch (dimension) {
        case 0:
          collect_faces(idx, 2, faces, plane);
          break;
        case 1:
          collect_faces(idx, 3, faces, plane);
          break;
        case 2:
          collect_faces(idx, 5, faces, plane);
          break;
        }


      // Is Index as type for faces sufficient?
      // Partition/face id
      // Access to volumes via Data_structure->face_to_volumes()
      // However, the Data_structure::m_face2volumes needs to have std::pair<Index, Index> type to reference two volumes (pair<int, Index> would also be possible as only one volume can lie outside
      return;
    }
    else {
      typename Intersection_kernel::Plane_3 pl2, pl3, pl4;
      if (lower)
        switch (dimension) {
        case 0://0246
          collect_faces(m_octree->child(node, 0), dimension, true, faces, plane);
          collect_faces(m_octree->child(node, 2), dimension, true, faces, pl2);
          collect_faces(m_octree->child(node, 4), dimension, true, faces, pl3);
          collect_faces(m_octree->child(node, 6), dimension, true, faces, pl4);
          break;
        case 1://0145
          collect_faces(m_octree->child(node, 0), dimension, true, faces, plane);
          collect_faces(m_octree->child(node, 1), dimension, true, faces, pl2);
          collect_faces(m_octree->child(node, 4), dimension, true, faces, pl3);
          collect_faces(m_octree->child(node, 5), dimension, true, faces, pl4);
          break;
        case 2://0123
          collect_faces(m_octree->child(node, 0), dimension, true, faces, plane);
          collect_faces(m_octree->child(node, 1), dimension, true, faces, pl2);
          collect_faces(m_octree->child(node, 2), dimension, true, faces, pl3);
          collect_faces(m_octree->child(node, 3), dimension, true, faces, pl4);
          break;
        }
      else
        switch (dimension) {
        case 0://1357
          collect_faces(m_octree->child(node, 1), dimension, false, faces, plane);
          collect_faces(m_octree->child(node, 3), dimension, false, faces, pl2);
          collect_faces(m_octree->child(node, 5), dimension, false, faces, pl3);
          collect_faces(m_octree->child(node, 7), dimension, false, faces, pl4);
          break;
        case 1://3467
          collect_faces(m_octree->child(node, 2), dimension, false, faces, plane);
          collect_faces(m_octree->child(node, 3), dimension, false, faces, pl2);
          collect_faces(m_octree->child(node, 6), dimension, false, faces, pl3);
          collect_faces(m_octree->child(node, 7), dimension, false, faces, pl4);
          break;
        case 2://4567
          collect_faces(m_octree->child(node, 4), dimension, false, faces, plane);
          collect_faces(m_octree->child(node, 5), dimension, false, faces, pl2);
          collect_faces(m_octree->child(node, 6), dimension, false, faces, pl3);
          collect_faces(m_octree->child(node, 7), dimension, false, faces, pl4);
          break;
        }

      bool same = plane == pl2;
      same = (same && plane == pl3);
      same = (same && plane == pl4);
      if (!same) {
        std::cout << "collect_faces: different plane, node: " << node << " lower: " << lower << std::endl;
        From_exact from_exact;
        std::cout << from_exact(plane) << std::endl;
        std::cout << from_exact(pl2) << " child: " << m_octree->child(node, 4) << std::endl;
        std::cout << from_exact(pl3) << " child: " << m_octree->child(node, 6) << std::endl;
        std::cout << from_exact(pl4) << " child: " << m_octree->child(node, 7) << std::endl << std::endl;
      }
    }
  }

  void collect_opposing_faces(Octree_node node, std::size_t dimension, std::vector<Index>& lower, std::vector<Index>& upper, typename Intersection_kernel::Plane_3 &plane) {
    // Nothing to do for a leaf node.
    if (m_octree->is_leaf(node))
      return;

    typename Intersection_kernel::Plane_3 pl[7];
    switch (dimension) {
    case 0:
      collect_faces(m_octree->child(node, 0), dimension, false, lower, plane);
      collect_faces(m_octree->child(node, 2), dimension, false, lower, pl[0]);
      collect_faces(m_octree->child(node, 4), dimension, false, lower, pl[1]);
      collect_faces(m_octree->child(node, 6), dimension, false, lower, pl[2]);
      collect_faces(m_octree->child(node, 1), dimension, true, upper, pl[3]);
      collect_faces(m_octree->child(node, 3), dimension, true, upper, pl[4]);
      collect_faces(m_octree->child(node, 5), dimension, true, upper, pl[5]);
      collect_faces(m_octree->child(node, 7), dimension, true, upper, pl[6]);
      break;
    case 1:
      collect_faces(m_octree->child(node, 0), dimension, false, lower, plane);
      collect_faces(m_octree->child(node, 1), dimension, false, lower, pl[0]);
      collect_faces(m_octree->child(node, 4), dimension, false, lower, pl[1]);
      collect_faces(m_octree->child(node, 5), dimension, false, lower, pl[2]);
      collect_faces(m_octree->child(node, 3), dimension, true, upper, pl[3]);
      collect_faces(m_octree->child(node, 2), dimension, true, upper, pl[4]);
      collect_faces(m_octree->child(node, 6), dimension, true, upper, pl[5]);
      collect_faces(m_octree->child(node, 7), dimension, true, upper, pl[6]);
      break;
    case 2:
      collect_faces(m_octree->child(node, 0), dimension, false, lower, plane);
      collect_faces(m_octree->child(node, 1), dimension, false, lower, pl[0]);
      collect_faces(m_octree->child(node, 2), dimension, false, lower, pl[1]);
      collect_faces(m_octree->child(node, 3), dimension, false, lower, pl[2]);
      collect_faces(m_octree->child(node, 4), dimension, true, upper, pl[3]);
      collect_faces(m_octree->child(node, 5), dimension, true, upper, pl[4]);
      collect_faces(m_octree->child(node, 6), dimension, true, upper, pl[5]);
      collect_faces(m_octree->child(node, 7), dimension, true, upper, pl[6]);
      break;
    }

    From_exact from_exact;
    //std::cout << from_exact(plane) << std::endl;

    bool same = true;
    for (std::size_t i = 0; i < 3; i++)
      same = (same && plane == pl[i]);

    for (std::size_t i = 3; i < 7; i++)
      same = (same && plane.opposite() == pl[i]);

    if (!same) {
      std::cout << "collect_opposing_faces: different plane, node: " << node << std::endl;
      std::cout << from_exact(plane) << std::endl;
      for (std::size_t i = 0; i < 3; i++)
        std::cout << from_exact(pl[i]) << std::endl;
      for (std::size_t i = 3; i < 7; i++)
        std::cout << from_exact(pl[i].opposite()) << std::endl;
      bool diff = (plane.b() == pl[6].opposite().b());
      std::cout << diff << std::endl;
      std::cout << std::endl;
    }
  }

  bool can_add_volume_to_lcc(std::size_t volume, const std::vector<bool>& added_volumes, const std::map<Index, std::size_t> &vtx2index, const std::vector<bool>& added_vertices) const {
    // get faces
    // check neighbors of face and check whether the neighboring volumes have already been added

    // added vertices that are not adjacent to an inserted face cause non-manifold configurations
    //
    // go through all faces and check whether the neighbor volume has been added
    //  if so insert all vertices into vertices_of_volume
    // go again through all faces and only consider faces where the neighbor volume has not been added
    //  check if the vertex is in vertices_of_volume
    //   if not, but the vertex is flagged in added_vertices return false
    // return true
    std::set<Index> vertices_of_volume;
    std::vector<Index> faces_of_volume;
    faces(volume, std::back_inserter(faces_of_volume));

    for (std::size_t i = 0; i < faces_of_volume.size(); i++) {
      std::vector<Index> vtx;
      auto n = neighbors(faces_of_volume[i]);
      int other = (n.first == volume) ? n.second : n.first;
      if (other < 0 || !added_volumes[other])
        continue;
      vertex_indices(faces_of_volume[i], std::back_inserter(vtx));

      for (std::size_t j = 0; j < vtx.size(); j++)
        vertices_of_volume.insert(vtx[j]);
    }

    for (std::size_t i = 0; i < faces_of_volume.size(); i++) {
      auto n = neighbors(faces_of_volume[i]);
      int other = (n.first == volume) ? n.second : n.first;
      if (other >= 0 && added_volumes[other])
        continue;
      std::vector<Index> vtx;
      vertex_indices(faces_of_volume[i], std::back_inserter(vtx));

      for (std::size_t j = 0; j < vtx.size(); j++) {
        auto it = vtx2index.find(vtx[j]);
        assert(it != vtx2index.end());
        if (vertices_of_volume.find(vtx[j]) == vertices_of_volume.end() && added_vertices[it->second])
          return false;
      }
    }

    return true;
  }

  void merge_partitions(std::size_t idx) {
    From_exact from_exact;
    if (!m_partition_nodes[idx].children.empty()) {
      std::size_t lower_idx = m_partition_nodes[idx].children[0];
      std::size_t upper_idx = m_partition_nodes[idx].children[1];

      Sub_partition& lower = m_partition_nodes[lower_idx];
      Sub_partition& upper = m_partition_nodes[upper_idx];

      std::size_t lower_sp = lower.split_plane;
      std::size_t upper_sp = upper.split_plane;

      // Collect faces and volumes that are on that plane (use the neighboring index in volumes)
      std::vector<std::pair<std::size_t, std::size_t> > face_idx_lower, face_idx_upper;
      std::vector<std::vector<std::size_t> >  faces_lower, faces_upper;
      std::vector<typename Intersection_kernel::Point_3> vertices_lower = lower.m_data->exact_vertices(), vertices_upper = upper.m_data->exact_vertices();

      collect_faces(lower_idx, lower_sp, face_idx_lower, faces_lower);
      collect_faces(upper_idx, upper_sp, face_idx_upper, faces_upper);

      /*
            const std::string vfilename = "lower.xyz";
            std::ofstream vout(vfilename);
            vout.precision(20);
            for (std::vector<std::size_t>& f : faces_lower)
              for (std::size_t p : f)
              vout << " " << vertices_lower[p] << std::endl;
            vout << std::endl;
            vout.close();


            const std::string vfilename2 = "upper.xyz";
            std::ofstream vout2(vfilename2);
            vout2.precision(20);
            for (auto f : faces_upper)
              for (auto p : f)
                vout2 << " " << vertices_upper[p] << std::endl;

            vout2 << std::endl;
            vout2.close();*/

      typename Intersection_kernel::Plane_3 plane = lower.m_data->support_plane(lower_sp).exact_plane();

      CDTplus lowerCDT, upperCDT;
      double lower_area = build_cdt(lowerCDT, vertices_lower, faces_lower, plane);
      double upper_area = build_cdt(upperCDT, vertices_upper, faces_upper, plane);

      // Collect Plane_3 from support planes (two vectors, one for each other)
      std::vector<std::size_t> planes_lower, planes_upper;
      collect_planes(lower_idx, lower_sp, planes_lower);
      collect_planes(upper_idx, upper_sp, planes_upper);

      // Remove common planes
      auto lower_it = planes_lower.begin();
      auto upper_it = planes_upper.begin();
      while (lower_it != planes_lower.end() && upper_it != planes_upper.end()) {
        if (*lower_it == *upper_it) {
          lower_it = planes_lower.erase(lower_it);
          upper_it = planes_upper.erase(upper_it);
          if (upper_it == planes_upper.end())
            break;
        }
        else if (*lower_it < *upper_it) {
          lower_it++;
        }
        else if (*lower_it > *upper_it) {
          upper_it++;
          if (upper_it == planes_upper.end())
            break;
        }
      }

      if (!planes_upper.empty()) {
        split_faces(lower_idx, upper_idx, lower_sp, faces_lower, face_idx_lower, vertices_lower, planes_upper);
      }

      if (!planes_lower.empty()) {
        split_faces(upper_idx, lower_idx, upper_sp, faces_upper, face_idx_upper, vertices_upper, planes_lower);
      }

      // How to merge the two Data_structures
      // Identification of common vertices
      // using support planes to check whether I need to check for common vertices? Seems difficult as every vertex is at the intersection of at least three support planes?

      //CDTplus lowerCDT, upperCDT;
      lower_area = build_cdt(lowerCDT, vertices_lower, faces_lower, plane);
      upper_area = build_cdt(upperCDT, vertices_upper, faces_upper, plane);

      CDTplus combined;
      std::pair<double, double> areas = overlay(combined, lowerCDT, upperCDT, plane);

      for (std::size_t i = 0; i < faces_lower.size(); i++) {
        typename Data_structure::Volume_cell& v = lower.m_data->volumes()[face_idx_lower[i].first];

        std::vector<typename Intersection_kernel::Point_3> pts;
        pts.reserve(faces_lower[i].size());
        for (std::size_t idx : faces_lower[i])
          pts.push_back(vertices_lower[idx]);

        typename Intersection_kernel::Point_3 c = CGAL::centroid(pts.begin(), pts.end(), CGAL::Dimension_tag<0>());
        Point_3 c_inexact = from_exact(c);

        typename CDTplus::Face_handle neighbor = upperCDT.locate(plane.to_2d(c));
        if (neighbor->info().id < faces_upper.size()) {
          //std::cout << "index " << i << " of lower set to " << face_idx_upper[neighbor->info().id].first << std::endl;
          v.neighbors[face_idx_lower[i].second] = face_idx_upper[neighbor->info().id].first;
        }
        else std::cout << "neighbor of face " << i << " of lower has neighbor " << face_idx_upper[neighbor->info().id].first << " in upper" << std::endl;
      }

      //check_faces(lower_idx, lower_sp);

      for (std::size_t i = 0; i < faces_upper.size(); i++) {
        typename Data_structure::Volume_cell& v = upper.m_data->volumes()[face_idx_upper[i].first];

        std::vector<typename Intersection_kernel::Point_3> pts;
        pts.reserve(faces_upper[i].size());
        for (std::size_t idx : faces_upper[i])
          pts.push_back(vertices_upper[idx]);

        typename Intersection_kernel::Point_3 c = CGAL::centroid(pts.begin(), pts.end(), CGAL::Dimension_tag<0>());

        typename CDTplus::Face_handle neighbor = lowerCDT.locate(plane.to_2d(c));
        if (neighbor->info().id < faces_lower.size()) {
          //std::cout << "index " << i << " of upper set to " << face_idx_lower[neighbor->info().id].first << std::endl;
          v.neighbors[face_idx_upper[i].second] = face_idx_lower[neighbor->info().id].first;
        }
        else std::cout << "neighbor of face " << i << " of upper has neighbor " << face_idx_lower[neighbor->info().id].first << " in upper" << std::endl;
      }

      //check_faces(upper_idx, upper_sp);
    }
  }

  bool same_face(const Face_handle& a, const Face_handle& b) const {
    return (b->info().idA2 == a->info().idA2 && b->info().idB2 == a->info().idB2);
  }

  void dump_face(const Face_handle& f, const std::string& filename) {
    From_exact from_exact;
    std::ofstream vout(filename);
    vout.precision(20);
    vout << "4 ";
    vout << " " << from_exact(f->vertex(0)->info().point_3);
    vout << " " << from_exact(f->vertex(1)->info().point_3);
    vout << " " << from_exact(f->vertex(2)->info().point_3);
    vout << " " << from_exact(f->vertex(0)->info().point_3);
    vout << std::endl;
    vout.close();
  }

  void dump_point(const Vertex_handle& v, const std::string& filename) {
    From_exact from_exact;
    std::ofstream vout3(filename);
    vout3.precision(20);
    vout3 << " " << from_exact(v->info().point_3);
    vout3 << std::endl;
    vout3.close();
  }

  void set_face(const Index& f, const Index& other, std::set<Index>& replaced, const std::vector<Vertex_handle>& polygon) {
    From_exact from_exact;
    auto pair = replaced.insert(f);
    std::size_t idx;
    assert(m_partition_nodes[f.first].face_neighbors[f.second].first.first == f.first);
    std::size_t vol_idx = m_partition_nodes[f.first].face_neighbors[f.second].first.second;

    if (!pair.second) {
      // New face has a new index
      idx = m_partition_nodes[f.first].face2vertices.size();
      // Add face into vector
      m_partition_nodes[f.first].face2vertices.push_back(std::vector<Index>());
      m_partition_nodes[f.first].m_data->face_is_part_of_input_polygon().push_back(m_partition_nodes[f.first].m_data->face_is_part_of_input_polygon()[f.second]);
      // Add face index into volume
      m_partition_nodes[f.first].m_data->volumes()[vol_idx].faces.push_back(idx);
      // Copy neighbor from already existing face
      m_partition_nodes[f.first].face_neighbors.push_back(m_partition_nodes[f.first].face_neighbors[f.second]);
      m_partition_nodes[f.first].m_data->face_to_support_plane().push_back(m_partition_nodes[f.first].m_data->face_to_support_plane()[f.second]);
    }
    else {
      idx = f.second;
      // All boundary faces should have a negative second neighbor.
      assert(m_partition_nodes[f.first].face_neighbors[idx].second.second >= std::size_t(-6));
    }
    std::vector<Index>& vertices = m_partition_nodes[f.first].face2vertices[idx];
    // First neighbor of other face should point to the inside volume in the other partition and thus cannot be negative
    assert(m_partition_nodes[other.first].face_neighbors[other.second].first.second < std::size_t(-6));
    m_partition_nodes[f.first].face_neighbors[idx].second = m_partition_nodes[other.first].face_neighbors[other.second].first;
    vertices.resize(polygon.size());
    for (std::size_t i = 0; i < polygon.size(); i++) {
      VI& vi = polygon[i]->info();
      // Is this check actually meaningless as partition indices now start at 0?
      // Check whether they are initialized as 0 and where it is used as indicator for something.
/*
      if (vi.idA2.first == 0 || vi.idB2.first == 0) {
        std::cout << "invalid vertex id" << std::endl;
      }*/
      if (vi.idA2.first < vi.idB2.first)
        vertices[i] = vi.idA2;
      else if (vi.idB2.first != -1)
        vertices[i] = vi.idB2;
      else {
        std::size_t vidx = m_partition_nodes[f.first].m_data->vertices().size();
        m_partition_nodes[f.first].m_data->vertices().push_back(from_exact(vi.point_3));
        m_partition_nodes[f.first].m_data->exact_vertices().push_back(vi.point_3);
        vertices[i] = vi.idA2 = std::make_pair(f.first, vidx);
      }
/*
      if (vi.idA2.first != std::size_t(-1))
        vertices[i] = vi.idA2;
      else if (vi.idB2.first != std::size_t(-1))
        vertices[i] = vi.idB2;
      else {
        std::size_t vidx = m_partition_nodes[f.first].m_data->vertices().size();
        m_partition_nodes[f.first].m_data->vertices().push_back(from_exact(vi.point_3));
        m_partition_nodes[f.first].m_data->exact_vertices().push_back(vi.point_3);
        vertices[i] = vi.idA2 = std::make_pair(f.first, vidx);
      }*/
    }
  }

  void adapt_faces(const CDTplus& cdt, std::vector<Index>& a, std::vector<Index>& b, typename Intersection_kernel::Plane_3& plane) {
    std::set<Index> replacedA, replacedB;
    From_exact from_exact;

    std::size_t extracted = 0;
    for (typename CDTplus::Face_handle fh : cdt.finite_face_handles()) {
      // when extracting each face, I only need to insert vertices, that don't exist on either side. Otherwise, I can just reference the vertex in the other partition.
      // using cit->info().id2.first as visited flag. -1 means not visited
      if (fh->info().id2.first != -1)
        continue;

      // 4 different cases: no border edge, 1, 2 or 3
      // Check 1, 2, 3
      // if first is not, continue
      // if first move in other direction? search start

      // Easier approach, don't make a list of edges, but a list of vertices
      // Find first pair of vertices, then just loop around last vertex using Face_circulator
      // -> Triangulation only has vertices and faces, no easy way to loop over edges

      std::vector<Vertex_handle> face;

      for (std::size_t i = 0; i < 3; i++)
        if (cdt.is_infinite(fh->neighbor(i)) || !same_face(fh, fh->neighbor(i))) {
          face.push_back(fh->vertex((i + 2) % 3));
          face.push_back(fh->vertex((i + 1) % 3));
          break;
        }

      // No border edge?
      if (face.empty())
        continue;
      else {
        //dump_point(face.back(), "last.xyz");
        Face_handle last = fh;

        // Mark seed face as segmented
        fh->info().id2.first = extracted;

        // edge is pair<Face_handle, int (vertex)>
        while (face.front() != face.back()) {
          auto eit = cdt.incident_edges(face.back(), last);
          // Skip the first edge as it always starts with the edge itself.
          //eit++;
/*
          auto eit2 = eit;
          for (std::size_t i = 0; i < 10; i++) {
            dump_point(eit2->first->vertex(eit2->second), std::to_string(i) + "p.xyz");
            dump_face(eit2->first, std::to_string(i) + "tri.polylines.txt");
            std::cout << i << " same: " << same_face(last, eit2->first->neighbor((eit2->second + 1) % 3)) << std::endl;
            eit2++;
          }*/
          auto first = eit;
          Point_3 p = from_exact(eit->first->vertex(eit->second)->info().point_3);

          assert(!cdt.is_infinite(eit->first));
          do {
            // Export tri

            //dump_point(eit->first->vertex(eit->second), "p.xyz");
            //dump_face(eit->first, "tri.polylines.txt");

            // Is the current edge to the infinite vertex?
            if (cdt.is_infinite(eit->first->neighbor((eit->second + 1) % 3))) {
              eit++;
              continue;
            }

            bool infinite = cdt.is_infinite(eit->first);

            /*            if (!infinite)
                          dump_face(eit->first, "neighbor.polylines.txt");*/

            if (infinite || !same_face(last, eit->first)) {
              last = eit->first->neighbor((eit->second + 1) % 3);
              last->info().id2.first = extracted;
              face.push_back(eit->first->vertex(eit->second));

              break;
            }
            eit++;
            assert(eit != first);
          } while (eit != first);
          // If last vertex is equal to first vertex, stop
          // Take last vertex and face
          // First find index of vertex in that face
          // Check if opposite face of next edge, if not same, add next vertex and reloop
          // if not, check next face

          assert(face.size() < 100);
        }

        // The last vertex is equal to the first one, so it should be removed.
        face.pop_back();

        // face ids in partitions are stored in fh->info
        ID& id = fh->info();
        set_face(id.idA2, id.idB2, replacedA, face);
        set_face(id.idB2, id.idA2, replacedB, face);
      }

      // Checking for border edges. If opposite faces do not exist or don't have the same indices, the edge belongs to a new face.
      // cit->neighbor(i) is the face opposite of vertex(i), meaning on the other side of the edge between vertex((i+1)%3) and vertex((i+2)%3)
    }
  }

  std::pair<std::size_t, int> find_portal(const std::vector<Index>& faces, const Index& vA, const Index& vB, const Index& entry, std::size_t& portal) const {
    portal = -1;
    for (std::size_t f = 0; f < faces.size(); f++) {
      if (faces[f] == entry)
        continue;

      const Index& face = faces[f];

      std::size_t idxA = -1;
      std::size_t numVtx = m_partition_nodes[face.first].face2vertices[face.second].size();
      for (std::size_t v = 0; v < numVtx; v++)
        if (m_partition_nodes[face.first].face2vertices[face.second][v] == vA) {
          idxA = v;
          break;
        }
      // If vertex wasn't found, skip to next face.
      if (idxA == -1)
        continue;

      std::size_t idxB = -1;
      int dir = 0;
      if (m_partition_nodes[face.first].face2vertices[face.second][(idxA + 1) % numVtx] == vB) {
        dir = 1;
        idxB = (idxA + 1) % numVtx;
      }
      else if (m_partition_nodes[face.first].face2vertices[face.second][(idxA + numVtx - 1) % numVtx] == vB) {
        dir = -1;
        idxB = (idxA + numVtx - 1) % numVtx;
      }

      // If only the first vertex was found, it is just an adjacent face.
      if (idxB == -1)
        continue;

      // Edge found
      // Save portal face for next volume.
      portal = f;

      return std::make_pair(idxA, dir);
    }
    return std::make_pair(-1, -1);
  }

  std::pair<std::size_t, int> find_portal(std::size_t volume, std::size_t former, const Index& vA, const Index& vB, std::size_t& portal) const {
    portal = -7;
    auto vol = m_volumes[volume];
    std::vector<std::size_t>& faces = m_partition_nodes[vol.first].m_data->volumes()[vol.second].faces;

    for (std::size_t f = 0; f < faces.size(); f++) {
      auto n = neighbors(std::make_pair(vol.first, faces[f]));
      if (n.first == former || n.second == former)
        continue;

      std::size_t idxA = -1;
      std::size_t numVtx = m_partition_nodes[vol.first].face2vertices[faces[f]].size();
      for (std::size_t v = 0; v < numVtx; v++)
        if (m_partition_nodes[vol.first].face2vertices[faces[f]][v] == vA) {
          idxA = v;
          break;
        }
      // If vertex wasn't found, skip to next face.
      if (idxA == -1)
        continue;

      std::size_t idxB = -1;
      int dir = 0;
      if (m_partition_nodes[vol.first].face2vertices[faces[f]][(idxA + 1) % numVtx] == vB) {
        dir = 1;
        idxB = (idxA + 1) % numVtx;
      }
      else if (m_partition_nodes[vol.first].face2vertices[faces[f]][(idxA + numVtx - 1) % numVtx] == vB) {
        dir = -1;
        idxB = (idxA + numVtx - 1) % numVtx;
      }

      // If only the first vertex was found, it is just an adjacent face.
      if (idxB == -1)
        continue;

      // Edge found
      // Save portal face for next volume.
      portal = f;

      return std::make_pair(idxA, dir);
    }
    return std::make_pair(-1, -1);
  }

  bool find_portals(const std::vector<Index>& faces, const Index& vA, const Index& vB, Index& a, Index& b) const {
    // ToDo: restrict to two faces?
    std::size_t count = 0;
    for (std::size_t f = 0; f < faces.size(); f++) {
      if (faces[f] == entry)
        continue;

      Index& face = faces[f];

      std::size_t idxA = -1;
      std::size_t numVtx = m_partition_nodes[face.first].face2vertices[face.second].size();
      for (std::size_t v = 0; v < numVtx; v++)
        if (m_partition_nodes[face.first].face2vertices[face.second][v] == c[f][e].vA) {
          idxA = v;
          break;
        }
      // If vertex wasn't found, skip to next face.
      if (idxA == -1)
        continue;

      std::size_t idxB = -1;
      int dir = 0;
      if (m_partition_nodes[face.first].face2vertices[face.second][(idxA + 1) % numVtx] == c[f][e].vB) {
        dir = 1;
        idxB = (idxA + 1) % numVtx;
      }
      else if (m_partition_nodes[face.first].face2vertices[face.second][(idxA + numVtx - 1) % numVtx] == c[f][e].vB) {
        dir = -1;
        idxB = (idxA + numVtx - 1) % numVtx;
      }

      // If only the first vertex was found, it is just an adjacent face.
      if (idxB == -1)
        continue;

      if (count == 0)
        a = face;
      else if (count == 1)
        b = face;
      else return false;

      count++;
    }
    return count == 2;
  }

  bool check_face(const Index& f) const {
    const std::vector<Index>& face = m_partition_nodes[f.first].face2vertices[f.second];

    typename Intersection_kernel::Point_3& a = m_partition_nodes[face[0].first].m_data->exact_vertices()[face[0].second];
    typename Intersection_kernel::Point_3& b = m_partition_nodes[face[1].first].m_data->exact_vertices()[face[1].second];

    for (std::size_t i = 3; i < face.size(); i++) {
      typename Intersection_kernel::Point_3& c = m_partition_nodes[face[i-1].first].m_data->exact_vertices()[face[i-1].second];
      typename Intersection_kernel::Point_3& d = m_partition_nodes[face[i].first].m_data->exact_vertices()[face[i].second];
      if (!CGAL::coplanar(a, b, c, d)) {
        return false;
      }
    }

    typename Intersection_kernel::Plane_3 p;
    for (std::size_t i = 2; i < face.size(); i++) {
      typename Intersection_kernel::Point_3& d = m_partition_nodes[face[i].first].m_data->exact_vertices()[face[i].second];
      if (!collinear(a, b, d)) {
        p = Intersection_kernel::Plane_3(a, b, d);
      }
    }

    std::vector<typename Intersection_kernel::Point_2> pts2d(face.size());

    for (std::size_t i = 0; i < face.size(); i++) {
      pts2d[i] = p.to_2d(m_partition_nodes[face[i].first].m_data->exact_vertices()[face[i].second]);
    }

    if (!CGAL::is_simple_2(pts2d.begin(), pts2d.end()))
      return false;

    return true;
  }

  void adapt_internal_edges(const CDTplus& cdtA, const CDTplus& cdtC, const std::vector<Index> &faces_node, std::vector<std::vector<Constraint_info> >& c) {
    assert(faces_node.size() == c.size());

    std::size_t not_skipped = 0;

    for (std::size_t f = 0; f < c.size(); f++) {
      std::vector<Index> faces_of_volume;
      // The face index is probably no longer valid and the full face has been replaced by a smaller face using merged indices
      // Each constraint has a volume.
      // Constraints of the same volume are subsequent
      for (std::size_t e = 0; e < c[f].size(); e++) {
        auto id = c[f][e].id_single;
        if (id == 0)
          continue;

        id = (c[f][e].id_merged != 0) ? c[f][e].id_merged : id;
        id = (c[f][e].id_overlay != 0) ? c[f][e].id_overlay : id;

        int volume = c[f][e].volume;

        //auto it = (c[f][e].vA < c[f][e].vB) ? constraint2edge.find(std::make_pair(c[f][e].vA, c[f][e].vB)) : constraint2edge.find(std::make_pair(c[f][e].vB, c[f][e].vA));

        // Extract edge
        std::vector<Index> vertices_of_edge;
        for (typename CDTplus::Vertices_in_constraint_iterator vi = cdtC.vertices_in_constraint_begin(id); vi != cdtC.vertices_in_constraint_end(id); vi++) {
          if ((*vi)->info().idA2.first == -1)
            vertices_of_edge.push_back((*vi)->info().idB2);
          else vertices_of_edge.push_back((*vi)->info().idA2);
        }

        /*if (it == constraint2edge.end())
          std::cout << ".";

        if (it != constraint2edge.end() && it->second.size() > vertices_of_edge.size())
          std::cout << f << " " << e << " (" << c[f][e].vA.first << "," << c[f][e].vA.second << ") (" << c[f][e].vB.first << "," << c[f][e].vB.second << ") cs " << it->second.size() << " " << vertices_of_edge.size() << std::endl;
*/

        // Not necessary, as I am replacing vertices anyway?
        if (vertices_of_edge.size() == 2)
          continue;

        not_skipped++;

/*
        for (std::size_t i = 2; i < vertices_of_edge.size(); i++) {
          typename Intersection_kernel::Point_3& a = m_partition_nodes[vertices_of_edge[0].first].m_data->exact_vertices()[vertices_of_edge[0].second];
          typename Intersection_kernel::Point_3& b = m_partition_nodes[vertices_of_edge[1].first].m_data->exact_vertices()[vertices_of_edge[1].second];
          typename Intersection_kernel::Point_3& c = m_partition_nodes[vertices_of_edge[i].first].m_data->exact_vertices()[vertices_of_edge[i].second];
          if (!CGAL::collinear(a, b, c)) {
            std::cout << "edge is not collinear " << f << " " << e << std::endl;
          }
        }*/

        // Check length of constraint
        // size 2 means it has not been split, thus there are no t-junctions.
        assert (vertices_of_edge.size() >= 2);

        faces_of_volume.clear();
        faces(volume, std::back_inserter(faces_of_volume));

        int starting_volume = volume;

        // Looping around edge until either the full loop has been made or a non connected face is encountered (either between not yet connected octree nodes or due to face on bbox)
        // Looping in both directions necessary? (only possible if edge.size is 2) How to handle? If I do both directions, I will not find an edge in handling the other side.
        // Looping in other partition may not work as I won't find the edge on the other side (as it uses different vertices!)
        // How to detect if a volume is in another partition? auto p = m_volumes[volume_index];
        // After the internal make_conformal call, the connected nodes should have unique vertices, i.e., no two vertices with the same position
        // make_conformal can contain plenty of nodes at once. How do I make sure that the vertices are unique? Is automatically taken care of during the process?
        // Edges inside the face will make a full loop. It is possible (or probably always the case), that once traversing the side, the next portal cannot be found due to changed vertex indices
        Index portal = Index(-1, -1);
        std::size_t idx, idx2;
        auto p = find_portal(volume, -7, c[f][e].vA, c[f][e].vB, idx);

        if (idx == -7) {
          continue;
        }
        auto n = neighbors(faces_of_volume[idx]);
        int other = (n.first == volume) ? n.second : n.first;
        auto p2 = find_portal(volume, other, c[f][e].vA, c[f][e].vB, idx2);

        // For cdtA, there should be two portals and for cdtB only one
        // How to discard the traversing one?
        if (idx != -7) {
          // Check if the portal idx is traversing.
          // The neighbors of a portal can be negative if it is not in the current face between the octree nodes.
          //auto n = neighbors(faces_of_volume[idx]);
          if (idx2 < -7 && m_volumes[volume].first != m_volumes[other].first) {
            idx = idx2;
            p = p2;
          }
        }
        else {
          idx = idx2;
          p = p2;
        }
        if (idx == -7)
          continue;

        std::size_t numVtx = m_partition_nodes[faces_of_volume[idx].first].face2vertices[faces_of_volume[idx].second].size();

        // Replace first and last vertex
        //m_partition_nodes[faces_of_volume[idx].first].face2vertices[faces_of_volume[idx].second][p.first] = vertices_of_edge[0];
        //m_partition_nodes[faces_of_volume[idx].first].face2vertices[faces_of_volume[idx].second][(p.first + p.second + numVtx) % numVtx] = vertices_of_edge.back();
/*

        if (!check_face(faces_of_volume[idx])) {
          std::cout << "face is not coplanar before " << f << " " << e << std::endl;
        }*/

        std::vector<Index> tmp = m_partition_nodes[faces_of_volume[idx].first].face2vertices[faces_of_volume[idx].second];

        // Insert vertices in between
        if (p.second == 1)
          for (std::size_t i = 1; i < vertices_of_edge.size() - 1; i++)
            m_partition_nodes[faces_of_volume[idx].first].face2vertices[faces_of_volume[idx].second].insert(m_partition_nodes[faces_of_volume[idx].first].face2vertices[faces_of_volume[idx].second].begin() + p.first + i, vertices_of_edge[i]);
        else
          for (std::size_t i = 1; i < vertices_of_edge.size() - 1; i++)
            m_partition_nodes[faces_of_volume[idx].first].face2vertices[faces_of_volume[idx].second].insert(m_partition_nodes[faces_of_volume[idx].first].face2vertices[faces_of_volume[idx].second].begin() + p.first, vertices_of_edge[i]);


        n = neighbors(faces_of_volume[idx]);

        if (n.first != volume && n.second != volume)
          std::cout << "portal does not belong to volume" << std::endl;
        volume = (n.first == volume) ? n.second : n.first;
        int former = (idx == idx2) ? -1 : idx2;

        while (volume >= 0 && volume != starting_volume) { // What are the stopping conditions? There are probably several ones, e.g., arriving at the starting volume, not finding a portal face
          int next;
          faces_of_volume.clear();
          faces(volume, std::back_inserter(faces_of_volume));

          auto p = find_portal(volume, former, c[f][e].vA, c[f][e].vB, idx);

          if (idx == -7)
            break;

          //Do I need to make sure I find both faces for the first volume?
          //  -> In some cases there will be two faces and IN some cases there won't (edge split or vertex from other side -> only one face)
          // for the first volume, I need to search completely and go both ways, but also check for loop
          //How to verify that I replaced all?

          // Insert vertices in between
          if (p.second == 1)
            for (std::size_t i = 1; i < vertices_of_edge.size() - 1; i++)
              m_partition_nodes[faces_of_volume[idx].first].face2vertices[faces_of_volume[idx].second].insert(m_partition_nodes[faces_of_volume[idx].first].face2vertices[faces_of_volume[idx].second].begin() + p.first + i, vertices_of_edge[i]);
          else
            for (std::size_t i = 1; i < vertices_of_edge.size() - 1; i++)
              m_partition_nodes[faces_of_volume[idx].first].face2vertices[faces_of_volume[idx].second].insert(m_partition_nodes[faces_of_volume[idx].first].face2vertices[faces_of_volume[idx].second].begin() + p.first, vertices_of_edge[i]);


          // This is redundant to get next?
          auto n = neighbors(faces_of_volume[idx]);

          if (n.first != volume && n.second != volume)
            std::cout << "portal does not belong to volume" << std::endl;
          volume = (n.first == volume) ? n.second : n.first;

          former = volume;
        }
      }
    }
  }

  void make_conformal(std::vector<Index>& a, std::vector<Index>& b, typename Intersection_kernel::Plane_3& plane) {
    // partition ids are in a[0].first and b[0].first
    // volume and face in volume ids are not available
    // there is face2volume and one of those volume indices will be an outside volume, e.g. std::size_t(-1) to std::size_t(-6)

    // Indices in a and b come from different partitions. Each face only has vertices from the same partition
    // Constraints in the cdt should have matching vertices and edges from different partitions -> opportunity to match vertices and faces between partitions

    // buildCDT needs only Index and exact_vertices for the points and Index for faces

    // Sorting the faces from sides a and b into partition nodes
    std::unordered_map<std::size_t, std::vector<Index> > a_sets, b_sets;
    for (const Index& i : a)
      a_sets[i.first].push_back(i);
    for (const Index& i : b)
      b_sets[i.first].push_back(i);

    // At first, one CDTplus is created for each partition node
    std::vector<CDTplus> a_cdts(a_sets.size()), b_cdts(b_sets.size());

    std::size_t newpts = 0;
    From_exact from_exact;
    Plane_3 pl = from_exact(plane);

    std::vector< std::vector<std::vector<Constraint_info> > > a_constraints;
    std::vector< std::vector<std::vector<Constraint_info> > > b_constraints;

    std::map<Index, Index> point_mapping;

    std::size_t idx = 0;
    a_constraints.resize(a_sets.size());

    std::set<std::size_t> partitions;
    for (auto& p : a_sets) {
      partitions.insert(p.first);
      build_cdt(a_cdts[idx], p.second, a_constraints[idx], plane);
/*
      newpts = 0;
      for (Vertex_handle v : a_cdts[idx].finite_vertex_handles()) {
        if (v->info().idA2 == Index(-1, -1))
          newpts++;
      }

      if (newpts > 0)
        std::cout << newpts << " vertices without references found in a_cdts" << idx << std::endl;

      if (check_cdt(a_cdts[idx], plane) != 0)
        std::cout << "lower " << p.first << ": " << p.second.size() << " " << a_cdts[idx].number_of_faces() << " with " << check_cdt(a_cdts[idx], plane) << " missing ids" << std::endl;*/
      idx++;
    }

    idx = 0;
    b_constraints.resize(b_sets.size());
    for (auto& p : b_sets) {
      partitions.insert(p.first);
      build_cdt(b_cdts[idx], p.second, b_constraints[idx], plane);
/*
      newpts = 0;
      for (Vertex_handle v : b_cdts[idx].finite_vertex_handles()) {
        if (v->info().idA2 == Index(-1, -1))
          newpts++;
      }

      if (newpts > 0)
        std::cout << newpts << " vertices without references found in b_cdts" << idx << std::endl;


      if (check_cdt(b_cdts[idx], plane) != 0)
        std::cout << "upper " << p.first << ": " << p.second.size() << " " << b_cdts[idx].number_of_faces() << " with " << check_cdt(b_cdts[idx], plane) << " missing ids" << std::endl;*/
      idx++;
    }

    CDTplus cdtA, cdtB, cdtC;
    build_cdt(cdtA, a_cdts, a_constraints, plane);

    // ToDo: remove checks
/*
    std::size_t missing = check_cdt(cdtA, plane);
    if (missing > 0)
      std::cout << "lower: " << a.size() << " " << cdtA.number_of_faces() << " faces " << cdtA.number_of_vertices() << " vertices with " << missing << " missing ids" << std::endl;
*/

    /*
        std::ofstream vout("cdtA.polylines.txt");
        vout.precision(20);
        for (typename CDTplus::Face_handle fh : cdtA.finite_face_handles()) {
          vout << "4 ";
          vout << " " << from_exact(fh->vertex(0)->info().point_3);
          vout << " " << from_exact(fh->vertex(1)->info().point_3);
          vout << " " << from_exact(fh->vertex(2)->info().point_3);
          vout << " " << from_exact(fh->vertex(0)->info().point_3);
          vout << std::endl;
        }
        vout << std::endl;
        vout.close();*/

        /*
            for (Vertex_handle v : cdtA.finite_vertex_handles()) {
              if (v->info().idA2 == g && v->info().idB2 == g)
                newpts++;
            }

            std::cout << newpts << " vertices without references found in cdtA" << std::endl;*/

    build_cdt(cdtB, b_cdts, b_constraints, plane);

    // ToDo: remove checks
/*
    missing = check_cdt(cdtB, plane);
    if (missing > 0)
      std::cout << "upper: " << b.size() << " " << cdtB.number_of_faces() << " faces " << cdtB.number_of_vertices() << " vertices with " << missing << " missing ids" << std::endl;
*/

    /*
        std::ofstream vout2("cdtB.polylines.txt");
        vout2.precision(20);
        for (typename CDTplus::Face_handle fh : cdtB.finite_face_handles()) {
          vout2 << "4 ";
          vout2 << " " << from_exact(fh->vertex(0)->info().point_3);
          vout2 << " " << from_exact(fh->vertex(1)->info().point_3);
          vout2 << " " << from_exact(fh->vertex(2)->info().point_3);
          vout2 << " " << from_exact(fh->vertex(0)->info().point_3);
          vout2 << std::endl;
        }
        vout2 << std::endl;
        vout2.close();*/

        /*
            newpts = 0;
            for (Vertex_handle v : cdtB.finite_vertex_handles()) {
              if (v->info().idA2 == g && v->info().idB2 == g)
                newpts++;
            }

            std::cout << newpts << " vertices without references found in cdtB" << std::endl;*/

    overlay(cdtC, cdtA, a_constraints, cdtB, b_constraints, plane);

    //std::map<std::pair<Index, Index>, std::vector<Vertex_handle> > constraint2edge;
    // Use the adjacent set for the two vertices. That should allow to identify two faces

/*
    check for constraints IN the cdtC that have at least 3 vertices and create a map before with start and end vertices to volume
      like this I ran recover missing constraints
      check whether all constraints are collinear?*/

/*
                idx = 0;
          std::vector<Vertex_handle> vertices;
          typename CDTplus::Constraint_id id28, id84;
          for (typename CDTplus::Constraint_iterator ci = cdtC.constraints_begin(); ci != cdtC.constraints_end(); ++ci) {
            for (typename CDTplus::Vertices_in_constraint_iterator vi = cdtC.vertices_in_constraint_begin(*ci); vi != cdtC.vertices_in_constraint_end(*ci); vi++) {
              vertices.push_back(*vi);
            }

            if (vertices[0]->info().idA2.first == -1 || vertices.back()->info().idA2.first == -1)
              continue;

            if (!vertices[0]->info().input || !vertices.back()->info().input)
              continue;

            if (vertices.size() > 2) {
              if (vertices[0]->info().idA2 < vertices.back()->info().idA2)
                constraint2edge[std::make_pair(vertices[0]->info().idA2, vertices.back()->info().idA2)] = vertices;
              else
                constraint2edge[std::make_pair(vertices.back()->info().idA2, vertices[0]->info().idA2)] = vertices;
            }

            idx++;
            vertices.clear();
          }*/

    adapt_faces(cdtC, a, b, plane);

    // Are two functions needed to treat each side? I can also do a set intersection of the adjacent faces set of both end vertices of each constraint
    //std::cout << constraint2edge.size() << std::endl;

/*
    Provide checks here that plot some data around conflicting edges from a/b_constraints as well as from constraint2edge
    I can also make check_tjunctions more specific, now they provide many hits for a single case
    check for a case which edge is longer. Like this I have an indication which edge has not been split
    it may certainly be another case of CDT copy instead of inserting constraints*/

    idx = 0;
    for (auto& p : a_sets) {
      adapt_internal_edges(a_cdts[idx], cdtC, p.second, a_constraints[idx]);
      idx++;
    }

    idx = 0;
    for (auto& p : b_sets) {
      adapt_internal_edges(b_cdts[idx], cdtC, p.second, b_constraints[idx]);
      idx++;
    }

    // Is there linkage between the cdts? I could create a map of vertex Index to cdt vertices
    // I can create an unordered map from face Index to vector of cdt_face iterator

    // Each input face can be split into several faces
    // Updating the neighbor volumes does not seem difficult but not completely trivial either as it has to be done after the face extraction (due to the neighbors array in volumes)
    // -> each face extracted has the same volume ids (or the same face ids on both sides)

    // Walk around the edges to identify faces
    // - segment by identical face ids on both sides
    // How to keep track of the face vector in volumes? I can change the first face in place

    // How to identify edges that are split? Can I add a property on edges to mark that they have been added? Seems difficult, because the vertex can be part of plenty new edges.
    // Can it? The overlay is a fusion of 2 cdts, so if there are more than two edges intersecting in a vertex, there were already two edges intersecting in one of the cdts
    // So no, each new vertex can only be part of 2 edges

    // Adjusting edges part of faces that are not part of the splitting plane is basically a function of split_edge(Index_head, Index_tail, new_mid_vertex)
    // Identifying the faces based on the vertices seems costly. PEdge to PFaces exists, check for PFace to volume
    // // -> does not work for newly inserted edges! Newly inserted edges do not have PEdge!
    // Otherwise it is possible to find adjacent volumes based on the participating faces. However, an edge on the boundary can be part of many faces/volumes

    // Approach:
    // Loop over finite faces of fusioned cdt
    // check if face was already handled (-> reuse field in face info?)
    // check if face is on border to face of another index pair
    //  start face extraction
    //   follow border and build up polygon vector
    //   check if there is a vertex index in the vertex info, if not insert vertex into partition.data_structure and update
    //   create map for boundary vertices correspondences
    //  check if replace face in data structure or create new one (set which contains replaced ones?)
  }

  void make_conformal(Octree_node node)  {
    // pts2index maps exact points to their indices with one unique index.
    // index_map maps indices to unique indices. Used to map the points inside a partition to a unique index.

    // Nothing to do for a leaf node.
    if (m_octree->is_leaf(node))
      return;

    // Make childs conformal
    for (std::size_t i = 0; i < 8; i++)
      make_conformal(m_octree->child(node, i));

    // Make itself conformal
    // Get faces between child nodes
    // do in inverse dimension order (like inverse splitting order, start by 2 or 1 and walk down to 0)
    // follow cdt approach in split_octree

    // Order of children?
    // x, y, z planes can be merged independently
    for (std::size_t dim = 0; dim < 3; dim++) {
      std::vector<Index> lower, upper;
      typename Intersection_kernel::Plane_3 plane;

      collect_opposing_faces(node, dim, lower, upper, plane);

      make_conformal(lower, upper, plane);

/*
      lower.clear();
      upper.clear();
      // Todo: remove check
      collect_opposing_faces(node, dim, lower, upper, plane);

      for (std::size_t i = 0; i < lower.size(); i++) {
        auto n = neighbors(lower[i]);
        assert(n.first >= 0 && n.second >= 0);
      }

      for (std::size_t i = 0; i < upper.size(); i++) {
        auto n = neighbors(upper[i]);
        assert(n.first >= 0 && n.second >= 0);
      }*/
    }
  }

  void split_faces(std::size_t idx, std::size_t other, std::size_t sp_idx, std::vector<std::vector<std::size_t> > &faces, std::vector<std::pair<std::size_t, std::size_t> >& face_idx, std::vector<typename Intersection_kernel::Point_3> &vertices, std::vector<std::size_t> &planes) {
    typename Intersection_kernel::Plane_3 plane = m_partition_nodes[idx].m_data->support_plane(sp_idx).exact_plane();
    std::vector<typename Intersection_kernel::Point_2> v2d(vertices.size());
    for (std::size_t i = 0; i < vertices.size(); i++)
      v2d[i] = plane.to_2d(vertices[i]);

    From_exact from_exact;

    for (std::size_t pl : planes) {
      typename Intersection_kernel::Line_3 line;
      bool intersect = Data_structure::intersection(plane, m_partition_nodes[other].m_data->support_plane(pl).exact_plane(), line);
      CGAL_assertion(intersect);
      typename Intersection_kernel::Line_2 l2 = m_partition_nodes[idx].m_data->support_plane(sp_idx).to_2d(line);
      //typename Kernel::Line_2 l2 = from_exact(l2_exact);

      std::size_t num_faces = faces.size();

      for (std::size_t f = 0; f < faces.size(); f++) {
        bool neg = false, pos = false;
        for (std::size_t p : faces[f]) {
          CGAL::Oriented_side s = l2.oriented_side(v2d[p]);
          if (s == CGAL::ON_POSITIVE_SIDE) {
            if (neg) {
              CGAL_assertion(f < num_faces);
              split_face(idx, f, faces, face_idx, v2d, plane, vertices, l2);
              break;
            }
            else pos = true;
          }

          if (s == CGAL::ON_NEGATIVE_SIDE) {
            if (pos) {
              CGAL_assertion(f < num_faces);
              split_face(idx, f, faces, face_idx, v2d, plane, vertices, l2);
              break;
            }
            else neg = true;
          }
        }
      }
    }
  }

  void split_face(std::size_t partition, std::size_t f, std::vector<std::vector<std::size_t> >& faces, std::vector<std::pair<std::size_t, std::size_t> > &face_idx, std::vector<typename Intersection_kernel::Point_2> &v2d, typename Intersection_kernel::Plane_3 &plane, std::vector<typename Intersection_kernel::Point_3> &pts, const typename Intersection_kernel::Line_2 &line) {
    std::vector<std::size_t> pos, neg;
    From_exact from_exact;

    const std::string vfilename = std::to_string(f) + "-before.polylines.txt";
    std::ofstream vout(vfilename);
    vout.precision(20);
    vout << std::to_string(faces[f].size() + 1);
    for (auto p : faces[f]) {
      vout << " " << from_exact(pts[p]);
    }
    vout << " " << from_exact(pts[faces[f][0]]);
    vout << std::endl;
    vout.close();

    CGAL::Oriented_side former = line.oriented_side(v2d[faces[f][0]]);

    if (former == CGAL::ON_POSITIVE_SIDE || former== CGAL::ON_ORIENTED_BOUNDARY)
      pos.push_back(faces[f][0]);

    if (former == CGAL::ON_NEGATIVE_SIDE || former == CGAL::ON_ORIENTED_BOUNDARY)
      neg.push_back(faces[f][0]);

    for (std::size_t i = 1; i < faces[f].size() + 1; i++) {
      // Wrap around index
      std::size_t idx = i % faces[f].size();
      CGAL::Oriented_side cur = line.oriented_side(v2d[faces[f][idx]]);
      if (cur == CGAL::ON_ORIENTED_BOUNDARY) {
        neg.push_back(faces[f][idx]);
        pos.push_back(faces[f][idx]);
        former = cur;
        continue;
      }

      // Switching sides without stepping on the line.
      if (cur != former && cur != CGAL::ON_ORIENTED_BOUNDARY && former != CGAL::ON_ORIENTED_BOUNDARY) {
        typename Intersection_kernel::Point_2 p;
        bool intersect = Data_structure::intersection(typename Intersection_kernel::Line_2(v2d[faces[f][idx]], v2d[faces[f][i - 1]]), line, p);
        v2d.push_back(p);
        pts.push_back(plane.to_3d(p));
        pos.push_back(v2d.size() - 1);
        neg.push_back(v2d.size() - 1);
      }

      if (cur == CGAL::ON_POSITIVE_SIDE) {
        pos.push_back(faces[f][idx]);
      }

      if (cur == CGAL::ON_NEGATIVE_SIDE) {
        neg.push_back(faces[f][idx]);
      }

      former = cur;
    }

    bool replaced = false;
    auto& face2vertices = m_partition_nodes[partition].m_data->face_to_vertices();
    auto& volumes = m_partition_nodes[partition].m_data->volumes();

    if (neg.size() > 2) {
      // Check collinearity
      bool collinear = true;
      for (std::size_t i = 2; i < neg.size(); i++) {
        if (!CGAL::collinear(v2d[neg[0]], v2d[neg[1]], v2d[neg[i]])) {
          collinear = false;
          break;
        }
      }

      faces[f] = neg;
      face2vertices[volumes[face_idx[f].first].faces[face_idx[f].second]] = neg;
      replaced = true;
    }

    if (pos.size() > 2) {
      // Check collinearity
      bool collinear = true;
      for (std::size_t i = 2; i < pos.size(); i++) {
        if (!CGAL::collinear(v2d[pos[0]], v2d[pos[1]], v2d[pos[i]])) {
          collinear = false;
          break;
        }
      }
      if (replaced) {
        faces.push_back(pos);
        face2vertices.push_back(pos);
        volumes[face_idx[f].first].faces.push_back(face2vertices.size());
        volumes[face_idx[f].first].neighbors.push_back(volumes[face_idx[f].first].neighbors[face_idx[f].second]);
        volumes[face_idx[f].first].pfaces.push_back(volumes[face_idx[f].first].pfaces[face_idx[f].second]);
        volumes[face_idx[f].first].pface_oriented_outwards.push_back(volumes[face_idx[f].first].pface_oriented_outwards[face_idx[f].second]);
        face_idx.push_back(std::make_pair(face_idx[f].first, volumes[face_idx[f].first].faces.size() - 1));

        m_partition_nodes[partition].m_data->face_to_support_plane().push_back(-1);
        m_partition_nodes[partition].m_data->face_to_volumes().push_back(std::make_pair(-1, -1));
      }
      else {
        faces[f] = pos;
        face2vertices[volumes[face_idx[f].first].faces[face_idx[f].second]] = pos;
      }
    }

    const std::string vfilename2 = std::to_string(f) + "-pos.polylines.txt";
    std::ofstream vout2(vfilename2);
    vout2.precision(20);
    vout2 << std::to_string(pos.size() + 1);
    for (auto p : pos) {
      vout2 << " " << from_exact(pts[p]);
    }
    vout2 << " " << from_exact(pts[pos[0]]);
    vout2 << std::endl;
    vout2.close();

    const std::string vfilename3 = std::to_string(f) + "-neg.polylines.txt";
    std::ofstream vout3(vfilename3);
    vout3.precision(20);
    vout3 << std::to_string(neg.size() + 1);
    for (auto p : neg) {
      vout3 << " " << from_exact(pts[p]);
    }
    vout3 << " " << from_exact(pts[neg[0]]);
    vout3 << std::endl;
    vout3.close();
  }

  void split_partition(std::size_t idx) {
    // Assuming the bbox is axis-aligned

    if (m_partition_nodes[idx].parent != -1) {
      m_partitions.push_back(idx);
      return;
    }

    // Create two children
    m_partition_nodes.resize(m_partition_nodes.size() + 2);

    std::size_t lower_y = m_partition_nodes.size() - 2;
    std::size_t upper_y = lower_y + 1;

    m_partition_nodes[idx].children.push_back(lower_y);
    m_partition_nodes[idx].children.push_back(upper_y);

    m_partition_nodes[lower_y].parent = idx;
    m_partition_nodes[upper_y].parent = idx;

    FT split = (m_partition_nodes[idx].bbox[0].y() + m_partition_nodes[idx].bbox[2].y()) * 0.5;

    // Create bbox and fill in support planes
    //partition2bbox[0] = Bbox_3(bbox.xmin(), bbox.ymin(), bbox.zmin(), bbox.xmax(), split, bbox.zmax());
    //partition2bbox[1] = Bbox_3(bbox.xmin(), split, bbox.zmin(), bbox.xmax(), bbox.ymax(), bbox.zmax());

    // Copy 4 bbox corner points on the lower y side to lower_y partition
    m_partition_nodes[lower_y].bbox[0] = m_partition_nodes[idx].bbox[0];
    m_partition_nodes[lower_y].bbox[1] = m_partition_nodes[idx].bbox[1];
    m_partition_nodes[lower_y].bbox[5] = m_partition_nodes[idx].bbox[5];
    m_partition_nodes[lower_y].bbox[6] = m_partition_nodes[idx].bbox[6];

    // Copy 4 bbox corner points on the upper y side to upper_y partition
    m_partition_nodes[upper_y].bbox[2] = m_partition_nodes[idx].bbox[2];
    m_partition_nodes[upper_y].bbox[3] = m_partition_nodes[idx].bbox[3];
    m_partition_nodes[upper_y].bbox[4] = m_partition_nodes[idx].bbox[4];
    m_partition_nodes[upper_y].bbox[7] = m_partition_nodes[idx].bbox[7];

    // Insert new bbox on split plane
    m_partition_nodes[lower_y].bbox[2] = m_partition_nodes[upper_y].bbox[1] = Point_3(m_partition_nodes[idx].bbox[1].x(), split, m_partition_nodes[idx].bbox[1].z());
    m_partition_nodes[lower_y].bbox[3] = m_partition_nodes[upper_y].bbox[0] = Point_3(m_partition_nodes[idx].bbox[3].x(), split, m_partition_nodes[idx].bbox[3].z());
    m_partition_nodes[lower_y].bbox[4] = m_partition_nodes[upper_y].bbox[5] = Point_3(m_partition_nodes[idx].bbox[4].x(), split, m_partition_nodes[idx].bbox[4].z());
    m_partition_nodes[lower_y].bbox[7] = m_partition_nodes[upper_y].bbox[6] = Point_3(m_partition_nodes[idx].bbox[6].x(), split, m_partition_nodes[idx].bbox[6].z());

    for (std::size_t i = 0; i < m_partition_nodes[idx].input_polygons.size(); i++) {
      bool neg = false, pos = false;
      for (const Point_3& p : m_input_polygons[m_partition_nodes[idx].input_polygons[i]]) {
        if (!neg && p.y() < split) {
          neg = true;
          m_partition_nodes[lower_y].input_polygons.push_back(i);
          if (pos)
            break;
        }
        else if (!pos && p.y() > split) {
          pos = true;
          m_partition_nodes[upper_y].input_polygons.push_back(i);
          if (neg)
            break;
        }
      }
    }

    m_partition_nodes[lower_y].split_plane = 3;
    m_partition_nodes[upper_y].split_plane = 1;

    split_partition(lower_y);
    split_partition(upper_y);
  }

  void split_octree() {
    // Octree creation for sub partition
    std::size_t count = 0;
    for (const auto& p : m_input_polygons)
      count += p.size();

    m_points.clear();
    m_points.reserve(count);
    m_polygons.reserve(m_input_polygons.size());

    for (const auto& p : m_input_polygons) {
      std::size_t idx = m_points.size();
      std::copy(p.begin(), p.end(), std::back_inserter(m_points));
      m_polygons.push_back(std::vector<std::size_t>(p.size()));
      std::iota(m_polygons.back().begin(), m_polygons.back().end(), idx);
    }

    m_octree = std::make_unique<Octree>(CGAL::Orthtree_traits_polygons<Kernel>(m_points, m_polygons, m_parameters.bbox_dilation_ratio));
    m_octree->refine(m_parameters.max_octree_depth, m_parameters.max_octree_node_size);

    std::size_t leaf_count = 0;
    std::size_t max_count = 0;

    for (Octree::Node_index node : m_octree->traverse(CGAL::Orthtrees::Leaves_traversal<Octree>(*m_octree))) {
      if (m_octree->is_leaf(node))
        leaf_count++;
      else
        std::cout << "Leaves_traversal traverses non-leaves" << std::endl;
      max_count = (std::max<std::size_t>)(max_count, node);
    }

    m_partition_nodes.resize(leaf_count);

    m_node2partition.resize(max_count + 1, std::size_t(-1));

    std::size_t idx = 0;
    for (Octree::Node_index node : m_octree->traverse(CGAL::Orthtrees::Leaves_traversal<Octree>(*m_octree)))
      if (m_octree->is_leaf(node)) {
        // Creating bounding box
        CGAL::Iso_cuboid_3<Kernel> box = m_octree->bbox(node);
        m_partition_nodes[idx].bbox[0] = typename Intersection_kernel::Point_3(box.xmin(), box.ymin(), box.zmin());
        m_partition_nodes[idx].bbox[1] = typename Intersection_kernel::Point_3(box.xmax(), box.ymin(), box.zmin());
        m_partition_nodes[idx].bbox[2] = typename Intersection_kernel::Point_3(box.xmax(), box.ymax(), box.zmin());
        m_partition_nodes[idx].bbox[3] = typename Intersection_kernel::Point_3(box.xmin(), box.ymax(), box.zmin());
        m_partition_nodes[idx].bbox[4] = typename Intersection_kernel::Point_3(box.xmin(), box.ymax(), box.zmax());
        m_partition_nodes[idx].bbox[5] = typename Intersection_kernel::Point_3(box.xmin(), box.ymin(), box.zmax());
        m_partition_nodes[idx].bbox[6] = typename Intersection_kernel::Point_3(box.xmax(), box.ymin(), box.zmax());
        m_partition_nodes[idx].bbox[7] = typename Intersection_kernel::Point_3(box.xmax(), box.ymax(), box.zmax());

/*
        auto bbox = m_octree->bbox(i);
        m_partition_nodes[idx].bbox[0] = Point_3(bbox.xmin(), bbox.ymin(), bbox.zmin());
        m_partition_nodes[idx].bbox[1] = Point_3(bbox.xmax(), bbox.ymin(), bbox.zmin());
        m_partition_nodes[idx].bbox[2] = Point_3(bbox.xmax(), bbox.ymax(), bbox.zmin());
        m_partition_nodes[idx].bbox[3] = Point_3(bbox.xmin(), bbox.ymax(), bbox.zmin());
        m_partition_nodes[idx].bbox[4] = Point_3(bbox.xmin(), bbox.ymax(), bbox.zmax());
        m_partition_nodes[idx].bbox[5] = Point_3(bbox.xmin(), bbox.ymin(), bbox.zmax());
        m_partition_nodes[idx].bbox[6] = Point_3(bbox.xmax(), bbox.ymin(), bbox.zmax());
        m_partition_nodes[idx].bbox[7] = Point_3(bbox.xmax(), bbox.ymax(), bbox.zmax());*/

        // Get consistent Plane_3 from Octree to generate exact planes

        auto polys = m_octree->data(node);
        for (std::size_t j = 0; j < polys.size(); j++) {
          m_partition_nodes[idx].input_polygons.push_back(polys[j].first);
          m_partition_nodes[idx].m_input_planes.push_back(m_input_planes[polys[j].first]);
        }

        m_partition_nodes[idx].clipped_polygons.resize(polys.size());
        for (std::size_t i = 0; i < polys.size(); i++) {
          m_partition_nodes[idx].clipped_polygons[i].resize(polys[i].second.size());
          for (std::size_t j = 0; j < polys[i].second.size(); j++)
            m_partition_nodes[idx].clipped_polygons[i][j] = polys[i].second[j];
        }

        // set node index
        m_partition_nodes[idx].node = node;
        m_node2partition[node] = idx;

        if (m_parameters.debug) {
          const std::string vfilename = std::to_string(idx) + "-box.polylines.txt";
          std::ofstream vout(vfilename);
          vout.precision(20);
          // zmin side
          vout << 5;
          vout << " " << m_partition_nodes[idx].bbox[0];
          vout << " " << m_partition_nodes[idx].bbox[1];
          vout << " " << m_partition_nodes[idx].bbox[2];
          vout << " " << m_partition_nodes[idx].bbox[3];
          vout << " " << m_partition_nodes[idx].bbox[0];
          // zmax side
          vout << std::endl << 5;
          vout << " " << m_partition_nodes[idx].bbox[4];
          vout << " " << m_partition_nodes[idx].bbox[5];
          vout << " " << m_partition_nodes[idx].bbox[6];
          vout << " " << m_partition_nodes[idx].bbox[7];
          vout << " " << m_partition_nodes[idx].bbox[4];
          // 4 edges between zmin and zmax
          vout << std::endl << 2;
          vout << " " << m_partition_nodes[idx].bbox[0];
          vout << " " << m_partition_nodes[idx].bbox[5];
          vout << std::endl << 2;
          vout << " " << m_partition_nodes[idx].bbox[1];
          vout << " " << m_partition_nodes[idx].bbox[6];
          vout << std::endl << 2;
          vout << " " << m_partition_nodes[idx].bbox[2];
          vout << " " << m_partition_nodes[idx].bbox[7];
          vout << std::endl << 2;
          vout << " " << m_partition_nodes[idx].bbox[3];
          vout << " " << m_partition_nodes[idx].bbox[4];
          vout << std::endl;
          vout.close();

          KSR_3::dump_polygons(m_partition_nodes[idx].clipped_polygons, std::to_string(idx) + "-polys.ply");
        }
        idx++;
      }

    std::cout << "input split into " << m_partition_nodes.size() << " partitions" << std::endl;
  }

  bool within_tolerance(const Plane_3& p1, const Point_2 &c1, const Plane_3& p2, const Point_2& c2) const {
    using FT = typename GeomTraits::FT;

    const auto va = p1.orthogonal_vector();
    const auto vb = p2.orthogonal_vector();

    // Are the planes parallel?
    // const FT vtol = KSR::vector_tolerance<FT>();
    // const FT aval = CGAL::abs(va * vb);

    // std::cout << "aval: " << aval << " : " << vtol << std::endl;
    // if (aval < vtol) {
    //   return false;
    // }

    FT aval = approximate_angle(va, vb);
    CGAL_assertion(aval >= FT(0) && aval <= FT(180));
    if (aval >= FT(90))
      aval = FT(180) - aval;

    if (aval >= m_parameters.angle_tolerance) {
      return false;
    }

    const auto pa1 = p1.to_3d(c1);
    const auto pb1 = p2.projection(pa1);
    const auto pb2 = p2.to_3d(c2);
    const auto pa2 = p1.projection(pb2);

    const FT bval1 = KSR::distance(pa1, pb1);
    const FT bval2 = KSR::distance(pa2, pb2);
    const FT bval = (CGAL::max)(bval1, bval2);
    CGAL_assertion(bval >= FT(0));

    if (bval >= m_parameters.distance_tolerance)
      return false;

    return true;
  }

  /*

  template<typename FaceOutputIterator>
  FaceOutputIterator output_partition_faces(
    FaceOutputIterator faces, KSR::Indexer<IVertex>& indexer,
    const std::size_t sp_idx) const {

    std::vector<std::size_t> face;
    const auto all_pfaces = m_data.pfaces(sp_idx);
    for (const auto pface : all_pfaces) {
      face.clear();
      const auto pvertices = m_data.pvertices_of_pface(pface);
      for (const auto pvertex : pvertices) {
        CGAL_assertion(m_data.has_ivertex(pvertex));
        const auto ivertex = m_data.ivertex(pvertex);
        const std::size_t idx = indexer(ivertex);
        face.push_back(idx);
      }
      *(faces++) = face;
    }
    return faces;
  }*/
};

} // namespace CGAL

#endif // CGAL_KINETIC_SHAPE_PARTITION_3_H
