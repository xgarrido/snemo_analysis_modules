// snemo_bb0nu_halflife_limit_module.cc

#include <stdexcept>
#include <sstream>
#include <set>

#include <snemo_bb0nu_halflife_limit_module.h>

// Utilities
#include <datatools/clhep_units.h>

// SuperNEMO event model
#include <sncore/models/data_model.h>
#include <sncore/models/event_header.h>
#include <snanalysis/models/data_model.h>
#include <snanalysis/models/particle_track_data.h>

// Feldman-Cousins calculation
//#include <snanalysis/tools/root_tools.h>

// Service manager
#include <datatools/service_manager.h>

// Services
#include <dpp/histogram_service.h>

// Histogram
#include <mygsl/histogram_pool.h>

namespace analysis {

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_bb0nu_halflife_limit_module,
                                    "analysis::snemo_bb0nu_halflife_limit_module");

  // Set the histogram pool used by the module :
  void snemo_bb0nu_halflife_limit_module::set_histogram_pool (mygsl::histogram_pool & pool_)
  {
    DT_THROW_IF (is_initialized (), std::logic_error,
                 "Module '" << get_name () << "' is already initialized !");
    _histogram_pool_ = &pool_;
    return;
  }

  // Grab the histogram pool used by the module :
  mygsl::histogram_pool & snemo_bb0nu_halflife_limit_module::grab_histogram_pool ()
  {
    DT_THROW_IF (! is_initialized (), std::logic_error,
                 "Module '" << get_name () << "' is not initialized !");
    return *_histogram_pool_;
  }

  void snemo_bb0nu_halflife_limit_module::_set_defaults ()
  {
    _key_fields_.clear ();

    _histogram_pool_ = 0;
    return;
  }

  // Initialization :
  DPP_MODULE_INITIALIZE_IMPLEMENT_HEAD (snemo_bb0nu_halflife_limit_module,
                                        config_,
                                        service_manager_,
                                        module_dict_)
  {
    DT_THROW_IF (is_initialized (),
                 std::logic_error,
                 "Module '" << get_name () << "' is already initialized ! ");

    dpp::base_module::_common_initialize (config_);

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
        _experiment_conditions_.isotope_mass = config_.fetch_real ("experiment.isotope_mass");
        if (! config_.has_explicit_unit ("experiment.isotope_mass")) {
          _experiment_conditions_.isotope_mass *= CLHEP::kg;
        }
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
  DPP_MODULE_RESET_IMPLEMENT_HEAD (snemo_bb0nu_halflife_limit_module)
  {
    DT_THROW_IF(! is_initialized (),
                std::logic_error,
                "Module '" << get_name () << "' is not initialized !");

    // Compute efficiency
    _compute_efficiency_ ();

    // Compute neutrinoless halflife limit
    _compute_halflife_ ();

    // Dump result
    dump_result (std::clog,
                 "snemo::analysis::processing::snemo_bb0nu_halflife_limit_module::_dump_result_: ",
                 "NOTICE: ");

    // Tag the module as un-initialized :
    _set_initialized (false);
    _set_defaults ();
    return;
  }

  // Constructor :
  DPP_MODULE_CONSTRUCTOR_IMPLEMENT_HEAD (snemo_bb0nu_halflife_limit_module, logging_priority)
  {
    _set_defaults ();
    return;
  }

  // Destructor :
  DPP_MODULE_DEFAULT_DESTRUCTOR_IMPLEMENT (snemo_bb0nu_halflife_limit_module);

  // Processing :
  DPP_MODULE_PROCESS_IMPLEMENT_HEAD (snemo_bb0nu_halflife_limit_module, data_record_)
  {
    DT_LOG_TRACE (get_logging_priority (), "Entering...");
    DT_THROW_IF (! is_initialized (), std::logic_error,
                 "Module '" << get_name () << "' is not initialized !");

    // Check if the 'event header' record bank is available :
    const std::string eh_label = snemo::core::model::data_info::EVENT_HEADER_LABEL;
    namespace scm = snemo::core::model;
    if (! data_record_.has (eh_label))
      {
        DT_LOG_ERROR (get_logging_priority (), "Could not find any bank with label '"
                      << eh_label << "' !");
        return dpp::PROCESS_STOP;
      }
    const snemo::core::model::event_header & eh
      = data_record_.get<snemo::core::model::event_header>(eh_label);

    // Check if the 'particle track' record bank is available :
    const std::string ptd_label = snemo::analysis::model::data_info::PARTICLE_TRACK_DATA_LABEL;
    if (! data_record_.has (ptd_label))
      {
        DT_LOG_ERROR (get_logging_priority (), "Could not find any bank with label '"
                      << ptd_label << "' !");
        return dpp::PROCESS_STOP;
      }
    const snemo::analysis::model::particle_track_data & ptd
      = data_record_.get<snemo::analysis::model::particle_track_data>(ptd_label);

    // if (is_debug ())
    //   {
    //     std::clog << datatools::utils::io::debug
    //               << "snemo::analysis::processing::"
    //               << "snemo_bb0nu_halflife_limit_module::process: "
    //               << "Event header : " << std::endl;
    //     eh.tree_dump (std::clog, "", "DEBUG: ");
    //     std::clog << datatools::utils::io::debug
    //               << "snemo::analysis::processing::"
    //               << "snemo_bb0nu_halflife_limit_module::process: "
    //               << "Particle track data : " << std::endl;
    //     ptd.tree_dump (std::clog, "", "DEBUG: ");
    //   }

    // // Particle Counters
    // size_t nelectron  = 0;
    // size_t npositron  = 0;
    // size_t nundefined = 0;

    // // Calibrated energies
    // double total_energy = 0.0;

    // // Store geom_id to avoid double inclusion of energy deposited
    // std::set<geomtools::geom_id> gids;

    // // Loop over all saved particles
    // const sam::particle_track_data::particle_collection_type & the_particles
    //   = ptd.get_particles ();

    // for (sam::particle_track_data::particle_collection_type::const_iterator
    //        iparticle = the_particles.begin ();
    //      iparticle != the_particles.end ();
    //      ++iparticle)
    //   {
    //     const sam::particle_track & a_particle = iparticle->get ();

    //     if (!a_particle.has_associated_calorimeters ()) continue;

    //     const sam::particle_track::calorimeter_collection_type & the_calorimeters
    //       = a_particle.get_associated_calorimeters ();

    //     if (the_calorimeters.size () > 2)
    //       {
    //         if (is_debug ())
    //           {
    //             std::clog << datatools::utils::io::debug
    //                       << "snemo::analysis::processing::"
    //                       << "snemo_bb0nu_halflife_limit_module::process: "
    //                       << "The particle is associated to more than 2 calorimeters !"
    //                       << std::endl;
    //           }
    //         continue;
    //       }

    //     for (size_t i = 0; i < the_calorimeters.size (); ++i)
    //       {
    //         const geomtools::geom_id & gid = the_calorimeters.at (i).get ().get_geom_id ();
    //         if (gids.find (gid) != gids.end ())
    //           continue;

    //         gids.insert (gid);
    //         total_energy += the_calorimeters.at (i).get ().get_energy ();
    //       }

    //     if      (a_particle.get_charge () == sam::particle_track::negative) nelectron++;
    //     else if (a_particle.get_charge () == sam::particle_track::positive) npositron++;
    //     else nundefined++;
    //   }

    // // Build unique key for histogram map:
    // std::ostringstream key;

    // // Retrieving info from header bank:
    // const datatools::utils::properties & eh_properties = eh.get_properties ();

    // for (std::vector<std::string>::const_iterator ifield = _key_fields_.begin ();
    //      ifield != _key_fields_.end (); ++ifield)
    //   {
    //     const std::string & a_field = *ifield;
    //     if (!eh_properties.has_key (a_field))
    //       {
    //         std::clog << datatools::utils::io::warning
    //                   << "snemo::analysis::processing::"
    //                   << "snemo_bb0nu_halflife_limit_module::process: "
    //                   << "No properties with key '" << a_field << "' "
    //                   << "has been found in event header !"
    //                   << std::endl;
    //         continue;
    //       }

    //     if (eh_properties.is_vector (a_field))
    //       {
    //         std::clog << datatools::utils::io::warning
    //                   << "snemo::analysis::processing::"
    //                   << "snemo_bb0nu_halflife_limit_module::process: "
    //                   << "Stored properties '" << a_field << "' "
    //                   << "must be scalar !"
    //                   << std::endl;
    //         continue;
    //       }

    //     if (eh_properties.is_boolean (a_field))
    //       {
    //         key << eh_properties.fetch_boolean (a_field);
    //       }
    //     else if (eh_properties.is_integer (a_field))
    //       {
    //         key << eh_properties.fetch_integer(a_field);
    //       }
    //     else if (eh_properties.is_real (a_field))
    //       {
    //         key << eh_properties.fetch_real (a_field);
    //       }
    //     else if (eh_properties.is_string (a_field))
    //       {
    //         key << eh_properties.fetch_string (a_field);
    //       }

    //     // Add a dash separator between field
    //     key << " - ";
    //   }

    // if (nelectron == 2)                        key << "2 electrons";
    // // else if (npositron == 1 && nelectron == 1) key << "1 electron/1 positron";
    // // else                                       key << "others";

    // // if (nelectron == 2 || (npositron == 1 && nelectron == 1))
    // //   {
    // //     key << "2 electrons";
    // //   }
    // // else return STOP;

    // // Arbitrary selection of "two-particles" channel
    // if (nelectron != 2) return STOP;

    // // Getting histogram pool
    // mygsl::histogram_pool & a_pool = grab_histogram_pool ();

    // const std::string & key_str = key.str ();
    // if (!a_pool.has (key_str))
    //   {
    //     mygsl::histogram_1d & h = a_pool.add_1d (key_str, "", "energy");
    //     datatools::utils::properties hconfig;
    //     hconfig.store_string ("mode", "mimic");
    //     hconfig.store_string ("mimic.histogram_1d", "energy_template");
    //     mygsl::histogram_pool::init_histo_1d (h, hconfig, &a_pool);
    //   }

    // // Getting the current histogram
    // mygsl::histogram_1d & a_histo = a_pool.grab_1d (key_str);
    // a_histo.fill (total_energy);

    // // Compute normalization factor given the total number of
    // // events generated and the weight of each event
    // double weight = 1.0;
    // if (eh_properties.has_key ("analysis.total_number_of_event"))
    //   {
    //     weight /= eh_properties.fetch_integer ("analysis.total_number_of_event");
    //   }
    // if (eh_properties.has_key (snemo::core::utils::sd_utils::EVENT_GENBB_WEIGHT))
    //   {
    //     weight /= 1.0/eh_properties.fetch_real (snemo::core::utils::sd_utils::EVENT_GENBB_WEIGHT);
    //   }

    // // Store the weight (which is fortunately a global properties
    // // of histogram and not of bin) into histogram properties
    // if (!a_histo.get_auxiliaries ().has_key ("weight"))
    //   {
    //     a_histo.grab_auxiliaries ().update ("weight", weight);
    //   }

    return dpp::PROCESS_SUCCESS;
  }

  void snemo_bb0nu_halflife_limit_module::_compute_efficiency_ (){}
  // {
  //   // Getting histogram pool
  //   mygsl::histogram_pool & a_pool = grab_histogram_pool ();

  //   // Get names of all 1D histograms saved
  //   std::vector<std::string> hnames;
  //   a_pool.names (hnames, "group=energy");

  //   if (hnames.empty ())
  //     {
  //       std::clog << datatools::utils::io::warning
  //                 << "snemo::analysis::processing::"
  //                 << "snemo_bb0nu_halflife_limit_module::_compute_efficiency_: "
  //                 << "No energy histograms have been stored !"
  //                 << std::endl;
  //       return;
  //     }

  //   // Calculate signal to halflife limit constant
  //   const double exposure_time          = _experiment_conditions_.exposure_time;         // CLHEP::year;
  //   const double isotope_bb2nu_halflife = _experiment_conditions_.isotope_bb2nu_halflife; // CLHEP::year;
  //   const double isotope_mass           = _experiment_conditions_.isotope_mass / CLHEP::g;
  //   const double isotope_molar_mass     = _experiment_conditions_.isotope_mass_number / CLHEP::g*CLHEP::mole;
  //   const double kbg = log (2) * isotope_mass * CLHEP::Avogadro * exposure_time / isotope_molar_mass / isotope_bb2nu_halflife;

  //   // Sum of number of events
  //   std::map<std::string, double> m_event;
  //   for (std::vector<std::string>::const_iterator
  //          iname = hnames.begin ();
  //        iname != hnames.end (); ++iname)
  //     {
  //       const std::string & a_name = *iname;
  //       if (! a_pool.has_1d (a_name))
  //         {
  //           std::ostringstream message;
  //           message << "snemo::analysis::processing::"
  //                   << "snemo_bb0nu_halflife_limit_module::_compute_efficiency_"
  //                   << "Histogram '" << a_name << "' is not 1D histogram !";
  //           throw std::logic_error (message.str ());
  //         }
  //       const mygsl::histogram_1d & a_histogram = a_pool.get_1d (a_name);

  //       // Loop over bin content
  //       for (size_t i = 0; i < a_histogram.bins (); ++i)
  //         {
  //           const double sum   = a_histogram.sum () + a_histogram.overflow ();
  //           const double value = a_histogram.get (i);

  //           if (! datatools::utils::is_valid (value))
  //             {
  //               std::clog << datatools::utils::io::warning
  //                         << "snemo::analysis::processing::"
  //                         << "snemo_bb0nu_halflife_limit_module::_compute_efficiency_: "
  //                         << "Skipping non valid value !"
  //                         << std::endl;
  //               continue;
  //             }

  //           // Retrieve histogram weight
  //           double weight = 1.0;
  //           if (a_histogram.get_auxiliaries ().has_key ("weight"))
  //             {
  //               weight = a_histogram.get_auxiliaries ().fetch_real ("weight");
  //             }

  //           // Compute fraction of event for each histogram bin
  //           const double efficiency = (sum - m_event[a_name]) * weight;
  //           m_event[a_name] += value;

  //           if (! datatools::utils::is_valid (efficiency))
  //             {
  //               std::clog << datatools::utils::io::warning
  //                         << "snemo::analysis::processing::"
  //                         << "snemo_bb0nu_halflife_limit_module::_compute_efficiency_: "
  //                         << "Skipping non valid efficiency computation !"
  //                         << std::endl;
  //               continue;
  //             }

  //           // Adding histogram efficiency
  //           const std::string & key_str = a_name + " - efficiency";
  //           if (!a_pool.has (key_str))
  //             {
  //               mygsl::histogram_1d & h = a_pool.add_1d (key_str, "", "efficiency");
  //               datatools::utils::properties hconfig;
  //               hconfig.store_string ("mode", "mimic");
  //               hconfig.store_string ("mimic.histogram_1d", "efficiency_template");
  //               mygsl::histogram_pool::init_histo_1d (h, hconfig, &a_pool);
  //             }

  //           // Getting the current histogram
  //           mygsl::histogram_1d & a_new_histogram = a_pool.grab_1d (key_str);
  //           a_new_histogram.set (i, efficiency);

  //           // Flag signal/background histogram
  //           datatools::utils::properties & a_aux = a_new_histogram.grab_auxiliaries ();
  //           if (a_name.find ("bb0nu") != std::string::npos)
  //             {
  //               if (! a_aux.has_flag ("__signal")) a_aux.update_flag ("__signal");
  //             }
  //           else
  //             {
  //               if (! a_aux.has_flag ("__background")) a_aux.update_flag ("__background");
  //             }

  //           // Extra stuff : store a string containing the
  //           // background counts given the experiment setup (for
  //           // plotting purpose)
  //           if (a_name.find ("bb2nu") != std::string::npos)
  //             {
  //               const std::pair<double, double> x = a_new_histogram.get_range (i);
  //               const double nbg_count = kbg * efficiency;

  //               if (nbg_count > 10.0 || nbg_count <= 0.0) continue;

  //               std::ostringstream text;
  //               text << (x.first + x.second)/2.0 << " " << efficiency << " ";
  //               if (nbg_count > 1.0) text << std::floor (nbg_count);
  //               else                 text << std::setprecision (2) << std::fixed << nbg_count;

  //               std::ostringstream label;
  //               label << "__display_text_" << i;
  //               a_aux.update (label.str (), text.str ());
  //             }
  //         } // end of bin content
  //     }// end of histogram loop

  //   return;
  // }

  void snemo_bb0nu_halflife_limit_module::_compute_halflife_ (){}
  // {
  //   // Getting histogram pool
  //   mygsl::histogram_pool & a_pool = grab_histogram_pool ();

  //   // Get names of all 1D histograms saved
  //   std::vector<std::string> hnames;
  //   a_pool.names (hnames, "group=efficiency");
  //   if (hnames.empty ())
  //     {
  //       std::clog << datatools::utils::io::warning
  //                 << "snemo::analysis::processing::"
  //                 << "snemo_bb0nu_halflife_limit_module::_compute_halflife_: "
  //                 << "No efficiency histograms have been stored !"
  //                 << std::endl;
  //       return;
  //     }

  //   // Get names of 'signal' histograms
  //   std::vector<std::string> signal_names;
  //   a_pool.names (signal_names, "flag=__signal");
  //   if (signal_names.empty ())
  //     {
  //       std::clog << datatools::utils::io::warning
  //                 << "snemo::analysis::processing::"
  //                 << "snemo_bb0nu_halflife_limit_module::_compute_halflife_: "
  //                 << "No 'signal' histograms have been stored !"
  //                 << std::endl;
  //       return;
  //     }
  //   // Get names of 'background' histograms
  //   std::vector<std::string> bkg_names;
  //   a_pool.names (bkg_names, "flag=__background");
  //   if (bkg_names.empty ())
  //     {
  //       std::clog << datatools::utils::io::warning
  //                 << "snemo::analysis::processing::"
  //                 << "snemo_bb0nu_halflife_limit_module::_compute_halflife_: "
  //                 << "No 'background' histograms have been stored !"
  //                 << std::endl;
  //       return;
  //     }

  //   // Calculate signal to halflife limit constant
  //   const double exposure_time          = _experiment_conditions_.exposure_time;         // CLHEP::year;
  //   const double isotope_bb2nu_halflife = _experiment_conditions_.isotope_bb2nu_halflife; // CLHEP::year;
  //   const double isotope_mass           = _experiment_conditions_.isotope_mass / CLHEP::g;
  //   const double isotope_molar_mass     = _experiment_conditions_.isotope_mass_number / CLHEP::g*CLHEP::mole;
  //   const double kbg = log (2) * isotope_mass * CLHEP::Avogadro * exposure_time / isotope_molar_mass / isotope_bb2nu_halflife;

  //   // Loop over 'background' histograms
  //   // Save the total number of events
  //   std::vector<double> vbkg_counts;
  //   for (std::vector<std::string>::const_iterator
  //          iname = bkg_names.begin ();
  //        iname != bkg_names.end (); ++iname)
  //     {
  //       const std::string & a_name = *iname;
  //       if (! a_pool.has_1d (a_name))
  //         {
  //           std::ostringstream message;
  //           message << "snemo::analysis::processing::"
  //                   << "snemo_bb0nu_halflife_limit_module::_compute_halflife_"
  //                   << "Histogram '" << a_name << "' is not 1D histogram !";
  //           throw std::logic_error (message.str ());
  //         }
  //       if (a_pool.get_group (a_name) != "efficiency")
  //         {
  //           std::cerr << datatools::utils::io::error
  //                     << "snemo::analysis::processing::"
  //                     << "snemo_bb0nu_halflife_limit_module::_compute_halflife_: "
  //                     << "Histogram '" << a_name << "' does not belongs to 'efficiency' group"
  //                     << std::endl;
  //           continue;
  //         }
  //       const mygsl::histogram_1d & a_histogram = a_pool.get_1d (a_name);
  //       // Loop over bin content
  //       for (size_t i = 0; i < a_histogram.bins (); ++i)
  //         {
  //           const double value = a_histogram.get (i);
  //           if (iname == bkg_names.begin ()) vbkg_counts.push_back (value);
  //           else                             vbkg_counts[i] += value;
  //         }
  //     }

  //   // Loop over 'signal' histograms
  //   for (std::vector<std::string>::const_iterator
  //          iname = signal_names.begin ();
  //        iname != signal_names.end (); ++iname)
  //     {
  //       double best_halflife_limit = 0.0;
  //       const std::string & a_name = *iname;
  //       if (! a_pool.has_1d (a_name))
  //         {
  //           std::ostringstream message;
  //           message << "snemo::analysis::processing::"
  //                   << "snemo_bb0nu_halflife_limit_module::_compute_halflife_"
  //                   << "Histogram '" << a_name << "' is not 1D histogram !";
  //           throw std::logic_error (message.str ());
  //         }
  //       if (a_pool.get_group (a_name) != "efficiency")
  //         {
  //           std::cerr << datatools::utils::io::error
  //                     << "snemo::analysis::processing::"
  //                     << "snemo_bb0nu_halflife_limit_module::_compute_halflife_: "
  //                     << "Histogram '" << a_name << "' does not belongs to 'efficiency' group"
  //                     << std::endl;
  //           continue;
  //         }
  //       const mygsl::histogram_1d & a_histogram = a_pool.get_1d (a_name);
  //       // Loop over bin content
  //       for (size_t i = 0; i < a_histogram.bins (); ++i)
  //         {
  //           const double value = a_histogram.get (i);

  //           // Compute the number of event excluded for the same energy bin
  //           const double nbkg = kbg * vbkg_counts.at (i);
  //           const double nexcluded = snemo::analysis::tool::get_number_of_excluded_events (nbkg);
  //           const double halflife = value / nexcluded * kbg * isotope_bb2nu_halflife;

  //           // Keeping larger limit
  //           best_halflife_limit = std::max (best_halflife_limit, halflife);

  //           // Adding histogram halflife
  //           const std::string & key_str = a_name + " - halflife";
  //           if (!a_pool.has (key_str))
  //             {
  //               mygsl::histogram_1d & h = a_pool.add_1d (key_str, "", "halflife");
  //               datatools::utils::properties hconfig;
  //               hconfig.store_string ("mode", "mimic");
  //               hconfig.store_string ("mimic.histogram_1d", "halflife_template");
  //               mygsl::histogram_pool::init_histo_1d (h, hconfig, &a_pool);
  //             }

  //           // Getting the current histogram
  //           mygsl::histogram_1d & a_new_histogram = a_pool.grab_1d (key_str);
  //           a_new_histogram.set (i, halflife);
  //         }
  //       std::clog << datatools::utils::io::notice
  //                 << "snemo::analysis::processing::"
  //                 << "snemo_bb0nu_halflife_limit_module::_compute_halflife_: "
  //                 << "Best halflife limit for bb0nu process is " << best_halflife_limit
  //                 << " yr" << std::endl;

  //     }
  // }

  void snemo_bb0nu_halflife_limit_module::dump_result (std::ostream      & out_,
                                                       const std::string & title_,
                                                       const std::string & indent_,
                                                       bool inherit_) const {}
  // {
  //   std::string indent;
  //   if (! indent_.empty ())
  //     {
  //       indent = indent_;
  //     }
  //   if ( !title_.empty () )
  //     {
  //       out_ << indent << title_ << std::endl;
  //     }
  //   namespace du = datatools::utils;

  //   {
  //     // Experiment setup:
  //     out_ << indent << du::i_tree_dumpable::tag
  //          << "Experimental setup : " << std::endl;
  //     out_ << indent << du::i_tree_dumpable::skip_tag << du::i_tree_dumpable::tag
  //          << "Isotope mass number : "
  //          << _experiment_conditions_.isotope_mass_number / CLHEP::g*CLHEP::mole
  //          << std::endl;
  //     out_ << indent << du::i_tree_dumpable::skip_tag << du::i_tree_dumpable::tag
  //          << "Isotope total mass : " << _experiment_conditions_.isotope_mass / CLHEP::kg
  //          << " kg" << std::endl;
  //     out_ << indent << du::i_tree_dumpable::skip_tag << du::i_tree_dumpable::tag
  //          << "Isotope bb2nu halflife : " << _experiment_conditions_.isotope_bb2nu_halflife
  //          << " yr" << std::endl;
  //     out_ << indent << du::i_tree_dumpable::skip_tag << du::i_tree_dumpable::last_tag
  //          << "Exposure time : " << _experiment_conditions_.exposure_time
  //          << " yr" << std::endl;
  //   }

  //   {
  //     // Histogram :
  //     out_ << indent << du::i_tree_dumpable::tag
  //          << "Particle energy histograms : ";
  //     if (_histogram_pool_->empty ())
  //       out_ << "<empty>";
  //     else
  //       out_ << _histogram_pool_->size ();
  //     out_ << std::endl;;

  //     std::vector<std::string> hnames;
  //     _histogram_pool_->names (hnames);
  //     for (std::vector<std::string>::const_iterator i = hnames.begin ();
  //          i != hnames.end (); ++i)
  //       {
  //         const std::string & a_name = *i;
  //         if (a_name.find ("template") != std::string::npos) continue;

  //         std::vector<std::string>::const_iterator j = i;
  //         out_ << indent;
  //         std::ostringstream indent_oss;
  //         if (++j == hnames.end ())
  //           {
  //             out_  << du::i_tree_dumpable::last_tag;
  //             indent_oss << indent << du::i_tree_dumpable::last_skip_tag;
  //           }
  //         else
  //           {
  //             out_ << du::i_tree_dumpable::tag;
  //             indent_oss << indent << du::i_tree_dumpable::skip_tag;
  //           }

  //         out_ << "Label " << a_name << std::endl;
  //         const mygsl::histogram_1d & a_histogram = _histogram_pool_->get_1d (a_name);
  //         a_histogram.tree_dump (out_, "", indent_oss.str (), inherit_);

  //         if (is_debug ())
  //           {
  //             a_histogram.print (std::clog);
  //           }
  //       }
  //   }

  //   return;
  // }

} // namespace analysis

// end of snemo_bb0nu_halflife_limit_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
