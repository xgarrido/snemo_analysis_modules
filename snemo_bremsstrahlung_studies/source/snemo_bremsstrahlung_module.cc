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

// // SuperNEMO event model
// #include <sncore/models/data_model.h>
// #include <sncore/models/line_trajectory_pattern.h>
// #include <sncore/models/helix_trajectory_pattern.h>
// #include <snanalysis/models/particle_track_data.h>


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

    // namespace sam = snemo::analysis::model;

    // // Check if the 'particle track' event record bank is available :
    // if (! DATATOOLS_UTILS_THINGS_CHECK_BANK (event_record_, _PTD_bank_label_, sam::particle_track_data))
    //   {
    //     std::clog << datatools::utils::io::error
    //               << "snemo::analysis::processing::"
    //               << "snemo_bremsstrahlung_module::process: "
    //               << "Could not find any bank with label '"
    //               << _PTD_bank_label_ << "' !"
    //               << std::endl;
    //     // Cannot find the event record bank :
    //     return STOP;
    //   }
    // // Get a const reference to this event record bank of interest :
    // DATATOOLS_UTILS_THINGS_CONST_BANK (event_record_, _PTD_bank_label_, sam::particle_track_data, ptd);
    // if (is_debug ())
    //   {
    //     std::clog << datatools::utils::io::debug
    //               << "snemo::analysis::processing::"
    //               << "snemo_bremsstrahlung_module::process: "
    //               << "Found the bank '" << _PTD_bank_label_ << "' !"
    //               << std::endl;
    //   }

    // if (is_debug ())
    //   {
    //     std::clog << datatools::utils::io::debug
    //               << "snemo::analysis::processing::"
    //               << "snemo_bremsstrahlung_module::process: "
    //               << "Particle track data : " << std::endl;
    //     ptd.tree_dump (std::clog, "", "DEBUG: ");
    //   }

    // _compute_energy_distribution_  (ptd);
    // _compute_angular_distribution_ (ptd);

    DT_LOG_TRACE(get_logging_priority(), "Exiting.");
    return dpp::base_module::PROCESS_SUCCESS;
  }

  // void snemo_bremsstrahlung_module::_compute_energy_distribution_ (const model::particle_track_data & ptd_) const
  // {
  //   namespace sam = snemo::analysis::model;

  //   // Loop over all saved particles
  //   const sam::particle_track_data::particle_collection_type & the_particles
  //     = ptd_.get_particles ();
  //   for (sam::particle_track_data::particle_collection_type::const_iterator
  //          iparticle = the_particles.begin ();
  //        iparticle != the_particles.end ();
  //        ++iparticle)
  //     {
  //       const sam::particle_track & a_particle = iparticle->get ();

  //       if (!a_particle.has_associated_calorimeters ()) continue;

  //       const sam::particle_track::calorimeter_collection_type & the_calorimeters
  //         = a_particle.get_associated_calorimeters ();

  //       if (the_calorimeters.size () > 1) continue;

  //       // Filling the histograms :
  //       if (_histogram_pool_->has_1d ("electron_energy"))
  //         {
  //           mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d ("electron_energy");
  //           h1d.fill (the_calorimeters.front ().get ().get_energy ());
  //         }
  //     }

  //   // Loop over all non associated calorimeters
  //   const sam::particle_track_data::calorimeter_collection_type & the_non_asso_calos
  //     = ptd_.get_non_associated_calorimeters ();
  //   double total_gamma_energy = 0.0;
  //   for (sam::particle_track_data::calorimeter_collection_type::const_iterator
  //          icalo = the_non_asso_calos.begin ();
  //        icalo != the_non_asso_calos.end (); ++icalo)
  //     {
  //       total_gamma_energy += icalo->get ().get_energy ();
  //     }
  //   // Filling the histograms :
  //   if (_histogram_pool_->has_1d ("gamma_energy"))
  //     {
  //       mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d ("gamma_energy");
  //       h1d.fill (total_gamma_energy);
  //     }

  //   return;
  // }

  // void snemo_bremsstrahlung_module::_compute_angular_distribution_ (const model::particle_track_data & ptd_) const
  // {
  //   namespace scm = snemo::core::model;
  //   namespace sam = snemo::analysis::model;
  //   namespace sgs = snemo::geometry::snemo;

  //   // Prepare locators
  //   const int module_number = 0;
  //   sgs::calo_locator  the_calo_locator  = sgs::calo_locator (get_geom_manager (), module_number);
  //   sgs::xcalo_locator the_xcalo_locator = sgs::xcalo_locator (get_geom_manager (), module_number);
  //   sgs::gveto_locator the_gveto_locator = sgs::gveto_locator (get_geom_manager (), module_number);

  //   // Loop over all saved particles
  //   const sam::particle_track_data::particle_collection_type & the_particles
  //     = ptd_.get_particles ();
  //   for (sam::particle_track_data::particle_collection_type::const_iterator
  //          iparticle = the_particles.begin ();
  //        iparticle != the_particles.end ();
  //        ++iparticle)
  //     {
  //       const sam::particle_track & a_particle = iparticle->get ();

  //       if (! a_particle.has_trajectory ()) continue;

  //       const scm::tracker_trajectory & a_trajectory = a_particle.get_trajectory ();
  //       const scm::base_trajectory_pattern & a_track_pattern = a_trajectory.get_pattern ();

  //       if (a_track_pattern.get_pattern_id () == scm::line_trajectory_pattern::PATTERN_ID)
  //         {
  //           if (is_debug ())
  //             {
  //               std::clog << datatools::utils::io::debug
  //                         << "snemo::analysis::processing::"
  //                         << "snemo_bremsstrahlung_module::_compute_angular_distribution_: "
  //                         << "Particle trajectory is a line" << std::endl;
  //             }
  //           continue;
  //         }

  //       // Retrieve helix trajectory
  //       const scm::helix_trajectory_pattern * ptr_helix = 0;
  //       if (a_track_pattern.get_pattern_id () == scm::helix_trajectory_pattern::PATTERN_ID)
  //         {
  //           ptr_helix = dynamic_cast<const scm::helix_trajectory_pattern *>(&a_track_pattern);
  //         }

  //       if (!ptr_helix)
  //         {
  //           std::cerr << datatools::utils::io::error
  //                     << "snemo::analysis::processing::"
  //                     << "snemo_bremsstrahlung_module::_compute_angular_distribution_: "
  //                     << "Tracker trajectory is not an 'helix' !"
  //                     << std::endl;
  //           return;
  //         }

  //       // Get helix parameters
  //       const geomtools::helix_3d & a_helix = ptr_helix->get_helix ();
  //       const geomtools::vector_3d & center = a_helix.get_center ();
  //       const double radius = a_helix.get_radius ();
  //       const geomtools::vector_3d foil = a_particle.has_negative_charge () ?
  //         a_helix.get_last () : a_helix.get_first ();
  //       // Compute orthogonal vector
  //       const geomtools::vector_3d vorth = (foil - center).orthogonal ();

  //       // Loop over all non associated calorimeters to compute angle difference
  //       const sam::particle_track_data::calorimeter_collection_type & the_non_asso_calos
  //         = ptd_.get_non_associated_calorimeters ();
  //       for (sam::particle_track_data::calorimeter_collection_type::const_iterator
  //              icalo = the_non_asso_calos.begin ();
  //            icalo != the_non_asso_calos.end (); ++icalo)
  //         {
  //           // Get block geom_id
  //           const geomtools::geom_id & a_gid = icalo->get ().get_geom_id ();

  //           // Get block position given
  //           geomtools::vector_3d vblock;
  //           geomtools::invalidate (vblock);

  //           if (the_calo_locator.is_calo_block (a_gid))
  //             {
  //               the_calo_locator.get_block_position (a_gid, vblock);
  //             }
  //           else if (the_xcalo_locator.is_calo_block (a_gid))
  //             {
  //               the_xcalo_locator.get_block_position (a_gid, vblock);
  //             }
  //           else if (the_gveto_locator.is_calo_block (a_gid))
  //             {
  //               the_gveto_locator.get_block_position (a_gid, vblock);
  //             }
  //           else
  //             {
  //               std::ostringstream message;
  //               message << "snemo::analysis::processing::"
  //                       << "snemo_bremsstrahlung_module::_compute_angular_distribution_: "
  //                       << "The calorimeter with geom_id '" << a_gid << "' has not be found !";
  //               throw std::logic_error (message.str ());
  //             }

  //           double angle;
  //           if (geomtools::is_valid (vblock))
  //             {
  //               angle = vorth.angle (vblock - foil);
  //               if (is_debug ())
  //                 {
  //                   std::clog << datatools::utils::io::debug
  //                             << "snemo::analysis::processing::"
  //                             << "snemo_bremsstrahlung_module::_compute_angular_distribution_: "
  //                             << "The angle between electron and gamma is "
  //                             << angle / CLHEP::degree << " degree" << std::endl;
  //                 }
  //             }
  //           // Filling the histograms :
  //           if (_histogram_pool_->has_1d ("gamma_angle"))
  //             {
  //               mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d ("gamma_angle");
  //               h1d.fill (angle);
  //             }
  //         }
  //     }

  //   return;
  // }


} // namespace analysis

// end of snemo_bremsstrahlung_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
