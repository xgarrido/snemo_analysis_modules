// -*- mode: c++ ; -*-
// snemo_control_plot_module.cc

#include <stdexcept>
#include <sstream>
#include <numeric>

#include <snemo_control_plot_module.h>

// SuperNEMO event model
#include <sncore/models/data_model.h>
#include <mctools/simulated_data.h>

// Service manager
#include <datatools/service_manager.h>

// Services
#include <dpp/histogram_service.h>

// Histogram
#include <mygsl/histogram_pool.h>

namespace analysis {

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_control_plot_module,
                                    "analysis::snemo_control_plot_module");

  // Set the histogram pool used by the module :
  void snemo_control_plot_module::set_histogram_pool (mygsl::histogram_pool & pool_)
  {
    DT_THROW_IF (is_initialized (), std::logic_error,
                 "Module '" << get_name () << "' is already initialized !");
    _histogram_pool_ = &pool_;
    return;
  }

  // Grab the histogram pool used by the module :
  mygsl::histogram_pool & snemo_control_plot_module::grab_histogram_pool ()
  {
    DT_THROW_IF (! is_initialized (), std::logic_error,
                 "Module '" << get_name () << "' is not initialized !");
    return *_histogram_pool_;
  }

  void snemo_control_plot_module::_set_defaults ()
  {
    _histogram_pool_ = 0;
    return;
  }

  // Initialization :
  DPP_MODULE_INITIALIZE_IMPLEMENT_HEAD (snemo_control_plot_module,
                                        config_,
                                        service_manager_,
                                        module_dict_)
  {
    DT_THROW_IF (is_initialized (),
                 std::logic_error,
                 "Module '" << get_name () << "' is already initialized ! ");

    dpp::base_module::_common_initialize (config_);

    // Service label
    std::string histogram_label;
    if (config_.has_key ("Histo_label"))
      {
        histogram_label = config_.fetch_string ("Histo_label");
      }

    if (! _histogram_pool_)
      {
        DT_THROW_IF (histogram_label.empty (), std::logic_error,
                     "Module '" << get_name () << "' has no valid 'Histo_label' property !");

        DT_THROW_IF (! service_manager_.has (histogram_label) ||
                     ! service_manager_.is_a<dpp::histogram_service> (histogram_label),
                     std::logic_error,
                     "Module '" << get_name () << "' has no '" << histogram_label << "' service !");
        dpp::histogram_service & Histo
          = service_manager_.get<dpp::histogram_service> (histogram_label);
        set_histogram_pool (Histo.grab_pool ());
      }

    // Tag the module as initialized :
    _set_initialized (true);
    return;
  }

  // Reset :
  DPP_MODULE_RESET_IMPLEMENT_HEAD (snemo_control_plot_module)
  {
    DT_THROW_IF(! is_initialized (),
                std::logic_error,
                "Module '" << get_name () << "' is not initialized !");

    // Tag the module as un-initialized :
    _set_initialized (false);
    _set_defaults ();
    return;
  }

  // Constructor :
  DPP_MODULE_CONSTRUCTOR_IMPLEMENT_HEAD (snemo_control_plot_module, logging_priority)
  {
    _set_defaults ();
    return;
  }

  // Destructor :
  DPP_MODULE_DEFAULT_DESTRUCTOR_IMPLEMENT (snemo_control_plot_module);

  // Processing :
  DPP_MODULE_PROCESS_IMPLEMENT_HEAD (snemo_control_plot_module, data_record_)
  {
    DT_LOG_TRACE (get_logging_priority (), "Entering...");
    DT_THROW_IF (! is_initialized (), std::logic_error,
                 "Module '" << get_name () << "' is not initialized !");

    const bool abort_at_missing_input = true;

    // Check if some 'simulated_data' are available in the data model:
    const std::string sd_label = snemo::core::model::data_info::SIMULATED_DATA_LABEL;
    if (! data_record_.has (sd_label))
      {
        DT_THROW_IF (abort_at_missing_input, std::logic_error,
                     "Missing simulated data to be processed !");
        // leave the data unchanged.
        return dpp::PROCESS_ERROR;
      }
    // grab the 'simulated_data' entry from the data model :
    mctools::simulated_data & sd = data_record_.grab<mctools::simulated_data> (sd_label);

    DT_LOG_DEBUG (get_logging_priority (), "Simulated data : ");
    if (get_logging_priority () >= datatools::logger::PRIO_DEBUG)
      {
        sd.tree_dump (std::clog);
      }

    // Filling the histograms :
    size_t nggs = 0;
    if (_histogram_pool_->has_1d ("SD::ngghits"))
      {
        mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d ("SD::ngghits");
        if (sd.has_step_hits ("gg")) nggs += sd.get_step_hits ("gg").size ();
        h1d.fill (nggs);
      }

    size_t ncalos = 0;
    if (_histogram_pool_->has_1d ("SD::ncalohits"))
      {
        mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d ("SD::ncalohits");
        if (sd.has_step_hits ("calo"))  ncalos += sd.get_step_hits ("calo").size ();
        if (sd.has_step_hits ("xcalo")) ncalos += sd.get_step_hits ("xcalo").size ();
        if (sd.has_step_hits ("gveto")) ncalos += sd.get_step_hits ("gveto").size ();
        h1d.fill (ncalos);
      }

    DT_LOG_TRACE (get_logging_priority (), "Exiting.");
    return dpp::PROCESS_SUCCESS;
  }

} // namespace analysis

// end of snemo_control_plot_module.cc
