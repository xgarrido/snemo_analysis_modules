/// \file topology_data_plotter.h
/* Author(s)     : Xavier Garrido <garrido@lal.in2p3.fr>
 * Creation date : 2015-05-26
 * Last modified : 2015-05-26
 *
 * Copyright (C) 2015 Xavier Garrido <garrido@lal.in2p3.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Description:
 *
 *   Topology data bank plotter.
 *
 * History:
 *
 */

#ifndef SNEMO_ANALYSIS_TOPOLOGY_DATA_PLOTTER_H
#define SNEMO_ANALYSIS_TOPOLOGY_DATA_PLOTTER_H 1

// This project:
#include <base_plotter.h>

namespace snemo {
// Forward declaration:
namespace datamodel {
  class topology_data;
  class topology_1e_pattern;
  class topology_1eNg_pattern;
  class topology_1e1a_pattern;
  class topology_2e_pattern;
  class topology_2eNg_pattern;
}
namespace analysis {

  /// \brief Topology data plotter class
  class topology_data_plotter : public ::snemo::analysis::base_plotter
  {
  public:

    /// Return id plotter
    static const std::string & get_id();

    /// Default constructor
    topology_data_plotter(datatools::logger::priority p_ = datatools::logger::PRIO_FATAL);

    /// Destructor
    virtual ~topology_data_plotter();

    /// The main initialization method (post-construction):
    virtual void initialize(const datatools::properties & config_);

    /// The main data model plotting method
    virtual void plot(const datatools::things & data_model_);

    /// The main termination method
    virtual void reset();

    /// OCD support
    static void init_ocd(datatools::object_configuration_description & ocd_);

  protected:

    /// Default class member value
    void _set_defaults();

    /// Specialized method for plotting 'TD' bank
    void _plot(const snemo::datamodel::topology_data & td_);

  private:

    /// Dedicated plot method for '1e' topology pattern
    void _plot_1e_(const snemo::datamodel::topology_1e_pattern & pattern_,
                   const std::string & prefix_ = "TD::1e::");

    /// Dedicated plot method for '1e1a' topology pattern
    void _plot_1e1a_(const snemo::datamodel::topology_1e1a_pattern & pattern_,
                     const std::string & prefix_ = "TD::1e1a::");

    /// Dedicated plot method for '1eNg' topology pattern
    void _plot_1eNg_(const snemo::datamodel::topology_1eNg_pattern & pattern_,
                     const std::string & prefix_ = "TD::1e");

    /// Dedicated plot method for '2e' topology pattern
    void _plot_2e_(const snemo::datamodel::topology_2e_pattern & pattern_,
                   const std::string & prefix_ = "TD::2e::");

    /// Dedicated plot method for '2eNg' topology pattern
    void _plot_2eNg_(const snemo::datamodel::topology_2eNg_pattern & pattern_,
                     const std::string & prefix_ = "TD::2e");

  };

}  // end of namespace analysis
} // end of namespace snemo

#include <datatools/ocd_macros.h>

// Declare the OCD interface of the module
DOCD_CLASS_DECLARATION(snemo::analysis::topology_data_plotter)

#endif // SNEMO_ANALYSIS_TOPOLOGY_DATA_PLOTTER_H

/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
