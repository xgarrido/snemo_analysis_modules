/* snemo_gamma_tracking_studies_module.h
 * Author(s)     : Xavier Garrido <garrido@lal.in2p3.fr>
 * Creation date : 2013-11-11
 * Last modified : 2013-11-11
 *
 * Copyright (C) 2012 Xavier Garrido <garrido@lal.in2p3.fr>
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

#ifndef ANALYSIS_SNEMO_CONTROL_PLOT_MODULE_H_
#define ANALYSIS_SNEMO_CONTROL_PLOT_MODULE_H_ 1

// Data processing module abstract base class
#include <dpp/base_module.h>

// Forward declarations
namespace mygsl {
  class histogram_pool;
}

namespace mctools {
  class simulated_data;
}
namespace snemo {
  namespace datamodel {
    class calibrated_data;
  }
}

namespace analysis {

  class snemo_gamma_tracking_studies_module : public dpp::base_module
  {
  public:

    /// Setting histogram pool
    void set_histogram_pool(mygsl::histogram_pool & pool_);

    /// Grabbing histogram pool
    mygsl::histogram_pool & grab_histogram_pool();

    /// Constructor
    snemo_gamma_tracking_studies_module(datatools::logger::priority = datatools::logger::PRIO_FATAL);

    /// Destructor
    virtual ~snemo_gamma_tracking_studies_module();

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

    /// Generate plot for the 'simulated_data' bank
    void _process_simulated_gammas(const mctools::simulated_data & sd_,
                                   const snemo::datamodel::calibrated_data & cd_);

  private:

    // The histogram pool:
    mygsl::histogram_pool * _histogram_pool_;

    // Macro to automate the registration of the module :
    DPP_MODULE_REGISTRATION_INTERFACE (snemo_gamma_tracking_studies_module);
  };

} // namespace analysis

#endif // ANALYSIS_SNEMO_CONTROL_PLOT_MODULE_H_

// end of snemo_gamma_tracking_studies_module.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
