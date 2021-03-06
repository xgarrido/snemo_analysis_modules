/* snemo_bremsstrahlung_module.h
 * Author(s)     : Xavier Garrido <garrido@lal.in2p3.fr>
 * Creation date : 2012-05-14
 * Last modified : 2014-09-17
 *
 * Copyright (C) 2012-2014 Xavier Garrido <garrido@lal.in2p3.fr>
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
 * A dedicated module to study bremsstrahlung production.
 *
 * History:
 *
 */

#ifndef ANALYSIS_SNEMO_BREMSSTRAHLUNG_MODULE_H
#define ANALYSIS_SNEMO_BREMSSTRAHLUNG_MODULE_H 1

// Data processing module abstract base class
#include <dpp/base_module.h>

namespace mygsl {
  class histogram_pool;
}

namespace snemo {
  namespace datamodel {
    class particle_track_data;
  }
  namespace geometry {
    class locator_plugin;
  }
}

namespace analysis {

  class snemo_bremsstrahlung_module : public dpp::base_module
  {
  public:

    /// Setting histogram pool
    void set_histogram_pool(mygsl::histogram_pool & pool_);

    /// Grabbing histogram pool
    mygsl::histogram_pool & grab_histogram_pool();

    /// Constructor
    snemo_bremsstrahlung_module(datatools::logger::priority = datatools::logger::PRIO_FATAL);

    /// Destructor
    virtual ~snemo_bremsstrahlung_module();

    /// Initialization
    virtual void initialize(const datatools::properties  & config_,
                            datatools::service_manager   & service_manager_,
                            dpp::module_handle_dict_type & module_dict_);

    /// Reset
    virtual void reset();

    /// Data record processing
    virtual process_status process(datatools::things & data_);

    // /// Dump
    // void dump_result (std::ostream      & out_    = std::clog,
    //                   const std::string & title_  = "",
    //                   const std::string & indent_ = "",
    //                   bool inherit_               = false) const;

  protected:

    /// Give default values to specific class members.
    void _set_defaults();

    /// Compute energy distributions.
    void _compute_energy_distribution(const snemo::datamodel::particle_track_data & ptd_) const;

    /// Compute angular distribution.
    void _compute_angular_distribution(const snemo::datamodel::particle_track_data & ptd_) const;

  private:

    // The histogram pool :
    mygsl::histogram_pool * _histogram_pool_;

    // Locator plugin
    const snemo::geometry::locator_plugin * _locator_plugin_;

    // Macro to automate the registration of the module :
    DPP_MODULE_REGISTRATION_INTERFACE(snemo_bremsstrahlung_module);
  };

} // namespace analysis

#endif // ANALYSIS_SNEMO_BREMSSTRAHLUNG_MODULE_H

// end of snemo_bremsstrahlung_module.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
