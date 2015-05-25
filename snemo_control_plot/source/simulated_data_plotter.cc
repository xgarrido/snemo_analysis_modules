/// simulated_data_plotter.cc

// Ourselves:
#include <simulated_data_plotter.h>

// Standard library
#include <stdexcept>
#include <string>
#include <sstream>

// Third party:
// - Bayeux/datatools:
#include <bayeux/datatools/properties.h>
#include <bayeux/datatools/ioutils.h>
#include <bayeux/datatools/object_configuration_description.h>
// - Bayeux/mctools:
#include <bayeux/mctools/simulated_data.h>
// - Bayeux/mygsl
#include <bayeux/mygsl/histogram_pool.h>

// - Falaise
#include <falaise/snemo/datamodels/data_model.h>

namespace snemo {
namespace analysis {

  const std::string & simulated_data_plotter::get_id()
  {
    static const std::string s("SPD");
    return s;
  }

  simulated_data_plotter::simulated_data_plotter(datatools::logger::priority p_) :
    ::snemo::analysis::base_plotter(p_)
  {
    _set_defaults();
    return;
  }

  simulated_data_plotter::~simulated_data_plotter()
  {
    if (is_initialized()) ::snemo::analysis::simulated_data_plotter::reset();
    return;
  }

  // Initialization :
  void simulated_data_plotter::initialize(const datatools::properties & setup_)
  {
    ::snemo::analysis::base_plotter::_common_initialize(setup_);

    if (setup_.has_key("SD_label")) {
      _SD_label_ = setup_.fetch_string("SD_label");
    }
    return;
  }

  // Reset the clusterizer
  void simulated_data_plotter::reset()
  {
    DT_THROW_IF (! is_initialized(), std::logic_error,
                 "Plotter '" << get_name() << "' is not initialized !");
    _set_initialized(false);
    _set_defaults();
    return;
  }

  void simulated_data_plotter::plot(const datatools::things & data_record_)
  {
    // Check if some 'simulated_data' are available in the data model:
    if (! data_record_.has(_SD_label_)) {
      DT_LOG_DEBUG(get_logging_priority(), "Missing simulated data to be processed !");
      return;
    }
    // Get the 'simulated_data' entry from the data model :
    const mctools::simulated_data & sd = data_record_.get<mctools::simulated_data>(_SD_label_);
    ::snemo::analysis::simulated_data_plotter::_plot(sd);
    return;
  }

  void simulated_data_plotter::tree_dump(std::ostream & out_,
                                         const std::string & title_,
                                         const std::string & indent_,
                                         bool inherit_) const
  {
    ::snemo::analysis::base_plotter::tree_dump(out_, title_, indent_, inherit_);
    return;
  }

  void simulated_data_plotter::_set_defaults()
  {
    _SD_label_ = snemo::datamodel::data_info::default_simulated_data_label();
    return;
  }

  void simulated_data_plotter::_plot(const mctools::simulated_data & sd_)
  {
    DT_LOG_DEBUG (get_logging_priority(), "Simulated data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) sd_.tree_dump(std::clog);

    mygsl::histogram_pool & a_pool = grab_histogram_pool();
    if (a_pool.has_1d("SD::ngghits")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d("SD::ngghits");
      int nggs = 0;
      if (sd_.has_step_hits("gg")) nggs += sd_.get_number_of_step_hits("gg");
      h1d.fill(nggs);
    }

    if (a_pool.has_1d("SD::ncalohits")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d("SD::ncalohits");
      int ncalos = 0;
      if (sd_.has_step_hits("calo"))  ncalos += sd_.get_number_of_step_hits("calo");
      if (sd_.has_step_hits("xcalo")) ncalos += sd_.get_number_of_step_hits("xcalo");
      if (sd_.has_step_hits("gveto")) ncalos += sd_.get_number_of_step_hits("gveto");
      h1d.fill(ncalos);
    }
    return;
  }

  void simulated_data_plotter::init_ocd(datatools::object_configuration_description & ocd_)
  {
    // Invoke OCD support from parent class :
    ::snemo::analysis::base_plotter::common_ocd(ocd_);

    return;
  }

} // end of namespace analysis
} // end of namespace snemo

// end of simulated_data_plotter.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
