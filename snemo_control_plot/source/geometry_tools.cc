/// geometry_tools.cc

// Ourselves:
#include <geometry_tools.h>

// - Bayeux/datatools:
#include <bayeux/datatools/exception.h>
#include <bayeux/datatools/properties.h>
#include <bayeux/datatools/service_manager.h>
// - Bayeux/geomtools:
#include <bayeux/geomtools/manager.h>
#include <bayeux/geomtools/geometry_service.h>

// - Falaise:
#include <falaise/snemo/geometry/locator_plugin.h>

namespace snemo {
namespace utils {

  const snemo::geometry::gg_locator & geometry_tools::get_gg_locator() const
  {
    DT_THROW_IF(! is_initialized(), std::logic_error, "Geometry toolbox is not initialized!");
    return _locator_plugin_->get_gg_locator();
  }

  const snemo::geometry::calo_locator & geometry_tools::get_calo_locator() const
  {
    DT_THROW_IF(! is_initialized(), std::logic_error, "Geometry toolbox is not initialized!");
    return _locator_plugin_->get_calo_locator();
  }

  const snemo::geometry::xcalo_locator & geometry_tools::get_xcalo_locator() const
  {
    DT_THROW_IF(! is_initialized(), std::logic_error, "Geometry toolbox is not initialized!");
    return _locator_plugin_->get_xcalo_locator();
  }

  const snemo::geometry::gveto_locator & geometry_tools::get_gveto_locator() const
  {
    DT_THROW_IF(! is_initialized(), std::logic_error, "Geometry toolbox is not initialized!");
    return _locator_plugin_->get_gveto_locator();
  }

  bool geometry_tools::is_initialized() const
  {
    return _initialized_;
  }

  geometry_tools::geometry_tools()
  {
    reset();
    return;
  }

  geometry_tools::~geometry_tools()
  {
    reset();
    return;
  }

  void geometry_tools::initialize(const datatools::properties & config_,
                                  const datatools::service_manager & service_manager_)
  {
    // Geometry service
    std::string geometry_label;
    if (config_.has_key("Geo_label")) {
      geometry_label = config_.fetch_string("Geo_label");
    }
    DT_THROW_IF(geometry_label.empty(), std::logic_error,
                "Geometry toolbox has no valid 'Geo_label' property !");

    DT_THROW_IF(! service_manager_.has(geometry_label) ||
                ! service_manager_.is_a<geomtools::geometry_service>(geometry_label),
                std::logic_error,
                "Geometry toolbox has no '" << geometry_label << "' service !");
    const geomtools::geometry_service & Geo
      = service_manager_.get<geomtools::geometry_service>(geometry_label);
    _geometry_manager_ = &(Geo.get_geom_manager());

    std::string locator_plugin_name;
    if (config_.has_key("locator_plugin_name")) {
      locator_plugin_name = config_.fetch_string("locator_plugin_name");
    } else {
      // If no locator plugin name is set, then search for the first one
      const geomtools::manager::plugins_dict_type & plugins = _geometry_manager_->get_plugins();
      for (geomtools::manager::plugins_dict_type::const_iterator ip = plugins.begin();
           ip != plugins.end();
           ip++) {
        const std::string & plugin_name = ip->first;
        if (_geometry_manager_->is_plugin_a<snemo::geometry::locator_plugin>(plugin_name)) {
          locator_plugin_name = plugin_name;
          break;
        }
      }
    }
    // Access to a given plugin by name and type :
    DT_THROW_IF(! _geometry_manager_->has_plugin(locator_plugin_name) ||
                ! _geometry_manager_->is_plugin_a<snemo::geometry::locator_plugin>(locator_plugin_name),
                std::logic_error,
                "Found no locator plugin named '" << locator_plugin_name << "'");
    _locator_plugin_ = &_geometry_manager_->get_plugin<snemo::geometry::locator_plugin>(locator_plugin_name);

    _initialized_ = true;
    return;
  }

  void geometry_tools::reset()
  {
    _geometry_manager_ = 0;
    _locator_plugin_ = 0;
    _initialized_ = false;
    return;
  }

} // end of namespace utils
} // end of namespace snemo

// end of snemo_control_plot_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
