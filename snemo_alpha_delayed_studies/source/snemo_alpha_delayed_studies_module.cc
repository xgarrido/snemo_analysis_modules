// snemo_alpha_delayed_studies_module.cc

// Ourselves:
#include <snemo_alpha_delayed_studies_module.h>

// Standard library:
#include <stdexcept>
#include <sstream>
#include <algorithm>

// Third party:
// - Bayeux/datatools:
#include <datatools/service_manager.h>
// - Bayeux/mygsl
#include <mygsl/histogram_pool.h>
// - Bayeux/dpp
#include <dpp/histogram_service.h>
// - Bayeux/mctools
#include <mctools/utils.h>
#include <mctools/simulated_data.h>

// - Falaise
#include <snemo/datamodels/data_model.h>
#include <snemo/datamodels/particle_track_data.h>
#include <snemo/datamodels/topology_data.h>
#include <snemo/datamodels/topology_1e1a_pattern.h>

namespace snemo {
namespace analysis {

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_alpha_delayed_studies_module,
                                    "snemo::analysis::snemo_alpha_delayed_studies_module");

  bool snemo_alpha_delayed_studies_module::has_histogram_pool() const
  {
    return _histogram_pool_ != 0;
  }

  // Set the histogram pool used by the module :
  void snemo_alpha_delayed_studies_module::set_histogram_pool(mygsl::histogram_pool & pool_)
  {
    DT_THROW_IF(is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is already initialized !");
    _histogram_pool_ = &pool_;
    return;
  }

  // Grab the histogram pool used by the module :
  mygsl::histogram_pool & snemo_alpha_delayed_studies_module::grab_histogram_pool()
  {
    return *_histogram_pool_;
  }

  void snemo_alpha_delayed_studies_module::_set_defaults()
  {
    _histogram_pool_ = 0;
    return;
  }

  // Initialization :
  void snemo_alpha_delayed_studies_module::initialize(const datatools::properties   & config_,
                                                       datatools::service_manager   & service_manager_,
                                                       dpp::module_handle_dict_type & /*module_dict_*/)
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
    if (! has_histogram_pool()) {
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
    DT_THROW_IF(! has_histogram_pool(), std::logic_error, "No histogram pool has been instantiated !");

    // Tag the module as initialized :
    _set_initialized(true);
    return;
  }

  // Reset :
  void snemo_alpha_delayed_studies_module::reset()
  {
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    // Tag the module as un-initialized :
    _set_initialized(false);
    _set_defaults();
    return;
  }

  // Constructor :
  snemo_alpha_delayed_studies_module::snemo_alpha_delayed_studies_module(datatools::logger::priority logging_priority_)
    : dpp::base_module(logging_priority_)
  {
    _set_defaults();
    return;
  }

  // Destructor :
  snemo_alpha_delayed_studies_module::~snemo_alpha_delayed_studies_module()
  {
    if (is_initialized()) snemo_alpha_delayed_studies_module::reset();
    return;
  }

  // Processing :
  dpp::base_module::process_status snemo_alpha_delayed_studies_module::process(datatools::things & data_record_)
  {
    DT_LOG_TRACE(get_logging_priority(), "Entering...");
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    alpha_list_type simulated_alphas;
    this->_process_simulated_alphas(data_record_, simulated_alphas);
    DT_LOG_DEBUG(get_logging_priority(), "Number of primary simulated alphas = " << simulated_alphas.size());
    DT_LOG_DEBUG(get_logging_priority(), "Particle path length = " << simulated_alphas.back().length/CLHEP::mm << " mm");

    alpha_list_type reconstructed_alphas;
    this->_process_reconstructed_alphas(data_record_, reconstructed_alphas);

    this->_compare_track_length(simulated_alphas, reconstructed_alphas);

    DT_LOG_TRACE(get_logging_priority(), "Exiting.");
    return dpp::base_module::PROCESS_SUCCESS;
  }

  void snemo_alpha_delayed_studies_module::_process_simulated_alphas(const datatools::things & data_record_,
                                                                     alpha_list_type & simulated_alphas_)
  {
    // Check if some 'simulated_data' are available in the data model:
    const std::string sd_label = snemo::datamodel::data_info::default_simulated_data_label();
    if (! data_record_.has(sd_label)) {
      DT_LOG_ERROR(get_logging_priority(), "Missing simulated data to be processed !");
      return;
    }
    // Get the 'simulated_data' entry from the data model :
    const mctools::simulated_data & sd = data_record_.get<mctools::simulated_data>(sd_label);

    DT_LOG_DEBUG(get_logging_priority(), "Simulated data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) sd.tree_dump();

    // Fetch simulated step hits
    const std::string hit_label = "__visu.tracks";
    if (! sd.has_step_hits(hit_label)) return;
    const mctools::simulated_data::hit_handle_collection_type & hit_collection
      = sd.get_step_hits(hit_label);
    if (hit_collection.empty()) {
      DT_LOG_DEBUG(get_logging_priority(), "No simulated step hits");
      return;
    }

    std::set<int> track_id_list;
    for (auto ihit : hit_collection) {
      const mctools::base_step_hit & a_hit = ihit.get();

      // Check if step comes from alpha particle
      const std::string a_label = a_hit.get_particle_name();
      if (a_label != "alpha") continue;

      // Check also if the alpha is a primary particle
      if (! a_hit.is_primary_particle()) continue;

      // Finally check track id
      if (! a_hit.has_track_id()) {
        DT_LOG_WARNING(get_logging_priority(), "Missing track id !");
        continue;
      }
      if (! track_id_list.count(a_hit.get_track_id())) {
        track_id_list.insert(a_hit.get_track_id());
        alpha_track_parameters dummy;
        simulated_alphas_.push_back(dummy);
      }
      alpha_track_parameters & alpha = simulated_alphas_.back();
      alpha.length += (a_hit.get_position_stop() - a_hit.get_position_start()).mag();
    }

    return;
  }

  void snemo_alpha_delayed_studies_module::_process_reconstructed_alphas(const datatools::things & data_record_,
                                                                         alpha_list_type & reconstructed_alphas_)
  {
    // Check if some 'topology_data' are available in the data model:
    const std::string td_label = "TD";//snemo::datamodel::data_info::default_particle_track_data_label();
    if (! data_record_.has(td_label)) {
      DT_LOG_ERROR(get_logging_priority(), "Missing topology data to be processed !");
      return;
    }
    // Get the 'topology_data' entry from the data model :
    const snemo::datamodel::topology_data & td
      = data_record_.get<snemo::datamodel::topology_data>(td_label);

    DT_LOG_DEBUG(get_logging_priority(), "Topology data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) td.tree_dump();

    if (! td.has_pattern()) return;
    const snemo::datamodel::base_topology_pattern & a_pattern = td.get_pattern();
    if (a_pattern.get_pattern_id() != snemo::datamodel::topology_1e1a_pattern::pattern_id()) {
      DT_LOG_WARNING(get_logging_priority(), "Topology pattern does not match '"
                     << snemo::datamodel::topology_1e1a_pattern::pattern_id() << "' topology !");
      return;
    }
    const snemo::datamodel::topology_1e1a_pattern & a_1e1a_pattern =
      dynamic_cast<const snemo::datamodel::topology_1e1a_pattern &>(a_pattern);
    alpha_track_parameters alpha;
    alpha.length = a_1e1a_pattern.get_alpha_track_length();
    reconstructed_alphas_.push_back(alpha);

    return;
  }

  void snemo_alpha_delayed_studies_module::_compare_track_length(const alpha_list_type & sim_alphas_,
                                                                 const alpha_list_type & rec_alphas_)
  {
    if (sim_alphas_.size() != 1 || rec_alphas_.size() != 1) {
      DT_LOG_DEBUG(get_logging_priority(), "Only one alpha is expected !");
      return;
    }

    mygsl::histogram_pool & a_pool = grab_histogram_pool();
    if (a_pool.has_1d("1e1a::delta_track_length")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d("1e1a::delta_track_length");
      const double ratio = rec_alphas_.front().length/sim_alphas_.front().length;
      h1d.fill(1.0 - ratio);
    }
    return;
  }

} // namespace analysis
} // namespace snemo

// end of snemo_alpha_delayed_studies_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
