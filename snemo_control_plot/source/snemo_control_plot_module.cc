// snemo_control_plot_module.cc

// Ourselves:
#include <snemo_control_plot_module.h>

// Standard library:
#include <stdexcept>
#include <sstream>
#include <numeric>

// Third party:
// - Boost :
#include <boost/foreach.hpp>
// - Bayeux/datatools:
#include <datatools/service_manager.h>
// - Bayeux/mygsl
#include <mygsl/histogram_pool.h>
// - Bayeux/dpp
#include <dpp/histogram_service.h>
// - Bayeux/mctools
#include <mctools/simulated_data.h>

// This project:
#include <simulated_data_plotter.h>
#include <calibrated_data_plotter.h>
#include <tracker_clustering_data_plotter.h>
#include <tracker_trajectory_data_plotter.h>

namespace snemo {
namespace analysis {

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_control_plot_module,
                                    "snemo::analysis::snemo_control_plot_module");

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
      if (config_.has_key("Histo.output_files")) {
        std::vector<std::string> output_files;
        config_.fetch("Histo.output_files", output_files);
        for (auto a_file : output_files) {
          Histo.add_output_file(a_file);
        }
      }
      if (config_.has_key("Histo.template_files")) {
        std::vector<std::string> template_files;
        config_.fetch("Histo.template_files", template_files);
        for (auto a_file : template_files) {
          Histo.grab_pool().load(a_file);
        }
      }
    }

    // Plotters
    DT_THROW_IF(! config_.has_key("plotters"), std::logic_error, "Missing 'plotters' key !");
    std::vector<std::string> plotter_names;
    config_.fetch("plotters", plotter_names);
    for (auto a_plotter_name : plotter_names) {
      if (a_plotter_name == snemo::analysis::simulated_data_plotter::get_id()) {
        _plotters_.push_back(new snemo::analysis::simulated_data_plotter);
      } else if (a_plotter_name == snemo::analysis::calibrated_data_plotter::get_id()) {
        _plotters_.push_back(new snemo::analysis::calibrated_data_plotter);
      } else if (a_plotter_name == snemo::analysis::tracker_clustering_data_plotter::get_id()) {
        _plotters_.push_back(new snemo::analysis::tracker_clustering_data_plotter);
      } else if (a_plotter_name == snemo::analysis::tracker_trajectory_data_plotter::get_id()) {
        _plotters_.push_back(new snemo::analysis::tracker_trajectory_data_plotter);
      } else {
        DT_THROW_IF(true, std::logic_error, "Unkown '" << a_plotter_name << "' plotter!");
      }
      snemo::analysis::base_plotter * a_plotter = _plotters_.back();
      a_plotter->set_histogram_pool(grab_histogram_pool());
      datatools::properties a_config;
      config_.export_and_rename_starting_with(a_config, a_plotter_name + ".", "");
      a_plotter->initialize(a_config);
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

    // Reset plotters
    for (auto a_plotter : _plotters_) {
      a_plotter->reset();
    }

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
    for (auto a_plotter : _plotters_) {
      a_plotter->plot(data_record_);
    }

    DT_LOG_TRACE(get_logging_priority(), "Exiting.");
    return dpp::base_module::PROCESS_SUCCESS;
  }

} // end of namespace analysis
} // end of namespace snemo

// end of snemo_control_plot_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
