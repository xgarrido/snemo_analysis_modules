/* snemo_detector_efficiency_module.h
 * Author(s)     : Xavier Garrido <garrido@lal.in2p3.fr>
 * Creation date : 2013-03-14
 * Last modified : 2013-03-14
 *
 * Copyright (C) 2013 Xavier Garrido <garrido@lal.in2p3.fr>
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
 * A dedicated module to compute the calorimeter/tracker efficiency.
 *
 * History:
 *
 */

#ifndef ANALYSIS_SNEMO_DETECTOR_EFFICIENCY_MODULE_H_
#define ANALYSIS_SNEMO_DETECTOR_EFFICIENCY_MODULE_H_ 1

// Data processing module abstract base class
#include <dpp/base_module.h>

#include <geomtools/geom_id.h>

#include <map>
#include <string>

namespace geomtools {
  class manager;
}

namespace snemo {
  namespace geometry {
    class calo_locator;
    class xcalo_locator;
    class gveto_locator;
    class gg_locator;
  }
}

namespace analysis {

  class snemo_detector_efficiency_module : public dpp::base_module
  {
  public:

    typedef std::map<geomtools::geom_id, unsigned int> efficiency_dict;

    /// Set Geometry manager
    void set_geometry_manager(const geomtools::manager & gmgr_);

    /// Get Geometry manager
    const geomtools::manager & get_geometry_manager() const;

    /// Constructor
    snemo_detector_efficiency_module(datatools::logger::priority = datatools::logger::PRIO_FATAL);

    /// Destructor
    virtual ~snemo_detector_efficiency_module();

    /// Initialization
    virtual void initialize(const datatools::properties  & setup_,
                            datatools::service_manager   & service_manager_,
                            dpp::module_handle_dict_type & module_dict_);

    /// Reset
    virtual void reset();

    /// Data record processing
    virtual process_status process(datatools::things & data_);

    void dump_result(std::ostream      & out_    = std::clog,
                     const std::string & title_  = "",
                     const std::string & indent_ = "",
                     bool inherit_               = false) const;

  protected:

    /// Give default values to specific class members.
    void _set_defaults();

    /// Compute calorimeter block efficiencies.
    void _compute_efficiency();

  private:

    // The label/name of the 'Geo' service accessible from the service manager :
    std::string _Geo_service_label_;

    // The label/name of the bank accessible from the event record :
    std::string _bank_label_;

    // The geometry manager :
    const geomtools::manager * _geometry_manager_;

    // Calorimeter locators
    boost::scoped_ptr<snemo::geometry::calo_locator>  _calo_locator_;
    boost::scoped_ptr<snemo::geometry::xcalo_locator> _xcalo_locator_;
    boost::scoped_ptr<snemo::geometry::gveto_locator> _gveto_locator_;

    // Geiger locators
    boost::scoped_ptr<snemo::geometry::gg_locator> _gg_locator_;

    // The calorimeter block efficiency dictionnary
    efficiency_dict _calo_efficiencies_;

    // The calorimeter block efficiency dictionnary
    efficiency_dict _gg_efficiencies_;

    // Macro to automate the registration of the module :
    DPP_MODULE_REGISTRATION_INTERFACE (snemo_detector_efficiency_module);

  };

} // namespace analysis

#endif // ANALYSIS_SNEMO_DETECTOR_EFFICIENCY_MODULE_H_

// end of snemo_detector_efficiency_module.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
