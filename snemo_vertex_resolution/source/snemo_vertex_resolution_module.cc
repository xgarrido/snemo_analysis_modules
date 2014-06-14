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

// SuperNEMO event model
#include <snemo/datamodels/data_model.h>
#include <snemo/datamodels/event_header.h>
#include <mctools/simulated_data.h>
#include <snemo/datamodels/particle_track_data.h>

namespace analysis {

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(snemo_vertex_resolution_module,
                                    "analysis::snemo_vertex_resolution_module");

  void snemo_vertex_resolution_module::set_parameter_label (const std::string & parameter_)
  {
    _parameter_ = parameter_;
    return;
  }

  const std::string & snemo_vertex_resolution_module::get_parameter_label () const
  {
    return _parameter_;
  }

  void snemo_vertex_resolution_module::_set_defaults ()
  {
    _parameter_      = "";

    _vertex_histograms_.clear ();
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

    if (_parameter_.empty ())
      {
        // If the label is not already setup, pickup from the configuration list:
        if (config_.has_key ("parameter"))
          {
            const std::string label = config_.fetch_string ("parameter");
            set_parameter_label (label);
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

    // // Generate ROOT plots
    // _generate_plots_ ();

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

    // Check if the 'event header' record bank is available :
    const std::string sd_label = snemo::datamodel::data_info::default_simulated_data_label();
    if (! data_record_.has(sd_label))
      {
        DT_LOG_ERROR(get_logging_priority(), "Could not find any bank with label '"
                     << sd_label << "' !");
        return dpp::base_module::PROCESS_STOP;
      }
    const mctools::simulated_data & sd
      = data_record_.get<mctools::simulated_data>(sd_label);

    // Check if the 'particle track' record bank is available :
    const std::string ptd_label = snemo::datamodel::data_info::default_particle_track_data_label();
    if (! data_record_.has(ptd_label))
      {
        DT_LOG_ERROR(get_logging_priority (), "Could not find any bank with label '"
                     << ptd_label << "' !");
        return dpp::base_module::PROCESS_STOP;
      }
    const snemo::datamodel::particle_track_data & ptd
      = data_record_.get<snemo::datamodel::particle_track_data>(ptd_label);

    if (get_logging_priority() >= datatools::logger::PRIO_DEBUG)
      {
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
         iparticle != the_particles.end(); ++iparticle)
      {
        const snemo::datamodel::particle_track & a_particle = iparticle->get();

    //     if (!a_particle.has_negative_charge ()) continue;

    //     if (!a_particle.has_vertices ()) continue;

        const snemo::datamodel::particle_track::vertex_collection_type & the_vertices
          = a_particle.get_vertices();

        geomtools::vector_3d delta;
        geomtools::invalidate(delta);
        for (snemo::datamodel::particle_track::vertex_collection_type::const_iterator
               ivertex = the_vertices.begin();
             ivertex != the_vertices.end(); ++ivertex)
          {
            const geomtools::blur_spot & a_vertex = ivertex->get();
            const datatools::properties & aux = a_vertex.get_auxiliaries();

            std::string vname;
            if (aux.has_flag("foil_vertex"))
              {
                vname = "foil_vertex";
              }
            else if (aux.has_flag("calo"))
              {
                vname = "calo";
              }
            else if (aux.has_flag("xcalo"))
              {
                vname = "xcalo";
              }
            else if (aux.has_flag("gveto"))
              {
                vname = "gveto";
              }
            else
              {
                DT_LOG_WARNING(get_logging_priority(),
                               "Vertex " << a_vertex.get_position()
                               << " is not associated to any special part of the detector ! Skip !");
                continue;
              }

            // Calculate delta vertex
            if (vname == "foil_vertex")
              {
                delta = a_vertex.get_position() - sd.get_vertex();
              }
            else
              {
                // Getting the simulated hit inside the calorimeter
                const mctools::simulated_data::hit_handle_collection_type * ptr_collection = 0;
                if (sd.has_step_hits(vname))
                  {
                    ptr_collection = &sd.get_step_hits(vname);
                  }
                else
                  {
                    DT_LOG_WARNING(get_logging_priority(), "Simulated data has not step hit associated to '" << vname << "' category ! Skip !");
                    continue;
                  }
                const mctools::simulated_data::hit_handle_collection_type & hits = *ptr_collection;
                if (hits.size() != 1)
                  {
                    DT_LOG_WARNING(get_logging_priority(), "More than one energy deposit ! Skip !");
                    continue;
                  }
                // Get the first calorimeter simulated step hit i.e. the one at
                // the entrance of the calorimeter block
                const mctools::base_step_hit & the_step_hit = hits.front().get();
                const geomtools::geom_id & the_step_hit_gid = the_step_hit.get_geom_id();

                // Get the associated calorimeter list
                const snemo::datamodel::calibrated_calorimeter_hit::collection_type calos
                  = a_particle.get_associated_calorimeters();
                if (calos.size() != 1)
                  {
                    DT_LOG_WARNING(get_logging_priority(), "More than one calorimeter associated to the particle track ! Skip !");
                    continue;
                  }
                const geomtools::geom_id & the_calo_gid = calos.front().get ().get_geom_id ();
                if (! geomtools::geom_id::match (the_calo_gid, the_step_hit_gid))
                  {
                    DT_LOG_WARNING(get_logging_priority(),
                                   "Simulated calorimeter does not match associated calorimeter ! Skip !");
                    continue;
                  }

                delta = a_vertex.get_position () - the_step_hit.get_position_start();
              }
            DT_LOG_DEBUG(get_logging_priority(), "Delta value = " << delta/CLHEP::mm << " mm");

    //         if (!geomtools::is_valid (delta))
    //           {
    //             return STOP;
    //           }
    //         else
    //           {
    //             delta /= CLHEP::mm;
    //           }

    //         {
    //           ostringstream key;
    //           key << "x position";
    //           mygsl::datapoint a_point (sim_energy, delta.x ());
    //           _vertex_graphs_[key.str ()].push_back (a_point);
    //           if (! _vertex_histograms_.count (key.str ()))
    //             {
    //               const size_t nbin = 100;
    //               const double min = -100.0 * CLHEP::mm;
    //               const double max = +100.0 * CLHEP::mm;
    //               _vertex_histograms_[key.str ()].init (nbin, min / CLHEP::mm, max / CLHEP::mm);
    //             }
    //           _vertex_histograms_[key.str ()].fill (delta.x ());
    //         }
    //         {
    //           ostringstream key;
    //           key << "y position";
    //           mygsl::datapoint a_point (sim_energy,delta.y() );
    //           _vertex_graphs_[key.str ()].push_back (a_point);
    //           if (! _vertex_histograms_.count (key.str ()))
    //             {
    //               const size_t nbin = 100;
    //               const double min = -50.0 * CLHEP::mm;
    //               const double max = +50.0 * CLHEP::mm;
    //               _vertex_histograms_[key.str ()].init (nbin, min / CLHEP::mm, max / CLHEP::mm);
    //             }
    //           _vertex_histograms_[key.str ()].fill (delta.y ());
    //         }
    //         {
    //           ostringstream key;
    //           key << "z position";
    //           mygsl::datapoint a_point (sim_energy,delta.z() );
    //           _vertex_graphs_[key.str ()].push_back (a_point);
    //           if (! _vertex_histograms_.count (key.str ()))
    //             {
    //               const size_t nbin = 100;
    //               const double min = -50.0 * CLHEP::mm;
    //               const double max = +50.0 * CLHEP::mm;
    //               _vertex_histograms_[key.str ()].init (nbin, min / CLHEP::mm, max / CLHEP::mm);
    //             }
    //           _vertex_histograms_[key.str ()].fill (delta.z ());
    //         }
          }
      }


    return dpp::base_module::PROCESS_SUCCESS;
  }

  // void snemo_vertex_resolution_module::_generate_plots_ ()
  // {
  //   // Instantiate ROOT interpreter to load style defined in
  //   // ~/.root_logon
  //   TRint * root_interpreter = new TRint ("ROOT Interpreter", 0, 0, 0, 0, true);

  //   // Set a global switch disabling the reference: no screwed up
  //   // with BRIO format
  //   TH1::AddDirectory (false);

  //   // Generate delta distribution value
  //   _generate_histogram_plot_ ();

  //   // Generate delta scatter plot value
  //   _generate_scatter_plot_ ();

  //   // Dump result
  //   _dump_result_ (clog,
  //                  "snemo::analysis::processing::snemo_vertex_resolution_module::_dump_result_: ",
  //                  "NOTICE: ");

  //   // Running ROOT interpreter with true option to be let the
  //   // reset option going to the end
  //   if (is_interactive ()) root_interpreter->Run (true);

  //   return;
  // }

  // void snemo_vertex_resolution_module::_generate_histogram_plot_ ()
  // {
  //   if (_vertex_histograms_.empty ())
  //     {
  //       if (is_debug ())
  //         {
  //           clog << datatools::utils::io::debug
  //                << "snemo::analysis::processing::"
  //                << "snemo_vertex_resolution_module::_generate_histogram_plot_: "
  //                << "No vertex histograms have been filled !"
  //                << std::endl;
  //         }
  //       return;
  //     }

  //   // Vertex histograms
  //   for (histogram_collection_type::const_iterator
  //          it_histo = _vertex_histograms_.begin ();
  //        it_histo != _vertex_histograms_.end ();
  //        ++it_histo)
  //     {
  //       const string & a_label               = it_histo->first;
  //       const mygsl::histogram & a_histogram = it_histo->second;

  //       // Build ROOT histograms
  //       TCanvas * canvas = new TCanvas (a_label.c_str (),
  //                                       a_label.c_str (), 700, 500);
  //       TLegend * legend = new TLegend (0.15, 0.9, 0.3, 0.7);
  //       legend->SetHeader ("SNWare simulations");
  //       //          legend->SetNColumns (3);

  //       TH1D * thistogram = new TH1D (a_label.c_str (), "",
  //                                     a_histogram.bins (),
  //                                     a_histogram.min  (),
  //                                     a_histogram.max  ());

  //       // Loop over bin content
  //       for (size_t i = 0; i < a_histogram.bins (); ++i)
  //         {
  //           const double value = a_histogram.get (i);
  //           if (datatools::utils::is_valid (value))
  //             {
  //               thistogram->SetBinContent (i, value);
  //               //thistogram->SetBinError   (i, sqrt (value));
  //             }
  //         }

  //       // Set histogram style
  //       thistogram->SetLineColor   (kBlack);
  //       thistogram->SetMarkerColor (kBlack);
  //       thistogram->SetMarkerStyle (8);
  //       thistogram->Scale (1./a_histogram.sum ());

  //       // X0 uses to suppress x-axis error bar
  //       thistogram->Draw ();
  //       const string title = ";#Delta" + a_label + " [mm]; probability";
  //       thistogram->SetTitle (title.c_str ());
  //       // thistogram->GetYaxis ()->SetRangeUser (0.0, 1.2);
  //       thistogram->GetYaxis ()->SetNdivisions (510);
  //       legend->SetTextSize (thistogram->GetXaxis ()->GetLabelSize ());
  //       legend->Draw ();
  //     }

  //   return;
  // }

  // void snemo_vertex_resolution_module::_generate_scatter_plot_ ()
  // {
  //   if (_vertex_graphs_.empty ())
  //     {
  //       if (is_debug ())
  //         {
  //           clog << datatools::utils::io::debug
  //                << "snemo::analysis::processing::"
  //                << "snemo_vertex_resolution_module::_generate_scatter_plot_: "
  //                << "No vertex graphs have been filled !"
  //                << std::endl;
  //         }
  //       return;
  //     }

  //   // Build ROOT histograms
  //   TCanvas * canvas = new TCanvas ("canvas_delta_vertex_distribution",
  //                                   "canvas_delta_vertex_distribution", 700, 500);
  //   TLegend * legend = new TLegend (0.15, 0.9, 0.3, 0.7);
  //   legend->SetHeader ("SNWare simulations");
  //   //          legend->SetNColumns (3);

  //   const bool draw_scatter_plot = true;
  //   size_t icolor = 1;
  //   // double ymin = +numeric_limits<double>::infinity ();
  //   // double ymax = -numeric_limits<double>::infinity ();
  //   for (graph_collection_type::const_iterator
  //          it_graph = _vertex_graphs_.begin ();
  //        it_graph != _vertex_graphs_.end ();
  //        ++it_graph, ++icolor)
  //     {
  //       const string & a_label     = it_graph->first;
  //       const datapoints & a_graph = it_graph->second;

  //       TGraphErrors * tgraph = new TGraphErrors (a_graph.size ());

  //       // Define constant for binned graph:
  //       const size_t nbin = 30;
  //       const double xmin = 0.0;
  //       const double xmax = 3000.0;
  //       const double dx = (xmax - xmin) / (double)nbin;

  //       typedef map<size_t, vector<double> > bin_collection_type;
  //       bin_collection_type mbinned;// Loop over bin content
  //       for (size_t i = 0; i < a_graph.size (); ++i)
  //         {
  //           const double x = a_graph[i].x ();
  //           const double y = a_graph[i].y ();

  //           double error = 0.0;
  //           if (a_graph[i].has_sigma ()) error = a_graph[i].sigma ();

  //           tgraph->SetPoint (i, x, y);
  //           tgraph->SetPointError (i, 0.0, error);
  //           const size_t index = int(x/dx);
  //           mbinned[index].push_back (y);
  //         }

  //       // Build legend entry
  //       ostringstream entry;
  //       entry << "#color[" << icolor << "]{" << a_label << "}";

  //       // Set histogram style
  //       tgraph->SetLineColor   (icolor);
  //       tgraph->SetMarkerColor (icolor);
  //       tgraph->SetMarkerStyle (6);

  //       if (draw_scatter_plot)
  //         {
  //           if (it_graph == _vertex_graphs_.begin ())
  //             {
  //               tgraph->Draw ("APE");
  //               tgraph->SetTitle (";E_{simulated} [keV]; #Delta_{sim/rec}");
  //               //tgraph->GetYaxis ()->SetRangeUser (ymin, ymax);
  //               //tgraph->GetYaxis ()->SetNdivisions (510);
  //               legend->SetTextSize (tgraph->GetXaxis ()->GetLabelSize ());
  //               legend->AddEntry (tgraph, entry.str ().c_str (), "P");
  //             }
  //           else
  //             {
  //               tgraph->Draw ("PE && same");
  //               legend->AddEntry (tgraph, entry.str ().c_str (), "P");
  //             }
  //         }

  //       // Create profile graph
  //       TGraphErrors * tprofile = new TGraphErrors ();

  //       // Fill profile graph
  //       size_t j = 0;
  //       for (bin_collection_type::const_iterator
  //              i = mbinned.begin ();
  //            i != mbinned.end ();
  //            ++i, ++j)
  //         {
  //           const size_t index        = i->first;
  //           const vector<double> & vy = i->second;

  //           if (vy.empty ()) continue;

  //           const double mean  = TMath::Mean (vy.size (),&vy[0]);
  //           const double sigma = TMath::RMS  (vy.size (),&vy[0]);
  //           tprofile->SetPoint (j, (index + 0.5)*dx, mean);
  //           tprofile->SetPointError (j, 0., sigma/sqrt (vy.size ()));
  //         }

  //       tprofile->SetMarkerColor (icolor);
  //       tprofile->SetLineColor   (icolor);
  //       tprofile->SetMarkerStyle (8);

  //       if (draw_scatter_plot || (it_graph != _vertex_graphs_.begin ()))
  //         {
  //           tprofile->Draw ("PEZ && same");
  //         }
  //       else
  //         {
  //           tprofile->Draw ("APEZ");
  //           tprofile->SetTitle (";E_{simulated} [keV]; #Delta_{sim/rec}");
  //           legend->SetTextSize (tprofile->GetXaxis ()->GetLabelSize ());
  //         }

  //       legend->AddEntry (tprofile, entry.str ().c_str (), "P");
  //     }

  //   legend->Draw ();

  //   return;
  // }

  void snemo_vertex_resolution_module::dump_result (std::ostream      & out_,
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
      if (_vertex_histograms_.empty ())
        out_ << "<empty>";
      else
        out_ << _vertex_histograms_.size ();
      out_ << std::endl;

      for (histogram_collection_type::const_iterator
             i = _vertex_histograms_.begin ();
           i != _vertex_histograms_.end ();
           ++i)
        {
          const std::string & a_label = i->first;
          histogram_collection_type::const_iterator j = i;
          out_ << indent;
          if (++j == _vertex_histograms_.end ())
            {
              out_  << datatools::i_tree_dumpable::last_tag;
            }
          else
            {
              out_ << datatools::i_tree_dumpable::tag;
            }
          out_ << "Label " << a_label << std::endl;

          out_ << indent << datatools::i_tree_dumpable::skip_tag <<  datatools::i_tree_dumpable::tag
               << "Number of events " << i->second.sum ()  << std::endl;
          out_ << indent << datatools::i_tree_dumpable::skip_tag <<  datatools::i_tree_dumpable::tag
               << "Mean  = " << i->second.mean () / CLHEP::mm << " mm" << std::endl;
          out_ << indent << datatools::i_tree_dumpable::skip_tag << datatools::i_tree_dumpable::last_tag
               << "Sigma = " << i->second.sigma () / CLHEP::mm << " mm" << std::endl;

          // if (is_debug ())
          //   {
          //     i->second.print (clog);
          //   }
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
