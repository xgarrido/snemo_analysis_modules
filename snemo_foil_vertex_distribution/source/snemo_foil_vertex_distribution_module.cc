// snemo_foil_vertex_distribution_module.cc

#include <stdexcept>
#include <sstream>
#include <numeric>

#include <snemo_foil_vertex_distribution_module.h>

// Third party:
// - Bayeux/datatools:
#include <datatools/services/service_manager.h>
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
  void snemo_detector_efficiency_module::reset()
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

  SNEMO_MODULE_PROCESS_IMPLEMENT_HEAD (snemo_vertex_distribution_module,
                                       event_record_)
  {
    if (! is_initialized ())
      {
        ostringstream message;
        message << "snemo::analysis::processing::"
                << "snemo_vertex_distribution_module::process: "
                << "Module '" << get_name () << "' is not initialized !";
        throw logic_error (message.str ());
      }

    if (_bank_label_ == snemo::core::model::data_info::SIMULATED_DATA_LABEL)
      {
        // Check if the 'calibrated data' record bank is available :
        namespace scm = snemo::core::model;
        if (! DATATOOLS_UTILS_THINGS_CHECK_BANK (event_record_, _bank_label_, scm::simulated_data))
          {
            std::clog << datatools::utils::io::error
                      << "snemo::analysis::processing::"
                      << "snemo_detector_efficiency_module::process: "
                      << "Could not find any bank with label '"
                      << _bank_label_ << "' !"
                      << std::endl;
            return STOP;
          }
        DATATOOLS_UTILS_THINGS_CONST_BANK (event_record_, _bank_label_, scm::simulated_data, sd);

        if (sd.has_vertex ())
          {
            const geomtools::vector_3d & a_vertex = sd.get_vertex ();
            // Getting histogram pool
            mygsl::histogram_pool & a_pool = grab_histogram_pool ();

            const std::string & key_str = "vertex_distribution";
            if (!a_pool.has (key_str))
              {
                mygsl::histogram_2d & h = a_pool.add_2d (key_str, "", "distribution");
                datatools::utils::properties hconfig;
                hconfig.store_string ("mode", "mimic");
                hconfig.store_string ("mimic.histogram_2d", "vertex_distribution_template");
                mygsl::histogram_pool::init_histo_2d (h, hconfig, &a_pool);
              }

            // Getting the current histogram
            mygsl::histogram_2d & a_histo = a_pool.grab_2d (key_str);
            a_histo.fill (a_vertex.y (), a_vertex.z ());
          }
      }
    else if (_bank_label_ == snemo::analysis::model::data_info::PARTICLE_TRACK_DATA_LABEL)
      {
        namespace sam = snemo::analysis::model;
        // Check if the 'particle track' event record bank is available :
        if (! DATATOOLS_UTILS_THINGS_CHECK_BANK (event_record_, _bank_label_, sam::particle_track_data))
          {
            clog << datatools::utils::io::error
                 << "snemo::analysis::processing::"
                 << "snemo_vertex_distribution_module::process: "
                 << "Could not find any bank with label '"
                 << _bank_label_ << "' !"
                 << endl;
            return STOP;
          }
        // Get a const reference to this event record bank of interest :
        DATATOOLS_UTILS_THINGS_CONST_BANK (event_record_, _bank_label_, sam::particle_track_data, ptd);

        if (is_debug ())
          {
            clog << datatools::utils::io::debug
                 << "snemo::analysis::processing::"
                 << "snemo_vertex_distribution_module::process: "
                 << "Particle track data : " << endl;
            ptd.tree_dump (clog, "", "DEBUG: ");
          }

        // Loop over all saved particles
        const sam::particle_track_data::particle_collection_type & the_particles
          = ptd.get_particles ();

        for (sam::particle_track_data::particle_collection_type::const_iterator
               iparticle = the_particles.begin ();
             iparticle != the_particles.end ();
             ++iparticle)
          {
            const sam::particle_track & a_particle = iparticle->get ();

            if (!a_particle.has_negative_charge ()) continue;

            if (!a_particle.has_vertices ()) continue;

            const sam::particle_track::vertex_collection_type & the_vertices
              = a_particle.get_vertices ();

            for (sam::particle_track::vertex_collection_type::const_iterator
                   ivertex = the_vertices.begin ();
                 ivertex != the_vertices.end (); ++ivertex)
              {
                const geomtools::blur_spot & a_vertex = ivertex->get ();
                const datatools::utils::properties & a_prop = a_vertex.get_auxiliaries ();

                if (!a_prop.has_flag ("foil_vertex")) continue;

                // Getting histogram pool
                mygsl::histogram_pool & a_pool = grab_histogram_pool ();

                const std::string & key_str = "vertex_distribution";
                if (!a_pool.has (key_str))
                  {
                    mygsl::histogram_2d & h = a_pool.add_2d (key_str, "", "distribution");
                    datatools::utils::properties hconfig;
                    hconfig.store_string ("mode", "mimic");
                    hconfig.store_string ("mimic.histogram_2d", "vertex_distribution_template");
                    mygsl::histogram_pool::init_histo_2d (h, hconfig, &a_pool);
                  }

                // Getting the current histogram
                mygsl::histogram_2d & a_histo = a_pool.grab_2d (key_str);
                a_histo.fill (a_vertex.get_position ().y (), a_vertex.get_position ().z ());

              }
          }
      }

    return SUCCESS;
  }

  void snemo_vertex_distribution_module::_dump_result_ (ostream      & out_,
                                                        const string & title_,
                                                        const string & indent_,
                                                        bool inherit_) const
  {
    string indent;
    if (! indent_.empty ())
      {
        indent = indent_;
      }
    if ( !title_.empty () )
      {
        out_ << indent << title_ << endl;
      }
    namespace du = datatools::utils;

    {
      // Histogram :
      out_ << indent << du::i_tree_dumpable::tag
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
              out_  << du::i_tree_dumpable::last_tag;
              indent_oss << indent << du::i_tree_dumpable::last_skip_tag;
            }
          else
            {
              out_ << du::i_tree_dumpable::tag;
              indent_oss << indent << du::i_tree_dumpable::skip_tag;
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

} // namespace processing

} // namespace analysis

} // namespace snemo

// end of snemo_vertex_distribution_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
