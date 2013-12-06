/* snemo_bb0nu_halflife_limit_module.h
 * Author(s)     : Steven Calvez <calvez@lal.in2p3.fr>
 * Creation date : 2012-06-13
 * Last modified : 2012-08-27
 *
 * Copyright (C) 2012 Steven Calvez <calvez@lal.in2p3.fr>
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
 * A dedicated module to compute the limit on the neutrinoless double
 * beta decay halflife.
 *
 * History:
 *
 */

#ifndef ANALYSIS_SNEMO_BB0NU_HALFLIFE_LIMIT_MODULE_H_
#define ANALYSIS_SNEMO_BB0NU_HALFLIFE_LIMIT_MODULE_H_ 1

// Data processing module abstract base class
#include <dpp/base_module.h>

#include <map>
#include <string>
#include <vector>

namespace mygsl {
  class histogram_pool;
}

namespace analysis {

  DPP_MODULE_CLASS_DECLARE (snemo_bb0nu_halflife_limit_module)
  {
  public:

    struct experiment_entry_type
    {
      double isotope_mass_number;
      double isotope_mass;
      double isotope_bb2nu_halflife;
      double exposure_time;
    };

  public:

    void set_histogram_pool (mygsl::histogram_pool & pool_);

    mygsl::histogram_pool & grab_histogram_pool ();

    void dump_result (std::ostream      & out_    = std::clog,
                      const std::string & title_  = "",
                      const std::string & indent_ = "",
                      bool inherit_               = false) const;

    // Macro to automate the public interface of the module (including ctor/dtor) :
    DPP_MODULE_INTERFACE_CTOR_DTOR (snemo_bb0nu_halflife_limit_module);

  protected:

    // Give default values to specific class members.
    void _set_defaults ();

  private:

    // Compute topology channel efficiencies.
    void _compute_efficiency_ ();

    // Compute topology channel efficiencies.
    void _compute_halflife_ ();

  private:

    // The label/name of the 'event header' bank accessible from the event record :
    std::string _EH_bank_label_;

    // The label/name of the 'particle track' bank accessible from the event record :
    std::string _PTD_bank_label_;

    // The key fields from 'event header' bank to build the histogram key:
    std::vector<std::string> _key_fields_;

    // The histogram pool :
    mygsl::histogram_pool * _histogram_pool_;

    // The experiment running condition
    experiment_entry_type _experiment_conditions_;

    // Macro to automate the registration of the module :
    DPP_MODULE_REGISTRATION_INTERFACE (snemo_bb0nu_halflife_limit_module);

  };

} // namespace analysis

#endif // DPP_ANALYSIS_SNEMO_BB0NU_HALFLIFE_LIMIT_MODULE_H

// end of snemo_bb0nu_halflife_limit_module.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
