/* snemo_alpha_delayed_studies_module.h
 * Author(s)     : Xavier Garrido <garrido@lal.in2p3.fr>
 * Creation date : 2015-05-29
 * Last modified : 2015-05-29
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
 *
 * Description:
 *
 * A module that compares alpha track reconstruction with the true alpha
 * informations.
 *
 * History:
 *
 */

#ifndef SNEMO_ANALYSIS_SNEMO_ALPHA_DELAYED_STUDIES_MODULE_H
#define SNEMO_ANALYSIS_SNEMO_ALPHA_DELAYED_STUDIES_MODULE_H 1

// Data processing module abstract base class
#include <dpp/base_module.h>

// Forward declaration
namespace mygsl {
  class histogram_pool;
}

namespace snemo {
namespace analysis {

  class snemo_alpha_delayed_studies_module : public dpp::base_module
  {
  public:

    /// Special structure to store and to compare alpha tracks
    struct alpha_track_parameters {
      double length;
      int nggs;
    };

    /// Typedef for a list of alpha tracks
    typedef std::vector<alpha_track_parameters> alpha_list_type;

    /// Check histogram pool existence
    bool has_histogram_pool() const;

    /// Setting histogram pool
    void set_histogram_pool(mygsl::histogram_pool & pool_);

    /// Grabbing histogram pool
    mygsl::histogram_pool & grab_histogram_pool();

    /// Constructor
    snemo_alpha_delayed_studies_module(datatools::logger::priority = datatools::logger::PRIO_FATAL);

    /// Destructor
    virtual ~snemo_alpha_delayed_studies_module();

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

    /// Get alphas simulated parameters from 'simulated_data' bank
    void _process_simulated_alphas(const datatools::things & data_, alpha_list_type & alphas_);

    /// Get alphas reconstructed parameters from 'topology_data' bank
    void _process_reconstructed_alphas(const datatools::things & data_, alpha_list_type & alphas_);

    /// Compare alpha track length
    void _compare_track_length(const alpha_list_type & sim_alphas_,
                               const alpha_list_type & rec_alphas_);

  private:

    mygsl::histogram_pool * _histogram_pool_;  //!< Histogram pool

    std::vector<int> _selected_geiger_range_; //!< Selection of alpha track with given geiger number

    // Macro to automate the registration of the module :
    DPP_MODULE_REGISTRATION_INTERFACE(snemo_alpha_delayed_studies_module);
  };

} // namespace analysis
} // namespace snemo

#endif // SNEMO_ANALYSIS_SNEMO_ALPHA_DELAYED_STUDIES_MODULE_H

// end of snemo_alpha_delayed_studies_module.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
