/// base_plotter.cc

// Ourselves:
#include <base_plotter.h>

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

namespace snemo {
namespace analysis {

  bool base_plotter::has_histogram_pool() const
  {
    return _histogram_pool_ != 0;
  }

  void base_plotter::set_histogram_pool(mygsl::histogram_pool & pool_)
  {
    _histogram_pool_ = &pool_;
    return;
  }

  mygsl::histogram_pool & base_plotter::grab_histogram_pool()
  {
    return *_histogram_pool_;
  }

  bool base_plotter::is_initialized() const
  {
    return _initialized;
  }

  void base_plotter::_set_initialized(bool initialized_)
  {
    _initialized = initialized_;
    return;
  }

  bool base_plotter::has_name() const
  {
    return ! _name.empty ();
  }

  void base_plotter::set_name(const std::string & name_)
  {
    DT_THROW_IF(is_initialized (), std::logic_error,
                "Plotter '" << _name << "' "
                << "is already initialized ! "
                << "Cannot change the name !");
    _set_name(name_);
    return;
  }

  const std::string & base_plotter::get_name() const
  {
    return _name;
  }

  void base_plotter::_set_name(const std::string & name_)
  {
    _name = name_;
    return;
  }

  bool base_plotter::has_description() const
  {
    return ! _description.empty ();
  }

  const std::string & base_plotter::get_description() const
  {
    return _description;
  }

  void base_plotter::set_description(const std::string & description_)
  {
    DT_THROW_IF(is_initialized (), std::logic_error,
                "Plotter '" << _name << "' "
                << "is already initialized ! "
                << "Cannot change the description !");
    _description = description_;
    return;
  }

  bool base_plotter::has_bank_label() const
  {
    return ! _bank_label.empty();
  }

  const std::string & base_plotter::get_bank_label() const
  {
    return _bank_label;
  }

  void base_plotter::set_bank_label(const std::string & label_)
  {
    _bank_label = label_;
    return;
  }

  base_plotter::base_plotter(datatools::logger::priority p_)
  {
    _initialized = false;
    _logging = datatools::logger::PRIO_FATAL;
    set_logging_priority(p_);
    return;
  }

  base_plotter::~base_plotter()
  {
    DT_THROW_IF(_initialized, std::logic_error,
                "Plotter '" << _name << "' "
                << "still has its 'initialized' flag on ! "
                << "Possible bug !");
    return;
  }

  void base_plotter::tree_dump(std::ostream & out_,
                               const std::string & title_,
                               const std::string & indent_,
                               bool inherit_) const
  {
    std::string indent;
    if (! indent_.empty ()) {
      indent = indent_;
    }
    if ( ! title_.empty () ) {
      out_ << indent << title_ << std::endl;
    }
    out_ << indent << datatools::i_tree_dumpable::tag
         << "Plotter name        : '" << _name << "'" << std::endl;
    out_ << indent << datatools::i_tree_dumpable::tag
         << "Plotter description : '" << _description << "'" << std::endl;
    out_ << indent << datatools::i_tree_dumpable::tag
         << "Plotter logging threshold : '"
         << datatools::logger::get_priority_label(_logging) << "'" << std::endl;
    out_ << indent << datatools::i_tree_dumpable::tag
         << "Plotter initialized : " << is_initialized() << std::endl;
    out_ << indent << datatools::i_tree_dumpable::tag
         << "Bank label name : " << get_bank_label() << std::endl;
    return;
  }

  void base_plotter::set_logging_priority(datatools::logger::priority priority_)
  {
    _logging = priority_;
    return;
  }

  datatools::logger::priority base_plotter::get_logging_priority() const
  {
    return _logging;
  }

  void base_plotter::_common_initialize(const datatools::properties & config_)
  {
    DT_THROW_IF(is_initialized(), std::logic_error,
                "Plotter '" << get_name() << "' is already initialized ! ");

    DT_THROW_IF(! has_histogram_pool(), std::logic_error, "Missing histogram pool !");
    DT_THROW_IF(! grab_histogram_pool().is_initialized(), std::logic_error,
                "Histogram pool is not initialized !");

    // Logging priority:
    datatools::logger::priority p =
      datatools::logger::extract_logging_configuration(config_,
                                                       datatools::logger::PRIO_UNDEFINED,
                                                       true);
    if (p != datatools::logger::PRIO_UNDEFINED) {
      set_logging_priority(p);
    }

    if (! has_name()) {
      if (config_.has_key("name")) {
        set_name(config_.fetch_string("name"));
      }
    }

    if (! has_description()) {
      if (config_.has_key("description")) {
        set_description(config_.fetch_string("description"));
      }
    }

    if (config_.has_key("bank_label")) {
      set_bank_label(config_.fetch_string("bank_label"));
    }

    return;
  }

  void base_plotter::common_ocd(datatools::object_configuration_description & ocd_)
  {
    datatools::logger::declare_ocd_logging_configuration(ocd_, "fatal", "");

    {
      datatools::configuration_property_description & cpd = ocd_.add_property_info();
      cpd.set_name_pattern("description")
        .set_from("analysis::base_plotter")
        .set_terse_description("The description of the plotter")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_long_description("A description of the plotter.")
        .add_example("Example::                                              \n"
                     "                                                       \n"
                     "  description : string = \"Plotting calibration data\" \n"
                     "                                                       \n"
                     )
        ;
    }

    {
      datatools::configuration_property_description & cpd = ocd_.add_property_info();
      cpd.set_name_pattern("name")
        .set_from("analysis::base_plotter")
        .set_terse_description("The name of the plotter")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_long_description("A name given to the plotter.")
        .add_example("Example::                       \n"
                     "                                \n"
                     "  name : string = \"CDPlotter\" \n"
                     "                                \n"
                     )
        ;
    }
    return;
  }

} // end of namespace snemo
} // end of namespace analysis

// end of base_plotter.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
