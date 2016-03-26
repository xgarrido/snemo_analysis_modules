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
#include <falaise/snemo/geometry/gg_locator.h>
#include <falaise/snemo/datamodels/data_model.h>
#include <falaise/snemo/datamodels/calibrated_data.h>

// This project
#include <geometry_tools.h>

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

    if (cd_.has_calibrated_calorimeter_hits()) {
      _plot_calorimeter_hits_(cd_);
    }
    if (cd_.has_calibrated_tracker_hits()) {
      _plot_tracker_hits_(cd_);
    }

    return;
  }

  void calibrated_data_plotter::_plot_calorimeter_hits_(const snemo::datamodel::calibrated_data & cd_)
  {
    auto & calo_hits = cd_.calibrated_calorimeter_hits();
    mygsl::histogram_pool & a_pool = grab_histogram_pool();
    if (a_pool.has_1d("CD::ncalohits")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d("CD::ncalohits");
      h1d.fill((int)calo_hits.size());
    }
    return;
  }

  void calibrated_data_plotter::_plot_tracker_hits_(const snemo::datamodel::calibrated_data & cd_)
  {
    auto & gg_hits = cd_.calibrated_tracker_hits();

    mygsl::histogram_pool & a_pool = grab_histogram_pool();
    if (a_pool.has_1d("CD::ngghits")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d("CD::ngghits");
      h1d.fill((int)gg_hits.size());
    }

    for (auto & gg_handle : gg_hits) {
      if (! gg_handle.has_data()) continue;
      auto & gg_hit = gg_handle.get();
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
      if (a_pool.has_2d("CD::gg_heatmap")) {
        const geomtools::geom_id & a_gid = gg_hit.get_geom_id();
        const snemo::geometry::gg_locator & a_gg_locator
          = snemo::utils::geometry_tools::get_instance().get_gg_locator();
        const int a_sign = (a_gg_locator.extract_side(a_gid)
                            == snemo::geometry::utils::SIDE_BACK ? 1 : -1);
        const int a_row = a_gg_locator.extract_row(a_gid);
        const int a_layer = a_sign * (a_gg_locator.extract_layer(a_gid) + 1);
        mygsl::histogram_2d & h2d = a_pool.grab_2d("CD::gg_heatmap");
        h2d.fill(a_layer, a_row);
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
