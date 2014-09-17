// snemo_bremsstrahlung_module.cc

// Ourselves:
#include <snemo_bremsstrahlung_module.h>

// Standard library:
#include <stdexcept>
#include <sstream>
#include <numeric>

// Third party:
// - Bayeux/datatools:
#include <datatools/clhep_units.h>
#include <datatools/service_manager.h>
// - Bayeux/geomtools:
#include <geomtools/geometry_service.h>
#include <geomtools/manager.h>
// - Bayeux/mygsl
#include <mygsl/histogram_pool.h>
// - Bayeux/dpp
#include <dpp/histogram_service.h>

// SuperNEMO event model
#include <falaise/snemo/processing/services.h>

// Geometry manager
#include <falaise/snemo/geometry/locator_plugin.h>
#include <falaise/snemo/geometry/calo_locator.h>
#include <falaise/snemo/geometry/xcalo_locator.h>
#include <falaise/snemo/geometry/gveto_locator.h>

// SuperNEMO event model
#include <snemo/datamodels/data_model.h>
#include <snemo/datamodels/particle_track_data.h>
#include <snemo/datamodels/helix_trajectory_pattern.h>

namespace analysis {

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_bremsstrahlung_module,
                                    "analysis::snemo_bremsstrahlung_module");

  // Set the histogram pool used by the module :
  void snemo_bremsstrahlung_module::set_histogram_pool (mygsl::histogram_pool & pool_)
  {
    DT_THROW_IF(is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is already initialized !");
    _histogram_pool_ = &pool_;
    return;
  }

  // Grab the histogram pool used by the module :
  mygsl::histogram_pool & snemo_bremsstrahlung_module::grab_histogram_pool ()
  {
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");
    return *_histogram_pool_;
  }

  void snemo_bremsstrahlung_module::_set_defaults ()
  {
    _histogram_pool_ = 0;
    return;
  }

  // Constructor :
  snemo_bremsstrahlung_module::snemo_bremsstrahlung_module(datatools::logger::priority logging_priority_)
    : dpp::base_module(logging_priority_)
  {
    _set_defaults();
    return;
  }

  // Destructor :
  snemo_bremsstrahlung_module::~snemo_bremsstrahlung_module()
  {
    if (is_initialized()) snemo_bremsstrahlung_module::reset();
    return;
  }

  // Initialization :
  void snemo_bremsstrahlung_module::initialize(const datatools::properties  & config_,
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

    // Geometry manager :
    std::string geo_label = snemo::processing::service_info::default_geometry_service_label();
    if (config_.has_key("Geo_label")) {
      geo_label = config_.fetch_string("Geo_label");
    }
    DT_THROW_IF(geo_label.empty(), std::logic_error,
                "Module '" << get_name() << "' has no valid '" << "Geo_label" << "' property !");
    DT_THROW_IF(! service_manager_.has(geo_label) ||
                ! service_manager_.is_a<geomtools::geometry_service>(geo_label),
                std::logic_error,
                "Module '" << get_name() << "' has no '" << geo_label << "' service !");
    geomtools::geometry_service & Geo
      = service_manager_.get<geomtools::geometry_service>(geo_label);

    // Get geometry locator plugin
    const geomtools::manager & geo_mgr = Geo.get_geom_manager();
    std::string locator_plugin_name;
    if (config_.has_key("locator_plugin_name"))
      {
        locator_plugin_name = config_.fetch_string("locator_plugin_name");
      }
    else
      {
        // If no locator plugin name is set, then search for the first one
        const geomtools::manager::plugins_dict_type & plugins = geo_mgr.get_plugins();
        for (geomtools::manager::plugins_dict_type::const_iterator ip = plugins.begin();
             ip != plugins.end();
             ip++) {
          const std::string & plugin_name = ip->first;
          if (geo_mgr.is_plugin_a<snemo::geometry::locator_plugin>(plugin_name)) {
            DT_LOG_DEBUG(get_logging_priority(), "Find locator plugin with name = " << plugin_name);
            locator_plugin_name = plugin_name;
            break;
          }
        }
      }
    // Access to a given plugin by name and type :
    DT_THROW_IF(! geo_mgr.has_plugin(locator_plugin_name) ||
                ! geo_mgr.is_plugin_a<snemo::geometry::locator_plugin>(locator_plugin_name),
                std::logic_error,
                "Found no locator plugin named '" << locator_plugin_name << "'");
    _locator_plugin_ = &geo_mgr.get_plugin<snemo::geometry::locator_plugin>(locator_plugin_name);

    // Tag the module as initialized :
    _set_initialized (true);
    return;
  }

  // Reset :
  void snemo_bremsstrahlung_module::reset()
  {
    DT_THROW_IF(! is_initialized(),
                std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    _set_defaults();

    // Tag the module as un-initialized :
    _set_initialized (false);
    return;
  }

  // Processing :
  dpp::base_module::process_status snemo_bremsstrahlung_module::process(datatools::things & data_record_)
  {
    DT_LOG_TRACE(get_logging_priority(), "Entering...");
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    // Check if the 'particle track data' record bank is available :
    const std::string ptd_label = snemo::datamodel::data_info::default_particle_track_data_label();
    if (! data_record_.has(ptd_label)) {
      DT_LOG_ERROR(get_logging_priority (), "Could not find any bank with label '"
                   << ptd_label << "' !");
      return dpp::base_module::PROCESS_STOP;
    }
    const snemo::datamodel::particle_track_data & ptd
      = data_record_.get<snemo::datamodel::particle_track_data>(ptd_label);

    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) {
      DT_LOG_DEBUG(get_logging_priority(), "Particle track data : ");
      ptd.tree_dump();
    }

    this->_compute_energy_distribution(ptd);
    this->_compute_angular_distribution(ptd);

    DT_LOG_TRACE(get_logging_priority(), "Exiting.");
    return dpp::base_module::PROCESS_SUCCESS;
  }

  void snemo_bremsstrahlung_module::_compute_energy_distribution(const snemo::datamodel::particle_track_data & ptd_) const
  {
    // Loop over all saved particles
    const snemo::datamodel::particle_track_data::particle_collection_type & the_particles
      = ptd_.get_particles();
    for (snemo::datamodel::particle_track_data::particle_collection_type::const_iterator
           iparticle = the_particles.begin();
         iparticle != the_particles.end();
         ++iparticle) {
      const snemo::datamodel::particle_track & a_particle = iparticle->get();

        if (!a_particle.has_associated_calorimeter_hits()) continue;

        const snemo::datamodel::calibrated_calorimeter_hit::collection_type & the_calorimeters
          = a_particle.get_associated_calorimeter_hits();

        if (the_calorimeters.size() > 1) continue;

        // Filling the histograms :
        if (_histogram_pool_->has_1d("electron_energy")) {
          mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d("electron_energy");
          h1d.fill(the_calorimeters.front().get().get_energy());
        }
    }

    // Loop over all non associated calorimeters
    const snemo::datamodel::calibrated_calorimeter_hit::collection_type & the_non_asso_calos
      = ptd_.get_non_associated_calorimeters();
    double total_gamma_energy = 0.0;
    for (snemo::datamodel::calibrated_calorimeter_hit::collection_type::const_iterator
           icalo = the_non_asso_calos.begin();
         icalo != the_non_asso_calos.end(); ++icalo) {
      total_gamma_energy += icalo->get().get_energy();
    }
    // Filling the histograms :
    if (_histogram_pool_->has_1d("gamma_energy")) {
      mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d("gamma_energy");
      h1d.fill(total_gamma_energy);
    }

    return;
  }

  void snemo_bremsstrahlung_module::_compute_angular_distribution(const snemo::datamodel::particle_track_data & ptd_) const
  {
    // Loop over all saved particles
    const snemo::datamodel::particle_track_data::particle_collection_type & the_particles
      = ptd_.get_particles();
    for (snemo::datamodel::particle_track_data::particle_collection_type::const_iterator
           iparticle = the_particles.begin();
         iparticle != the_particles.end();
         ++iparticle) {
      const snemo::datamodel::particle_track & a_particle = iparticle->get();

      if (! a_particle.has_trajectory()) {
        DT_LOG_DEBUG(get_logging_priority(), "Current particle has no trajectory !");
        continue;
      }

      // Retrieve a subset of vertices
      snemo::datamodel::particle_track::vertex_collection_type vertices;
      const size_t nvtx = a_particle.compute_vertices(vertices, snemo::datamodel::particle_track::VERTEX_ON_SOURCE_FOIL);
      if (nvtx != 1) {
        DT_LOG_DEBUG(get_logging_priority(), "Current particle has more than one vertex on source foil !");
        continue;
      }

      // Get helix parameters
      const snemo::datamodel::tracker_trajectory & a_trajectory = a_particle.get_trajectory();
      const snemo::datamodel::base_trajectory_pattern & a_track_pattern = a_trajectory.get_pattern();
      DT_THROW_IF(a_track_pattern.get_pattern_id() != snemo::datamodel::helix_trajectory_pattern::pattern_id(),
                  std::logic_error, "Trajectory must be an helix !");

      const snemo::datamodel::helix_trajectory_pattern * ptr_helix
        = dynamic_cast<const snemo::datamodel::helix_trajectory_pattern *>(&a_track_pattern);
      const geomtools::helix_3d & a_helix = ptr_helix->get_helix();
      const geomtools::vector_3d & vcenter = a_helix.get_center();
      const geomtools::vector_3d & vfoil = vertices.front().get().get_position();
      // Compute orthogonal vector
      const geomtools::vector_3d vorth = (vfoil - vcenter).orthogonal();

      DT_LOG_DEBUG(get_logging_priority(), "Particle direction = " << vorth);

      // Loop over all non associated calorimeters to compute angle difference
      const snemo::datamodel::calibrated_calorimeter_hit::collection_type & the_non_asso_calos
        = ptd_.get_non_associated_calorimeters();
      for (snemo::datamodel::calibrated_calorimeter_hit::collection_type::const_iterator
             icalo = the_non_asso_calos.begin();
           icalo != the_non_asso_calos.end(); ++icalo)
        {
          // Get block geom_id
          const geomtools::geom_id & a_gid = icalo->get().get_geom_id();

          // Get block position given
          geomtools::vector_3d vblock;
          geomtools::invalidate(vblock);

          const snemo::geometry::calo_locator & calo_locator = _locator_plugin_->get_calo_locator();
          const snemo::geometry::xcalo_locator & xcalo_locator = _locator_plugin_->get_xcalo_locator();
          const snemo::geometry::gveto_locator & gveto_locator = _locator_plugin_->get_gveto_locator();
          if (calo_locator.is_calo_block(a_gid)) {
            calo_locator.get_block_position(a_gid, vblock);
          } else if (xcalo_locator.is_calo_block(a_gid)) {
            xcalo_locator.get_block_position(a_gid, vblock);
          } else if (gveto_locator.is_calo_block(a_gid)) {
            gveto_locator.get_block_position(a_gid, vblock);
          }
          DT_THROW_IF(! geomtools::is_valid(vblock), std::logic_error,
                      "The calorimeter with geom_id '" << a_gid << "' has not be found !");

          const double angle = vorth.angle(vblock - vfoil);
          DT_LOG_DEBUG(get_logging_priority(), "The angle between electron and gamma is "
                       << angle / CLHEP::degree << " degree");
          // Filling the histograms :
          if (_histogram_pool_->has_1d("gamma_angle")) {
            mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d("gamma_angle");
            h1d.fill(angle);
          }
        }
    }

    return;
  }


} // namespace analysis

// end of snemo_bremsstrahlung_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
