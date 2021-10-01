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

#ifndef CGAL_KSR_3_EVENT_H
#define CGAL_KSR_3_EVENT_H

// #include <CGAL/license/Kinetic_shape_reconstruction.h>

// CGAL includes.
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

// Internal includes.
#include <CGAL/KSR/utils.h>

namespace CGAL {
namespace KSR_3 {

template<typename Data_structure>
class Event_queue;

// This class works only with inexact number types because it is a base for the
// multi index container in the Event_queue class, which cannot handle exact number types.
template<typename Data_structure>
class Event {

public:
  // Kernel types.
  using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
  using NT     = typename Data_structure::Kernel::FT;
  using FT     = typename Kernel::FT;

  // Data structure types.
  using PVertex = typename Data_structure::PVertex;
  using PEdge   = typename Data_structure::PEdge;
  using PFace   = typename Data_structure::PFace;
  using IVertex = typename Data_structure::IVertex;
  using IEdge   = typename Data_structure::IEdge;

  // Event queue types.
  using Queue = Event_queue<Data_structure>;
  friend Queue;

  struct ETime {
    ETime(
      const NT event_time,
      const bool is_pv_to_iv,
      const bool is_vt = false) :
    m_time(static_cast<FT>(CGAL::to_double(event_time))),
    m_is_pvertex_to_ivertex(is_pv_to_iv), m_is_virtual(is_vt)
    { }

  private:
    const FT m_time;
    const bool m_is_pvertex_to_ivertex;
    const bool m_is_virtual;

  public:
    bool operator<(const ETime& e) const {

      const FT tol = KSR::tolerance<FT>();
      const FT time_diff = CGAL::abs(this->time() - e.time());
      if (time_diff < tol && !this->is_virtual() && !e.is_virtual()) {
        const std::size_t la = this->is_pvertex_to_ivertex() ? 1 : 0;
        const std::size_t lb = e.is_pvertex_to_ivertex() ? 1 : 0;

        // std::cout << "la: " << la << ", time: " << this->time() << std::endl;
        // std::cout << "lb: " << lb << ", time: " << e.time() << std::endl;

        if (la != lb) return la < lb;
      }
      return this->time() < e.time();
    }

    FT time() const { return m_time; }
    bool is_pvertex_to_ivertex() const { return m_is_pvertex_to_ivertex; }
    bool is_virtual() const { return m_is_virtual; }
  };

  // Event types.

  // Empty event.
  Event() :
  m_is_constrained(false),
  m_pvertex(Data_structure::null_pvertex()),
  m_pother(Data_structure::null_pvertex()),
  m_ivertex(Data_structure::null_ivertex()),
  m_iedge(Data_structure::null_iedge()),
  m_time(ETime(NT(0), (
    m_pother  == Data_structure::null_pvertex() &&
    m_ivertex != Data_structure::null_ivertex()))),
  m_support_plane_idx(m_pvertex.first)
  { }

  // An event that occurs between two polygon vertices.
  Event(
    const bool is_constrained,
    const PVertex pvertex,
    const PVertex pother,
    const NT time) :
  m_is_constrained(is_constrained),
  m_pvertex(pvertex),
  m_pother(pother),
  m_ivertex(Data_structure::null_ivertex()),
  m_iedge(Data_structure::null_iedge()),
  m_time(ETime(time, (
    m_pother  == Data_structure::null_pvertex() &&
    m_ivertex != Data_structure::null_ivertex()))),
  m_support_plane_idx(m_pvertex.first) {

    CGAL_assertion_msg(is_constrained,
    "ERROR: THIS EVENT CANNOT EVER HAPPEN IN THE UNCONSTRAINED SETTING!");
  }

  // An event that occurs between a polygon vertex and an intersection graph edge.
  Event(
    const bool is_constrained,
    const PVertex pvertex,
    const IEdge iedge,
    const NT time) :
  m_is_constrained(is_constrained),
  m_pvertex(pvertex),
  m_pother(Data_structure::null_pvertex()),
  m_ivertex(Data_structure::null_ivertex()),
  m_iedge(iedge),
  m_time(ETime(time, (
    m_pother  == Data_structure::null_pvertex() &&
    m_ivertex != Data_structure::null_ivertex()))),
  m_support_plane_idx(m_pvertex.first) {

    CGAL_assertion_msg(!is_constrained,
    "ERROR: THIS EVENT CANNOT EVER HAPPEN IN THE CONSTRAINED SETTING!");
  }

  // An event that occurs between a polygon vertex and an intersection graph vertex.
  Event(
    const bool is_constrained,
    const PVertex pvertex,
    const IVertex ivertex,
    const NT time) :
  m_is_constrained(is_constrained),
  m_pvertex(pvertex),
  m_pother(Data_structure::null_pvertex()),
  m_ivertex(ivertex),
  m_iedge(Data_structure::null_iedge()),
  m_time(ETime(time, (
    m_pother  == Data_structure::null_pvertex() &&
    m_ivertex != Data_structure::null_ivertex()))),
  m_support_plane_idx(m_pvertex.first)
  { }

  // An event that occurs between two polygon vertices and an intersection graph vertex.
  Event(
    const bool is_constrained,
    const PVertex pvertex,
    const PVertex pother,
    const IVertex ivertex,
    const NT time) :
  m_is_constrained(is_constrained),
  m_pvertex(pvertex),
  m_pother(pother),
  m_ivertex(ivertex),
  m_iedge(Data_structure::null_iedge()),
  m_time(ETime(time, (
    m_pother  == Data_structure::null_pvertex() &&
    m_ivertex != Data_structure::null_ivertex()))),
  m_support_plane_idx(m_pvertex.first) {

    CGAL_assertion_msg(is_constrained,
    "ERROR: THIS EVENT CANNOT EVER HAPPEN IN THE UNCONSTRAINED SETTING!");
  }

  bool operator<(const Event& e) const {
    return time() < e.time();
  }

  // Data access.
  const PVertex& pvertex() const { return m_pvertex; }
  const PVertex& pother() const { return m_pother; }
  const IVertex& ivertex() const { return m_ivertex; }
  const IEdge& iedge() const { return m_iedge; }
  NT time() const { return static_cast<NT>(m_time.time()); }
  std::size_t support_plane() const { return m_support_plane_idx; }

  // Predicates.
  bool is_constrained() const { return m_is_constrained; }

  // Event types. See constructors above.
  bool is_pvertex_to_pvertex() const {
    return (pother() != Data_structure::null_pvertex()); }
  bool is_pvertex_to_iedge()   const {
    return (iedge() != Data_structure::null_iedge()); }
  bool is_pvertex_to_ivertex() const {
    return (pother() == Data_structure::null_pvertex() && ivertex() != Data_structure::null_ivertex()); }
  bool is_pvertices_to_ivertex() const {
    return (pother() != Data_structure::null_pvertex() && ivertex() != Data_structure::null_ivertex()); }

  // Output.
  friend std::ostream& operator<<(std::ostream& os, const Event& event) {

    const std::string constr_type = ( event.is_constrained() ? "constrained " : "unconstrained " );
    if (event.is_pvertices_to_ivertex()) {
      os << constr_type << "event at t = " << event.time() << " between PVertex("
         << event.pvertex().first << ":" << event.pvertex().second
         << "), PVertex(" << event.pother().first << ":" << event.pother().second
         << "), and IVertex(" << event.ivertex() << ")";
    } else if (event.is_pvertex_to_pvertex()) {
      os << constr_type << "event at t = " << event.time() << " between PVertex("
         << event.pvertex().first << ":" << event.pvertex().second
         << ") and PVertex(" << event.pother().first << ":" << event.pother().second << ")";
    } else if (event.is_pvertex_to_iedge()) {
      os << constr_type << "event at t = " << event.time() << " between PVertex("
         << event.pvertex().first << ":" << event.pvertex().second
         << ") and IEdge" << event.iedge();
    } else if (event.is_pvertex_to_ivertex()) {
      os << constr_type << "event at t = " << event.time() << " between PVertex("
         << event.pvertex().first << ":" << event.pvertex().second
         << ") and IVertex(" << event.ivertex() << ")";
    } else {
      os << "ERROR: INVALID EVENT at t = " << event.time();
    }
    return os;
  }

private:
  bool m_is_constrained;
  PVertex m_pvertex;
  PVertex m_pother;
  IVertex m_ivertex;
  IEdge   m_iedge;
  ETime   m_time;
  std::size_t m_support_plane_idx;
};

} // namespace KSR_3
} // namespace CGAL

#endif // CGAL_KSR_3_EVENT_H
