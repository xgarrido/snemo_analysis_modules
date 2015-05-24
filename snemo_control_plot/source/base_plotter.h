/// \file base_plotter.h
/* Author(s)     : Xavier Garrido <garrido@lal.in2p3.fr>
 * Creation date : 2015-05-24
 * Last modified : 2013-12-13
 *
 * Copyright (C) 2015 Francois Mauger <mauger@lpccaen.in2p3.fr>
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
 *   Base plotter.
 *
 * History:
 *
 */

#ifndef ANALYSIS_BASE_PLOTTER_H
#define ANALYSIS_BASE_PLOTTER_H 1

// Standard library:
#include <string>

// Third party:
// - Bayeux/datatools:
#include <datatools/i_tree_dump.h>
#include <datatools/logger.h>

namespace datatools {
  // Forward class declarations :
  class properties;
  class things;
}

namespace analysis {

  /// \brief Base plotter class (abstract interface)
  class base_plotter : public datatools::i_tree_dumpable
  {
  public:

    /// Default constructor :
    base_plotter(datatools::logger::priority p = datatools::logger::PRIO_FATAL);

    /// Destructor :
    virtual ~base_plotter();

    /// Check the plotter name
    bool has_name() const;

    /// Set the plotter name
    void set_name(const std::string &);

    /// Return the plotter name
    const std::string & get_name() const;

    /// Check the plotter description
    bool has_description() const;

    /// Return the plotter description
    const std::string & get_description() const;

    /// Set the plotter description
    void set_description(const std::string & a_description);

    /// Check initialization flag
    bool is_initialized() const;

    /// The main initialization method (post-construction):
    virtual void initialize(const datatools::properties & a_config) = 0;

    /// The main data model plotting method
    virtual void plot(datatools::things & a_data_model) = 0;

    /// The main termination method
    virtual void reset() = 0;

    /// Smart print
    virtual void tree_dump(std::ostream &      a_out = std::clog,
                           const std::string & a_title  = "",
                           const std::string & a_indent = "",
                           bool                a_inherit = false) const;

    /// Default print
    void print(std::ostream & a_out = std::clog) const;

    /// Set logging priority
    void set_logging_priority(datatools::logger::priority p);

    /// Returns logging priority
    datatools::logger::priority get_logging_priority() const;

    /// Basic OCD support shared by all inherited modules
    static void common_ocd(datatools::object_configuration_description & ocd_);

  protected:

    /// Set the name of the module
    void _set_name(const std::string & a_name);

    /// Set the initialization flag of the module
    void _set_initialized(bool a_initialized);

    /// Basic initialization shared by all inherited modules
    void _common_initialize(const datatools::properties & a_config);

  protected:

    std::string _name;           //!< The name of the module
    std::string _description;    //!< The description of the module
    datatools::logger::priority _logging; //!< The logging priority threshold

    bool        _initialized;    //!< The initialization flag

  };

}  // end of namespace analysis

#endif // ANALYSIS_BASE_PLOTTER_H

/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
