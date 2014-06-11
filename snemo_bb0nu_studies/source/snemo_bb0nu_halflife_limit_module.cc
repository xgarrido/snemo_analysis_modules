// snemo_bb0nu_halflife_limit_module.cc

#include <stdexcept>
#include <sstream>
#include <set>

#include <snemo_bb0nu_halflife_limit_module.h>

// Utilities
#include <datatools/clhep_units.h>

// SuperNEMO event model
#include <mctools/utils.h>
#include <snemo/datamodels/data_model.h>
#include <snemo/datamodels/event_header.h>
#include <snemo/datamodels/particle_track_data.h>

// Service manager
#include <datatools/service_manager.h>

// Services
#include <dpp/histogram_service.h>

// Histogram
#include <mygsl/histogram_pool.h>

namespace analysis {

  // Character separator between key for histogram dict.
  const char KEY_FIELD_SEPARATOR = '_';

  // Continuous FeldmanCousin functions computed by M. Bongrand
  // <bongrand@lal.in2p3.fr>
  double get_number_of_excluded_events (const double number_of_events_)
  {
    double number_of_excluded_events = 0.0;

    if ( number_of_events_ < 29.0 )
      {
        double x = number_of_events_;

        number_of_excluded_events =
          2.5617 + 0.747661 * x - 0.0666176 * pow (x,2)
          + 0.00432457 * pow (x,3) - 0.000139343 * pow (x,4)
          + 1.71509e-06 * pow (x,5);
      }
    else
      {
        number_of_excluded_events = 1.64 * sqrt (number_of_events_);
      }

    return number_of_excluded_events;
  }


  void snemo_bb0nu_halflife_limit_module::experiment_entry_type::initialize(const datatools::properties & config_)
  {
    // Get experimental conditions
    if (config_.has_key("isotope_mass_number"))
      {
        isotope_mass_number = config_.fetch_integer("isotope_mass_number");
        isotope_mass_number *= CLHEP::g/CLHEP::mole;
      }
    if (config_.has_key("isotope_mass"))
      {
        isotope_mass = config_.fetch_real("isotope_mass");
        if (! config_.has_explicit_unit("isotope_mass")) {
          isotope_mass *= CLHEP::kg;
        }
      }
    if (config_.has_key("isotope_bb2nu_halflife"))
      {
        isotope_bb2nu_halflife = config_.fetch_real("isotope_bb2nu_halflife");
      }
    if (config_.has_key("exposure_time"))
      {
        exposure_time = config_.fetch_real("exposure_time");
      }
    if (config_.has_key("background_list"))
      {
        std::vector<std::string> bkgs;
        config_.fetch("background_list", bkgs);
        for (std::vector<std::string>::const_iterator ibkg = bkgs.begin();
             ibkg != bkgs.end(); ++ibkg)
          {
            const std::string & bkgname = *ibkg;
            const std::string key = bkgname + ".activity";
            DT_THROW_IF(! config_.has_key(key), std::logic_error,
                        "No background activity for " << bkgname << " element");
            background_activities[bkgname] = config_.fetch_real(key);
            if (! config_.has_explicit_unit(key))
              {
                background_activities[bkgname] *= CLHEP::becquerel/CLHEP::kg;
              }
            DT_LOG_NOTICE(datatools::logger::PRIO_NOTICE,
                          "Adding '" << bkgname << "' background with an activity of "
                          << background_activities[bkgname]/CLHEP::becquerel*CLHEP::kg << " Bq/kg");
          }
      }
    return;
  }

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_bb0nu_halflife_limit_module,
                                    "analysis::snemo_bb0nu_halflife_limit_module");

  // Setting signal flag name for histogram properties
  const std::string & snemo_bb0nu_halflife_limit_module::signal_flag()
  {
    static const std::string flag("__signal");
    return flag;
  }

  // Setting background flag name for histogram properties
  const std::string & snemo_bb0nu_halflife_limit_module::background_flag()
  {
    static const std::string flag("__background");
    return flag;
  }

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
  void snemo_bb0nu_halflife_limit_module::initialize(const datatools::properties  & config_,
                                                     datatools::service_manager   & service_manager_,
                                                     dpp::module_handle_dict_type & module_dict_)
  {
    DT_THROW_IF (is_initialized (),
                 std::logic_error,
                 "Module '" << get_name () << "' is already initialized ! ");

    dpp::base_module::_common_initialize (config_);

    // Get the experimental conditions
    datatools::properties exp_config;
    config_.export_and_rename_starting_with(exp_config, "experiment.", "");
    _experiment_conditions_.initialize(exp_config);

    // Get the keys from 'Event Header' bank
    if (config_.has_key ("key_fields"))
      {
        config_.fetch ("key_fields", _key_fields_);
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
        if (config_.has_key ("Histo_output_files"))
          {
            std::vector<std::string> output_files;
            config_.fetch ("Histo_output_files", output_files);
            for (size_t i = 0; i < output_files.size(); i++) {
              Histo.add_output_file(output_files[i]);
            }
          }
        if (config_.has_key ("Histo_input_files"))
          {
            std::vector<std::string> input_files;
            config_.fetch ("Histo_input_files", input_files);
            for (size_t i = 0; i < input_files.size(); i++) {
              Histo.load_from_boost_file(input_files[i]);
            }
          }
        if (config_.has_key ("Histo_template_files"))
          {
            std::vector<std::string> template_files;
            config_.fetch ("Histo_template_files", template_files);
            for (size_t i = 0; i < template_files.size(); i++) {
              Histo.grab_pool().load(template_files[i]);
            }
          }
      }

    // Tag the module as initialized :
    _set_initialized (true);
    return;
  }

  // Reset :
  void snemo_bb0nu_halflife_limit_module::reset()
  {
    DT_THROW_IF(! is_initialized (),
                std::logic_error,
                "Module '" << get_name () << "' is not initialized !");

    // Compute efficiency
    _compute_efficiency_();

    // Compute neutrinoless halflife limit
    _compute_halflife_();

    // Dump result
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG)
      {
        DT_LOG_NOTICE(get_logging_priority (), "Halflife limit module dump: ");
        dump_result();
      }

    // Tag the module as un-initialized :
    _set_initialized(false);
    _set_defaults();
    return;
  }

  // Constructor :
  snemo_bb0nu_halflife_limit_module::snemo_bb0nu_halflife_limit_module(datatools::logger::priority logging_priority_)
    : dpp::base_module(logging_priority_)
  {
    _set_defaults ();
    return;
  }

  // Destructor :
  snemo_bb0nu_halflife_limit_module::~snemo_bb0nu_halflife_limit_module()
  {
    if (is_initialized()) snemo_bb0nu_halflife_limit_module::reset();
    return;
  }

  // Processing :
  dpp::base_module::process_status snemo_bb0nu_halflife_limit_module::process(datatools::things & data_record_)
  {
    DT_THROW_IF (! is_initialized (), std::logic_error,
                 "Module '" << get_name () << "' is not initialized !");

    // Check if the 'event header' record bank is available :
    const std::string eh_label = snemo::datamodel::data_info::default_event_header_label();
    if (! data_record_.has (eh_label))
      {
        DT_LOG_ERROR (get_logging_priority (), "Could not find any bank with label '"
                      << eh_label << "' !");
        return dpp::base_module::PROCESS_STOP;
      }
    const snemo::datamodel::event_header & eh
      = data_record_.get<snemo::datamodel::event_header>(eh_label);

    // Check if the 'particle track' record bank is available :
    const std::string ptd_label = snemo::datamodel::data_info::default_particle_track_data_label();
    if (! data_record_.has (ptd_label))
      {
        DT_LOG_ERROR (get_logging_priority (), "Could not find any bank with label '"
                      << ptd_label << "' !");
        return dpp::base_module::PROCESS_STOP;
      }
    const snemo::datamodel::particle_track_data & ptd
      = data_record_.get<snemo::datamodel::particle_track_data>(ptd_label);

    if (get_logging_priority () >= datatools::logger::PRIO_DEBUG)
      {
        DT_LOG_DEBUG (get_logging_priority (), "Event header : ");
        eh.tree_dump ();
        DT_LOG_DEBUG (get_logging_priority (), "Particle track data : ");
        ptd.tree_dump ();
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
    const snemo::datamodel::particle_track_data::particle_collection_type &
      the_particles = ptd.get_particles ();

    for (snemo::datamodel::particle_track_data::particle_collection_type::const_iterator
           iparticle = the_particles.begin ();
         iparticle != the_particles.end ();
         ++iparticle)
      {
        const snemo::datamodel::particle_track & a_particle = iparticle->get ();

        if (! a_particle.has_associated_calorimeters ())
          {
            DT_LOG_DEBUG(get_logging_priority(),
                         "Particle track is not associated to any calorimeter block !");
            continue;
          }

        const snemo::datamodel::calibrated_calorimeter_hit::collection_type &
          the_calorimeters = a_particle.get_associated_calorimeters ();

        if (the_calorimeters.size () > 2)
          {
            DT_LOG_DEBUG (get_logging_priority (),
                          "The particle is associated to more than 2 calorimeters !");
            continue;
          }

        for (size_t i = 0; i < the_calorimeters.size (); ++i)
          {
            const geomtools::geom_id & gid = the_calorimeters.at (i).get ().get_geom_id ();
            if (gids.find (gid) != gids.end ()) continue;
            gids.insert (gid);
            total_energy += the_calorimeters.at (i).get ().get_energy ();
          }

        if      (a_particle.get_charge () == snemo::datamodel::particle_track::negative) nelectron++;
        else if (a_particle.get_charge () == snemo::datamodel::particle_track::positive) npositron++;
        else nundefined++;
      }

    // Build unique key for histogram map:
    std::ostringstream key;

    // Retrieving info from header bank:
    const datatools::properties & eh_properties = eh.get_properties ();
    for (std::vector<std::string>::const_iterator
           ifield = _key_fields_.begin ();
         ifield != _key_fields_.end (); ++ifield)
      {
        const std::string & a_field = *ifield;
        if (! eh_properties.has_key (a_field))
          {
            DT_LOG_WARNING (get_logging_priority (),
                            "No properties with key '" << a_field << "' "
                            << "has been found in event header !");
            continue;
          }

        if (eh_properties.is_vector (a_field))
          {
            DT_LOG_WARNING (get_logging_priority (),
                            "Stored properties '" << a_field << "' " << "must be scalar !");
            continue;
          }
        if (eh_properties.is_boolean (a_field))      key << eh_properties.fetch_boolean (a_field);
        else if (eh_properties.is_integer (a_field)) key << eh_properties.fetch_integer (a_field);
        else if (eh_properties.is_real (a_field))    key << eh_properties.fetch_real (a_field);
        else if (eh_properties.is_string (a_field))  key << eh_properties.fetch_string (a_field);

        // Add a underscore separator between fields
        key << KEY_FIELD_SEPARATOR;
      }
    // Add charge multiplicity
    key << nelectron << "e-" << npositron << "e+" << nundefined << "u";

    DT_LOG_TRACE(get_logging_priority(), "Total energy = " << total_energy / CLHEP::keV << " keV");
    DT_LOG_TRACE(get_logging_priority(), "Number of electrons = " << nelectron);
    DT_LOG_TRACE(get_logging_priority(), "Number of positrons = " << npositron);
    DT_LOG_TRACE(get_logging_priority(), "Number of undefined = " << nundefined);
    DT_LOG_TRACE(get_logging_priority(), "Key = " << key.str());

    // Arbitrary selection of "two-particles" channel
    if (nelectron != 2)
      {
        DT_LOG_WARNING (get_logging_priority (), "Selecting only two-electrons events!");
        return dpp::base_module::PROCESS_CONTINUE;
      }

    // Getting histogram pool
    mygsl::histogram_pool & a_pool = grab_histogram_pool ();

    if (! a_pool.has (key.str ()))
      {
        mygsl::histogram_1d & h = a_pool.add_1d (key.str (), "", "energy");
        datatools::properties hconfig;
        hconfig.store_string ("mode", "mimic");
        hconfig.store_string ("mimic.histogram_1d", "energy_template");
        mygsl::histogram_pool::init_histo_1d (h, hconfig, &a_pool);
      }

    // Getting the current histogram
    mygsl::histogram_1d & a_histo = a_pool.grab_1d (key.str ());
    a_histo.fill (total_energy);

    // Compute normalization factor given the total number of events generated
    // and the weight of each event
    double weight = 1.0;
    if (eh_properties.has_key ("analysis.total_number_of_event"))
      {
        weight /= eh_properties.fetch_integer ("analysis.total_number_of_event");
      }
    if (eh_properties.has_key (mctools::event_utils::EVENT_GENBB_WEIGHT))
      {
        weight *= eh_properties.fetch_real (mctools::event_utils::EVENT_GENBB_WEIGHT);
      }

    // Store the weight into histogram properties
    if (! a_histo.get_auxiliaries ().has_key ("weight"))
      {
        a_histo.grab_auxiliaries ().update ("weight", weight);
      }

    return dpp::base_module::PROCESS_SUCCESS;
  }

  void snemo_bb0nu_halflife_limit_module::_compute_efficiency_ ()
  {
    // Getting histogram pool
    mygsl::histogram_pool & a_pool = grab_histogram_pool ();

    // Get names of all saved 1D histograms belonging to 'energy' group
    std::vector<std::string> hnames;
    a_pool.names (hnames, "group=energy");

    if (hnames.empty ())
      {
        DT_LOG_WARNING (get_logging_priority (), "No 'energy' histograms have been stored !");
        return;
      }

    // Sum of number of events
    std::map<std::string, double> m_event;
    for (std::vector<std::string>::const_iterator iname = hnames.begin ();
         iname != hnames.end (); ++iname)
      {
        const std::string & a_name = *iname;
        DT_THROW_IF (! a_pool.has_1d (a_name), std::logic_error,
                     "Histogram '" << a_name << "' is not 1D histogram !");
        const mygsl::histogram_1d & a_histogram = a_pool.get_1d (a_name);

        // Loop over bin content
        for (size_t i = 0; i < a_histogram.bins (); ++i)
          {
            const double sum   = a_histogram.sum () + a_histogram.overflow ();
            const double value = a_histogram.get (i);

            if (! datatools::is_valid (value))
              {
                DT_LOG_WARNING (get_logging_priority (), "Skipping non valid value !");
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

            if (! datatools::is_valid (efficiency))
              {
                DT_LOG_WARNING (get_logging_priority (), "Skipping non valid efficiency computation !");
                continue;
              }

            // Adding histogram efficiency
            const std::string & key_str = a_name + KEY_FIELD_SEPARATOR + "efficiency";
            if (! a_pool.has (key_str))
              {
                mygsl::histogram_1d & h = a_pool.add_1d (key_str, "", "efficiency");
                datatools::properties hconfig;
                hconfig.store_string ("mode", "mimic");
                hconfig.store_string ("mimic.histogram_1d", "efficiency_template");
                mygsl::histogram_pool::init_histo_1d (h, hconfig, &a_pool);
              }

            // Getting & updating the current histogram
            mygsl::histogram_1d & a_new_histogram = a_pool.grab_1d (key_str);
            a_new_histogram.set (i, efficiency);

            // Flag signal/background histogram
            datatools::properties & a_aux = a_new_histogram.grab_auxiliaries ();
            if (a_name.find ("0nubb") != std::string::npos)
              {
                a_aux.update_flag (snemo_bb0nu_halflife_limit_module::signal_flag());
              }
            else
              {
                a_aux.update_flag (snemo_bb0nu_halflife_limit_module::background_flag());
              }
          } // end of bin content
      }// end of histogram loop

    return;
  }

  void snemo_bb0nu_halflife_limit_module::_compute_halflife_ ()
  {
    // Get SuperNEMO experiment setup
    // Calculate signal to halflife limit constant
    const double exposure_time          = _experiment_conditions_.exposure_time;          // year;
    const double isotope_bb2nu_halflife = _experiment_conditions_.isotope_bb2nu_halflife; // year;
    const double isotope_mass           = _experiment_conditions_.isotope_mass / CLHEP::g;
    const double isotope_molar_mass     = _experiment_conditions_.isotope_mass_number / CLHEP::g*CLHEP::mole;
    const double kbg
      = log (2) * isotope_mass * CLHEP::Avogadro * exposure_time
      / isotope_molar_mass / isotope_bb2nu_halflife;

    // Getting histogram pool
    mygsl::histogram_pool & a_pool = grab_histogram_pool ();

    // Get names of all saved 1D histograms belonging to 'efficiency' group
    std::vector<std::string> hnames;
    a_pool.names (hnames, "group=efficiency");
    if (hnames.empty ())
      {
        DT_LOG_WARNING (get_logging_priority (), "No 'efficiency' histograms have been stored !");
        return;
      }

    // Get names of 'background' histograms
    std::vector<std::string> bkg_names;
    a_pool.names (bkg_names, "flag=" + snemo_bb0nu_halflife_limit_module::background_flag());
    if (bkg_names.empty ())
      {
        DT_LOG_WARNING (get_logging_priority (), "No 'background' histograms have been stored !");
        return;
      }

    // Loop over 'background' histograms
    std::vector<double> vbkg_counts;
    for (std::vector<std::string>::const_iterator iname = bkg_names.begin ();
         iname != bkg_names.end (); ++iname)
      {
        const std::string & a_name = *iname;
        DT_THROW_IF (! a_pool.has_1d (a_name), std::logic_error,
                     "Histogram '" << a_name << "' is not 1D histogram !");
        if (a_pool.get_group (a_name) != "efficiency")
          {
            DT_LOG_ERROR (get_logging_priority (),
                          "Histogram '" << a_name << "' does not belong to 'efficiency' group !");
            continue;
          }
        // Get normalization factor
        double norm_factor;
        datatools::invalidate(norm_factor);
        if (a_name.find("2nubb") != std::string::npos)
          {
            norm_factor = kbg;
          }
        else
          {
            const experiment_entry_type::background_dict_type & bkgs = _experiment_conditions_.background_activities;
            for (experiment_entry_type::background_dict_type::const_iterator
                   ibkg = bkgs.begin();
                 ibkg != bkgs.end(); ++ibkg)
              {
                if (a_name.find(ibkg->first) != std::string::npos)
                  {
                    DT_LOG_TRACE(get_logging_priority(),
                                 "Found background element '" << ibkg->first << "'");
                    const double year2sec = 3600 * 24 * 365;
                    norm_factor = ibkg->second * exposure_time * year2sec * isotope_mass;
                  }
              }
          }
        DT_LOG_TRACE(datatools::logger::PRIO_TRACE, "norm_factor = " << norm_factor);
        if (! datatools::is_valid(norm_factor)) {
          DT_LOG_WARNING(get_logging_priority(),
                         "No background activity has been found ! Skip histogram '" << a_name << "'");
          continue;
        }

        const mygsl::histogram_1d & a_histogram = a_pool.get_1d (a_name);
        for (size_t i = 0; i < a_histogram.bins (); ++i)
          {
            const double value = a_histogram.get (i) * norm_factor;
            if (iname == bkg_names.begin()) vbkg_counts.push_back (value);
            else                            vbkg_counts[i] += value;
          }
      }// end of background loop

    // Get names of 'signal' histograms
    std::vector<std::string> signal_names;
    a_pool.names (signal_names, "flag=" + snemo_bb0nu_halflife_limit_module::signal_flag());
    if (signal_names.empty ())
      {
        DT_LOG_WARNING (get_logging_priority (), "No 'signal' histograms have been stored !");
        return;
      }
    // Loop over 'signal' histograms
    for (std::vector<std::string>::const_iterator iname = signal_names.begin ();
         iname != signal_names.end (); ++iname)
      {
        double best_halflife_limit = 0.0;
        const std::string & a_name = *iname;
        DT_THROW_IF(! a_pool.has_1d (a_name), std::logic_error,
                    "Histogram '" << a_name << "' is not 1D histogram !");
        if (a_pool.get_group (a_name) != "efficiency")
          {
            DT_LOG_ERROR (get_logging_priority (),
                          "Histogram '" << a_name << "' does not belongs to 'efficiency' group !");
            continue;
          }
        const mygsl::histogram_1d & a_histogram = a_pool.get_1d (a_name);
        // Loop over bin content
        for (size_t i = 0; i < a_histogram.bins (); ++i)
          {
            const double value = a_histogram.get (i);

            // Compute the number of event excluded for the same energy bin
            const double nbkg = vbkg_counts.at (i);
            const double nexcluded = analysis::get_number_of_excluded_events (nbkg);
            const double halflife = value / nexcluded * kbg * isotope_bb2nu_halflife;

            // Keeping larger limit
            best_halflife_limit = std::max (best_halflife_limit, halflife);

            // Adding histogram halflife
            const std::string & key_str = a_name + KEY_FIELD_SEPARATOR + "halflife";
            if (!a_pool.has (key_str))
              {
                mygsl::histogram_1d & h = a_pool.add_1d (key_str, "", "halflife");
                datatools::properties hconfig;
                hconfig.store_string ("mode", "mimic");
                hconfig.store_string ("mimic.histogram_1d", "halflife_template");
                mygsl::histogram_pool::init_histo_1d (h, hconfig, &a_pool);
              }

            // Getting the current histogram
            mygsl::histogram_1d & a_new_histogram = a_pool.grab_1d (key_str);
            a_new_histogram.set (i, halflife);
          }
        DT_LOG_NOTICE (get_logging_priority (),
                       "Best halflife limit for bb0nu process is " << best_halflife_limit << " yr");
      }// end of signal loop
  }

  void snemo_bb0nu_halflife_limit_module::dump_result (std::ostream      & out_,
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

    {
      // Experiment setup:
      out_ << indent << datatools::i_tree_dumpable::tag
           << "Experimental setup : " << std::endl;
      out_ << indent << datatools::i_tree_dumpable::skip_tag << datatools::i_tree_dumpable::tag
           << "Isotope mass number : "
           << _experiment_conditions_.isotope_mass_number / CLHEP::g*CLHEP::mole
           << std::endl;
      out_ << indent << datatools::i_tree_dumpable::skip_tag << datatools::i_tree_dumpable::tag
           << "Isotope total mass : " << _experiment_conditions_.isotope_mass / CLHEP::kg
           << " kg" << std::endl;
      out_ << indent << datatools::i_tree_dumpable::skip_tag << datatools::i_tree_dumpable::tag
           << "Isotope bb2nu halflife : " << _experiment_conditions_.isotope_bb2nu_halflife
           << " yr" << std::endl;
      out_ << indent << datatools::i_tree_dumpable::skip_tag << datatools::i_tree_dumpable::tag
           << "Exposure time : " << _experiment_conditions_.exposure_time
           << " yr" << std::endl;
      out_ << indent << datatools::i_tree_dumpable::skip_tag << datatools::i_tree_dumpable::last_tag
           << "Backgrounds : " << _experiment_conditions_.background_activities.size() << std::endl;
    }

    {
      // Histogram :
      out_ << indent << datatools::i_tree_dumpable::tag
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
              out_  << datatools::i_tree_dumpable::last_tag;
              indent_oss << indent << datatools::i_tree_dumpable::last_skip_tag;
            }
          else
            {
              out_ << datatools::i_tree_dumpable::tag;
              indent_oss << indent << datatools::i_tree_dumpable::skip_tag;
            }

          out_ << "Label " << a_name << std::endl;
          const mygsl::histogram_1d & a_histogram = _histogram_pool_->get_1d (a_name);
          a_histogram.tree_dump (out_, "", indent_oss.str (), inherit_);

          if (get_logging_priority () >= datatools::logger::PRIO_DEBUG)
            {
              DT_LOG_DEBUG (get_logging_priority (), "Histogram " << a_name << " dump:");
              a_histogram.print (std::clog);
            }
        }
    }

    return;
  }

} // namespace analysis

// end of snemo_bb0nu_halflife_limit_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
