/// \file base_plotter.h
/* Author(s)     : Xavier Garrido <garrido@lal.in2p3.fr>
 * Creation date : 2015-05-24
 * Last modified : 2015-05-24
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
 *   Base plotter.
 *
 * History:
 *
 */

#ifndef SNEMO_ANALYSIS_BASE_PLOTTER_H
#define SNEMO_ANALYSIS_BASE_PLOTTER_H 1

// Standard library:
#include <string>

// Third party:
// - Bayeux/datatools:
#include <bayeux/datatools/i_tree_dump.h>
#include <bayeux/datatools/logger.h>

// Forward declarations
namespace datatools {
  // Forward class declarations :
  class properties;
  class things;
}

namespace mygsl {
  class histogram_pool;
}

namespace snemo {
namespace analysis {

  /// \brief Base plotter class (abstract interface)
  class base_plotter : public datatools::i_tree_dumpable
  {
  public:

    /// Check histogram pool existence
    bool has_histogram_pool() const;

    /// Set histogram pool pointer
    void set_histogram_pool(mygsl::histogram_pool & pool_);

    /// Return a mutable reference to histogra pool
    mygsl::histogram_pool & grab_histogram_pool();

    /// Default constructor
    base_plotter(datatools::logger::priority p_ = datatools::logger::PRIO_FATAL);

    /// Destructor
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
    void set_description(const std::string & description_);

    /// Check initialization flag
    bool is_initialized() const;

    /// The main initialization method (post-construction):
    virtual void initialize(const datatools::properties & config_) = 0;

    /// The main data model plotting method
    virtual void plot(const datatools::things & data_model_) = 0;

    /// The main termination method
    virtual void reset() = 0;

    /// Smart print
    virtual void tree_dump(std::ostream &      out_ = std::clog,
                           const std::string & title_  = "",
                           const std::string & indent_ = "",
                           bool                inherit_ = false) const;

    /// Set logging priority
    void set_logging_priority(datatools::logger::priority p_);

    /// Returns logging priority
    datatools::logger::priority get_logging_priority() const;

    /// Basic OCD support shared by all inherited modules
    static void common_ocd(datatools::object_configuration_description & ocd_);

  protected:

    /// Set the name of the module
    void _set_name(const std::string & name_);

    /// Set the initialization flag of the module
    void _set_initialized(bool initialized_);

    /// Basic initialization shared by all inherited modules
    void _common_initialize(const datatools::properties & config_);

  protected:

    bool _initialized;                    //!< The initialization flag
    datatools::logger::priority _logging; //!< The logging priority threshold
    std::string _name;                    //!< The name of the module
    std::string _description;             //!< The description of the module

  private:

    mygsl::histogram_pool * _histogram_pool_;//!< Histogram pool

  };

} // end of namespace snemo
} // end of namespace analysis

#endif // SNEMO_ANALYSIS_BASE_PLOTTER_H

/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
