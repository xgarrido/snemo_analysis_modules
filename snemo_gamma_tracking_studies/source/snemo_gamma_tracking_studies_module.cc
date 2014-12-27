// snemo_gamma_tracking_studies_module.cc

// Ourselves:
#include <snemo_gamma_tracking_studies_module.h>

// Standard library:
#include <stdexcept>
#include <sstream>
#include <algorithm>

// Third party:
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

  void snemo_gamma_tracking_studies_module::_set_defaults()
  {
    _efficiency_ = {0, 0, 0, 0, 0};
    return;
  }

  // Initialization :
  void snemo_gamma_tracking_studies_module::initialize(const datatools::properties  & config_,
                                                       datatools::service_manager   & /*service_manager_*/,
                                                       dpp::module_handle_dict_type & /*module_dict_*/)
  {
    DT_THROW_IF(is_initialized(),
                std::logic_error,
                "Module '" << get_name() << "' is already initialized ! ");

    dpp::base_module::_common_initialize(config_);

    // Tag the module as initialized :
    _set_initialized(true);
    return;
  }

  // Reset :
  void snemo_gamma_tracking_studies_module::reset()
  {
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    // Present results
    DT_LOG_NOTICE(get_logging_priority(),
                  "Number of gammas well reconstructed = " << _efficiency_.ngood << "/" << _efficiency_.ntotal
                  << " (" << _efficiency_.ngood/(double)_efficiency_.ntotal*100 << "%)");
    DT_LOG_NOTICE(get_logging_priority(),
                  "Number of gammas missed = " << _efficiency_.nmiss << "/" << _efficiency_.nevent
                  << " (" << _efficiency_.nmiss/(double)_efficiency_.nevent*100 << "%)");

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
                                                                                                  gamma_dict_type & simulated_gammas_)
  {
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

    // Get total number of gammas simulated
    _efficiency_.ngamma = 0;
    for (auto i : sd.get_primary_event().get_particles()) {
      if (i.is_gamma()) _efficiency_.ngamma++;
    }

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

    // Stop proccess if no calibrated calorimeters
    if (! cd.has_calibrated_calorimeter_hits()) return dpp::base_module::PROCESS_CONTINUE;
    const snemo::datamodel::calibrated_data::calorimeter_hit_collection_type & cch
      = cd.calibrated_calorimeter_hits();

    // Fetch simulated step hits from calorimeter blocks
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
                                                                                                      gamma_dict_type & reconstructed_gammas_)
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
    _efficiency_.nevent++;
    _efficiency_.ntotal += _efficiency_.ngamma;
    if (reconstructed_gammas_.empty() && simulated_gammas_.empty()) {
      DT_LOG_DEBUG(get_logging_priority(), "No gammas have been catched and reconstructed !");
      _efficiency_.nmiss++;
      _efficiency_.ngood += _efficiency_.ngamma;
      return;
    }

    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) {
      DT_LOG_DEBUG(get_logging_priority(), "Simulated gammas :");
      for (auto i : simulated_gammas_) {
        std::ostringstream oss;
        oss << "Gamma #" << i.first << " :";
        for (auto icalo : i.second) {
          oss << " -> " << icalo;
        }
        DT_LOG_DEBUG(get_logging_priority(), oss.str());
      }
      DT_LOG_DEBUG(get_logging_priority(), "Reconstructed gammas :");
      for (auto i : reconstructed_gammas_) {
        std::ostringstream oss;
        oss << "Gamma #" << i.first << " :";
        for (auto icalo : i.second) {
          oss << " -> " << icalo;
        }
        DT_LOG_DEBUG(get_logging_priority(), oss.str());
      }
    }

    for (auto irec : reconstructed_gammas_) {
      const calo_list_type & a_rec_list = irec.second;
      for (auto isim : simulated_gammas_) {
        const calo_list_type & a_sim_list = isim.second;
        if (a_rec_list.size() != a_sim_list.size()) continue;
        const bool are_same = std::equal(a_rec_list.begin(), a_rec_list.end(), a_sim_list.begin());
        if (are_same) {
          _efficiency_.ngood++;
          DT_LOG_DEBUG(get_logging_priority(), "Sequences are identical !");
          break;
        }
      }
    }
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
