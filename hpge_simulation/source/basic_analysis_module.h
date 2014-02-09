/* basic_analysis_module.h
 * Author(s)     : Pia Loiaza <loiaza@lal.in2p3.fr>
 * Creation date : 2013-05-16
 * Last modified : 2013-05-16
 *
 * Copyright (C) 2013 Pia Loiaza <loaiza@lal.in2p3.fr>
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
 * A very simple module to do basic germanium analysis
 *
 * History:
 *
 */

#ifndef BASIC_ANALYSIS_MODULE_H_
#define BASIC_ANALYSIS_MODULE_H_ 1

#include <string>

#include <dpp/base_module.h>    // data processing module abstract base class

// Forward declaration
class TFile;
class TTree;

namespace hpge {

  class basic_analysis_module : public dpp::base_module
  {
  public:

    /// Constructor
    basic_analysis_module(datatools::logger::priority = datatools::logger::PRIO_FATAL);

    /// Destructor
    virtual ~basic_analysis_module();

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
    void _set_defaults ();

  private:

    // The label/name of the 'simulated' bank accessible from the event record :
    std::string _SD_bank_label_;

    // ROOT variables:
    std::string _ROOT_filename_;

    TFile * _hfile_;
    TTree * _calo_tree_;
    TTree * _genbb_tree_;

    double _primary_energy_;
    double _total_energy_;

    // Macro to automate the registration of the module :
    DPP_MODULE_REGISTRATION_INTERFACE (basic_analysis_module);
  };

} // namespace hpge

#endif // BASIC_ANALYSIS_MODULE_H_

// end of basic_analysis_module.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
