/// topology_data_plotter.cc

// Ourselves:
#include <topology_data_plotter.h>

// Standard library
#include <stdexcept>
#include <string>
#include <sstream>

// Third party:
// - Bayeux/datatools:
#include <bayeux/datatools/properties.h>
#include <bayeux/datatools/ioutils.h>
#include <bayeux/datatools/object_configuration_description.h>
// - Bayeux/mygsl
#include <bayeux/mygsl/histogram_pool.h>

// - Falaise
#include <falaise/snemo/datamodels/data_model.h>
#include <falaise/snemo/datamodels/topology_data.h>
#include <falaise/snemo/datamodels/topology_1e_pattern.h>
#include <falaise/snemo/datamodels/topology_1e1a_pattern.h>
#include <falaise/snemo/datamodels/topology_1eNg_pattern.h>
#include <falaise/snemo/datamodels/topology_2e_pattern.h>
#include <falaise/snemo/datamodels/topology_2eNg_pattern.h>

namespace snemo {
namespace analysis {

  const std::string & topology_data_plotter::get_id()
  {
    static const std::string s("TDP");
    return s;
  }

  topology_data_plotter::topology_data_plotter(datatools::logger::priority p_) :
    ::snemo::analysis::base_plotter(p_)
  {
    _set_defaults();
    return;
  }

  topology_data_plotter::~topology_data_plotter()
  {
    if (is_initialized()) ::snemo::analysis::topology_data_plotter::reset();
    return;
  }

  // Initialization :
  void topology_data_plotter::initialize(const datatools::properties & setup_)
  {
    ::snemo::analysis::base_plotter::_common_initialize(setup_);

    _set_initialized(true);
    return;
  }

  // Reset the clusterizer
  void topology_data_plotter::reset()
  {
    DT_THROW_IF (! is_initialized(), std::logic_error,
                 "Plotter '" << get_name() << "' is not initialized !");
    _set_initialized(false);
    _set_defaults();
    return;
  }

  void topology_data_plotter::plot(const datatools::things & data_record_)
  {
    DT_THROW_IF(! has_bank_label(), std::logic_error, "Missing bank label !");

    // Check if some 'topology_data' are available in the data model:
    if (! data_record_.has(get_bank_label())) {
      DT_LOG_DEBUG(get_logging_priority(), "Missing topology data to be processed !");
      return;
    }
    // Get the 'topology_data' entry from the data model :
    const snemo::datamodel::topology_data & td
      = data_record_.get<snemo::datamodel::topology_data>(get_bank_label());
    ::snemo::analysis::topology_data_plotter::_plot(td);
    return;
  }

  void topology_data_plotter::_set_defaults()
  {
    set_bank_label("TD");//snemo::datamodel::data_info::default_topology_data_label());
    return;
  }

  void topology_data_plotter::_plot(const snemo::datamodel::topology_data & td_)
  {
    DT_LOG_DEBUG (get_logging_priority(), "Topology data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) td_.tree_dump(std::clog);

    if (! td_.has_pattern()) return;

    if (td_.has_pattern_as<snemo::datamodel::topology_1e_pattern>()) {
      auto a_pattern = td_.get_pattern_as<snemo::datamodel::topology_1e_pattern>();
      _plot_1e_(a_pattern);
    }
    if (td_.has_pattern_as<snemo::datamodel::topology_1e1a_pattern>()) {
      auto a_pattern = td_.get_pattern_as<snemo::datamodel::topology_1e1a_pattern>();
      _plot_1e1a_(a_pattern);
    }
    if (td_.has_pattern_as<snemo::datamodel::topology_1eNg_pattern>()) {
      auto a_pattern = td_.get_pattern_as<snemo::datamodel::topology_1eNg_pattern>();
      _plot_1eNg_(a_pattern);
    }
    if (td_.has_pattern_as<snemo::datamodel::topology_2e_pattern>()) {
      auto a_pattern = td_.get_pattern_as<snemo::datamodel::topology_2e_pattern>();
      _plot_2e_(a_pattern);
    }
    if (td_.has_pattern_as<snemo::datamodel::topology_2eNg_pattern>()) {
      auto a_pattern = td_.get_pattern_as<snemo::datamodel::topology_2eNg_pattern>();
      _plot_2eNg_(a_pattern);
    }
    return;
  }

  void topology_data_plotter::_plot_1e_(const snemo::datamodel::topology_1e_pattern & pattern_,
                                        const std::string & prefix_)
  {
    std::string a_key;
    mygsl::histogram_pool & a_pool = grab_histogram_pool();
    if (a_pool.has_1d(a_key = prefix_ + "electron_energy")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
      const double energy = pattern_.get_electron_energy();
      if (datatools::is_valid(energy)) h1d.fill(energy);
    }
    if (a_pool.has_1d(a_key = prefix_ + "electron_track_length")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
      const double length = pattern_.get_electron_track_length();
      if (datatools::is_valid(length)) h1d.fill(length);
    }
    if (a_pool.has_1d(a_key = prefix_ + "electron_angle")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
      const double angle = pattern_.get_electron_angle();
      if (datatools::is_valid(angle)) h1d.fill(angle);
    }
    return;
  }

  void topology_data_plotter::_plot_1e1a_(const snemo::datamodel::topology_1e1a_pattern & pattern_,
                                          const std::string & prefix_)
  {
    _plot_1e_(pattern_, prefix_);
    std::string a_key;
    mygsl::histogram_pool & a_pool = grab_histogram_pool();
    if (a_pool.has_1d(a_key = prefix_ + "alpha_delayed_time")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
      const double time = pattern_.get_alpha_delayed_time();
      if (datatools::is_valid(time)) h1d.fill(time);
    }
    if (a_pool.has_1d(a_key = prefix_ + "alpha_track_length")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
      const double length = pattern_.get_alpha_track_length();
      if (datatools::is_valid(length)) h1d.fill(length);
    }

    return;
  }

  void topology_data_plotter::_plot_1eNg_(const snemo::datamodel::topology_1eNg_pattern & pattern_,
                                          const std::string & prefix_)
  {
    const size_t nbr_gammas = pattern_.get_number_of_gammas();
    std::ostringstream a_prefix;
    a_prefix << prefix_ << nbr_gammas << "g::";
    _plot_1e_(pattern_, a_prefix.str());

    // Fetch gamma energies
    snemo::datamodel::topology_1eNg_pattern::energy_collection_type gammas_energies;
    pattern_.fetch_gammas_energies(gammas_energies);
    std::sort(gammas_energies.begin(), gammas_energies.end());

    if (nbr_gammas == 1) {
      std::string a_key;
      mygsl::histogram_pool & a_pool = grab_histogram_pool();
      if (a_pool.has_1d(a_key = a_prefix.str() + "gamma_energy")) {
        mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
        const double energy = gammas_energies.front();
        if (datatools::is_valid(energy)) h1d.fill(energy);
      }
      if (a_pool.has_2d(a_key = a_prefix.str() + "electron_energy_vs_gamma_energy")) {
        mygsl::histogram_2d & h2d = a_pool.grab_2d(a_key);
        const double gamma_energy = gammas_energies.front();
        const double electron_energy = pattern_.get_electron_energy();
        if (datatools::is_valid(gamma_energy) &&
            datatools::is_valid(electron_energy)) {
          h2d.fill(gamma_energy, electron_energy);
        }
      }
    } else if (nbr_gammas == 2) {
      std::string a_key;
      mygsl::histogram_pool & a_pool = grab_histogram_pool();
      if (a_pool.has_1d(a_key = a_prefix.str() + "gamma_minimal_energy")) {
        mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
        const double energy = gammas_energies.front();
        if (datatools::is_valid(energy)) h1d.fill(energy);
      }
      if (a_pool.has_1d(a_key = a_prefix.str() + "gamma_maximal_energy")) {
        mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
        const double energy = gammas_energies.back();
        if (datatools::is_valid(energy)) h1d.fill(energy);
      }
    } else if (nbr_gammas == 3) {
      std::string a_key;
      mygsl::histogram_pool & a_pool = grab_histogram_pool();
      if (a_pool.has_1d(a_key = a_prefix.str() + "gamma_minimal_energy")) {
        mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
        const double energy = gammas_energies.front();
        if (datatools::is_valid(energy)) h1d.fill(energy);
      }
      if (a_pool.has_1d(a_key = a_prefix.str() + "gamma_mid_energy")) {
        mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
        const double energy = gammas_energies.at(2);
        if (datatools::is_valid(energy)) h1d.fill(energy);
      }
      if (a_pool.has_1d(a_key = a_prefix.str() + "gamma_maximal_energy")) {
        mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
        const double energy = gammas_energies.back();
        if (datatools::is_valid(energy)) h1d.fill(energy);
      }
    }
    return;
  }

  void topology_data_plotter::_plot_2e_(const snemo::datamodel::topology_2e_pattern & pattern_,
                                        const std::string & prefix_)
  {
    std::string a_key;
    mygsl::histogram_pool & a_pool = grab_histogram_pool();
    if (a_pool.has_1d(a_key = prefix_ + "electron_minimal_energy")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
      const double energy = pattern_.get_electron_minimal_energy();
      if (datatools::is_valid(energy)) h1d.fill(energy);
    }
    if (a_pool.has_1d(a_key = prefix_ + "electron_maximal_energy")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
      const double energy = pattern_.get_electron_maximal_energy();
      if (datatools::is_valid(energy)) h1d.fill(energy);
    }
    if (a_pool.has_1d(a_key = prefix_ + "electrons_energy_sum")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
      const double energy = pattern_.get_electrons_energy_sum();
      if (datatools::is_valid(energy)) h1d.fill(energy);
    }
    if (a_pool.has_1d(a_key = prefix_ + "electrons_angle")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d(a_key);
      const double angle = pattern_.get_electrons_angle();
      if (datatools::is_valid(angle)) h1d.fill(angle);
    }
    return;
  }

  void topology_data_plotter::_plot_2eNg_(const snemo::datamodel::topology_2eNg_pattern & pattern_,
                                          const std::string & /*prefix_*/)
  {
    std::ostringstream a_prefix;
    a_prefix << "TD::2e" << pattern_.get_number_of_gammas() << "g::";
    _plot_2e_(pattern_, a_prefix.str());
    return;
  }

  void topology_data_plotter::init_ocd(datatools::object_configuration_description & ocd_)
  {
    // Invoke OCD support from parent class :
    ::snemo::analysis::base_plotter::common_ocd(ocd_);

    return;
  }

} // end of namespace analysis
} // end of namespace snemo

// end of topology_data_plotter.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
