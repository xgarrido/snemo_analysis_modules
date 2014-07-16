// snemo_vertex_resolution_module.cc

// Ourselves:
#include <snemo_vertex_resolution_module.h>

// Standard library:
#include <stdexcept>
#include <sstream>
#include <numeric>

// Third party:
// - Bayeux/datatools:
#include <datatools/clhep_units.h>
#include <datatools/service_manager.h>
// - Bayeux/mygsl
#include <mygsl/histogram_pool.h>
// - Bayeux/dpp
#include <dpp/histogram_service.h>
// - Bayeux/mctools
#include <mctools/simulated_data.h>

// SuperNEMO event model
#include <snemo/datamodels/data_model.h>
#include <snemo/datamodels/particle_track_data.h>

namespace analysis {

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_vertex_resolution_module,
                                    "analysis::snemo_vertex_resolution_module");

  // Set the histogram pool used by the module :
  void snemo_vertex_resolution_module::set_histogram_pool(mygsl::histogram_pool & pool_)
  {
    DT_THROW_IF(is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is already initialized !");
    _histogram_pool_ = &pool_;
    return;
  }

  // Grab the histogram pool used by the module :
  mygsl::histogram_pool & snemo_vertex_resolution_module::grab_histogram_pool()
  {
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");
    return *_histogram_pool_;
  }

  void snemo_vertex_resolution_module::_set_defaults()
  {
    _histogram_pool_ = 0;
    return;
  }

  // Constructor :
  snemo_vertex_resolution_module::snemo_vertex_resolution_module(datatools::logger::priority logging_priority_)
    : dpp::base_module(logging_priority_)
  {
    _set_defaults();
    return;
  }

  // Destructor :
  snemo_vertex_resolution_module::~snemo_vertex_resolution_module()
  {
    if (is_initialized()) snemo_vertex_resolution_module::reset();
    return;
  }

  // Initialization :
  void snemo_vertex_resolution_module::initialize(const datatools::properties  & config_,
                                                  datatools::service_manager   & service_manager_,
                                                  dpp::module_handle_dict_type & module_dict_)
  {
    DT_THROW_IF(is_initialized(),
                std::logic_error,
                "Module '" << get_name() << "' is already initialized ! ");

    dpp::base_module::_common_initialize(config_);

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
      if (config_.has_key("Histo_template_files")) {
        std::vector<std::string> template_files;
        config_.fetch("Histo_template_files", template_files);
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
  void snemo_vertex_resolution_module::reset()
  {
    DT_THROW_IF(! is_initialized(),
                std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    // Dump result
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) {
      DT_LOG_NOTICE(get_logging_priority (), "Vertex resolution module dump: ");
      dump_result();
    }

    _set_defaults();

    // Tag the module as un-initialized :
    _set_initialized (false);
    return;
  }

  // Processing :
  dpp::base_module::process_status snemo_vertex_resolution_module::process(datatools::things & data_record_)
  {
    DT_LOG_TRACE(get_logging_priority(), "Entering...");
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");

    // Check if the 'simulated data' record bank is available :
    const std::string sd_label = snemo::datamodel::data_info::default_simulated_data_label();
    if (! data_record_.has(sd_label)) {
      DT_LOG_ERROR(get_logging_priority(), "Could not find any bank with label '"
                   << sd_label << "' !");
      return dpp::base_module::PROCESS_STOP;
    }
    const mctools::simulated_data & sd
      = data_record_.get<mctools::simulated_data>(sd_label);

    // Check if the 'particle track data' record bank is available :
    const std::string ptd_label = snemo::datamodel::data_info::default_particle_track_data_label();
    if (! data_record_.has(ptd_label)) {
      DT_LOG_ERROR(get_logging_priority (), "Could not find any bank with label '"
                   << ptd_label << "' !");
      return dpp::base_module::PROCESS_STOP;
    }
    const snemo::datamodel::particle_track_data & ptd
      = data_record_.get<snemo::datamodel::particle_track_data>(ptd_label);

    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG) {
      DT_LOG_DEBUG(get_logging_priority(), "Simulated data : ");
      sd.tree_dump();
      DT_LOG_DEBUG(get_logging_priority(), "Particle track data : ");
      ptd.tree_dump();
    }

    // Loop over all saved particles
    const snemo::datamodel::particle_track_data::particle_collection_type & the_particles
      = ptd.get_particles();

    for (snemo::datamodel::particle_track_data::particle_collection_type::const_iterator
           iparticle = the_particles.begin();
         iparticle != the_particles.end(); ++iparticle) {
      const snemo::datamodel::particle_track & a_particle = iparticle->get();
      const snemo::datamodel::particle_track::vertex_collection_type & the_vertices
        = a_particle.get_vertices();

      geomtools::vector_3d delta;
      geomtools::invalidate(delta);
      for (snemo::datamodel::particle_track::vertex_collection_type::const_iterator
             ivertex = the_vertices.begin();
           ivertex != the_vertices.end(); ++ivertex) {
        const geomtools::blur_spot & a_vertex = ivertex->get();
        const datatools::properties & aux = a_vertex.get_auxiliaries();

        if (!aux.has_key(snemo::datamodel::particle_track::vertex_type_key())) {
          DT_LOG_WARNING(get_logging_priority(), "Current vertex has no vertex type !");
          continue;
        }
        std::string vname = aux.fetch_string(snemo::datamodel::particle_track::vertex_type_key());

        // Calculate delta vertex
        if (snemo::datamodel::particle_track::vertex_is_on_source_foil(a_vertex)) {
          delta = a_vertex.get_position() - sd.get_vertex();
        } else {
          // Getting the simulated hit inside the calorimeter
          const mctools::simulated_data::hit_handle_collection_type * ptr_collection = 0;
          if (sd.has_step_hits(vname)) {
            ptr_collection = &sd.get_step_hits(vname);
          } else {
            DT_LOG_WARNING(get_logging_priority(), "Simulated data has not step hit associated to '" << vname << "' category ! Skip !");
            continue;
          }
          const mctools::simulated_data::hit_handle_collection_type & hits = *ptr_collection;
          if (hits.size() != 1) {
            DT_LOG_WARNING(get_logging_priority(), "More than one energy deposit ! Skip !");
            continue;
          }
          // Get the first calorimeter simulated step hit i.e. the one at
          // the entrance of the calorimeter block
          const mctools::base_step_hit & the_step_hit = hits.front().get();
          const geomtools::geom_id & the_step_hit_gid = the_step_hit.get_geom_id();

          // Get the associated calorimeter list
          const snemo::datamodel::calibrated_calorimeter_hit::collection_type calos
            = a_particle.get_associated_calorimeter_hits();
          if (calos.size() != 1) {
            DT_LOG_WARNING(get_logging_priority(), "More than one calorimeter associated to the particle track ! Skip !");
            continue;
          }
          const geomtools::geom_id & the_calo_gid = calos.front().get().get_geom_id();
          if (! geomtools::geom_id::match (the_calo_gid, the_step_hit_gid)) {
            DT_LOG_WARNING(get_logging_priority(),
                           "Simulated calorimeter does not match associated calorimeter ! Skip !");
            continue;
          }

          delta = a_vertex.get_position() - the_step_hit.get_position_start();
        }
        DT_LOG_DEBUG(get_logging_priority(), "Delta value = " << delta/CLHEP::mm << " mm");

        DT_THROW_IF(!geomtools::is_valid(delta), std::logic_error,
                    "Something gets wrong when vertex difference has been calculated");

        // Getting histogram pool
        mygsl::histogram_pool & a_pool = grab_histogram_pool();
        const std::string label[3] = { "x", "y", "z"};
        for (size_t i = 0; i < 3; i++) {
          std::ostringstream key;
          key << sd.get_primary_event().get_total_kinetic_energy()/CLHEP::keV << "keV_";
          std::ostringstream group;
          group << vname << "_" << label[i] << "_position";
          key << group.str();
          if (! a_pool.has(key.str())) {
            mygsl::histogram_1d & h = a_pool.add_1d(key.str(), "", group.str());
            datatools::properties hconfig;
            hconfig.store_string("mode", "mimic");
            hconfig.store_string("mimic.histogram_1d", "delta_" + label[i]);
            mygsl::histogram_pool::init_histo_1d(h, hconfig, &a_pool);
          }
          // Getting the current histogram
          mygsl::histogram_1d & a_histo = a_pool.grab_1d(key.str ());
          if (label[i] == "x") a_histo.fill(delta.x());
          if (label[i] == "y") a_histo.fill(delta.y());
          if (label[i] == "z") a_histo.fill(delta.z());
        }
      }// end of vertex list
    }// end of particle list

    return dpp::base_module::PROCESS_SUCCESS;
  }

  void snemo_vertex_resolution_module::dump_result(std::ostream      & out_,
                                                   const std::string & title_,
                                                   const std::string & indent_,
                                                   bool inherit_) const
  {
    std::string indent;
    if (! indent_.empty ()) {
      indent = indent_;
    }
    if (!title_.empty ()) {
      out_ << indent << title_ << std::endl;
    }

    // Histogram :
    out_ << indent << datatools::i_tree_dumpable::tag
         << "Vertex resolution histograms : ";
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
      const std::string & a_group = _histogram_pool_->get_group(a_name);
      if (a_group.find("template") != std::string::npos) continue;

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
        DT_LOG_DEBUG(get_logging_priority(), "Histogram '" << a_name << "' dump :");
        a_histogram.print(std::clog);
      }
    }

    return;
  }

} // namespace analysis

// end of snemo_vertex_resolution_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
