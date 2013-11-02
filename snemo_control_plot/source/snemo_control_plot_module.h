/* snemo_control_plot_module.h
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

namespace analysis {

  DPP_MODULE_CLASS_DECLARE (snemo_control_plot_module)
  {
  public:

    void set_histogram_pool (mygsl::histogram_pool & pool_);

    mygsl::histogram_pool & grab_histogram_pool ();

    // Macro to automate the public interface of the module (including ctor/dtor) :
    DPP_MODULE_INTERFACE_CTOR_DTOR (snemo_control_plot_module);

  protected:

    // Give default values to specific class members.
    void _set_defaults ();

    // Generate plot for the 'simulated_data' bank
    void _process_simulated_data (const datatools::things & data_record_);

  private:

    // The histogram pool:
    mygsl::histogram_pool * _histogram_pool_;

    // Macro to automate the registration of the module :
    DPP_MODULE_REGISTRATION_INTERFACE (snemo_control_plot_module);
  };

} // namespace analysis

#endif // ANALYSIS_SNEMO_CONTROL_PLOT_MODULE_H_

// end of snemo_control_plot_module.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
