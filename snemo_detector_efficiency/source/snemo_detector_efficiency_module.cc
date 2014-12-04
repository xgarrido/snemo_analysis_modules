// snemo_detector_efficiency_module.cc

#include <stdexcept>
#include <sstream>
#include <set>
#include <numeric>

#include <snemo_detector_efficiency_module.h>

#include <boost/bind.hpp>

// Third party:
// - Bayeux/datatools:
#include <datatools/service_manager.h>
// - Bayeux/geomtools:
#include <geomtools/geometry_service.h>
#include <geomtools/manager.h>

// SuperNEMO event model
#include <falaise/snemo/processing/services.h>
#include <falaise/snemo/datamodels/data_model.h>
#include <falaise/snemo/datamodels/calibrated_data.h>
#include <falaise/snemo/datamodels/tracker_trajectory.h>
#include <falaise/snemo/datamodels/particle_track_data.h>

// Geometry manager
#include <falaise/snemo/geometry/locator_plugin.h>
#include <falaise/snemo/geometry/calo_locator.h>
#include <falaise/snemo/geometry/xcalo_locator.h>
#include <falaise/snemo/geometry/gveto_locator.h>
#include <falaise/snemo/geometry/gg_locator.h>

namespace analysis {

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_detector_efficiency_module,
                                    "analysis::snemo_detector_efficiency_module");

  void snemo_detector_efficiency_module::_set_defaults()
  {
    _bank_label_        = "";
    _output_filename_   = "";

    _calo_efficiencies_.clear();
    _gg_efficiencies_.clear();

    return;
  }

  // Constructor :
  snemo_detector_efficiency_module::snemo_detector_efficiency_module(datatools::logger::priority logging_priority_)
    : dpp::base_module(logging_priority_)
  {
    _set_defaults();
    return;
  }

  // Destructor :
  snemo_detector_efficiency_module::~snemo_detector_efficiency_module()
  {
    // Make sure all internal resources are terminated
    // before destruction :
    if (is_initialized()) reset();
    return;
  }

  // Reset :
  void snemo_detector_efficiency_module::reset()
  {
    DT_THROW_IF(! is_initialized(),
                std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    // Compute efficiency
    _compute_efficiency();

    // Dump result
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG)
      {
        DT_LOG_DEBUG(get_logging_priority(), "snemo_detector_efficiency_module::dump_result:");
        dump_result(std::clog);
      }

    _set_defaults();
    // Tag the module as un-initialized :
    _set_initialized(false);
    return;
  }

  // Initialization :
  void snemo_detector_efficiency_module::initialize(const datatools::properties  & config_,
                                                    datatools::service_manager   & service_manager_,
                                                    dpp::module_handle_dict_type & module_dict_)
  {
    DT_THROW_IF(is_initialized(),
                std::logic_error,
                "Module '" << get_name() << "' is already initialized ! ");

    dpp::base_module::_common_initialize(config_);

    DT_THROW_IF(!config_.has_key("bank_label"), std::logic_error, "Missing bank label !");
    _bank_label_ = config_.fetch_string("bank_label");

    DT_THROW_IF(!config_.has_key("output_filename"), std::logic_error, "Missing output filename !");
    _output_filename_ = config_.fetch_string("output_filename");
    datatools::fetch_path_with_env(_output_filename_);

    // Geometry manager :
    std::string geo_label = snemo::processing::service_info::default_geometry_service_label();
    if (config_.has_key("Geo_label")) {
      geo_label = config_.fetch_string("Geo_label");
    }
    DT_THROW_IF(geo_label.empty(), std::logic_error,
                "Module '" << get_name() << "' has no valid '" << "Geo_label" << "' property !");
    DT_THROW_IF(! service_manager_.has(geo_label) ||
                ! service_manager_.is_a<geomtools::geometry_service>(geo_label),
                std::logic_error,
                "Module '" << get_name() << "' has no '" << geo_label << "' service !");
    geomtools::geometry_service & Geo
      = service_manager_.get<geomtools::geometry_service>(geo_label);

    // Get geometry locator plugin
    const geomtools::manager & geo_mgr = Geo.get_geom_manager();
    std::string locator_plugin_name;
    if (config_.has_key("locator_plugin_name")) {
      locator_plugin_name = config_.fetch_string("locator_plugin_name");
    } else {
      // If no locator plugin name is set, then search for the first one
      const geomtools::manager::plugins_dict_type & plugins = geo_mgr.get_plugins();
      for (geomtools::manager::plugins_dict_type::const_iterator ip = plugins.begin();
           ip != plugins.end(); ip++) {
        const std::string & plugin_name = ip->first;
        if (geo_mgr.is_plugin_a<snemo::geometry::locator_plugin>(plugin_name)) {
          DT_LOG_DEBUG(get_logging_priority(), "Find locator plugin with name = " << plugin_name);
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
    _locator_plugin_ = &geo_mgr.get_plugin<snemo::geometry::locator_plugin>(locator_plugin_name);

    // Tag the module as initialized :
    _set_initialized(true);
    return;
  }

  // Processing :
  dpp::base_module::process_status
  snemo_detector_efficiency_module::process(datatools::things & data_record_)
  {
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    // Check if the record bank is available :
    if (!data_record_.has(_bank_label_))
      {
        DT_LOG_ERROR(get_logging_priority(),
                     "Could not find any bank with label '" << _bank_label_ << "' !");
        return dpp::base_module::PROCESS_STOP;
      }
    namespace sdm = snemo::datamodel;
    if (_bank_label_ == sdm::data_info::default_calibrated_data_label())
      {
        const sdm::calibrated_data & cd
          = data_record_.get<sdm::calibrated_data>(_bank_label_);

        const sdm::calibrated_data::calorimeter_hit_collection_type & the_calo_hits
          = cd.calibrated_calorimeter_hits();
        const sdm::calibrated_data::tracker_hit_collection_type & the_tracker_hits
          = cd.calibrated_tracker_hits();

        for (sdm::calibrated_data::calorimeter_hit_collection_type::const_iterator
               i = the_calo_hits.begin();
             i != the_calo_hits.end(); ++i)
          {
            const sdm::calibrated_calorimeter_hit & a_hit = i->get();
            _calo_efficiencies_[a_hit.get_geom_id()]++;
          }
        for (sdm::calibrated_data::tracker_hit_collection_type::const_iterator
               i = the_tracker_hits.begin();
             i != the_tracker_hits.end(); ++i)
          {
            const sdm::calibrated_tracker_hit & a_hit = i->get();
            _gg_efficiencies_[a_hit.get_geom_id()]++;
          }
      }
    else if (_bank_label_ == sdm::data_info::default_particle_track_data_label())
      {
        const sdm::particle_track_data & ptd
          = data_record_.get<sdm::particle_track_data>(_bank_label_);

        // Store geom_id to avoid double inclusion of calorimeter hits
        std::set<geomtools::geom_id> gids;

        // Loop over all saved particles
        const sdm::particle_track_data::particle_collection_type & the_particles
          = ptd.get_particles();

        for (sdm::particle_track_data::particle_collection_type::const_iterator
               iparticle = the_particles.begin();
             iparticle != the_particles.end();
             ++iparticle)
          {
            const sdm::particle_track & a_particle = iparticle->get();

            if (!a_particle.has_associated_calorimeter_hits()) continue;

            const sdm::calibrated_calorimeter_hit::collection_type & the_calorimeters
              = a_particle.get_associated_calorimeter_hits();

            if (the_calorimeters.size() > 2)
              {
                DT_LOG_DEBUG(get_logging_priority(),
                             "The particle is associated to more than 2 calorimeters !");
                continue;
              }

            for (size_t i = 0; i < the_calorimeters.size(); ++i)
              {
                const geomtools::geom_id & a_gid = the_calorimeters.at(i).get().get_geom_id();
                if (gids.find(a_gid) != gids.end()) continue;
                gids.insert(a_gid);
                _calo_efficiencies_[a_gid]++;
              }

            // Get trajectory and attached geiger cells
            const sdm::tracker_trajectory & a_trajectory = a_particle.get_trajectory();
            const sdm::tracker_cluster & a_cluster = a_trajectory.get_cluster();
            const sdm::calibrated_tracker_hit::collection_type & the_hits = a_cluster.get_hits();
            for (size_t i = 0; i < the_hits.size(); ++i)
              {
                const geomtools::geom_id & a_gid = the_hits.at(i).get().get_geom_id();
                if (gids.find(a_gid) != gids.end()) continue;
                gids.insert(a_gid);
                _gg_efficiencies_[a_gid]++;
              }
          }
      }
    else
      {
        DT_THROW_IF(true, std::logic_error,
                    "Bank label '" << _bank_label_ << "' is not supported !");
      }
    return dpp::base_module::PROCESS_SUCCESS;
  }

  void snemo_detector_efficiency_module::_compute_efficiency()
  {
    // Handling geom_id is done in this place where geom_id are split into
    // main wall, xwall and gveto calorimeters. For such task we use
    // sngeometry locators

    std::ofstream fout(_output_filename_.c_str());
    {
      efficiency_dict::const_iterator found =
        std::max_element(_calo_efficiencies_.begin(), _calo_efficiencies_.end(),
                         (boost::bind(&efficiency_dict::value_type::second, _1) <
                          boost::bind(&efficiency_dict::value_type::second, _2)));
      const int calo_total = found->second;

      for (efficiency_dict::const_iterator i = _calo_efficiencies_.begin();
           i != _calo_efficiencies_.end(); ++i)
        {
          const geomtools::geom_id & a_gid = i->first;

          const snemo::geometry::calo_locator & calo_locator = _locator_plugin_->get_calo_locator();
          if (calo_locator.is_calo_block_in_current_module(a_gid))
            {
              fout << "calo ";
              geomtools::vector_3d position;
              calo_locator.get_block_position(a_gid, position);
              fout << position.x() << " "
                   << position.y() << " "
                   << position.z() << " ";
            }
          const snemo::geometry::xcalo_locator & xcalo_locator = _locator_plugin_->get_xcalo_locator();
          if (xcalo_locator.is_calo_block_in_current_module(a_gid))
            {
              fout << "xcalo ";
              geomtools::vector_3d position;
              xcalo_locator.get_block_position(a_gid, position);
              fout << position.x() << " "
                   << position.y() << " "
                   << position.z() << " ";
            }
          const snemo::geometry::gveto_locator & gveto_locator = _locator_plugin_->get_gveto_locator();
          if (gveto_locator.is_calo_block_in_current_module(a_gid))
            {
              fout << "gveto ";
              geomtools::vector_3d position;
              gveto_locator.get_block_position(a_gid, position);
              fout << position.x() << " "
                   << position.y() << " "
                   << position.z() << " ";
            }
          fout << i->second/double(calo_total) << std::endl;
        }
    }

    {
      efficiency_dict::const_iterator found =
        std::max_element(_gg_efficiencies_.begin(), _gg_efficiencies_.end(),
                         (boost::bind(&efficiency_dict::value_type::second, _1) <
                          boost::bind(&efficiency_dict::value_type::second, _2)));
      const int gg_total = found->second;

      for (efficiency_dict::const_iterator i = _gg_efficiencies_.begin();
           i != _gg_efficiencies_.end(); ++i)
        {
          const geomtools::geom_id & a_gid = i->first;

          const snemo::geometry::gg_locator & gg_locator
            = dynamic_cast<const snemo::geometry::gg_locator&>(_locator_plugin_->get_gg_locator());
          if (gg_locator.is_drift_cell_volume_in_current_module(a_gid))
            {
              fout << "gg ";
              geomtools::vector_3d position;
              gg_locator.get_cell_position(a_gid, position);
              fout << position.x() << " " << position.y() << " ";
            }
          fout << i->second/double(gg_total) << std::endl;
        }
    }

    return;
  }

  void snemo_detector_efficiency_module::dump_result(std::ostream      & out_,
                                                     const std::string & title_,
                                                     const std::string & indent_,
                                                     bool inherit_) const
  {
    std::string indent;
    if (! indent_.empty())
      {
        indent = indent_;
      }
    if ( !title_.empty() )
      {
        out_ << indent << title_ << std::endl;
      }

    {
      out_ << indent << datatools::i_tree_dumpable::tag
           << "Calorimeters efficiency [" << _calo_efficiencies_.size() << "]"
           << std::endl;
      for (efficiency_dict::const_iterator i = _calo_efficiencies_.begin();
           i != _calo_efficiencies_.end(); ++i)
        {
          out_ << indent << datatools::i_tree_dumpable::skip_tag;
          efficiency_dict::const_iterator j = i;
          if (++j == _calo_efficiencies_.end())
            {
              out_ << datatools::i_tree_dumpable::last_tag;
            }
          else
            {
              out_ << datatools::i_tree_dumpable::tag;
            }
          out_ << i->first << " = " << i->second << std::endl;
        }
    }
    {
      out_ << indent << datatools::i_tree_dumpable::last_tag
           << "Tracker efficiency [" << _gg_efficiencies_.size() << "]" << std::endl;
      for (efficiency_dict::const_iterator i = _gg_efficiencies_.begin();
           i != _gg_efficiencies_.end(); ++i)
        {
          out_ << indent << datatools::i_tree_dumpable::last_skip_tag;
          efficiency_dict::const_iterator j = i;
          if (++j == _gg_efficiencies_.end())
            {
              out_ << datatools::i_tree_dumpable::last_tag;
            }
          else
            {
              out_ << datatools::i_tree_dumpable::tag;
            }
          out_ << i->first << " = " << i->second << std::endl;
        }
    }

    return;
  }

} // namespace analysis

// end of snemo_detector_efficiency_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
