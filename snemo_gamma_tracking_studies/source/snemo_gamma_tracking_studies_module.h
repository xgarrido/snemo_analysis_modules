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

// Standard libraires:
#include <set>
#include <map>
// Third party:
// - Bayeux/geomtools:
#include <geomtools/geom_id.h>

// Data processing module abstract base class
#include <dpp/base_module.h>

// Forward declarations
namespace mygsl {
  class histogram_pool;
}

namespace analysis {

  class snemo_gamma_tracking_studies_module : public dpp::base_module
  {
  public:

    /// Typedef for calorimeters collection
    typedef std::set<geomtools::geom_id> calo_list_type;

    /// Typedef for gamma dictionnaries
    typedef std::map<int, calo_list_type> gamma_dict_type;

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

    /// Get gammas sequence from 'simulated_data' bank
    dpp::base_module::process_status _process_simulated_gammas(const datatools::things & data_,
                                                               gamma_dict_type & gammas_) const;

    /// Get gammas sequence from 'particle_track_data' bank
    dpp::base_module::process_status _process_reconstructed_gammas(const datatools::things & data_,
                                                                   gamma_dict_type & gammas_) const;

    /// Compare 2 sequences of calorimeters
    void _compare_sequences(const gamma_dict_type & simulated_gammas_,
                            const gamma_dict_type & reconstructed_gammas_);

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
