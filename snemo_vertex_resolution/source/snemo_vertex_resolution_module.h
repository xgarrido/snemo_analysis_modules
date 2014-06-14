/* snemo_vertex_resolution_module.h
 * Author(s)     : Xavier Garrido <garrido@lal.in2p3.fr>
 * Creation date : 2012-05-14
 * Last modified : 2014-06-13
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
 * A dedicated module to study vertex reconstruction.
 *
 * History:
 *
 */

#ifndef ANALYSIS_SNEMO_VERTEX_RESOLUTION_MODULE_H
#define ANALYSIS_SNEMO_VERTEX_RESOLUTION_MODULE_H 1

// Data processing module abstract base class
#include <dpp/base_module.h>

#include <map>
#include <string>
#include <vector>

#include <mygsl/datapoint.h>
#include <mygsl/histogram.h>

namespace analysis {

  class snemo_vertex_resolution_module : public dpp::base_module
  {
  public:

    typedef std::vector<mygsl::datapoint>     datapoints;
    typedef std::map<std::string, datapoints> graph_collection_type;
    typedef std::map<std::string, mygsl::histogram> histogram_collection_type;

    void set_parameter_label (const std::string & parameter_);

    const std::string & get_parameter_label () const;

    /// Constructor
    snemo_vertex_resolution_module(datatools::logger::priority = datatools::logger::PRIO_FATAL);

    /// Destructor
    virtual ~snemo_vertex_resolution_module();

    /// Initialization
    virtual void initialize(const datatools::properties  & config_,
                            datatools::service_manager   & service_manager_,
                            dpp::module_handle_dict_type & module_dict_);

    /// Reset
    virtual void reset();

    /// Data record processing
    virtual process_status process(datatools::things & data_);

    /// Dump
    void dump_result (std::ostream      & out_    = std::clog,
                      const std::string & title_  = "",
                      const std::string & indent_ = "",
                      bool inherit_               = false) const;

  protected:

    /// Give default values to specific class members.
    void _set_defaults();

    // // Generate ROOT plots.
    // void _generate_plots_ ();

    // // Generate scatter plot chi2/ndof value vs. simulated energy
    // void _generate_scatter_plot_ ();

    // // Generate chi2/ndof distribution value
    // void _generate_histogram_plot_ ();

  private:

    // The parameter name for vertex (either 'foil' or 'calorimeter')
    std::string _parameter_;

    // Distribution of the event charge:
    graph_collection_type _vertex_graphs_;
    histogram_collection_type _vertex_histograms_;

    // Macro to automate the registration of the module :
    DPP_MODULE_REGISTRATION_INTERFACE(snemo_vertex_resolution_module);

  };

} // namespace analysis

#endif // ANALYSIS_SNEMO_VERTEX_RESOLUTION_MODULE_H

// end of snemo_vertex_resolution_module.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
