// snemo_foil_vertex_distribution_module.cc

#include <stdexcept>
#include <sstream>
#include <numeric>

#include <snanalysis/processing/snemo_vertex_distribution_module.h>

#include <datatools/utils/things_macros.h>
#include <datatools/utils/units.h>
#include <datatools/utils/utils.h>
#include <datatools/utils/ioutils.h>
#include <datatools/utils/properties.h>
#include <datatools/services/service_manager.h>

// SuperNEMO event model
#include <sncore/models/data_model.h>
#include <sncore/models/simulated_data.h>
#include <snanalysis/models/data_model.h>
#include <snanalysis/models/particle_track_data.h>

// SuperNEMO services
#include <sncore/services/histogram_service.h>
#include <mygsl/histogram_pool.h>

namespace snemo {

  namespace analysis {

    namespace processing {

      using namespace std;

      // Registration instantiation macro :
      SNEMO_MODULE_REGISTRATION_IMPLEMENT(snemo_vertex_distribution_module,
                                          "snemo::analysis::processing::snemo_vertex_distribution_module");

      // Set the label of the 'Histogram' service used by the module :
      void snemo_vertex_distribution_module::set_Histo_service_label (const std::string & Histo_label_)
      {
        _Histo_service_label_ = Histo_label_;
        return;
      }

      // Get the label of the 'Histogram' service used by the module :
      const std::string & snemo_vertex_distribution_module::get_Histo_service_label () const
      {
        return _Histo_service_label_;
      }

      // Set the histogram pool used by the module :
      void snemo_vertex_distribution_module::set_histogram_pool (mygsl::histogram_pool & pool_)
      {
        if (is_initialized ())
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_vertex_distribution_module::set_histogram_pool: "
                    << "Module '" << get_name () << "' is already initialized ! ";
            throw std::logic_error (message.str ());
          }
        _histogram_pool_ = &pool_;
        return;
      }

      // Grab the histogram pool used by the module :
      mygsl::histogram_pool & snemo_vertex_distribution_module::grab_histogram_pool ()
      {
        if (! is_initialized ())
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_vertex_distribution_module::grab_histogram_pool: "
                    << "Module '" << get_name () << "' is not initialized ! ";
            throw std::logic_error (message.str ());
          }
        return *_histogram_pool_;
      }

      void snemo_vertex_distribution_module::_init_defaults_ ()
      {
        _bank_label_ = "";

        _Histo_service_label_ = "";
        _histogram_pool_      = 0;

        return;
      }

      /*** Implementation of the interface ***/

      // Constructor :
      SNEMO_MODULE_CONSTRUCTOR_IMPLEMENT_HEAD (snemo_vertex_distribution_module,
                                               debug_level_,
                                               "snemo_vertex_distribution_module",
                                               "An module processor to plot vertices position",
                                               "0.1")
      {
        _init_defaults_ ();
        return;
      }

      // Destructor :
      SNEMO_MODULE_DESTRUCTOR_IMPLEMENT_HEAD (snemo_vertex_distribution_module)
      {
        // Make sure all internal resources are terminated
        // before destruction :
        if (is_initialized ()) reset ();
        return;
      }

      // Initialization :
      SNEMO_MODULE_INITIALIZE_IMPLEMENT_HEAD (snemo_vertex_distribution_module,
                                              config_,
                                              service_manager_,
                                              module_dict_)
      {
        if (is_initialized ())
          {
            ostringstream message;
            message << "snemo_vertex_distribution_module::initialize: "
                    << "Module '" << get_name () << "' is already initialized ! ";
            throw logic_error (message.str ());
          }

        /**************************************************************
         *   fetch setup parameters from the configuration container  *
         **************************************************************/

        if (! is_debug ())
          {
            if (config_.has_flag ("debug"))
              {
                set_debug (true);
              }
          }

        if (_bank_label_.empty ())
          {
            if (config_.has_key ("bank_label"))
              {
                _bank_label_ = config_.fetch_string ("bank_label");
              }
          }

        if (_histogram_pool_ == 0)
          {
            if (_Histo_service_label_.empty ())
              {
                if (config_.has_key ("Histo_label"))
                  {
                    const std::string label = config_.fetch_string ("Histo_label");
                    set_Histo_service_label (label);
                  }
              }
            if (service_manager_.has (_Histo_service_label_) &&
                service_manager_.is_a<snemo::core::service::histogram_service> (_Histo_service_label_))
              {
                snemo::core::service::histogram_service & Histo
                  = service_manager_.get<snemo::core::service::histogram_service> (_Histo_service_label_);
                set_histogram_pool (Histo.grab_pool ());
              }
            else
              {
                std::ostringstream message;
                message << "snemo::analysis::processing::"
                        << "snemo_vertex_distribution_module::initialize: "
                        << "Module '" << get_name ()
                        << "' has no '" << _Histo_service_label_ << "' service !";
                throw std::logic_error (message.str ());
              }
          }

        if (_histogram_pool_ == 0)
          {
            std::ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_vertex_distribution_module::initialize: "
                    << "Missing histogram pool !";
            throw std::logic_error (message.str ());
          }


       /*************************************
         *  end of the initialization step   *
         *************************************/

        // Tag the module as initialized :
        _set_initialized (true);
        return;
      }

      // Reset :
      SNEMO_MODULE_RESET_IMPLEMENT_HEAD (snemo_vertex_distribution_module)
      {
        if (! is_initialized ())
          {
            ostringstream message;
            message << "snemo::analysis::processing::"
                    << "snemo_vertex_distribution_module::initialize: "
                    << "Module '" << get_name () << "' is not initialized !";
            throw logic_error (message.str ());
          }

        /****************************
         *  revert to some defaults *
         ****************************/

        this->_init_defaults_ ();

        /****************************
         *  end of the reset step   *
         ****************************/

        // Tag the module as un-initialized :
        _set_initialized (false);
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
