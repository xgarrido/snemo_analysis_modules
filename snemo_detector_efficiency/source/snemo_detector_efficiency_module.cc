/* snemo_detector_efficiency_module.cc
 *
 * Copyright (C) 2013 Xavier Garrido <garrido@lal.in2p3.fr>

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include <stdexcept>
#include <sstream>
#include <set>
#include <numeric>

#include <boost/bind.hpp>

#include <snanalysis/processing/snemo_detector_efficiency_module.h>

#include <datatools/utils/things_macros.h>
#include <datatools/utils/units.h>
#include <datatools/utils/utils.h>
#include <datatools/utils/ioutils.h>
#include <datatools/utils/properties.h>

// Third party:
// - Bayeux/datatools:
#include <datatools/service_manager.h>
// - Bayeux/geomtools:
#include <geomtools/geometry_service.h>
#include <geomtools/manager.h>



// SuperNEMO event model
#include <falaise/snemo/datamodels/data_model.h>
#include <falaise/snemo/datamodels/simulated_data.h>
#include <falaise/snemo/datamodels/calibrated_data.h>
#include <famaise/snemo/dataodels/tracker_trajectory.h>
#include <snanalysis/models/data_model.h>
#include <snanalysis/models/particle_track_data.h>

// Geometry manager
#include <datatools/services/service_manager.h>
#include <sncore/services/geometry_service.h>
#include <sngeometry/manager.h>
#include <sngeometry/snemo/calo_locator.h>
#include <sngeometry/snemo/xcalo_locator.h>
#include <sngeometry/snemo/gveto_locator.h>
#include <sngeometry/snemo/gg_locator.h>

namespace snemo {

  namespace analysis {

    namespace processing {

      // Registration instantiation macro :
      SNEMO_MODULE_REGISTRATION_IMPLEMENT(snemo_detector_efficiency_module,
                                          "snemo::analysis::processing::snemo_detector_efficiency_module");

      void snemo_detector_efficiency_module::set_geom_manager (const snemo::geometry::manager & gmgr_)
      {
        if (is_initialized ())
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_detector_efficiency_module::set_geom_manager: "
                    << "Module '" << get_name () << "' is already initialized ! ";
            throw std::logic_error (message.str ());
          }
        _geom_manager_ = &gmgr_;

        // Check setup label:
        const std::string & setup_label = _geom_manager_->get_setup_label ();
        if (!(setup_label == "snemo"))
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_bb0nu_caloeimeter_efficiency_module::set_geom_manager: "
                    << "Setup label '" << setup_label << "' is not supported !";
            throw std::logic_error (message.str ());
          }
        return;
      }

      const snemo::geometry::manager & snemo_detector_efficiency_module::get_geom_manager () const
      {
        if (! has_geom_manager ())
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_detector_efficiency_module::get_geom_manager: "
                    << "No geometry manager is setup !";
            throw std::logic_error (message.str ());
          }
        return *_geom_manager_;
      }

      bool snemo_detector_efficiency_module::has_geom_manager () const
      {
        return _geom_manager_ != 0;
      }

      void snemo_detector_efficiency_module::_init_defaults_ ()
      {
        _bank_label_ = "";

        _Geo_service_label_ = "";
        _geom_manager_  = 0;

        _calo_locator_.reset ();
        _xcalo_locator_.reset ();
        _gveto_locator_.reset ();

        _calo_efficiencies_.clear ();
        _gg_efficiencies_.clear ();

        return;
      }

      /*** Implementation of the interface ***/

      // Constructor :
      SNEMO_MODULE_CONSTRUCTOR_IMPLEMENT_HEAD (snemo_detector_efficiency_module,
                                               debug_level_,
                                               "snemo_detector_efficiency_module",
                                               "A module processor to compute the calorimeter wall efficiency",
                                               "0.1")
      {
        _init_defaults_ ();
        return;
      }

      // Destructor :
      SNEMO_MODULE_DESTRUCTOR_IMPLEMENT_HEAD (snemo_detector_efficiency_module)
      {
        // Make sure all internal resources are terminated
        // before destruction :
        if (is_initialized ()) reset ();
        return;
      }

      // Initialization :
      SNEMO_MODULE_INITIALIZE_IMPLEMENT_HEAD (snemo_detector_efficiency_module,
                                              config_,
                                              service_manager_,
                                              module_dict_)
      {
        if (is_initialized ())
          {
            std::ostringstream message;
            message << "snemo_detector_efficiency_module::initialize: "
                    << "Module '" << get_name () << "' is already initialized ! ";
            throw std::logic_error (message.str ());
          }

        /**************************************************************
         *   fetch setup parameters from the configuration container  *
         **************************************************************/

        if (! is_debug ())
          {
            if (config_.has_flag ("debug"))
              {
                set_debug (true);
              }
          }

        if (_bank_label_.empty ())
          {
            if (config_.has_key ("bank_label"))
              {
                _bank_label_ = config_.fetch_string ("bank_label");
              }
          }

        if (config_.has_key ("Geo_label"))
          {
            _Geo_service_label_ = config_.fetch_string ("Geo_label");
          }

        if (_Geo_service_label_.empty ())
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "basic_particle_tracking_module::initialize: "
                    << "Module '" << get_name () << "' has no valid label for the 'Geo' service !";
            throw std::logic_error (message.str ());
          }

        if (! service_manager_.has (_Geo_service_label_))
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "basic_particle_tracking_module::initialize: "
                    << "Module '" << get_name () << "' cannot access service with label '"
                    << _Geo_service_label_  << "' !";
            throw std::logic_error (message.str ());
          }

        if (! service_manager_.is_a<snemo::core::service::geometry_service> (_Geo_service_label_))
          {
            std::ostringstream message;
            message << "snemo:analysis::processing::"
                    << "basic_particle_tracking_module::initialize: "
                    << "Module '" << get_name ()
                    << "' cannot access the '" << _Geo_service_label_
                    << "' service with the proper type !";
            throw std::logic_error (message.str ());
          }
        snemo::core::service::geometry_service & Geo
          = service_manager_.get<snemo::core::service::geometry_service> (_Geo_service_label_);
        set_geom_manager (Geo.get_geom_manager ());

        // Initialize locators
        namespace sgs = snemo::geometry::snemo;
        const int mn = 0;
        _calo_locator_.reset  (new sgs::calo_locator  (get_geom_manager (), mn));
        _xcalo_locator_.reset (new sgs::xcalo_locator (get_geom_manager (), mn));
        _gveto_locator_.reset (new sgs::gveto_locator (get_geom_manager (), mn));
        _gg_locator_.reset    (new sgs::gg_locator    (get_geom_manager (), mn));

        /*************************************
         *  end of the initialization step   *
         *************************************/

        // Tag the module as initialized :
        _set_initialized (true);
        return;
      }

      // Reset :
      SNEMO_MODULE_RESET_IMPLEMENT_HEAD (snemo_detector_efficiency_module)
      {
        if (! is_initialized ())
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_detector_efficiency_module::initialize: "
                    << "Module '" << get_name () << "' is not initialized !";
            throw std::logic_error (message.str ());
          }

        // Compute efficiency
        _compute_efficiency_ ();

        // Dump result
        _dump_result_ (std::clog,
                       "snemo::analysis::processing::snemo_detector_efficiency_module::_dump_result_: ",
                       "NOTICE: ");

        /****************************
         *  revert to some defaults *
         ****************************/

        this->_init_defaults_ ();

        /****************************
         *  end of the reset step   *
         ****************************/

        // Tag the module as un-initialized :
        _set_initialized (false);
        return;
      }

      // Processing :
      SNEMO_MODULE_PROCESS_IMPLEMENT_HEAD (snemo_detector_efficiency_module,
                                           event_record_)
      {
        if (! is_initialized ())
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_detector_efficiency_module::process: "
                    << "Module '" << get_name () << "' is not initialized !";
            throw std::logic_error (message.str ());
          }

        if (_bank_label_ == snemo::core::model::data_info::CALIBRATED_DATA_LABEL)
          {
            // Check if the 'calibrated data' record bank is available :
            namespace scm = snemo::core::model;
            if (! DATATOOLS_UTILS_THINGS_CHECK_BANK (event_record_, _bank_label_, scm::calibrated_data))
              {
                std::clog << datatools::utils::io::error
                          << "snemo::analysis::processing::"
                          << "snemo_detector_efficiency_module::process: "
                          << "Could not find any bank with label '"
                          << _bank_label_ << "' !"
                          << std::endl;
                return STOP;
              }
            DATATOOLS_UTILS_THINGS_CONST_BANK (event_record_, _bank_label_, scm::calibrated_data, cd);

            const scm::calibrated_data::calorimeter_hit_collection_type & the_calo_hits
              = cd.calibrated_calorimeter_hits ();
            const scm::calibrated_data::tracker_hit_collection_type & the_tracker_hits
              = cd.calibrated_tracker_hits ();

            for (scm::calibrated_data::calorimeter_hit_collection_type::const_iterator
                   i = the_calo_hits.begin ();
                 i != the_calo_hits.end (); ++i)
              {
                const scm::calibrated_calorimeter_hit & a_hit = i->get ();
                _calo_efficiencies_[a_hit.get_geom_id ()]++;
              }
            for (scm::calibrated_data::tracker_hit_collection_type::const_iterator
                   i = the_tracker_hits.begin ();
                 i != the_tracker_hits.end (); ++i)
              {
                const scm::calibrated_tracker_hit & a_hit = i->get ();
                _gg_efficiencies_[a_hit.get_geom_id ()]++;
              }
          }
        else if (_bank_label_ == snemo::analysis::model::data_info::PARTICLE_TRACK_DATA_LABEL)
          {
            // Check if the 'particle track' record bank is available :
            namespace scm = snemo::core::model;
            namespace sam = snemo::analysis::model;
            if (! DATATOOLS_UTILS_THINGS_CHECK_BANK (event_record_, _bank_label_, sam::particle_track_data))
              {
                std::clog << datatools::utils::io::error
                          << "snemo::analysis::processing::"
                          << "snemo_detector_efficiency_module::process: "
                          << "Could not find any bank with label '"
                          << _bank_label_ << "' !"
                          << std::endl;
                return STOP;
              }
            DATATOOLS_UTILS_THINGS_CONST_BANK (event_record_, _bank_label_, sam::particle_track_data, ptd);

            // Store geom_id to avoid double inclusion of calorimeter hits
            std::set<geomtools::geom_id> gids;

            // Loop over all saved particles
            const sam::particle_track_data::particle_collection_type & the_particles
              = ptd.get_particles ();

            for (sam::particle_track_data::particle_collection_type::const_iterator
                   iparticle = the_particles.begin ();
                 iparticle != the_particles.end ();
                 ++iparticle)
              {
                const sam::particle_track & a_particle = iparticle->get ();

                if (!a_particle.has_associated_calorimeters ()) continue;

                const sam::particle_track::calorimeter_collection_type & the_calorimeters
                  = a_particle.get_associated_calorimeters ();

                if (the_calorimeters.size () > 2)
                  {
                    if (is_debug ())
                      {
                        std::clog << datatools::utils::io::debug
                                  << "snemo::analysis::processing::"
                                  << "snemo_detector_efficiency_module::process: "
                                  << "The particle is associated to more than 2 calorimeters !"
                                  << std::endl;
                      }
                    continue;
                  }

                for (size_t i = 0; i < the_calorimeters.size (); ++i)
                  {
                    const geomtools::geom_id & a_gid = the_calorimeters.at (i).get ().get_geom_id ();
                    if (gids.find (a_gid) != gids.end ())
                      continue;

                    gids.insert (a_gid);
                    _calo_efficiencies_[a_gid]++;
                  }

                // Get trajectory and attached geiger cells
                const scm::tracker_trajectory & a_trajectory = a_particle.get_trajectory ();
                const scm::tracker_cluster & a_cluster = a_trajectory.get_cluster ();
                const scm::calibrated_tracker_hit::collection_type & the_hits = a_cluster.get_hits ();
                for (size_t i = 0; i < the_hits.size (); ++i)
                  {
                    const geomtools::geom_id & a_gid = the_hits.at (i).get ().get_geom_id ();
                    if (gids.find (a_gid) != gids.end ())
                      continue;

                    gids.insert (a_gid);
                    _gg_efficiencies_[a_gid]++;
                  }
              }
          }
        else
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_detector_efficiency_module::process: "
                    << "Bank label '" << _bank_label_ << "' is not supported !";
            throw std::logic_error (message.str ());
          }
        return SUCCESS;
      }

      void snemo_detector_efficiency_module::_compute_efficiency_ ()
      {
        // Handling geom_id is done in this place where geom_id are split into
        // main wall, xwall and gveto calorimeters. For such task we use
        // sngeometry locators

        std::ofstream fout ("/tmp/efficiency.dat");
        {
          efficiency_dict::const_iterator found =
            std::max_element (_calo_efficiencies_.begin (), _calo_efficiencies_.end (),
                              (boost::bind(&efficiency_dict::value_type::second, _1) <
                               boost::bind(&efficiency_dict::value_type::second, _2)));
          const int calo_total = found->second;

          for (efficiency_dict::const_iterator i = _calo_efficiencies_.begin ();
               i != _calo_efficiencies_.end (); ++i)
            {
              const geomtools::geom_id & a_gid = i->first;

              if (_calo_locator_->is_calo_block_in_current_module (a_gid))
                {
                  fout << "calo ";
                  geomtools::vector_3d position;
                  _calo_locator_->get_block_position (a_gid, position);
                  fout << position.x () << " "
                       << position.y () << " "
                       << position.z () << " ";
                }
              else if (_xcalo_locator_->is_calo_block_in_current_module (a_gid))
                {
                  fout << "xcalo ";
                  geomtools::vector_3d position;
                  _xcalo_locator_->get_block_position (a_gid, position);
                  fout << position.x () << " "
                       << position.y () << " "
                       << position.z () << " ";
                }
              else if (_gveto_locator_->is_calo_block_in_current_module (a_gid))
                {
                  fout << "gveto ";
                  geomtools::vector_3d position;
                  _gveto_locator_->get_block_position (a_gid, position);
                  fout << position.x () << " "
                       << position.y () << " "
                       << position.z () << " ";
                  }
                fout << i->second/double (calo_total) << std::endl;
            }
        }

        {
          efficiency_dict::const_iterator found =
            std::max_element (_gg_efficiencies_.begin (), _gg_efficiencies_.end (),
                              (boost::bind(&efficiency_dict::value_type::second, _1) <
                               boost::bind(&efficiency_dict::value_type::second, _2)));
          const int gg_total = found->second;

          for (efficiency_dict::const_iterator i = _gg_efficiencies_.begin ();
               i != _gg_efficiencies_.end (); ++i)
            {
              const geomtools::geom_id & a_gid = i->first;

              if (_gg_locator_->is_drift_cell_volume_in_current_module (a_gid))
                {
                  fout << "gg ";
                  geomtools::vector_3d position;
                  _gg_locator_->get_cell_position (a_gid, position);
                  fout << position.x () << " " << position.y () << " ";
                }
              fout << i->second/double (gg_total) << std::endl;
            }
        }

        return;
      }

      void snemo_detector_efficiency_module::_dump_result_ (std::ostream      & out_,
                                                                     const std::string & title_,
                                                                     const std::string & indent_,
                                                                     bool inherit_) const
      {
        std::string indent;
        if (! indent_.empty ())
          {
            indent = indent_;
          }
        if ( !title_.empty () )
          {
            out_ << indent << title_ << std::endl;
          }
        namespace du = datatools::utils;

        {
          for (efficiency_dict::const_iterator i = _calo_efficiencies_.begin ();
               i != _calo_efficiencies_.end (); ++i)
            {
              out_ << indent << du::i_tree_dumpable::tag
                   << i->first << " = " << i->second << std::endl;
            }
        }

        return;
      }

    } // namespace processing

  } // namespace analysis

} // namespace snemo

// end of snemo_detector_efficiency_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
