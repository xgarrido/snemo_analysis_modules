/* snemo_control_plot_module.h
 * Author(s)     : Xavier Garrido <garrido@lal.in2p3.fr>
 * Creation date : 2013-11-11
 * Last modified : 2015-05-25
 *
 * Copyright (C) 2013-2015 Xavier Garrido <garrido@lal.in2p3.fr>
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
 *
 * Description:
 *
 * A module to plot several basic parameters from different banks (SD, CD,
 * ...). The purpose of this module is to generate automatically plots to do
 * comparisons between software version.
 *
 * History:
 *
 */

#ifndef SNEMO_ANALYSIS_SNEMO_CONTROL_PLOT_MODULE_H
#define SNEMO_ANALYSIS_SNEMO_CONTROL_PLOT_MODULE_H 1

// Data processing module abstract base class
#include <dpp/base_module.h>

// Forward declarations
namespace mygsl {
  class histogram_pool;
}

namespace snemo {
namespace analysis {

  // Forward declaration
  class base_plotter;

  class snemo_control_plot_module : public dpp::base_module
  {
  public:

    /// Typedef for the list of plotters type
    typedef std::vector<snemo::analysis::base_plotter *> plotter_list_type;

    /// Setting histogram pool
    void set_histogram_pool(mygsl::histogram_pool & pool_);

    /// Grabbing histogram pool
    mygsl::histogram_pool & grab_histogram_pool();

    /// Constructor
    snemo_control_plot_module(datatools::logger::priority = datatools::logger::PRIO_FATAL);

    /// Destructor
    virtual ~snemo_control_plot_module();

    /// Initialization
    virtual void initialize(const datatools::properties  & setup_,
                            datatools::service_manager   & service_manager_,
                            dpp::module_handle_dict_type & module_dict_);

    /// Reset
    virtual void reset();

    /// Data record processing
    virtual process_status process(datatools::things & data_);

  protected:

    /// Give default values to specific class members.
    void _set_defaults();

    /// Generate plot for the 'tracker_clustering_data' bank
    void _process_tracker_clustering_data(const datatools::things & data_record_);

  private:

    mygsl::histogram_pool * _histogram_pool_; //!< Histogram pool
    plotter_list_type _plotters_;             //!< List of plotters

    // Macro to automate the registration of the module :
    DPP_MODULE_REGISTRATION_INTERFACE(snemo_control_plot_module);
  };

} // end of namespace analysis
} // end of namespace snemo

#endif // SNEMO_ANALYSIS_SNEMO_CONTROL_PLOT_MODULE_H

// end of snemo_control_plot_module.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
