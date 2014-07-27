// snemo_control_plot_module.cc

// Ourselves:
#include <snemo_control_plot_module.h>

// Standard library:
#include <stdexcept>
#include <sstream>
#include <numeric>

// Third party:
// - Bayeux/datatools:
#include <datatools/service_manager.h>
// - Bayeux/mygsl
#include <mygsl/histogram_pool.h>
// - Bayeux/dpp
#include <dpp/histogram_service.h>
// - Bayeux/mctools
#include <mctools/simulated_data.h>

// - Falaise
#include <snemo/datamodels/data_model.h>
#include <snemo/datamodels/calibrated_data.h>
#include <snemo/datamodels/tracker_clustering_data.h>

namespace analysis {

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_control_plot_module,
                                    "analysis::snemo_control_plot_module");

  // Set the histogram pool used by the module :
  void snemo_control_plot_module::set_histogram_pool(mygsl::histogram_pool & pool_)
  {
    DT_THROW_IF(is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is already initialized !");
    _histogram_pool_ = &pool_;
    return;
  }

  // Grab the histogram pool used by the module :
  mygsl::histogram_pool & snemo_control_plot_module::grab_histogram_pool()
  {
    DT_THROW_IF (! is_initialized(), std::logic_error,
                 "Module '" << get_name() << "' is not initialized !");
    return *_histogram_pool_;
  }

  void snemo_control_plot_module::_set_defaults()
  {
    _histogram_pool_ = 0;
    return;
  }

  // Initialization :
  void snemo_control_plot_module::initialize(const datatools::properties  & config_,
                                             datatools::service_manager   & service_manager_,
                                             dpp::module_handle_dict_type & module_dict_)
  {
    DT_THROW_IF(is_initialized(),
                std::logic_error,
                "Module '" << get_name() << "' is already initialized ! ");

    dpp::base_module::_common_initialize(config_);

    // Service label
    std::string histogram_label;
    if (config_.has_key("Histo_label")) {
      histogram_label = config_.fetch_string("Histo_label");
    }
    if (! _histogram_pool_) {
      DT_THROW_IF(histogram_label.empty(), std::logic_error,
                  "Module '" << get_name() << "' has no valid 'Histo_label' property !");

      DT_THROW_IF(! service_manager_.has(histogram_label) ||
                  ! service_manager_.is_a<dpp::histogram_service>(histogram_label),
                  std::logic_error,
                  "Module '" << get_name() << "' has no '" << histogram_label << "' service !");
      dpp::histogram_service & Histo
        = service_manager_.get<dpp::histogram_service>(histogram_label);
      set_histogram_pool(Histo.grab_pool());
    }

    // Tag the module as initialized :
    _set_initialized(true);
    return;
  }

  // Reset :
  void snemo_control_plot_module::reset()
  {
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    // Tag the module as un-initialized :
    _set_initialized(false);
    _set_defaults();
    return;
  }

  // Constructor :
  snemo_control_plot_module::snemo_control_plot_module(datatools::logger::priority logging_priority_)
    : dpp::base_module(logging_priority_)
  {
    _set_defaults();
    return;
  }

  // Destructor :
  snemo_control_plot_module::~snemo_control_plot_module()
  {
    if (is_initialized()) snemo_control_plot_module::reset();
    return;
  }

  // Processing :
  dpp::base_module::process_status snemo_control_plot_module::process(datatools::things & data_record_)
  {
    DT_LOG_TRACE(get_logging_priority(), "Entering...");
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    // Filling the histograms :
    _process_simulated_data(data_record_);
    _process_calibrated_data(data_record_);
    _process_tracker_clustering_data(data_record_);

    DT_LOG_TRACE(get_logging_priority(), "Exiting.");
    return dpp::base_module::PROCESS_SUCCESS;
  }

  void snemo_control_plot_module::_process_simulated_data(const datatools::things & data_record_)
  {
    // Check if some 'simulated_data' are available in the data model:
    const std::string sd_label = snemo::datamodel::data_info::default_simulated_data_label();
    if (! data_record_.has(sd_label)) {
      DT_LOG_DEBUG(get_logging_priority(), "Missing simulated data to be processed !");
      return;
    }
    // Grab the 'simulated_data' entry from the data model :
    const mctools::simulated_data & sd = data_record_.get<mctools::simulated_data>(sd_label);

    DT_LOG_DEBUG (get_logging_priority(), "Simulated data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) sd.tree_dump(std::clog);

    if (_histogram_pool_->has_1d("SD::ngghits")) {
      mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d("SD::ngghits");
      size_t nggs = 0;
      if (sd.has_step_hits("gg")) nggs += sd.get_number_of_step_hits("gg");
      h1d.fill(nggs);
    }

    if (_histogram_pool_->has_1d("SD::ncalohits")) {
      mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d("SD::ncalohits");
      size_t ncalos = 0;
      if (sd.has_step_hits("calo"))  ncalos += sd.get_number_of_step_hits("calo");
      if (sd.has_step_hits("xcalo")) ncalos += sd.get_number_of_step_hits("xcalo");
      if (sd.has_step_hits("gveto")) ncalos += sd.get_number_of_step_hits("gveto");
      h1d.fill(ncalos);
    }
    return;
  }

  void snemo_control_plot_module::_process_calibrated_data(const datatools::things & data_record_)
  {
    // Check if some 'simulated_data' are available in the data model:
    const std::string cd_label = snemo::datamodel::data_info::default_calibrated_data_label();
    if (! data_record_.has(cd_label)) {
      DT_LOG_DEBUG(get_logging_priority(), "Missing calibrated data to be processed !");
      return;
    }
    // Grab the 'simulated_data' entry from the data model :
    const snemo::datamodel::calibrated_data & cd
      = data_record_.get<snemo::datamodel::calibrated_data>(cd_label);

    DT_LOG_DEBUG(get_logging_priority(), "Calibrated data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) cd.tree_dump(std::clog);

    if (_histogram_pool_->has_1d("CD::ngghits")) {
      mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d("CD::ngghits");
      size_t nggs = 0;
      if (cd.has_calibrated_tracker_hits()) nggs += cd.calibrated_tracker_hits().size();
      h1d.fill(nggs);
    }

    if (_histogram_pool_->has_1d("CD::ncalohits")) {
      mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d("CD::ncalohits");
      size_t ncalos = 0;
      if (cd.has_calibrated_calorimeter_hits()) ncalos += cd.calibrated_calorimeter_hits().size();
      h1d.fill(ncalos);
    }

    return;
  }

  void snemo_control_plot_module::_process_tracker_clustering_data(const datatools::things & data_record_)
  {
    // Check if some 'simulated_data' are available in the data model:
    const std::string tcd_label = snemo::datamodel::data_info::default_tracker_clustering_data_label();
    if (! data_record_.has(tcd_label)) {
      DT_LOG_DEBUG(get_logging_priority(), "Missing tracker clustering data to be processed !");
      return;
    }
    // Grab the 'simulated_data' entry from the data model :
    const snemo::datamodel::tracker_clustering_data & tcd
      = data_record_.get<snemo::datamodel::tracker_clustering_data>(tcd_label);

    DT_LOG_DEBUG(get_logging_priority(), "Tracker clustering data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) tcd.tree_dump(std::clog);

    if (_histogram_pool_->has_1d("TCD::nclusters")) {
      mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d("TCD::nclusters");
      size_t nclusters = 0;
      if (tcd.has_default_solution()) nclusters += tcd.get_default_solution().get_clusters().size();
      h1d.fill(nclusters);
    }

    return;
  }

} // namespace analysis

// end of snemo_control_plot_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
