/// topology_data_plotter.cc

// Ourselves:
#include <topology_data_plotter.h>

// Standard library
#include <stdexcept>
#include <string>
#include <sstream>

// Third party:
// - Bayeux/datatools:
#include <bayeux/datatools/properties.h>
#include <bayeux/datatools/ioutils.h>
#include <bayeux/datatools/object_configuration_description.h>
// - Bayeux/mygsl
#include <bayeux/mygsl/histogram_pool.h>

// - Falaise
#include <falaise/snemo/datamodels/data_model.h>
#include <falaise/snemo/datamodels/topology_data.h>

namespace snemo {
namespace analysis {

  const std::string & topology_data_plotter::get_id()
  {
    static const std::string s("TDP");
    return s;
  }

  topology_data_plotter::topology_data_plotter(datatools::logger::priority p_) :
    ::snemo::analysis::base_plotter(p_)
  {
    _set_defaults();
    return;
  }

  topology_data_plotter::~topology_data_plotter()
  {
    if (is_initialized()) ::snemo::analysis::topology_data_plotter::reset();
    return;
  }

  // Initialization :
  void topology_data_plotter::initialize(const datatools::properties & setup_)
  {
    ::snemo::analysis::base_plotter::_common_initialize(setup_);

    _set_initialized(true);
    return;
  }

  // Reset the clusterizer
  void topology_data_plotter::reset()
  {
    DT_THROW_IF (! is_initialized(), std::logic_error,
                 "Plotter '" << get_name() << "' is not initialized !");
    _set_initialized(false);
    _set_defaults();
    return;
  }

  void topology_data_plotter::plot(const datatools::things & data_record_)
  {
    DT_THROW_IF(! has_bank_label(), std::logic_error, "Missing bank label !");

    // Check if some 'topology_data' are available in the data model:
    if (! data_record_.has(get_bank_label())) {
      DT_LOG_DEBUG(get_logging_priority(), "Missing topology data to be processed !");
      return;
    }
    // Get the 'topology_data' entry from the data model :
    const snemo::datamodel::topology_data & td
      = data_record_.get<snemo::datamodel::topology_data>(get_bank_label());
    ::snemo::analysis::topology_data_plotter::_plot(td);
    return;
  }

  void topology_data_plotter::_set_defaults()
  {
    set_bank_label("TD");//snemo::datamodel::data_info::default_topology_data_label());
    return;
  }

  void topology_data_plotter::_plot(const snemo::datamodel::topology_data & td_)
  {
    DT_LOG_DEBUG (get_logging_priority(), "Topology data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) td_.tree_dump(std::clog);

    mygsl::histogram_pool & a_pool = grab_histogram_pool();
    return;
  }

  void topology_data_plotter::init_ocd(datatools::object_configuration_description & ocd_)
  {
    // Invoke OCD support from parent class :
    ::snemo::analysis::base_plotter::common_ocd(ocd_);

    return;
  }

} // end of namespace analysis
} // end of namespace snemo

// end of topology_data_plotter.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
