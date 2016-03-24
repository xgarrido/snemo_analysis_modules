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

    const geomtools::manager & geo_mgr = Geo.get_geom_manager();
    std::string locator_plugin_name;
    if (config_.has_key("locator_plugin_name")) {
      locator_plugin_name = config_.fetch_string("locator_plugin_name");
    } else {
      // If no locator plugin name is set, then search for the first one
      const geomtools::manager::plugins_dict_type & plugins = geo_mgr.get_plugins();
      for (geomtools::manager::plugins_dict_type::const_iterator ip = plugins.begin();
           ip != plugins.end();
           ip++) {
        const std::string & plugin_name = ip->first;
        if (geo_mgr.is_plugin_a<snemo::geometry::locator_plugin>(plugin_name)) {
          locator_plugin_name = plugin_name;
          break;
        }
      }
    }
    // Access to a given plugin by name and type :
    DT_THROW_IF(! geo_mgr.has_plugin(locator_plugin_name) ||
                ! geo_mgr.is_plugin_a<snemo::geometry::locator_plugin>(locator_plugin_name),
                std::logic_error,
                "Found no locator plugin named '" << locator_plugin_name << "'");
    // _locator_plugin_ = &geo_mgr.get_plugin<snemo::geometry::locator_plugin>(locator_plugin_name);

    return;
  }

  void geometry_tools::reset()
  {
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
