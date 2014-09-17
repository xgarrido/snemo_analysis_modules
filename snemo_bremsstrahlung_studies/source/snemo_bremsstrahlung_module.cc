// /* nemo3_Y90_study_module.cc
//  *
//  * Copyright (C) 2012 Xavier Garrido <garrido@lal.in2p3.fr>

//  * This program is free software; you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License as published by
//  * the Free Software Foundation; either version 3 of the License, or (at
//  * your option) any later version.
//  *
//  * This program is distributed in the hope that it will be useful, but
//  * WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  * General Public License for more details.
//  *
//  * You should have received a copy of the GNU General Public License
//  * along with this program; if not, write to the Free Software
//  * Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  * Boston, MA 02110-1301, USA.
//  *
//  */

// #include <stdexcept>
// #include <sstream>
// #include <numeric>

// #include <snanalysis/processing/nemo3_Y90_study_module.h>

// #include <datatools/utils/things_macros.h>
// #include <datatools/utils/units.h>
// #include <datatools/utils/utils.h>
// #include <datatools/utils/ioutils.h>
// #include <datatools/utils/properties.h>
// #include <datatools/services/service_manager.h>

// // SuperNEMO event model
// #include <sncore/models/data_model.h>
// #include <sncore/models/line_trajectory_pattern.h>
// #include <sncore/models/helix_trajectory_pattern.h>
// #include <snanalysis/models/particle_track_data.h>

// // SuperNEMO services
// #include <sncore/services/histogram_service.h>
// #include <sncore/services/geometry_service.h>

// #include <sngeometry/manager.h>
// #include <sngeometry/snemo/calo_locator.h>
// #include <sngeometry/snemo/xcalo_locator.h>
// #include <sngeometry/snemo/gveto_locator.h>

// // Histogram
// #include <mygsl/histogram_pool.h>

// namespace snemo {

//   namespace analysis {

//     namespace processing {

//       // Registration instantiation macro :
//       SNEMO_MODULE_REGISTRATION_IMPLEMENT(nemo3_Y90_study_module,
//                                           "snemo::analysis::processing::nemo3_Y90_study_module");

//       // Default label for the 'particle track' event record bank :
//       const std::string nemo3_Y90_study_module::DEFAULT_PTD_LABEL = "PTD";

//       // Set the label of the 'particle track' bank used by the module :
//       void nemo3_Y90_study_module::set_PTD_bank_label (const std::string & PTD_label_)
//       {
//         _PTD_bank_label_ = PTD_label_;
//         return;
//       }

//       // Get the label of the 'particle track' bank used by the module :
//       const std::string & nemo3_Y90_study_module::get_PTD_bank_label () const
//       {
//         return _PTD_bank_label_;
//       }

//       // Set the label of the 'Histogram' service used by the module :
//       void nemo3_Y90_study_module::set_Histo_service_label (const std::string & Histo_label_)
//       {
//         _Histo_service_label_ = Histo_label_;
//         return;
//       }

//       // Get the label of the 'Histogram' service used by the module :
//       const std::string & nemo3_Y90_study_module::get_Histo_service_label () const
//       {
//         return _Histo_service_label_;
//       }

//       // Set the histogram pool used by the module :
//       void nemo3_Y90_study_module::set_histogram_pool (mygsl::histogram_pool & pool_)
//       {
//         if (is_initialized ())
//           {
//             std::ostringstream message;
//             message << "snemo::analysis::processing::"
//                     << "nemo3_Y90_study_module::set_histogram_pool: "
//                     << "Module '" << get_name () << "' is already initialized ! ";
//             throw std::logic_error (message.str ());
//           }
//         _histogram_pool_ = &pool_;
//         return;
//       }

//       // Grab the histogram pool used by the module :
//       mygsl::histogram_pool & nemo3_Y90_study_module::grab_histogram_pool ()
//       {
//         if (! is_initialized ())
//           {
//             std::ostringstream message;
//             message << "snemo::analysis::processing::"
//                     << "nemo3_Y90_study_module::grab_histogram_pool: "
//                     << "Module '" << get_name () << "' is not initialized ! ";
//             throw std::logic_error (message.str ());
//           }
//         return *_histogram_pool_;
//       }

//       // Set the label of the 'Geo' service used by the module :
//       void nemo3_Y90_study_module::set_Geo_service_label (const std::string & Geo_label_)
//       {
//         _Geo_service_label_ = Geo_label_;
//         return;
//       }

//       // Get the label of the 'Geometry' service used by the module :
//       const std::string & nemo3_Y90_study_module::get_Geo_service_label () const
//       {
//         return _Geo_service_label_;
//       }

//       void nemo3_Y90_study_module::set_geom_manager (const snemo::geometry::manager & gmgr_)
//       {
//         if (is_initialized ())
//           {
//             std::ostringstream message;
//             message << "snemo::analysis::processing::"
//                     << "nemo3_Y90_study_module::set_geom_manager: "
//                     << "Driver is already initialized !";
//             throw std::logic_error (message.str ());
//           }
//         _geom_manager_ = &gmgr_;
//         return;
//       }

//       const snemo::geometry::manager &
//       nemo3_Y90_study_module::get_geom_manager () const
//       {
//         if (! has_geom_manager ())
//           {
//             std::ostringstream message;
//             message << "snemo::analysis::processing::"
//                     << "nemo3_Y90_study_module::get_geom_manager: "
//                     << "No geometry manager is setup !";
//             throw std::logic_error (message.str ());
//           }
//         return *_geom_manager_;
//       }

//       bool nemo3_Y90_study_module::has_geom_manager () const
//       {
//         return _geom_manager_ != 0;
//       }

//       void nemo3_Y90_study_module::_init_defaults_ ()
//       {
//         _PTD_bank_label_.clear ();
//         _Histo_service_label_.clear ();
//         _Geo_service_label_.clear ();

//         _geom_manager_  = 0;
//         _histogram_pool_ = 0;
//         return;
//       }

//       /*** Implementation of the interface ***/

//       // Constructor :
//       SNEMO_MODULE_CONSTRUCTOR_IMPLEMENT_HEAD (nemo3_Y90_study_module,
//                                                debug_level_,
//                                                "nemo3_Y90_study_module",
//                                                "A module to study Y90 bremsstrahlung within source foil",
//                                                "0.1")
//       {
//         _init_defaults_ ();
//         return;
//       }

//       // Destructor :
//       SNEMO_MODULE_DESTRUCTOR_IMPLEMENT_HEAD (nemo3_Y90_study_module)
//       {
//         // Make sure all internal resources are terminated
//         // before destruction :
//         if (is_initialized ()) reset ();
//         return;
//       }

//       // Initialization :
//       SNEMO_MODULE_INITIALIZE_IMPLEMENT_HEAD (nemo3_Y90_study_module,
//                                               config_,
//                                               service_manager_,
//                                               module_dict_)
//       {
//         if (is_initialized ())
//           {
//             std::ostringstream message;
//             message << "nemo3_Y90_study_module::initialize: "
//                     << "Module '" << get_name () << "' is already initialized ! ";
//             throw std::logic_error (message.str ());
//           }

//         /**************************************************************
//          *   fetch setup parameters from the configuration container  *
//          **************************************************************/

//         if (! is_debug ())
//           {
//             if (config_.has_flag ("debug"))
//               {
//                 set_debug (true);
//               }
//           }

//         if (_PTD_bank_label_.empty ())
//           {
//             if (config_.has_key ("PTD_label"))
//               {
//                 const std::string label = config_.fetch_string ("PTD_label");
//                 set_PTD_bank_label (label);
//               }
//           }

//         if (_histogram_pool_ == 0)
//           {
//             if (_Histo_service_label_.empty ())
//               {
//                 if (config_.has_key ("Histo_label"))
//                   {
//                     const std::string label = config_.fetch_string ("Histo_label");
//                     set_Histo_service_label (label);
//                   }
//               }
//             if (service_manager_.has (_Histo_service_label_) &&
//                 service_manager_.is_a<snemo::core::service::histogram_service> (_Histo_service_label_))
//               {
//                 snemo::core::service::histogram_service & Histo
//                   = service_manager_.get<snemo::core::service::histogram_service> (_Histo_service_label_);
//                 set_histogram_pool (Histo.grab_pool ());
//               }
//             else
//               {
//                 std::ostringstream message;
//                 message << "snemo::analysis::processing::"
//                         << "nemo3_Y90_study_module::initialize: "
//                         << "Module '" << get_name ()
//                         << "' has no '" << _Histo_service_label_ << "' service !";
//                 throw std::logic_error (message.str ());
//               }
//           }

//         if (_histogram_pool_ == 0)
//           {
//             std::ostringstream message;
//             message << "snemo::analysis::processing::"
//                     << "nemo3_Y90_study_module::initialize: "
//                     << "Missing histogram pool !";
//             throw std::logic_error (message.str ());
//           }

//         // Geometry manager :
//         if (_geom_manager_ == 0)
//           {
//             if (_Geo_service_label_.empty ())
//               {
//                 if (config_.has_key ("Geo_label"))
//                   {
//                     const std::string label = config_.fetch_string ("Geo_label");
//                     set_Geo_service_label (label);
//                   }
//               }
//             if (service_manager_.has (_Geo_service_label_)
//                 && service_manager_.is_a< ::snemo::core::service::geometry_service> (_Geo_service_label_))
//               {
//                 snemo::core::service::geometry_service & Geo
//                   = service_manager_.get< ::snemo::core::service::geometry_service> (_Geo_service_label_);
//                 set_geom_manager (Geo.get_geom_manager ());
//               }
//             else
//               {
//                 std::ostringstream message;
//                 message << "snemo::analysis::processing::"
//                         << "nemo3_Y90_study_module::initialize: "
//                         << "Module '" << get_name ()
//                         << "' has no '" << _Geo_service_label_ << "' service !";
//                 throw std::logic_error (message.str ());
//               }
//           }

//         /*********************************************
//          *   do some check on the setup parameters   *
//          *********************************************/

//         if (_PTD_bank_label_.empty ())
//           {
//             _PTD_bank_label_ = DEFAULT_PTD_LABEL;
//           }


//         /*************************************
//          *  end of the initialization step   *
//          *************************************/

//         // Tag the module as initialized :
//         _set_initialized (true);
//         return;
//       }

//       // Reset :
//       SNEMO_MODULE_RESET_IMPLEMENT_HEAD (nemo3_Y90_study_module)
//       {
//         if (! is_initialized ())
//           {
//             std::ostringstream message;
//             message << "snemo::analysis::processing::"
//                     << "nemo3_Y90_study_module::initialize: "
//                     << "Module '" << get_name () << "' is not initialized !";
//             throw std::logic_error (message.str ());
//           }

//         /****************************
//          *  revert to some defaults *
//          ****************************/

//         this->_init_defaults_ ();

//         /****************************
//          *  end of the reset step   *
//          ****************************/

//         // Tag the module as un-initialized :
//         _set_initialized (false);
//         return;
//       }

//       // Processing :
//       SNEMO_MODULE_PROCESS_IMPLEMENT_HEAD (nemo3_Y90_study_module,
//                                            event_record_)
//       {
//         if (! is_initialized ())
//           {
//             std::ostringstream message;
//             message << "snemo::analysis::processing::"
//                     << "nemo3_Y90_study_module::process: "
//                     << "Module '" << get_name () << "' is not initialized !";
//             throw std::logic_error (message.str ());
//           }

//         namespace sam = snemo::analysis::model;

//         // Check if the 'particle track' event record bank is available :
//         if (! DATATOOLS_UTILS_THINGS_CHECK_BANK (event_record_, _PTD_bank_label_, sam::particle_track_data))
//           {
//             std::clog << datatools::utils::io::error
//                       << "snemo::analysis::processing::"
//                       << "nemo3_Y90_study_module::process: "
//                       << "Could not find any bank with label '"
//                       << _PTD_bank_label_ << "' !"
//                       << std::endl;
//             // Cannot find the event record bank :
//             return STOP;
//           }
//         // Get a const reference to this event record bank of interest :
//         DATATOOLS_UTILS_THINGS_CONST_BANK (event_record_, _PTD_bank_label_, sam::particle_track_data, ptd);
//         if (is_debug ())
//           {
//             std::clog << datatools::utils::io::debug
//                       << "snemo::analysis::processing::"
//                       << "nemo3_Y90_study_module::process: "
//                       << "Found the bank '" << _PTD_bank_label_ << "' !"
//                       << std::endl;
//           }

//         if (is_debug ())
//           {
//             std::clog << datatools::utils::io::debug
//                       << "snemo::analysis::processing::"
//                       << "nemo3_Y90_study_module::process: "
//                       << "Particle track data : " << std::endl;
//             ptd.tree_dump (std::clog, "", "DEBUG: ");
//           }

//         _compute_energy_distribution_  (ptd);
//         _compute_angular_distribution_ (ptd);

//          return SUCCESS;
//       }

//       void nemo3_Y90_study_module::_compute_energy_distribution_ (const model::particle_track_data & ptd_) const
//       {
//         namespace sam = snemo::analysis::model;

//         // Loop over all saved particles
//         const sam::particle_track_data::particle_collection_type & the_particles
//           = ptd_.get_particles ();
//         for (sam::particle_track_data::particle_collection_type::const_iterator
//                iparticle = the_particles.begin ();
//              iparticle != the_particles.end ();
//              ++iparticle)
//           {
//             const sam::particle_track & a_particle = iparticle->get ();

//             if (!a_particle.has_associated_calorimeters ()) continue;

//             const sam::particle_track::calorimeter_collection_type & the_calorimeters
//               = a_particle.get_associated_calorimeters ();

//             if (the_calorimeters.size () > 1) continue;

//             // Filling the histograms :
//             if (_histogram_pool_->has_1d ("electron_energy"))
//               {
//                 mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d ("electron_energy");
//                 h1d.fill (the_calorimeters.front ().get ().get_energy ());
//               }
//           }

//         // Loop over all non associated calorimeters
//         const sam::particle_track_data::calorimeter_collection_type & the_non_asso_calos
//           = ptd_.get_non_associated_calorimeters ();
//         double total_gamma_energy = 0.0;
//         for (sam::particle_track_data::calorimeter_collection_type::const_iterator
//                icalo = the_non_asso_calos.begin ();
//              icalo != the_non_asso_calos.end (); ++icalo)
//           {
//             total_gamma_energy += icalo->get ().get_energy ();
//           }
//         // Filling the histograms :
//         if (_histogram_pool_->has_1d ("gamma_energy"))
//           {
//             mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d ("gamma_energy");
//             h1d.fill (total_gamma_energy);
//           }

//         return;
//       }

//       void nemo3_Y90_study_module::_compute_angular_distribution_ (const model::particle_track_data & ptd_) const
//       {
//         namespace scm = snemo::core::model;
//         namespace sam = snemo::analysis::model;
//         namespace sgs = snemo::geometry::snemo;

//         // Prepare locators
//         const int module_number = 0;
//         sgs::calo_locator  the_calo_locator  = sgs::calo_locator (get_geom_manager (), module_number);
//         sgs::xcalo_locator the_xcalo_locator = sgs::xcalo_locator (get_geom_manager (), module_number);
//         sgs::gveto_locator the_gveto_locator = sgs::gveto_locator (get_geom_manager (), module_number);

//         // Loop over all saved particles
//         const sam::particle_track_data::particle_collection_type & the_particles
//           = ptd_.get_particles ();
//         for (sam::particle_track_data::particle_collection_type::const_iterator
//                iparticle = the_particles.begin ();
//              iparticle != the_particles.end ();
//              ++iparticle)
//           {
//             const sam::particle_track & a_particle = iparticle->get ();

//             if (! a_particle.has_trajectory ()) continue;

//             const scm::tracker_trajectory & a_trajectory = a_particle.get_trajectory ();
//             const scm::base_trajectory_pattern & a_track_pattern = a_trajectory.get_pattern ();

//             if (a_track_pattern.get_pattern_id () == scm::line_trajectory_pattern::PATTERN_ID)
//               {
//                 if (is_debug ())
//                   {
//                     std::clog << datatools::utils::io::debug
//                               << "snemo::analysis::processing::"
//                               << "nemo3_Y90_study_module::_compute_angular_distribution_: "
//                               << "Particle trajectory is a line" << std::endl;
//                   }
//                 continue;
//               }

//             // Retrieve helix trajectory
//             const scm::helix_trajectory_pattern * ptr_helix = 0;
//             if (a_track_pattern.get_pattern_id () == scm::helix_trajectory_pattern::PATTERN_ID)
//               {
//                 ptr_helix = dynamic_cast<const scm::helix_trajectory_pattern *>(&a_track_pattern);
//               }

//             if (!ptr_helix)
//               {
//                 std::cerr << datatools::utils::io::error
//                           << "snemo::analysis::processing::"
//                           << "nemo3_Y90_study_module::_compute_angular_distribution_: "
//                           << "Tracker trajectory is not an 'helix' !"
//                           << std::endl;
//                 return;
//               }

//             // Get helix parameters
//             const geomtools::helix_3d & a_helix = ptr_helix->get_helix ();
//             const geomtools::vector_3d & center = a_helix.get_center ();
//             const double radius = a_helix.get_radius ();
//             const geomtools::vector_3d foil = a_particle.has_negative_charge () ?
//               a_helix.get_last () : a_helix.get_first ();
//             // Compute orthogonal vector
//             const geomtools::vector_3d vorth = (foil - center).orthogonal ();

//             // Loop over all non associated calorimeters to compute angle difference
//             const sam::particle_track_data::calorimeter_collection_type & the_non_asso_calos
//               = ptd_.get_non_associated_calorimeters ();
//             for (sam::particle_track_data::calorimeter_collection_type::const_iterator
//                    icalo = the_non_asso_calos.begin ();
//                  icalo != the_non_asso_calos.end (); ++icalo)
//               {
//                 // Get block geom_id
//                 const geomtools::geom_id & a_gid = icalo->get ().get_geom_id ();

//                 // Get block position given
//                 geomtools::vector_3d vblock;
//                 geomtools::invalidate (vblock);

//                 if (the_calo_locator.is_calo_block (a_gid))
//                   {
//                     the_calo_locator.get_block_position (a_gid, vblock);
//                   }
//                 else if (the_xcalo_locator.is_calo_block (a_gid))
//                   {
//                     the_xcalo_locator.get_block_position (a_gid, vblock);
//                   }
//                 else if (the_gveto_locator.is_calo_block (a_gid))
//                   {
//                     the_gveto_locator.get_block_position (a_gid, vblock);
//                   }
//                 else
//                   {
//                     std::ostringstream message;
//                     message << "snemo::analysis::processing::"
//                             << "nemo3_Y90_study_module::_compute_angular_distribution_: "
//                             << "The calorimeter with geom_id '" << a_gid << "' has not be found !";
//                     throw std::logic_error (message.str ());
//                   }

//                 double angle;
//                 if (geomtools::is_valid (vblock))
//                   {
//                     angle = vorth.angle (vblock - foil);
//                     if (is_debug ())
//                       {
//                         std::clog << datatools::utils::io::debug
//                                   << "snemo::analysis::processing::"
//                                   << "nemo3_Y90_study_module::_compute_angular_distribution_: "
//                                   << "The angle between electron and gamma is "
//                                   << angle / CLHEP::degree << " degree" << std::endl;
//                       }
//                   }
//                 // Filling the histograms :
//                 if (_histogram_pool_->has_1d ("gamma_angle"))
//                   {
//                     mygsl::histogram_1d & h1d = _histogram_pool_->grab_1d ("gamma_angle");
//                     h1d.fill (angle);
//                   }
//               }
//           }

//         return;
//       }

//     } // namespace processing

//   } // namespace analysis

// } // namespace snemo

// // end of nemo3_Y90_study_module.cc
// /*
// ** Local Variables: --
// ** mode: c++ --
// ** c-file-style: "gnu" --
// ** tab-width: 2 --
// ** End: --
// */
