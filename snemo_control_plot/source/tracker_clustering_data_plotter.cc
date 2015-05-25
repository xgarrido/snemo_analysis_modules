/// tracker_clustering_data_plotter.cc

// Ourselves:
#include <tracker_clustering_data_plotter.h>

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
#include <falaise/snemo/datamodels/tracker_clustering_data.h>

namespace snemo {
namespace analysis {

  const std::string & tracker_clustering_data_plotter::get_id()
  {
    static const std::string s("TCDP");
    return s;
  }

  tracker_clustering_data_plotter::tracker_clustering_data_plotter(datatools::logger::priority p_) :
    ::snemo::analysis::base_plotter(p_)
  {
    _set_defaults();
    return;
  }

  tracker_clustering_data_plotter::~tracker_clustering_data_plotter()
  {
    if (is_initialized()) ::snemo::analysis::tracker_clustering_data_plotter::reset();
    return;
  }

  // Initialization :
  void tracker_clustering_data_plotter::initialize(const datatools::properties & setup_)
  {
    ::snemo::analysis::base_plotter::_common_initialize(setup_);

    _set_initialized(true);
    return;
  }

  // Reset the clusterizer
  void tracker_clustering_data_plotter::reset()
  {
    DT_THROW_IF (! is_initialized(), std::logic_error,
                 "Plotter '" << get_name() << "' is not initialized !");
    _set_initialized(false);
    _set_defaults();
    return;
  }

  void tracker_clustering_data_plotter::plot(const datatools::things & data_record_)
  {
    DT_THROW_IF(! has_bank_label(), std::logic_error, "Missing bank label !");

    // Check if some 'tracker_clustering_data' are available in the data model:
    if (! data_record_.has(get_bank_label())) {
      DT_LOG_DEBUG(get_logging_priority(), "Missing tracker clustering data to be processed !");
      return;
    }
    // Get the 'tracker_clustering_data' entry from the data model :
    const snemo::datamodel::tracker_clustering_data & tcd
      = data_record_.get<snemo::datamodel::tracker_clustering_data>(get_bank_label());
    ::snemo::analysis::tracker_clustering_data_plotter::_plot(tcd);
    return;
  }

  void tracker_clustering_data_plotter::_set_defaults()
  {
    set_bank_label(snemo::datamodel::data_info::default_tracker_clustering_data_label());
    return;
  }

  void tracker_clustering_data_plotter::_plot(const snemo::datamodel::tracker_clustering_data & tcd_)
  {
    DT_LOG_DEBUG (get_logging_priority(), "Tracker clustering data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) tcd_.tree_dump(std::clog);

    mygsl::histogram_pool & a_pool = grab_histogram_pool();
    if (a_pool.has_1d("TCD::nclusters")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d("TCD::nclusters");
      int nclusters = 0;
      if (tcd_.has_default_solution()) nclusters += tcd_.get_default_solution().get_clusters().size();
      h1d.fill(nclusters);
    }

    return;
  }

  void tracker_clustering_data_plotter::init_ocd(datatools::object_configuration_description & ocd_)
  {
    // Invoke OCD support from parent class :
    ::snemo::analysis::base_plotter::common_ocd(ocd_);

    return;
  }

} // end of namespace analysis
} // end of namespace snemo

// end of tracker_clustering_data_plotter.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
