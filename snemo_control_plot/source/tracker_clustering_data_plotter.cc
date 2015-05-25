/// tracker_clustering_data_plotter.cc

// Ourselves:
#include <tracker_clustering_data_plotter.h>

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
#include <falaise/snemo/datamodels/tracker_clustering_data.h>

namespace snemo {
namespace analysis {

  const std::string & tracker_clustering_data_plotter::get_id()
  {
    static const std::string s("TCDP");
    return s;
  }

  tracker_clustering_data_plotter::tracker_clustering_data_plotter(datatools::logger::priority p_) :
    ::snemo::analysis::base_plotter(p_)
  {
    _set_defaults();
    return;
  }

  tracker_clustering_data_plotter::~tracker_clustering_data_plotter()
  {
    if (is_initialized()) ::snemo::analysis::tracker_clustering_data_plotter::reset();
    return;
  }

  // Initialization :
  void tracker_clustering_data_plotter::initialize(const datatools::properties & setup_)
  {
    ::snemo::analysis::base_plotter::_common_initialize(setup_);

    if (setup_.has_key("TCD_label")) {
      _TCD_label_ = setup_.fetch_string("TCD_label");
    }

    _set_initialized(true);
    return;
  }

  // Reset the clusterizer
  void tracker_clustering_data_plotter::reset()
  {
    DT_THROW_IF (! is_initialized(), std::logic_error,
                 "Plotter '" << get_name() << "' is not initialized !");
    _set_initialized(false);
    _set_defaults();
    return;
  }

  void tracker_clustering_data_plotter::plot(const datatools::things & data_record_)
  {
    // Check if some 'tracker_clustering_data' are available in the data model:
    if (! data_record_.has(_TCD_label_)) {
      DT_LOG_DEBUG(get_logging_priority(), "Missing tracker clustering data to be processed !");
      return;
    }
    // Get the 'tracker_clustering_data' entry from the data model :
    const snemo::datamodel::tracker_clustering_data & tcd
      = data_record_.get<snemo::datamodel::tracker_clustering_data>(_TCD_label_);
    ::snemo::analysis::tracker_clustering_data_plotter::_plot(tcd);
    return;
  }

  void tracker_clustering_data_plotter::tree_dump(std::ostream & out_,
                                                  const std::string & title_,
                                                  const std::string & indent_,
                                                  bool inherit_) const
  {
    ::snemo::analysis::base_plotter::tree_dump(out_, title_, indent_, inherit_);
    return;
  }

  void tracker_clustering_data_plotter::_set_defaults()
  {
    _TCD_label_ = snemo::datamodel::data_info::default_tracker_clustering_data_label();
    return;
  }

  void tracker_clustering_data_plotter::_plot(const snemo::datamodel::tracker_clustering_data & tcd_)
  {
    DT_LOG_DEBUG (get_logging_priority(), "Tracker clustering data : ");
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) tcd_.tree_dump(std::clog);

    mygsl::histogram_pool & a_pool = grab_histogram_pool();
    if (a_pool.has_1d("TCD::nclusters")) {
      mygsl::histogram_1d & h1d = a_pool.grab_1d("TCD::nclusters");
      int nclusters = 0;
      if (tcd_.has_default_solution()) nclusters += tcd_.get_default_solution().get_clusters().size();
      h1d.fill(nclusters);
    }

    return;
  }

  void tracker_clustering_data_plotter::init_ocd(datatools::object_configuration_description & ocd_)
  {
    // Invoke OCD support from parent class :
    ::snemo::analysis::base_plotter::common_ocd(ocd_);

    {
      // Description of the 'TCD_label' configuration property :
      datatools::configuration_property_description & cpd
        = ocd_.add_property_info();
      cpd.set_name_pattern("TCD_label")
        .set_terse_description("The label/name of the 'tracker clustering data' bank")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_long_description("This is the name of the bank to be used \n"
                              "as the source of input.                 \n")
        .set_default_value_string(snemo::datamodel::data_info::default_tracker_clustering_data_label())
        .add_example("Use an alternative name for the  \n"
                     "'tracker clustering data' bank:: \n"
                     "                                 \n"
                     "  TCD_label : string = \"TCD2\"  \n"
                     "                                 \n"
                     )
        ;
    }

    return;
  }

} // end of namespace analysis
} // end of namespace snemo

// end of tracker_clustering_data_plotter.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
