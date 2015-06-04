// snemo_bb0nu_halflife_limit_module.cc

// Ourselves:
#include <snemo_bb0nu_halflife_limit_module.h>

// Standard library:
#include <stdexcept>
#include <sstream>
#include <set>

// Third party:
// - Bayeux/datatools:
#include <datatools/clhep_units.h>
#include <datatools/units.h>
#include <datatools/service_manager.h>
// - Bayeux/mygsl
#include <mygsl/histogram_pool.h>
// - Bayeux/mtools
#include <mctools/utils.h>
// - Bayeux/dpp
#include <dpp/histogram_service.h>

// - Falaise
#include <snemo/datamodels/data_model.h>
#include <snemo/datamodels/event_header.h>
#include <snemo/datamodels/topology_data.h>
#include <snemo/datamodels/topology_2e_pattern.h>

namespace snemo {
namespace analysis {

  // Character separator between key for histogram dict.
  const char KEY_FIELD_SEPARATOR = '_';

  // Continuous FeldmanCousin functions computed by M. Bongrand
  // <bongrand@lal.in2p3.fr>
  double get_number_of_excluded_events(const double number_of_events_)
  {
    double number_of_excluded_events = 0.0;
    if (number_of_events_ < 29.0) {
      double x = number_of_events_;
      number_of_excluded_events =
        2.5617 + 0.747661 * x - 0.0666176 * std::pow(x,2)
        + 0.00432457 * std::pow(x,3) - 0.000139343 * std::pow(x,4)
        + 1.71509e-06 * std::pow(x,5);
    } else {
      number_of_excluded_events = 1.64 * std::sqrt(number_of_events_);
    }
    return number_of_excluded_events;
  }

  void snemo_bb0nu_halflife_limit_module::experiment_entry_type::initialize(const datatools::properties & config_)
  {
    // Get experimental conditions
    DT_THROW_IF(! config_.has_key("isotope_mass_number"), std::logic_error,
                "Missing 'isotope_mass_number' value !");
    isotope_mass_number = config_.fetch_real("isotope_mass_number");
    if (! config_.has_explicit_unit("isotope_mass_number")) {
      isotope_mass_number *= CLHEP::g/CLHEP::mole;
    }

    DT_THROW_IF(! config_.has_key("isotope_mass"), std::logic_error, "Missing 'isotope_mass' value !");
    isotope_mass = config_.fetch_real("isotope_mass");
    if (! config_.has_explicit_unit("isotope_mass")) {
      isotope_mass *= CLHEP::kg;
    }

    DT_THROW_IF(! config_.has_key("isotope_bb2nu_halflife"), std::logic_error,
                "Missing 'isotope_bb2nu_halflife' value !");
    isotope_bb2nu_halflife = config_.fetch_real("isotope_bb2nu_halflife");

    DT_THROW_IF(! config_.has_key("isotope_bb2nu_halflife"), std::logic_error,
                "Missing 'exposure_time' value !");
    exposure_time = config_.fetch_real("exposure_time");

    if (config_.has_key("background_list")) {
      std::vector<std::string> bkgs;
      config_.fetch("background_list", bkgs);
      for (auto ibkg : bkgs) {
        const std::string & bkgname = ibkg;
        std::string key;
        DT_THROW_IF(! config_.has_key(key = bkgname + ".activity"), std::logic_error,
                    "Missing activity for '" << bkgname << "' background !");
        const double activity = config_.fetch_real(key);
        DT_THROW_IF(! config_.has_unit_symbol(key), std::logic_error, "Missing activity unit !");
        const std::string & unit_symbol = config_.get_unit_symbol(key);
        DT_LOG_NOTICE(datatools::logger::PRIO_NOTICE,
                      "Adding '" << bkgname << "' background with an activity of "
                      << activity/datatools::units::get_unit(unit_symbol) << " " << unit_symbol);
        double norm = 1.0;
        if (datatools::units::is_unit_in_dimension_from(unit_symbol, "volume_activity")) {
          DT_THROW_IF(! config_.has_key(key = bkgname + ".volume"), std::logic_error,
                      "Missing volume normalization for '" << bkgname << "' background !");
          norm = config_.fetch_real(key);
        } else if (datatools::units::is_unit_in_dimension_from(unit_symbol, "surface_activity")) {
          DT_THROW_IF(! config_.has_key(key = bkgname + ".surface"), std::logic_error,
                      "Missing surface normalization for '" << bkgname << "' background !");
          norm = config_.fetch_real(key);
        } else if (datatools::units::is_unit_in_dimension_from(unit_symbol, "mass_activity")) {
          DT_THROW_IF(! config_.has_key(key = bkgname + ".mass"), std::logic_error,
                      "Missing mass normalization for '" << bkgname << "' background !");
          norm = config_.fetch_real(key);
        } else if (datatools::units::is_unit_in_dimension_from(unit_symbol, "activity")) {
          // Nothing to be done
        } else {
          DT_THROW_IF(true, std::logic_error, "Activity of '" << bkgname << "' has unknown units !");
        }
        background_activities[bkgname] = activity * norm;
      }
    }
    return;
  }

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_bb0nu_halflife_limit_module,
                                    "snemo::analysis::snemo_bb0nu_halflife_limit_module");

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
  void snemo_bb0nu_halflife_limit_module::set_histogram_pool(mygsl::histogram_pool & pool_)
  {
    DT_THROW_IF(is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is already initialized !");
    _histogram_pool_ = &pool_;
    return;
  }

  // Grab the histogram pool used by the module :
  mygsl::histogram_pool & snemo_bb0nu_halflife_limit_module::grab_histogram_pool()
  {
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");
    return *_histogram_pool_;
  }

  void snemo_bb0nu_halflife_limit_module::_set_defaults()
  {
    _key_fields_.clear();
    _histogram_pool_ = 0;
    return;
  }

  // Initialization :
  void snemo_bb0nu_halflife_limit_module::initialize(const datatools::properties  & config_,
                                                     datatools::service_manager   & service_manager_,
                                                     dpp::module_handle_dict_type & /*module_dict_*/)
  {
    DT_THROW_IF(is_initialized(),
                std::logic_error,
                "Module '" << get_name() << "' is already initialized ! ");

    dpp::base_module::_common_initialize(config_);

    // Get the experimental conditions
    datatools::properties exp_config;
    config_.export_and_rename_starting_with(exp_config, "experiment.", "");
    _experiment_conditions_.initialize(exp_config);

    // Get the keys from 'Event Header' bank
    if (config_.has_key("key_fields")) {
      config_.fetch("key_fields", _key_fields_);
    }

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
      if (config_.has_key("Histo_input_file")) {
        const std::string input_file = config_.fetch_string("Histo_input_file");
        Histo.load_from_boost_file(input_file);
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
  void snemo_bb0nu_halflife_limit_module::reset()
  {
    DT_THROW_IF(! is_initialized(),
                std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    // Compute efficiency
    _compute_efficiency();

    // Compute neutrinoless halflife limit
    _compute_halflife();

    // Dump result
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG)
      {
        DT_LOG_NOTICE(get_logging_priority(), "Halflife limit module dump: ");
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
    _set_defaults();
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
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    // Check if the 'event header' record bank is available :
    const std::string eh_label = snemo::datamodel::data_info::default_event_header_label();
    if (! data_record_.has(eh_label)) {
      DT_LOG_ERROR(get_logging_priority(), "Could not find any bank with label '"
                   << eh_label << "' !");
      return dpp::base_module::PROCESS_STOP;
    }
    const snemo::datamodel::event_header & eh
      = data_record_.get<snemo::datamodel::event_header>(eh_label);

    // Check if the 'topology data' record bank is available :
    const std::string td_label = "TD";//snemo::datamodel::data_info::default_particle_track_data_label();
    if (! data_record_.has(td_label)) {
      DT_LOG_ERROR(get_logging_priority(), "Could not find any bank with label '"
                   << td_label << "' !");
      return dpp::base_module::PROCESS_STOP;
    }
    const snemo::datamodel::topology_data & td
      = data_record_.get<snemo::datamodel::topology_data>(td_label);

    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) {
      DT_LOG_DEBUG(get_logging_priority(), "Event header : ");
      eh.tree_dump();
      DT_LOG_DEBUG(get_logging_priority(), "Topology data : ");
      td.tree_dump();
    }

    // Build unique key for histogram map:
    std::ostringstream key;

    // Retrieving info from header bank:
    const datatools::properties & eh_properties = eh.get_properties();
    for (std::vector<std::string>::const_iterator ifield = _key_fields_.begin();
         ifield != _key_fields_.end(); ++ifield) {
      const std::string & a_field = *ifield;
      if (! eh_properties.has_key(a_field)) {
        DT_LOG_WARNING(get_logging_priority(), "No properties with key '" << a_field << "' "
                       << "has been found in event header !");
        continue;
      }

      if (eh_properties.is_vector(a_field)) {
        DT_LOG_WARNING(get_logging_priority(),
                       "Stored properties '" << a_field << "' " << "must be scalar !");
        continue;
      }
      if (eh_properties.is_boolean(a_field))      key << eh_properties.fetch_boolean(a_field);
      else if (eh_properties.is_integer(a_field)) key << eh_properties.fetch_integer(a_field);
      else if (eh_properties.is_real(a_field))    key << eh_properties.fetch_real(a_field);
      else if (eh_properties.is_string(a_field))  key << eh_properties.fetch_string(a_field);
    }
    DT_LOG_TRACE(get_logging_priority(), "Key = " << key.str());

    // Get total energy
    if (! td.has_pattern()) {
      DT_LOG_ERROR(get_logging_priority(), "Missing topology pattern !");
      return dpp::base_module::PROCESS_STOP;
    }
    const snemo::datamodel::base_topology_pattern & a_pattern = td.get_pattern();
    if (a_pattern.get_pattern_id() != snemo::datamodel::topology_2e_pattern::pattern_id()) {
      DT_LOG_ERROR(get_logging_priority(), "0nu sensitivity can only be calculated for '2e' topology!");
      return dpp::base_module::PROCESS_STOP;
    }
    const snemo::datamodel::topology_2e_pattern & a_2e_pattern
      = dynamic_cast<const snemo::datamodel::topology_2e_pattern &>(a_pattern);

    // Getting histogram pool
    mygsl::histogram_pool & a_pool = grab_histogram_pool();

    if (! a_pool.has(key.str())) {
      mygsl::histogram_1d & h = a_pool.add_1d(key.str(), "", "energy");
      datatools::properties hconfig;
      hconfig.store_string("mode", "mimic");
      hconfig.store_string("mimic.histogram_1d", "energy_template");
      mygsl::histogram_pool::init_histo_1d(h, hconfig, &a_pool);
    }

    // Getting the current histogram
    mygsl::histogram_1d & a_histo = a_pool.grab_1d(key.str ());
    a_histo.fill(a_2e_pattern.get_total_energy());

    // Compute normalization factor given the total number of events generated
    // and the weight of each event
    double weight = 1.0;
    if (eh_properties.has_key("analysis.total_number_of_event")) {
      weight /= eh_properties.fetch_integer("analysis.total_number_of_event");
    }
    if (eh_properties.has_key(mctools::event_utils::EVENT_GENBB_WEIGHT)) {
      weight *= eh_properties.fetch_real(mctools::event_utils::EVENT_GENBB_WEIGHT);
    }

    // Store the weight into histogram properties
    if (! a_histo.get_auxiliaries().has_key("weight")) {
      a_histo.grab_auxiliaries().update("weight", weight);
    }

    return dpp::base_module::PROCESS_SUCCESS;
  }

  void snemo_bb0nu_halflife_limit_module::_compute_efficiency()
  {
    // Getting histogram pool
    mygsl::histogram_pool & a_pool = grab_histogram_pool();

    // Get names of all saved 1D histograms belonging to 'energy' group
    std::vector<std::string> hnames;
    a_pool.names(hnames, "group=energy");

    if (hnames.empty()) {
      DT_LOG_WARNING(get_logging_priority(), "No 'energy' histograms have been stored !");
      return;
    }

    // Sum of number of events
    std::map<std::string, double> m_event;
    for (std::vector<std::string>::const_iterator iname = hnames.begin();
         iname != hnames.end(); ++iname) {
      const std::string & a_name = *iname;
      DT_THROW_IF(! a_pool.has_1d(a_name), std::logic_error,
                  "Histogram '" << a_name << "' is not 1D histogram !");
      const mygsl::histogram_1d & a_histogram = a_pool.get_1d(a_name);

      // Loop over bin content
      for (size_t i = 0; i < a_histogram.bins(); ++i) {
        const double sum   = a_histogram.sum() + a_histogram.overflow();
        const double value = a_histogram.get(i);

        if (! datatools::is_valid(value)) {
          DT_LOG_WARNING(get_logging_priority(), "Skipping non valid value !");
          continue;
        }

        // Retrieve histogram weight
        double weight = 1.0;
        if (a_histogram.get_auxiliaries().has_key("weight")) {
          weight = a_histogram.get_auxiliaries().fetch_real("weight");
        }

        // Compute fraction of event for each histogram bin
        const double efficiency = (sum - m_event[a_name]) * weight;
        m_event[a_name] += value;

        if (! datatools::is_valid(efficiency)) {
          DT_LOG_WARNING(get_logging_priority(), "Skipping non valid efficiency computation !");
          continue;
        }

        // Adding histogram efficiency
        const std::string & key_str = a_name + KEY_FIELD_SEPARATOR + "efficiency";
        if (! a_pool.has(key_str)) {
          mygsl::histogram_1d & h = a_pool.add_1d(key_str, "", "efficiency");
          datatools::properties hconfig;
          hconfig.store_string("mode", "mimic");
          hconfig.store_string("mimic.histogram_1d", "efficiency_template");
          mygsl::histogram_pool::init_histo_1d(h, hconfig, &a_pool);
        }

        // Getting & updating the current histogram
        mygsl::histogram_1d & a_new_histogram = a_pool.grab_1d(key_str);
        a_new_histogram.set(i, efficiency);

        // Flag signal/background histogram
        datatools::properties & a_aux = a_new_histogram.grab_auxiliaries();
        if (a_name.find("0nubb") != std::string::npos) {
          a_aux.update_flag(snemo_bb0nu_halflife_limit_module::signal_flag());
        } else {
          a_aux.update_flag(snemo_bb0nu_halflife_limit_module::background_flag());
        }
      } // end of bin content
    }// end of histogram loop

    return;
  }

  void snemo_bb0nu_halflife_limit_module::_compute_halflife()
  {
    // Get SuperNEMO experiment setup
    // Calculate signal to halflife limit constant
    const double exposure_time          = _experiment_conditions_.exposure_time;          // year;
    const double isotope_bb2nu_halflife = _experiment_conditions_.isotope_bb2nu_halflife; // year;
    const double isotope_mass           = _experiment_conditions_.isotope_mass;
    const double isotope_molar_mass     = _experiment_conditions_.isotope_mass_number;
    const double kbg = std::log(2) * isotope_mass * CLHEP::Avogadro * exposure_time
      / isotope_molar_mass / CLHEP::mole / isotope_bb2nu_halflife;

    // Getting histogram pool
    mygsl::histogram_pool & a_pool = grab_histogram_pool();

    // Get names of all saved 1D histograms belonging to 'efficiency' group
    std::vector<std::string> hnames;
    a_pool.names(hnames, "group=efficiency");
    if (hnames.empty()) {
      DT_LOG_WARNING(get_logging_priority(), "No 'efficiency' histograms have been stored !");
      return;
    }

    // Get names of 'background' histograms
    std::vector<std::string> bkg_names;
    a_pool.names(bkg_names, "flag=" + snemo_bb0nu_halflife_limit_module::background_flag());
    if (bkg_names.empty()) {
      DT_LOG_WARNING(get_logging_priority(), "No 'background' histograms have been stored !");
      return;
    }

    // Loop over 'background' histograms and count the number of background
    // events within the energy window
    std::vector<double> vbkg_counts;
    for (std::vector<std::string>::const_iterator iname = bkg_names.begin();
         iname != bkg_names.end(); ++iname) {
      const std::string & a_name = *iname;
      DT_THROW_IF(! a_pool.has_1d(a_name), std::logic_error,
                  "Histogram '" << a_name << "' is not 1D histogram !");
      if (a_pool.get_group(a_name) != "efficiency") {
        DT_LOG_ERROR(get_logging_priority(),
                     "Histogram '" << a_name << "' does not belong to 'efficiency' group !");
        continue;
      }
      // Get normalization factor
      double norm_factor;
      datatools::invalidate(norm_factor);
      if (a_name.find("2nubb") != std::string::npos) {
        norm_factor = kbg;
      } else {
        const experiment_entry_type::background_dict_type & bkgs = _experiment_conditions_.background_activities;
        for (experiment_entry_type::background_dict_type::const_iterator
               ibkg = bkgs.begin();
             ibkg != bkgs.end(); ++ibkg) {
          if (a_name.find(ibkg->first) != std::string::npos) {
            DT_LOG_TRACE(get_logging_priority(),
                         "Found background element '" << ibkg->first << "'");
            const double year2sec = 3600 * 24 * 365;
            norm_factor = ibkg->second/CLHEP::becquerel * exposure_time * year2sec;
          }
        }
      }
      if (! datatools::is_valid(norm_factor)) {
        DT_LOG_WARNING(get_logging_priority(),
                       "No background activity has been found ! Skip histogram '" << a_name << "'");
        continue;
      }
      DT_LOG_TRACE(get_logging_priority(),
                   "Total number of decay for '" << a_name << "' = " << norm_factor);

      const mygsl::histogram_1d & a_histogram = a_pool.get_1d(a_name);
      for (size_t i = 0; i < a_histogram.bins(); ++i) {
        const double value = a_histogram.get(i) * norm_factor;
        if (vbkg_counts.empty()) vbkg_counts.assign(a_histogram.bins(), 0.0);
        vbkg_counts.at(i) += value;
      }

      // Adding histogram efficiency in terms of number of events
      const std::string & key_str = a_name + KEY_FIELD_SEPARATOR + "event_number";
      if (a_pool.has(key_str)) {
        DT_LOG_WARNING(get_logging_priority(), "Histogram '" << key_str << "' already exists ! Remove it !");
        a_pool.remove(key_str);
      }
      mygsl::histogram_1d & h = a_pool.add_1d(key_str, "", "efficiency_event_number");
      std::vector<std::string> dummy;
      h.initialize(a_histogram*norm_factor, dummy);
      h.grab_auxiliaries().update("display.yaxis.label", "dN/dE");
    }// end of background loop

    // Get names of 'signal' histograms
    std::vector<std::string> signal_names;
    a_pool.names(signal_names, "flag=" + snemo_bb0nu_halflife_limit_module::signal_flag());
    if (signal_names.empty()) {
      DT_LOG_WARNING(get_logging_priority(), "No 'signal' histograms have been stored !");
      return;
    }
    // Loop over 'signal' histograms
    for (std::vector<std::string>::const_iterator iname = signal_names.begin();
         iname != signal_names.end(); ++iname) {
      double best_halflife_limit = 0.0;
      const std::string & a_name = *iname;
      DT_THROW_IF(! a_pool.has_1d(a_name), std::logic_error,
                  "Histogram '" << a_name << "' is not 1D histogram !");
      if (a_pool.get_group(a_name) != "efficiency") {
        DT_LOG_ERROR(get_logging_priority(),
                     "Histogram '" << a_name << "' does not belongs to 'efficiency' group !");
        continue;
      }
      const mygsl::histogram_1d & a_histogram = a_pool.get_1d(a_name);
      // Loop over bin content
      for (size_t i = 0; i < a_histogram.bins(); ++i) {
        const double value = a_histogram.get(i);

        // Compute the number of event excluded for the same energy bin
        const double nbkg = vbkg_counts.at(i);
        const double nexcluded = analysis::get_number_of_excluded_events(nbkg);
        const double halflife = value / nexcluded * kbg * isotope_bb2nu_halflife;

        // Keeping larger limit
        best_halflife_limit = std::max(best_halflife_limit, halflife);

        // Adding histogram halflife
        const std::string & key_str = a_name + KEY_FIELD_SEPARATOR + "halflife";
        if (!a_pool.has(key_str)) {
          mygsl::histogram_1d & h = a_pool.add_1d(key_str, "", "halflife");
          datatools::properties hconfig;
          hconfig.store_string("mode", "mimic");
          hconfig.store_string("mimic.histogram_1d", "halflife_template");
          mygsl::histogram_pool::init_histo_1d(h, hconfig, &a_pool);
        }

        // Getting the current histogram
        mygsl::histogram_1d & a_new_histogram = a_pool.grab_1d(key_str);
        a_new_histogram.set(i, halflife);
      }
      DT_LOG_NOTICE(get_logging_priority(),
                    "Best halflife limit for bb0nu process is " << best_halflife_limit << " yr");
    }// end of signal loop
  }

  void snemo_bb0nu_halflife_limit_module::dump_result(std::ostream      & out_,
                                                      const std::string & title_,
                                                      const std::string & indent_,
                                                      bool inherit_) const
  {
    std::string indent;
    if (! indent_.empty()) {
      indent = indent_;
    }
    if (! title_.empty()) {
      out_ << indent << title_ << std::endl;
    }

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

    // Histogram :
    out_ << indent << datatools::i_tree_dumpable::tag
         << "Particle energy histograms : ";
    if (_histogram_pool_->empty())
      out_ << "<empty>";
    else
      out_ << _histogram_pool_->size();
    out_ << std::endl;;

    std::vector<std::string> hnames;
    _histogram_pool_->names(hnames);
    for (std::vector<std::string>::const_iterator i = hnames.begin();
         i != hnames.end(); ++i) {
      const std::string & a_name = *i;
      if (a_name.find("template") != std::string::npos) continue;

      std::vector<std::string>::const_iterator j = i;
      out_ << indent;
      std::ostringstream indent_oss;
      if (++j == hnames.end()) {
        out_  << datatools::i_tree_dumpable::last_tag;
        indent_oss << indent << datatools::i_tree_dumpable::last_skip_tag;
      } else {
        out_ << datatools::i_tree_dumpable::tag;
        indent_oss << indent << datatools::i_tree_dumpable::skip_tag;
      }

      out_ << "Label " << a_name << std::endl;
      const mygsl::histogram_1d & a_histogram = _histogram_pool_->get_1d(a_name);
      a_histogram.tree_dump(out_, "", indent_oss.str(), inherit_);

      if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) {
        DT_LOG_DEBUG(get_logging_priority(), "Histogram " << a_name << " dump:");
        a_histogram.print(std::clog);
      }
    }

  return;
}

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
