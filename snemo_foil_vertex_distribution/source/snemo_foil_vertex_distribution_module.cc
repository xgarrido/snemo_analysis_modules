// snemo_foil_vertex_distribution_module.cc

#include <stdexcept>
#include <sstream>
#include <numeric>

#include <snemo_foil_vertex_distribution_module.h>

// Third party:
// - Bayeux/datatools:
#include <datatools/service_manager.h>
// - Bayeux/mygsl
#include <mygsl/histogram_pool.h>
// - Bayeux/mtools
#include <mctools/utils.h>
#include <mctools/simulated_data.h>
// - Bayeux/dpp
#include <dpp/histogram_service.h>

// SuperNEMO event model
#include <falaise/snemo/processing/services.h>
#include <falaise/snemo/datamodels/data_model.h>
#include <falaise/snemo/datamodels/particle_track_data.h>


namespace analysis {

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_foil_vertex_distribution_module,
                                    "analysis::snemo_foil_vertex_distribution_module");

  // Set the histogram pool used by the module :
  void snemo_foil_vertex_distribution_module::set_histogram_pool(mygsl::histogram_pool & pool_)
  {
    DT_THROW_IF(is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is already initialized !");
    _histogram_pool_ = &pool_;
    return;
  }

  // Grab the histogram pool used by the module :
  mygsl::histogram_pool & snemo_foil_vertex_distribution_module::grab_histogram_pool()
  {
    DT_THROW_IF(! is_initialized(), std::logic_error,
                "Module '" << get_name() << "' is not initialized !");
    return *_histogram_pool_;
  }

  void snemo_foil_vertex_distribution_module::_set_defaults()
  {
    _bank_label_     = "";
    _histogram_pool_ = 0;
    return;
  }

  // Constructor :
  snemo_foil_vertex_distribution_module::snemo_foil_vertex_distribution_module(datatools::logger::priority logging_priority_)
    : dpp::base_module(logging_priority_)
  {
    _set_defaults();
    return;
  }

  // Destructor :
  snemo_foil_vertex_distribution_module::~snemo_foil_vertex_distribution_module()
  {
    if (is_initialized()) snemo_foil_vertex_distribution_module::reset();
    return;
  }

  // Initialization :
  void snemo_foil_vertex_distribution_module::initialize(const datatools::properties  & config_,
                                                         datatools::service_manager   & service_manager_,
                                                         dpp::module_handle_dict_type & module_dict_)
  {
    DT_THROW_IF(is_initialized(),
                std::logic_error,
                "Module '" << get_name() << "' is already initialized ! ");

    dpp::base_module::_common_initialize(config_);

    DT_THROW_IF(!config_.has_key("bank_label"), std::logic_error, "Missing bank label !");
    _bank_label_ = config_.fetch_string("bank_label");

    // Service label
    std::string histogram_label;
    if (config_.has_key("Histo_label"))
      {
        histogram_label = config_.fetch_string("Histo_label");
      }
    if (! _histogram_pool_)
      {
        DT_THROW_IF(histogram_label.empty(), std::logic_error,
                    "Module '" << get_name() << "' has no valid 'Histo_label' property !");

        DT_THROW_IF(! service_manager_.has(histogram_label) ||
                    ! service_manager_.is_a<dpp::histogram_service>(histogram_label),
                    std::logic_error,
                    "Module '" << get_name() << "' has no '" << histogram_label << "' service !");
        dpp::histogram_service & Histo
          = service_manager_.get<dpp::histogram_service>(histogram_label);
        set_histogram_pool(Histo.grab_pool());
        if (config_.has_key("Histo_output_files"))
          {
            std::vector<std::string> output_files;
            config_.fetch("Histo_output_files", output_files);
            for (size_t i = 0; i < output_files.size(); i++) {
              Histo.add_output_file(output_files[i]);
            }
          }
        if (config_.has_key("Histo_template_files"))
          {
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
  void snemo_foil_vertex_distribution_module::reset()
  {
    DT_THROW_IF (! is_initialized (),
                 std::logic_error,
                 "Module '" << get_name() << "' is not initialized !");

    // Dump result
    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG)
      {
        DT_LOG_DEBUG(get_logging_priority(), "snemo_foil_vertex_distribution_module::dump_result:");
        dump_result(std::clog);
      }

    _set_defaults();
    // Tag the module as un-initialized :
    _set_initialized(false);
    return;
  }

  // Processing :
  dpp::base_module::process_status
  snemo_foil_vertex_distribution_module::process(datatools::things & data_record_)
  {
    DT_THROW_IF (! is_initialized(), std::logic_error,
                 "Module '" << get_name() << "' is not initialized !");

    if (!data_record_.has(_bank_label_))
      {
        DT_LOG_ERROR(get_logging_priority(),
                     "Could not find any bank with label '" << _bank_label_ << "' !");
        return dpp::base_module::PROCESS_STOP;
      }

    const geomtools::vector_3d * vertex = 0;
    namespace sdm = snemo::datamodel;
    if (_bank_label_ == sdm::data_info::default_simulated_data_label())
      {
        // Check if the 'calibrated data' record bank is available :
        const mctools::simulated_data & sd
          = data_record_.get<mctools::simulated_data>(_bank_label_);

        if (sd.has_vertex()) vertex = &(sd.get_vertex());
      }
    else if (_bank_label_ == sdm::data_info::default_particle_track_data_label())
      {
        const sdm::particle_track_data & ptd
          = data_record_.get<sdm::particle_track_data>(_bank_label_);
        // Loop over all saved particles
        const sdm::particle_track_data::particle_collection_type & the_particles
          = ptd.get_particles ();
        for (sdm::particle_track_data::particle_collection_type::const_iterator
               iparticle = the_particles.begin();
             iparticle != the_particles.end();
             ++iparticle)
          {
            const sdm::particle_track & a_particle = iparticle->get();
            if (!a_particle.has_vertices()) continue;

            const sdm::particle_track::vertex_collection_type & the_vertices
              = a_particle.get_vertices ();
            for (sdm::particle_track::vertex_collection_type::const_iterator
                   ivertex = the_vertices.begin();
                 ivertex != the_vertices.end(); ++ivertex)
              {
                const geomtools::blur_spot & a_vertex = ivertex->get ();
                const datatools::properties & a_prop = a_vertex.get_auxiliaries ();

                if (!a_prop.has_flag("foil_vertex")) continue;

                vertex = &(a_vertex.get_position());
              }
          }
      }
    else
      {
        DT_THROW_IF(true, std::logic_error,
                    "Bank label '" << _bank_label_ << "' is not supported !");
      }

    if (vertex == 0)
      {
        DT_LOG_WARNING(get_logging_priority(), "No vertex has been set !");
        return dpp::base_module::PROCESS_STOP;
      }
    // Getting histogram pool
    mygsl::histogram_pool & a_pool = grab_histogram_pool();

    const std::string & key_str = "vertex_distribution";
    if (!a_pool.has(key_str))
      {
        mygsl::histogram_2d & h = a_pool.add_2d(key_str, "", "distribution");
        datatools::properties hconfig;
        hconfig.store_string("mode", "mimic");
        hconfig.store_string("mimic.histogram_2d", "vertex_distribution_template");
        mygsl::histogram_pool::init_histo_2d(h, hconfig, &a_pool);
      }

    // Getting the current histogram
    mygsl::histogram_2d & a_histo = a_pool.grab_2d(key_str);
    a_histo.fill(vertex->y(), vertex->z());

    return dpp::base_module::PROCESS_SUCCESS;
  }

  void snemo_foil_vertex_distribution_module::dump_result(std::ostream      & out_,
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
      // Histogram :
      out_ << indent << datatools::i_tree_dumpable::tag
           << "Vertex histograms : ";
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
          // const mygsl::histogram_1d & a_histogram = _histogram_pool_->get_1d (a_name);
          // a_histogram.tree_dump (out_, "", indent_oss.str (), inherit_);

          if (is_debug ())
            {
              //                  a_histogram.print (std::clog);
            }
        }
    }

    return;
  }

} // namespace analysis

// end of snemo_foil_vertex_distribution_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
