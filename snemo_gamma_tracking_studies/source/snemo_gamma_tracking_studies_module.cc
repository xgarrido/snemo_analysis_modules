// snemo_gamma_tracking_studies_module.cc

// Ourselves:
#include <snemo_gamma_tracking_studies_module.h>

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
#include <mctools/utils.h>
#include <mctools/simulated_data.h>

// - Falaise
#include <snemo/datamodels/data_model.h>
#include <snemo/datamodels/calibrated_data.h>
#include <snemo/datamodels/particle_track_data.h>

namespace analysis {

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_gamma_tracking_studies_module,
                                    "analysis::snemo_gamma_tracking_studies_module");

  // Set the histogram pool used by the module :
  void snemo_gamma_tracking_studies_module::set_histogram_pool(mygsl::histogram_pool & pool_)
  {
    DT_THROW_IF(is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is already initialized !");
    _histogram_pool_ = &pool_;
    return;
  }

  // Grab the histogram pool used by the module :
  mygsl::histogram_pool & snemo_gamma_tracking_studies_module::grab_histogram_pool()
  {
    DT_THROW_IF (! is_initialized(), std::logic_error,
                 "Module '" << get_name() << "' is not initialized !");
    return *_histogram_pool_;
  }

  void snemo_gamma_tracking_studies_module::_set_defaults()
  {
    _histogram_pool_ = 0;
    return;
  }

  // Initialization :
  void snemo_gamma_tracking_studies_module::initialize(const datatools::properties  & config_,
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
      if (config_.has_key("Histo_output_files")) {
        std::vector<std::string> output_files;
        config_.fetch("Histo_output_files", output_files);
        for (size_t i = 0; i < output_files.size(); i++) {
          Histo.add_output_file(output_files[i]);
        }
      }
      if (config_.has_key("Histo_template_files")) {
        std::vector<std::string> template_files;
        config_.fetch("Histo_template_files", template_files);
        for (size_t i = 0; i < template_files.size(); i++) {
          Histo.grab_pool().load(template_files[i]);
        }
      }
    }

    // Tag the module as initialized :
    _set_initialized(true);
    return;
  }

  // Reset :
  void snemo_gamma_tracking_studies_module::reset()
  {
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    // Tag the module as un-initialized :
    _set_initialized(false);
    _set_defaults();
    return;
  }

  // Constructor :
  snemo_gamma_tracking_studies_module::snemo_gamma_tracking_studies_module(datatools::logger::priority logging_priority_)
    : dpp::base_module(logging_priority_)
  {
    _set_defaults();
    return;
  }

  // Destructor :
  snemo_gamma_tracking_studies_module::~snemo_gamma_tracking_studies_module()
  {
    if (is_initialized()) snemo_gamma_tracking_studies_module::reset();
    return;
  }

  // Processing :
  dpp::base_module::process_status snemo_gamma_tracking_studies_module::process(datatools::things & data_record_)
  {
    DT_LOG_TRACE(get_logging_priority(), "Entering...");
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    gamma_dict_type simulated_gammas;
    {
      const process_status status = _process_simulated_gammas(data_record_, simulated_gammas);
      if (status != dpp::base_module::PROCESS_OK) {
        DT_LOG_ERROR(get_logging_priority(), "Processing of simulated data fails !");
        return status;
      }
    }
    gamma_dict_type reconstructed_gammas;
    {
      const process_status status = _process_reconstructed_gammas(data_record_, reconstructed_gammas);
      if (status != dpp::base_module::PROCESS_OK) {
        DT_LOG_ERROR(get_logging_priority(), "Processing of particle track data fails !");
        return status;
      }
    }

    _compare_sequences(simulated_gammas, reconstructed_gammas);

    DT_LOG_TRACE(get_logging_priority(), "Exiting.");
    return dpp::base_module::PROCESS_SUCCESS;
  }

  dpp::base_module::process_status snemo_gamma_tracking_studies_module::_process_simulated_gammas(const datatools::things & data_record_,
                                                                                                  gamma_dict_type & simulated_gammas_) const
  {
    // Check if some 'calibrated_data' are available in the data model:
    const std::string cd_label = snemo::datamodel::data_info::default_calibrated_data_label();
    if (! data_record_.has(cd_label)) {
      DT_LOG_ERROR(get_logging_priority(), "Missing calibrated data to be processed !");
      return dpp::base_module::PROCESS_ERROR;
    }
    // Get the 'calibrated_data' entry from the data model :
    const snemo::datamodel::calibrated_data & cd
      = data_record_.get<snemo::datamodel::calibrated_data>(cd_label);

    DT_LOG_DEBUG(get_logging_priority(), "Calibrated data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) cd.tree_dump();

    if (! cd.has_calibrated_calorimeter_hits()) return dpp::base_module::PROCESS_CONTINUE;
    const snemo::datamodel::calibrated_data::calorimeter_hit_collection_type & cch
      = cd.calibrated_calorimeter_hits();

    // Check if some 'simulated_data' are available in the data model:
    const std::string sd_label = snemo::datamodel::data_info::default_simulated_data_label();
    if (! data_record_.has(sd_label)) {
      DT_LOG_ERROR(get_logging_priority(), "Missing simulated data to be processed !");
      return dpp::base_module::PROCESS_ERROR;
    }
    // Get the 'simulated_data' entry from the data model :
    const mctools::simulated_data & sd = data_record_.get<mctools::simulated_data>(sd_label);

    DT_LOG_DEBUG(get_logging_priority(), "Simulated data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) sd.tree_dump();

    const std::string hit_label = "__visu.tracks.calo";
    if (! sd.has_step_hits(hit_label)) return dpp::base_module::PROCESS_CONTINUE;
    const mctools::simulated_data::hit_handle_collection_type & hit_collection
      = sd.get_step_hits(hit_label);
    if (hit_collection.empty()) {
      DT_LOG_DEBUG(get_logging_priority(), "No simulated calorimeter hits");
      return dpp::base_module::PROCESS_CONTINUE;
    }

    for (auto ihit : hit_collection) {
      const mctools::base_step_hit & a_hit = ihit.get();
      const datatools::properties & a_aux = a_hit.get_auxiliaries();

      int track_id = -1;
      if (a_aux.has_key(mctools::track_utils::TRACK_ID_KEY)) {
        track_id = a_aux.fetch_integer(mctools::track_utils::TRACK_ID_KEY);
      }
      if (a_aux.has_key(mctools::track_utils::PARENT_TRACK_ID_KEY)) {
        track_id = a_aux.fetch_integer(mctools::track_utils::PARENT_TRACK_ID_KEY);
      }
      DT_THROW_IF(track_id == -1, std::logic_error, "Missing primary track id !");
      if (track_id == 0) continue; // Primary particles


      // Check if calorimeter has been calibrated
      const geomtools::geom_id & gid = a_hit.get_geom_id();
      if (std::find_if(cch.begin(), cch.end(), [gid] (auto hit_) {
            return gid == hit_.get().get_geom_id();
          }) == cch.end()) continue;
      simulated_gammas_[track_id].insert(gid);
    }

    return dpp::base_module::PROCESS_OK;
  }

  dpp::base_module::process_status snemo_gamma_tracking_studies_module::_process_reconstructed_gammas(const datatools::things & data_record_,
                                                                                                      gamma_dict_type & reconstructed_gammas_) const
  {
    // Check if some 'particle_track_data' are available in the data model:
    const std::string ptd_label = snemo::datamodel::data_info::default_particle_track_data_label();
    if (! data_record_.has(ptd_label)) {
      DT_LOG_ERROR(get_logging_priority(), "Missing particle track data to be processed !");
      return dpp::base_module::PROCESS_ERROR;
    }
    // Get the 'particle_track_data' entry from the data model :
    const snemo::datamodel::particle_track_data & ptd
      = data_record_.get<snemo::datamodel::particle_track_data>(ptd_label);

    DT_LOG_DEBUG(get_logging_priority(), "Particle track data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) ptd.tree_dump();

    snemo::datamodel::particle_track_data::particle_collection_type gammas;
    const size_t ngammas = ptd.fetch_particles(gammas, snemo::datamodel::particle_track::NEUTRAL);
    if (ngammas == 0) return dpp::base_module::PROCESS_CONTINUE;

    for (auto igamma : gammas) {
      for (auto icalo : igamma.get().get_associated_calorimeter_hits()) {
        const geomtools::geom_id & gid = icalo.get().get_geom_id();
        reconstructed_gammas_[igamma.get().get_track_id()].insert(gid);
      }
    }

    return dpp::base_module::PROCESS_OK;
  }

  void snemo_gamma_tracking_studies_module::_compare_sequences(const gamma_dict_type & simulated_gammas_,
                                                               const gamma_dict_type & reconstructed_gammas_)
  {


  }

} // namespace analysis

// end of snemo_gamma_tracking_studies_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
