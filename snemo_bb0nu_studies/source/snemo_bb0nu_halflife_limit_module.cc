/* snemo_bb0nu_halflife_limit_module.cc
 *
 * Copyright (C) 2012 Steven Calvez <calvez@lal.in2p3.fr>

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

#include <snanalysis/processing/snemo_bb0nu_halflife_limit_module.h>

#include <datatools/utils/things_macros.h>
#include <datatools/utils/units.h>
#include <datatools/utils/utils.h>
#include <datatools/utils/ioutils.h>
#include <datatools/utils/properties.h>
#include <datatools/services/service_manager.h>

// SuperNEMO event model
#include <sncore/models/data_model.h>
#include <sncore/models/event_header.h>
#include <snanalysis/models/particle_track_data.h>

// Feldman-Cousins calculation
#include <snanalysis/tools/root_tools.h>

// SuperNEMO services
#include <sncore/services/histogram_service.h>

// Histogram
#include <mygsl/histogram_pool.h>

namespace snemo {

  namespace analysis {

    namespace processing {

      // Registration instantiation macro :
      SNEMO_MODULE_REGISTRATION_IMPLEMENT(snemo_bb0nu_halflife_limit_module,
                                          "snemo::analysis::processing::snemo_bb0nu_halflife_limit_module");

      // Default label for the 'event header' event record bank :
      const std::string snemo_bb0nu_halflife_limit_module::DEFAULT_EH_LABEL = "EH";

      // Default label for the 'particle track' event record bank :
      const std::string snemo_bb0nu_halflife_limit_module::DEFAULT_PTD_LABEL = "PTD";

      // Set the label of the 'event header' bank used by the module :
      void snemo_bb0nu_halflife_limit_module::set_EH_bank_label (const std::string & EH_label_)
      {
        _EH_bank_label_ = EH_label_;
        return;
      }

      // Get the label of the 'event header' bank used by the module :
      const std::string & snemo_bb0nu_halflife_limit_module::get_EH_bank_label () const
      {
        return _EH_bank_label_;
      }

      // Set the label of the 'particle track' bank used by the module :
      void snemo_bb0nu_halflife_limit_module::set_PTD_bank_label (const std::string & PTD_label_)
      {
        _PTD_bank_label_ = PTD_label_;
        return;
      }

      // Get the label of the 'particle track' bank used by the module :
      const std::string & snemo_bb0nu_halflife_limit_module::get_PTD_bank_label () const
      {
        return _PTD_bank_label_;
      }

      // Set the label of the 'Histogram' service used by the module :
      void snemo_bb0nu_halflife_limit_module::set_Histo_service_label (const std::string & Histo_label_)
      {
        _Histo_service_label_ = Histo_label_;
        return;
      }

      // Get the label of the 'Histogram' service used by the module :
      const std::string & snemo_bb0nu_halflife_limit_module::get_Histo_service_label () const
      {
        return _Histo_service_label_;
      }

      // Set the histogram pool used by the module :
      void snemo_bb0nu_halflife_limit_module::set_histogram_pool (mygsl::histogram_pool & pool_)
      {
        if (is_initialized ())
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_bb0nu_halflife_limit_module::set_histogram_pool: "
                    << "Module '" << get_name () << "' is already initialized ! ";
            throw std::logic_error (message.str ());
          }
        _histogram_pool_ = &pool_;
        return;
      }

      // Grab the histogram pool used by the module :
      mygsl::histogram_pool & snemo_bb0nu_halflife_limit_module::grab_histogram_pool ()
      {
        if (! is_initialized ())
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_bb0nu_halflife_limit_module::grab_histogram_pool: "
                    << "Module '" << get_name () << "' is not initialized ! ";
            throw std::logic_error (message.str ());
          }
        return *_histogram_pool_;
      }

      void snemo_bb0nu_halflife_limit_module::_init_defaults_ ()
      {
        _EH_bank_label_       = "";
        _PTD_bank_label_      = "";
        _Histo_service_label_ = "";

        _key_fields_.clear ();

        _histogram_pool_ = 0;

        return;
      }

      /*** Implementation of the interface ***/

      // Constructor :
      SNEMO_MODULE_CONSTRUCTOR_IMPLEMENT_HEAD (snemo_bb0nu_halflife_limit_module,
                                               debug_level_,
                                               "snemo_bb0nu_halflife_limit_module",
                                               "A module processor to compute neutrinoless double beta decay lifetime limit",
                                               "0.1")
      {
        _init_defaults_ ();
        return;
      }

      // Destructor :
      SNEMO_MODULE_DESTRUCTOR_IMPLEMENT_HEAD (snemo_bb0nu_halflife_limit_module)
      {
        // Make sure all internal resources are terminated
        // before destruction :
        if (is_initialized ()) reset ();
        return;
      }

      // Initialization :
      SNEMO_MODULE_INITIALIZE_IMPLEMENT_HEAD (snemo_bb0nu_halflife_limit_module,
                                              config_,
                                              service_manager_,
                                              module_dict_)
      {
        if (is_initialized ())
          {
            std::ostringstream message;
            message << "snemo_bb0nu_halflife_limit_module::initialize: "
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

        // Get the keys from 'Event Header' bank
        if (config_.has_key ("key_fields"))
          {
            config_.fetch ("key_fields", _key_fields_);
          }

        // Get experiment conditions
        if (config_.has_key ("experiment.isotope_mass_number"))
          {
            _experiment_conditions_.isotope_mass_number
              = config_.fetch_integer ("experiment.isotope_mass_number");
            _experiment_conditions_.isotope_mass_number *= CLHEP::g/CLHEP::mole;
          }
        if (config_.has_key ("experiment.isotope_mass"))
          {
            const std::string value = config_.fetch_string ("experiment.isotope_mass");
            _experiment_conditions_.isotope_mass
              = datatools::utils::units::get_value_with_unit (value);
          }
        if (config_.has_key ("experiment.isotope_bb2nu_halflife"))
          {
            _experiment_conditions_.isotope_bb2nu_halflife
              = config_.fetch_real ("experiment.isotope_bb2nu_halflife");
          }
        if (config_.has_key ("experiment.exposure_time"))
          {
            _experiment_conditions_.exposure_time
              = config_.fetch_real ("experiment.exposure_time");
          }

        // Check if a special event record bank is available :
        if (_EH_bank_label_.empty ())
          {
            // If the label is not already setup, pickup from the configuration list:
            if (config_.has_key ("EH_label"))
              {
                const std::string label = config_.fetch_string ("EH_label");
                set_EH_bank_label (label);
              }
          }

        if (_PTD_bank_label_.empty ())
          {
            if (config_.has_key ("PTD_label"))
              {
                const std::string label = config_.fetch_string ("PTD_label");
                set_PTD_bank_label (label);
              }
          }

        if (_histogram_pool_ == 0)
          {
            if (_Histo_service_label_.empty ())
              {
                if (config_.has_key ("Histo_label"))
                  {
                    const std::string label = config_.fetch_string ("Histo_label");
                    set_Histo_service_label (label);
                  }
              }
            if (service_manager_.has (_Histo_service_label_) &&
                service_manager_.is_a<snemo::core::service::histogram_service> (_Histo_service_label_))
              {
                snemo::core::service::histogram_service & Histo
                  = service_manager_.get<snemo::core::service::histogram_service> (_Histo_service_label_);
                set_histogram_pool (Histo.grab_pool ());
              }
            else
              {
                std::ostringstream message;
                message << "snemo::analysis::processing::"
                        << "basic_plot_module::initialize: "
                        << "Module '" << get_name ()
                        << "' has no '" << _Histo_service_label_ << "' service !";
                throw std::logic_error (message.str ());
              }
          }

        if (_histogram_pool_ == 0)
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "basic_plot_module::initialize: "
                    << "Missing histogram pool !";
            throw std::logic_error (message.str ());
          }

        /*********************************************
         *   do some check on the setup parameters   *
         *********************************************/

        // What to do if the label of the first used bank is not setup :
        if (_EH_bank_label_.empty ())
          {
            // or use a default (conventionnal) label :
            _EH_bank_label_ = DEFAULT_EH_LABEL;
          }

        if (_PTD_bank_label_.empty ())
          {
            _PTD_bank_label_ = DEFAULT_PTD_LABEL;
          }


        /*************************************
         *  end of the initialization step   *
         *************************************/

        // Tag the module as initialized :
        _set_initialized (true);
        return;
      }

      // Reset :
      SNEMO_MODULE_RESET_IMPLEMENT_HEAD (snemo_bb0nu_halflife_limit_module)
      {
        if (! is_initialized ())
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_bb0nu_halflife_limit_module::initialize: "
                    << "Module '" << get_name () << "' is not initialized !";
            throw std::logic_error (message.str ());
          }

        // Compute efficiency
        _compute_efficiency_ ();

        // Compute neutrinoless halflife limit
        _compute_halflife_ ();

        // Dump result
        _dump_result_ (std::clog,
                       "snemo::analysis::processing::snemo_bb0nu_halflife_limit_module::_dump_result_: ",
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
      SNEMO_MODULE_PROCESS_IMPLEMENT_HEAD (snemo_bb0nu_halflife_limit_module,
                                           event_record_)
      {
        if (! is_initialized ())
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_bb0nu_halflife_limit_module::process: "
                    << "Module '" << get_name () << "' is not initialized !";
            throw std::logic_error (message.str ());
          }

        // Check if the 'event header' record bank is available :
        namespace scm = snemo::core::model;
        if (! DATATOOLS_UTILS_THINGS_CHECK_BANK (event_record_, _EH_bank_label_, scm::event_header))
          {
            std::cerr << datatools::utils::io::error
                      << "snemo::analysis::processing::"
                      << "snemo_bb0nu_halflife_limit_module::process: "
                      << "Could not find any bank with label '"
                      << _EH_bank_label_ << "' !"
                      << std::endl;
            return STOP;
          }
        DATATOOLS_UTILS_THINGS_CONST_BANK (event_record_, _EH_bank_label_, scm::event_header, eh);

        // Check if the 'particle track' record bank is available :
        namespace sam = snemo::analysis::model;
        if (! DATATOOLS_UTILS_THINGS_CHECK_BANK (event_record_, _PTD_bank_label_, sam::particle_track_data))
          {
            std::clog << datatools::utils::io::error
                      << "snemo::analysis::processing::"
                      << "snemo_bb0nu_discrimination_module::process: "
                      << "Could not find any bank with label '"
                      << _PTD_bank_label_ << "' !"
                      << std::endl;
            return STOP;
          }
        DATATOOLS_UTILS_THINGS_CONST_BANK (event_record_, _PTD_bank_label_, sam::particle_track_data, ptd);

        if (is_debug ())
          {
            std::clog << datatools::utils::io::debug
                      << "snemo::analysis::processing::"
                      << "snemo_bb0nu_halflife_limit_module::process: "
                      << "Event header : " << std::endl;
            eh.tree_dump (std::clog, "", "DEBUG: ");
            std::clog << datatools::utils::io::debug
                      << "snemo::analysis::processing::"
                      << "snemo_bb0nu_halflife_limit_module::process: "
                      << "Particle track data : " << std::endl;
            ptd.tree_dump (std::clog, "", "DEBUG: ");
          }

        // Particle Counters
        size_t nelectron  = 0;
        size_t npositron  = 0;
        size_t nundefined = 0;

        // Calibrated energies
        double total_energy = 0.0;

        // Store geom_id to avoid double inclusion of energy deposited
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
                              << "snemo_bb0nu_halflife_limit_module::process: "
                              << "The particle is associated to more than 2 calorimeters !"
                              << std::endl;
                  }
                continue;
              }

            for (size_t i = 0; i < the_calorimeters.size (); ++i)
              {
                const geomtools::geom_id & gid = the_calorimeters.at (i).get ().get_geom_id ();
                if (gids.find (gid) != gids.end ())
                  continue;

                gids.insert (gid);
                total_energy += the_calorimeters.at (i).get ().get_energy ();
              }

            if      (a_particle.get_charge () == sam::particle_track::negative) nelectron++;
            else if (a_particle.get_charge () == sam::particle_track::positive) npositron++;
            else nundefined++;
          }

        // Build unique key for histogram map:
        std::ostringstream key;

        // Retrieving info from header bank:
        const datatools::utils::properties & eh_properties = eh.get_properties ();

        for (std::vector<std::string>::const_iterator ifield = _key_fields_.begin ();
             ifield != _key_fields_.end (); ++ifield)
          {
            const std::string & a_field = *ifield;
            if (!eh_properties.has_key (a_field))
              {
                std::clog << datatools::utils::io::warning
                          << "snemo::analysis::processing::"
                          << "snemo_bb0nu_halflife_limit_module::process: "
                          << "No properties with key '" << a_field << "' "
                          << "has been found in event header !"
                          << std::endl;
                continue;
              }

            if (eh_properties.is_vector (a_field))
              {
                std::clog << datatools::utils::io::warning
                          << "snemo::analysis::processing::"
                          << "snemo_bb0nu_halflife_limit_module::process: "
                          << "Stored properties '" << a_field << "' "
                          << "must be scalar !"
                          << std::endl;
                continue;
              }

            if (eh_properties.is_boolean (a_field))
              {
                key << eh_properties.fetch_boolean (a_field);
              }
            else if (eh_properties.is_integer (a_field))
              {
                key << eh_properties.fetch_integer(a_field);
              }
            else if (eh_properties.is_real (a_field))
              {
                key << eh_properties.fetch_real (a_field);
              }
            else if (eh_properties.is_string (a_field))
              {
                key << eh_properties.fetch_string (a_field);
              }

            // Add a dash separator between field
            key << " - ";
          }

        if (nelectron == 2)                        key << "2 electrons";
        // else if (npositron == 1 && nelectron == 1) key << "1 electron/1 positron";
        // else                                       key << "others";

        // if (nelectron == 2 || (npositron == 1 && nelectron == 1))
        //   {
        //     key << "2 electrons";
        //   }
        // else return STOP;

        // Arbitrary selection of "two-particles" channel
        if (nelectron != 2) return STOP;

        // Getting histogram pool
        mygsl::histogram_pool & a_pool = grab_histogram_pool ();

        const std::string & key_str = key.str ();
        if (!a_pool.has (key_str))
          {
            mygsl::histogram_1d & h = a_pool.add_1d (key_str, "", "energy");
            datatools::utils::properties hconfig;
            hconfig.store_string ("mode", "mimic");
            hconfig.store_string ("mimic.histogram_1d", "energy_template");
            mygsl::histogram_pool::init_histo_1d (h, hconfig, &a_pool);
          }

        // Getting the current histogram
        mygsl::histogram_1d & a_histo = a_pool.grab_1d (key_str);
        a_histo.fill (total_energy);

        // Compute normalization factor given the total number of
        // events generated and the weight of each event
        double weight = 1.0;
        if (eh_properties.has_key ("analysis.total_number_of_event"))
          {
            weight /= eh_properties.fetch_integer ("analysis.total_number_of_event");
          }
        if (eh_properties.has_key (snemo::core::utils::sd_utils::EVENT_GENBB_WEIGHT))
          {
            weight /= 1.0/eh_properties.fetch_real (snemo::core::utils::sd_utils::EVENT_GENBB_WEIGHT);
          }

        // Store the weight (which is fortunately a global properties
        // of histogram and not of bin) into histogram properties
        if (!a_histo.get_auxiliaries ().has_key ("weight"))
          {
            a_histo.grab_auxiliaries ().update ("weight", weight);
          }

        return SUCCESS;
      }

      void snemo_bb0nu_halflife_limit_module::_compute_efficiency_ ()
      {
        // Getting histogram pool
        mygsl::histogram_pool & a_pool = grab_histogram_pool ();

        // Get names of all 1D histograms saved
        std::vector<std::string> hnames;
        a_pool.names (hnames, "group=energy");

        if (hnames.empty ())
          {
            std::clog << datatools::utils::io::warning
                      << "snemo::analysis::processing::"
                      << "snemo_bb0nu_halflife_limit_module::_compute_efficiency_: "
                      << "No energy histograms have been stored !"
                      << std::endl;
            return;
          }

        // Calculate signal to halflife limit constant
        const double exposure_time          = _experiment_conditions_.exposure_time;         // CLHEP::year;
        const double isotope_bb2nu_halflife = _experiment_conditions_.isotope_bb2nu_halflife; // CLHEP::year;
        const double isotope_mass           = _experiment_conditions_.isotope_mass / CLHEP::g;
        const double isotope_molar_mass     = _experiment_conditions_.isotope_mass_number / CLHEP::g*CLHEP::mole;
        const double kbg = log (2) * isotope_mass * CLHEP::Avogadro * exposure_time / isotope_molar_mass / isotope_bb2nu_halflife;

        // Sum of number of events
        std::map<std::string, double> m_event;
        for (std::vector<std::string>::const_iterator
               iname = hnames.begin ();
             iname != hnames.end (); ++iname)
          {
            const std::string & a_name = *iname;
            if (! a_pool.has_1d (a_name))
              {
                std::ostringstream message;
                message << "snemo::analysis::processing::"
                        << "snemo_bb0nu_halflife_limit_module::_compute_efficiency_"
                        << "Histogram '" << a_name << "' is not 1D histogram !";
                throw std::logic_error (message.str ());
              }
            const mygsl::histogram_1d & a_histogram = a_pool.get_1d (a_name);

            // Loop over bin content
            for (size_t i = 0; i < a_histogram.bins (); ++i)
              {
                const double sum   = a_histogram.sum () + a_histogram.overflow ();
                const double value = a_histogram.get (i);

                if (! datatools::utils::is_valid (value))
                  {
                    std::clog << datatools::utils::io::warning
                              << "snemo::analysis::processing::"
                              << "snemo_bb0nu_halflife_limit_module::_compute_efficiency_: "
                              << "Skipping non valid value !"
                              << std::endl;
                    continue;
                  }

                // Retrieve histogram weight
                double weight = 1.0;
                if (a_histogram.get_auxiliaries ().has_key ("weight"))
                  {
                    weight = a_histogram.get_auxiliaries ().fetch_real ("weight");
                  }

                // Compute fraction of event for each histogram bin
                const double efficiency = (sum - m_event[a_name]) * weight;
                m_event[a_name] += value;

                if (! datatools::utils::is_valid (efficiency))
                  {
                    std::clog << datatools::utils::io::warning
                              << "snemo::analysis::processing::"
                              << "snemo_bb0nu_halflife_limit_module::_compute_efficiency_: "
                              << "Skipping non valid efficiency computation !"
                              << std::endl;
                    continue;
                  }

                // Adding histogram efficiency
                const std::string & key_str = a_name + " - efficiency";
                if (!a_pool.has (key_str))
                  {
                    mygsl::histogram_1d & h = a_pool.add_1d (key_str, "", "efficiency");
                    datatools::utils::properties hconfig;
                    hconfig.store_string ("mode", "mimic");
                    hconfig.store_string ("mimic.histogram_1d", "efficiency_template");
                    mygsl::histogram_pool::init_histo_1d (h, hconfig, &a_pool);
                  }

                // Getting the current histogram
                mygsl::histogram_1d & a_new_histogram = a_pool.grab_1d (key_str);
                a_new_histogram.set (i, efficiency);

                // Flag signal/background histogram
                datatools::utils::properties & a_aux = a_new_histogram.grab_auxiliaries ();
                if (a_name.find ("bb0nu") != std::string::npos)
                  {
                    if (! a_aux.has_flag ("__signal")) a_aux.update_flag ("__signal");
                  }
                else
                  {
                    if (! a_aux.has_flag ("__background")) a_aux.update_flag ("__background");
                  }

                // Extra stuff : store a string containing the
                // background counts given the experiment setup (for
                // plotting purpose)
                if (a_name.find ("bb2nu") != std::string::npos)
                  {
                    const std::pair<double, double> x = a_new_histogram.get_range (i);
                    const double nbg_count = kbg * efficiency;

                    if (nbg_count > 10.0 || nbg_count <= 0.0) continue;

                    std::ostringstream text;
                    text << (x.first + x.second)/2.0 << " " << efficiency << " ";
                    if (nbg_count > 1.0) text << std::floor (nbg_count);
                    else                 text << std::setprecision (2) << std::fixed << nbg_count;

                    std::ostringstream label;
                    label << "__display_text_" << i;
                    a_aux.update (label.str (), text.str ());
                  }
              } // end of bin content
          }// end of histogram loop

        return;
      }

      void snemo_bb0nu_halflife_limit_module::_compute_halflife_ ()
      {
        // Getting histogram pool
        mygsl::histogram_pool & a_pool = grab_histogram_pool ();

        // Get names of all 1D histograms saved
        std::vector<std::string> hnames;
        a_pool.names (hnames, "group=efficiency");
        if (hnames.empty ())
          {
            std::clog << datatools::utils::io::warning
                      << "snemo::analysis::processing::"
                      << "snemo_bb0nu_halflife_limit_module::_compute_halflife_: "
                      << "No efficiency histograms have been stored !"
                      << std::endl;
            return;
          }

        // Get names of 'signal' histograms
        std::vector<std::string> signal_names;
        a_pool.names (signal_names, "flag=__signal");
        if (signal_names.empty ())
          {
            std::clog << datatools::utils::io::warning
                      << "snemo::analysis::processing::"
                      << "snemo_bb0nu_halflife_limit_module::_compute_halflife_: "
                      << "No 'signal' histograms have been stored !"
                      << std::endl;
            return;
          }
        // Get names of 'background' histograms
        std::vector<std::string> bkg_names;
        a_pool.names (bkg_names, "flag=__background");
        if (bkg_names.empty ())
          {
            std::clog << datatools::utils::io::warning
                      << "snemo::analysis::processing::"
                      << "snemo_bb0nu_halflife_limit_module::_compute_halflife_: "
                      << "No 'background' histograms have been stored !"
                      << std::endl;
            return;
          }

        // Calculate signal to halflife limit constant
        const double exposure_time          = _experiment_conditions_.exposure_time;         // CLHEP::year;
        const double isotope_bb2nu_halflife = _experiment_conditions_.isotope_bb2nu_halflife; // CLHEP::year;
        const double isotope_mass           = _experiment_conditions_.isotope_mass / CLHEP::g;
        const double isotope_molar_mass     = _experiment_conditions_.isotope_mass_number / CLHEP::g*CLHEP::mole;
        const double kbg = log (2) * isotope_mass * CLHEP::Avogadro * exposure_time / isotope_molar_mass / isotope_bb2nu_halflife;

        // Loop over 'background' histograms
        // Save the total number of events
        std::vector<double> vbkg_counts;
        for (std::vector<std::string>::const_iterator
               iname = bkg_names.begin ();
             iname != bkg_names.end (); ++iname)
          {
            const std::string & a_name = *iname;
            if (! a_pool.has_1d (a_name))
              {
                std::ostringstream message;
                message << "snemo::analysis::processing::"
                        << "snemo_bb0nu_halflife_limit_module::_compute_halflife_"
                        << "Histogram '" << a_name << "' is not 1D histogram !";
                throw std::logic_error (message.str ());
              }
            if (a_pool.get_group (a_name) != "efficiency")
              {
                std::cerr << datatools::utils::io::error
                          << "snemo::analysis::processing::"
                          << "snemo_bb0nu_halflife_limit_module::_compute_halflife_: "
                          << "Histogram '" << a_name << "' does not belongs to 'efficiency' group"
                          << std::endl;
                continue;
              }
            const mygsl::histogram_1d & a_histogram = a_pool.get_1d (a_name);
            // Loop over bin content
            for (size_t i = 0; i < a_histogram.bins (); ++i)
              {
                const double value = a_histogram.get (i);
                if (iname == bkg_names.begin ()) vbkg_counts.push_back (value);
                else                             vbkg_counts[i] += value;
              }
          }

        // Loop over 'signal' histograms
        for (std::vector<std::string>::const_iterator
               iname = signal_names.begin ();
             iname != signal_names.end (); ++iname)
          {
            double best_halflife_limit = 0.0;
            const std::string & a_name = *iname;
            if (! a_pool.has_1d (a_name))
              {
                std::ostringstream message;
                message << "snemo::analysis::processing::"
                        << "snemo_bb0nu_halflife_limit_module::_compute_halflife_"
                        << "Histogram '" << a_name << "' is not 1D histogram !";
                throw std::logic_error (message.str ());
              }
            if (a_pool.get_group (a_name) != "efficiency")
              {
                std::cerr << datatools::utils::io::error
                          << "snemo::analysis::processing::"
                          << "snemo_bb0nu_halflife_limit_module::_compute_halflife_: "
                          << "Histogram '" << a_name << "' does not belongs to 'efficiency' group"
                          << std::endl;
                continue;
              }
            const mygsl::histogram_1d & a_histogram = a_pool.get_1d (a_name);
            // Loop over bin content
            for (size_t i = 0; i < a_histogram.bins (); ++i)
              {
                const double value = a_histogram.get (i);

                // Compute the number of event excluded for the same energy bin
                const double nbkg = kbg * vbkg_counts.at (i);
                const double nexcluded = snemo::analysis::tool::get_number_of_excluded_events (nbkg);
                const double halflife = value / nexcluded * kbg * isotope_bb2nu_halflife;

                // Keeping larger limit
                best_halflife_limit = std::max (best_halflife_limit, halflife);

                // Adding histogram halflife
                const std::string & key_str = a_name + " - halflife";
                if (!a_pool.has (key_str))
                  {
                    mygsl::histogram_1d & h = a_pool.add_1d (key_str, "", "halflife");
                    datatools::utils::properties hconfig;
                    hconfig.store_string ("mode", "mimic");
                    hconfig.store_string ("mimic.histogram_1d", "halflife_template");
                    mygsl::histogram_pool::init_histo_1d (h, hconfig, &a_pool);
                  }

                // Getting the current histogram
                mygsl::histogram_1d & a_new_histogram = a_pool.grab_1d (key_str);
                a_new_histogram.set (i, halflife);
              }
            std::clog << datatools::utils::io::notice
                      << "snemo::analysis::processing::"
                      << "snemo_bb0nu_halflife_limit_module::_compute_halflife_: "
                      << "Best halflife limit for bb0nu process is " << best_halflife_limit
                      << " yr" << std::endl;

          }
      }

      void snemo_bb0nu_halflife_limit_module::_dump_result_ (std::ostream      & out_,
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
          // Experiment setup:
          out_ << indent << du::i_tree_dumpable::tag
               << "Experimental setup : " << std::endl;
          out_ << indent << du::i_tree_dumpable::skip_tag << du::i_tree_dumpable::tag
               << "Isotope mass number : "
               << _experiment_conditions_.isotope_mass_number / CLHEP::g*CLHEP::mole
               << std::endl;
          out_ << indent << du::i_tree_dumpable::skip_tag << du::i_tree_dumpable::tag
               << "Isotope total mass : " << _experiment_conditions_.isotope_mass / CLHEP::kg
               << " kg" << std::endl;
          out_ << indent << du::i_tree_dumpable::skip_tag << du::i_tree_dumpable::tag
               << "Isotope bb2nu halflife : " << _experiment_conditions_.isotope_bb2nu_halflife
               << " yr" << std::endl;
          out_ << indent << du::i_tree_dumpable::skip_tag << du::i_tree_dumpable::last_tag
               << "Exposure time : " << _experiment_conditions_.exposure_time
               << " yr" << std::endl;
        }

        {
          // Histogram :
          out_ << indent << du::i_tree_dumpable::tag
               << "Particle energy histograms : ";
          if (_histogram_pool_->empty ())
            out_ << "<empty>";
          else
            out_ << _histogram_pool_->size ();
          out_ << std::endl;;

          std::vector<std::string> hnames;
          _histogram_pool_->names (hnames);
          for (std::vector<std::string>::const_iterator i = hnames.begin ();
               i != hnames.end (); ++i)
            {
              const std::string & a_name = *i;
              if (a_name.find ("template") != std::string::npos) continue;

              std::vector<std::string>::const_iterator j = i;
              out_ << indent;
              std::ostringstream indent_oss;
              if (++j == hnames.end ())
                {
                  out_  << du::i_tree_dumpable::last_tag;
                  indent_oss << indent << du::i_tree_dumpable::last_skip_tag;
                }
              else
                {
                  out_ << du::i_tree_dumpable::tag;
                  indent_oss << indent << du::i_tree_dumpable::skip_tag;
                }

              out_ << "Label " << a_name << std::endl;
              const mygsl::histogram_1d & a_histogram = _histogram_pool_->get_1d (a_name);
              a_histogram.tree_dump (out_, "", indent_oss.str (), inherit_);

              if (is_debug ())
                {
                  a_histogram.print (std::clog);
                }
            }
        }

        return;
      }

    } // namespace processing

  } // namespace analysis

} // namespace snemo

// end of snemo_bb0nu_halflife_limit_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
