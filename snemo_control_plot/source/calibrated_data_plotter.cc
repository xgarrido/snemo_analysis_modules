/// calibrated_data_plotter.cc

// Ourselves:
#include <calibrated_data_plotter.h>

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
#include <falaise/snemo/datamodels/calibrated_data.h>

namespace snemo {
namespace analysis {

  const std::string & calibrated_data_plotter::get_id()
  {
    static const std::string s("CDP");
    return s;
  }

  calibrated_data_plotter::calibrated_data_plotter(datatools::logger::priority p_) :
    ::snemo::analysis::base_plotter(p_)
  {
    _set_defaults();
    return;
  }

  calibrated_data_plotter::~calibrated_data_plotter()
  {
    if (is_initialized()) ::snemo::analysis::calibrated_data_plotter::reset();
    return;
  }

  // Initialization :
  void calibrated_data_plotter::initialize(const datatools::properties & setup_)
  {
    ::snemo::analysis::base_plotter::_common_initialize(setup_);

    _set_initialized(true);
    return;
  }

  // Reset the clusterizer
  void calibrated_data_plotter::reset()
  {
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Plotter '" << get_name() << "' is not initialized !");
    _set_initialized(false);
    _set_defaults();
    return;
  }

  void calibrated_data_plotter::plot(const datatools::things & data_record_)
  {
    DT_THROW_IF(! has_bank_label(), std::logic_error, "Missing bank label !");

    // Check if some 'calibrated_data' are available in the data model:
    if (! data_record_.has(get_bank_label())) {
      DT_LOG_DEBUG(get_logging_priority(), "Missing calibrated data to be processed !");
      return;
    }
    // Get the 'calibrated_data' entry from the data model :
    const snemo::datamodel::calibrated_data & cd
      = data_record_.get<snemo::datamodel::calibrated_data>(get_bank_label());
    ::snemo::analysis::calibrated_data_plotter::_plot(cd);
    return;
  }

  void calibrated_data_plotter::_set_defaults()
  {
    set_bank_label(snemo::datamodel::data_info::default_calibrated_data_label());
    return;
  }

  void calibrated_data_plotter::_plot(const snemo::datamodel::calibrated_data & cd_)
  {
    DT_LOG_DEBUG (get_logging_priority(), "Calibrated data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) cd_.tree_dump(std::clog);

    mygsl::histogram_pool & a_pool = grab_histogram_pool();
    if (a_pool.has_1d("CD::ngghits")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d("CD::ngghits");
      int nggs = 0;
      if (cd_.has_calibrated_tracker_hits()) nggs += cd_.calibrated_tracker_hits().size();
      h1d.fill(nggs);
    }

    if (a_pool.has_1d("CD::ncalohits")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d("CD::ncalohits");
      int ncalos = 0;
      if (cd_.has_calibrated_calorimeter_hits()) ncalos += cd_.calibrated_calorimeter_hits().size();
      h1d.fill(ncalos);
    }

    if (! cd_.has_calibrated_tracker_hits()) return;
    const snemo::datamodel::calibrated_data::tracker_hit_collection_type gg_hits
      = cd_.calibrated_tracker_hits();

    for (auto gg_handle : gg_hits) {
      if (! gg_handle.has_data()) continue;
      const snemo::datamodel::calibrated_tracker_hit & gg_hit = gg_handle.get();
      if (a_pool.has_1d("CD::drift_radius")) {
        mygsl::histogram_1d & h1d = a_pool.grab_1d("CD::drift_radius");
        h1d.fill(gg_hit.get_r());
      }
      if (a_pool.has_1d("CD::drift_radius_error")) {
        mygsl::histogram_1d & h1d = a_pool.grab_1d("CD::drift_radius_error");
        h1d.fill(gg_hit.get_sigma_r());
      }
      if (a_pool.has_1d("CD::long_position")) {
        mygsl::histogram_1d & h1d = a_pool.grab_1d("CD::long_position");
        h1d.fill(gg_hit.get_z());
      }
      if (a_pool.has_1d("CD::long_position_error")) {
        mygsl::histogram_1d & h1d = a_pool.grab_1d("CD::long_position_error");
        h1d.fill(gg_hit.get_sigma_z());
      }
    }


    return;
  }

  void calibrated_data_plotter::init_ocd(datatools::object_configuration_description & ocd_)
  {
    // Invoke OCD support from parent class :
    ::snemo::analysis::base_plotter::common_ocd(ocd_);

    return;
  }

} // end of namespace analysis
} // end of namespace snemo

// end of calibrated_data_plotter.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
