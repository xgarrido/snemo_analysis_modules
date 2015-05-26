/// tracker_trajectory_data_plotter.cc

// Ourselves:
#include <tracker_trajectory_data_plotter.h>

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
#include <falaise/snemo/datamodels/tracker_trajectory_data.h>
#include <falaise/snemo/datamodels/helix_trajectory_pattern.h>

namespace snemo {
namespace analysis {

  const std::string & tracker_trajectory_data_plotter::get_id()
  {
    static const std::string s("TTDP");
    return s;
  }

  tracker_trajectory_data_plotter::tracker_trajectory_data_plotter(datatools::logger::priority p_) :
    ::snemo::analysis::base_plotter(p_)
  {
    _set_defaults();
    return;
  }

  tracker_trajectory_data_plotter::~tracker_trajectory_data_plotter()
  {
    if (is_initialized()) ::snemo::analysis::tracker_trajectory_data_plotter::reset();
    return;
  }

  // Initialization :
  void tracker_trajectory_data_plotter::initialize(const datatools::properties & setup_)
  {
    ::snemo::analysis::base_plotter::_common_initialize(setup_);

    _set_initialized(true);
    return;
  }

  // Reset the clusterizer
  void tracker_trajectory_data_plotter::reset()
  {
    DT_THROW_IF (! is_initialized(), std::logic_error,
                 "Plotter '" << get_name() << "' is not initialized !");
    _set_initialized(false);
    _set_defaults();
    return;
  }

  void tracker_trajectory_data_plotter::plot(const datatools::things & data_record_)
  {
    DT_THROW_IF(! has_bank_label(), std::logic_error, "Missing bank label !");

    // Check if some 'tracker_trajectory_data' are available in the data model:
    if (! data_record_.has(get_bank_label())) {
      DT_LOG_DEBUG(get_logging_priority(), "Missing tracker trajectory data to be processed !");
      return;
    }
    // Get the 'tracker_trajectory_data' entry from the data model :
    const snemo::datamodel::tracker_trajectory_data & ttd
      = data_record_.get<snemo::datamodel::tracker_trajectory_data>(get_bank_label());
    ::snemo::analysis::tracker_trajectory_data_plotter::_plot(ttd);
    return;
  }

  void tracker_trajectory_data_plotter::_set_defaults()
  {
    set_bank_label(snemo::datamodel::data_info::default_tracker_trajectory_data_label());
    return;
  }

  void tracker_trajectory_data_plotter::_plot(const snemo::datamodel::tracker_trajectory_data & ttd_)
  {
    DT_LOG_DEBUG (get_logging_priority(), "Tracker trajectory data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) ttd_.tree_dump(std::clog);

    if (! ttd_.has_default_solution()) return;
    auto a_default_solution = ttd_.get_default_solution();
    if (! a_default_solution.has_trajectories()) return;
    auto trajectories = a_default_solution.get_trajectories();
    for (auto itrajectory : trajectories) {
      auto a_trajectory = itrajectory.get();
      auto a_auxiliaries = a_trajectory.get_auxiliaries();
      if (! a_auxiliaries.has_flag("default")) continue;
      auto a_pattern_id = a_trajectory.get_pattern().get_pattern_id();
      if (a_pattern_id != snemo::datamodel::helix_trajectory_pattern::pattern_id())
        continue;
      auto a_helix_pattern
        = dynamic_cast<const snemo::datamodel::helix_trajectory_pattern&>(a_trajectory.get_pattern());
      auto a_helix = a_helix_pattern.get_helix();
      mygsl::histogram_pool & a_pool = grab_histogram_pool();
      if (a_pool.has_1d("TTD::helix_radius")) {
        mygsl::histogram_1d & h1d = a_pool.grab_1d("TTD::helix_radius");
        h1d.fill(a_helix.get_radius());
      }
    }


    return;
  }

  void tracker_trajectory_data_plotter::init_ocd(datatools::object_configuration_description & ocd_)
  {
    // Invoke OCD support from parent class :
    ::snemo::analysis::base_plotter::common_ocd(ocd_);

    return;
  }

} // end of namespace analysis
} // end of namespace snemo

// end of tracker_trajectory_data_plotter.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
