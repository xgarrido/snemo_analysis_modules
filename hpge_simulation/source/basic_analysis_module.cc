
/* basic_analysis_module.cc
 *
 * Copyright (C) 2013 Pia Loaiza <loaiza@lal.in2p3.fr>

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include <stdexcept>
#include <sstream>
#include <numeric>

#include <TFile.h>
#include <TTree.h>

#include <basic_analysis_module.h>

// Simulated data model
#include <mctools/simulated_data.h>

namespace hpge {

  // Registration instantiation macro :
  DPP_MODULE_REGISTRATION_IMPLEMENT(basic_analysis_module, "hpge::basic_analysis_module");

  void basic_analysis_module::_set_defaults ()
  {
    _SD_bank_label_.clear ();
    _ROOT_filename_.clear ();
    return;
  }

  /*** Implementation of the interface ***/

  // Constructor :
  basic_analysis_module::basic_analysis_module(datatools::logger::priority logging_priority_)
    : dpp::base_module(logging_priority_)
  {
    _set_defaults ();
    return;
  }

  // Destructor :
  basic_analysis_module::~basic_analysis_module ()
  {
    // Make sure all internal resources are terminated
    // before destruction :
    if (is_initialized ()) reset ();
    return;
  }

  // Initialization :
  void basic_analysis_module::initialize (const datatools::properties  & setup_,
                                          datatools::service_manager   & service_manager_,
                                          dpp::module_handle_dict_type & /*module_dict_*/)
  {
    DT_THROW_IF (is_initialized (),
                 std::logic_error,
                 "Module '" << get_name () << "' is already initialized ! ");

    dpp::base_module::_common_initialize (setup_);

    /**************************************************************
     *   fetch setup parameters from the configuration container  *
     **************************************************************/

    if (setup_.has_key ("SD_label"))
      {
        _SD_bank_label_ = setup_.fetch_string ("SD_label");
      }

    if (setup_.has_key ("ROOT_filename"))
      {
        std::string label = setup_.fetch_string ("ROOT_filename");
        datatools::fetch_path_with_env (label);
        _ROOT_filename_ = label;
      }

    /*********************************************
     *   do some check on the setup parameters   *
     *********************************************/

    DT_THROW_IF (_SD_bank_label_.empty (), std::logic_error,
                 "Missing 'SD_label' bank !");

    DT_THROW_IF (_ROOT_filename_.empty (), std::logic_error,
                 "Missing 'ROOT_filename' bank !");

    /******************************
     *   Initialize ROOT output   *
     ******************************/
    _hfile_ = new TFile (_ROOT_filename_.c_str(), "RECREATE",
                         "Output file of HPGE Simulation data from mctools/G4");
    DT_THROW_IF (! _hfile_->IsWritable (), std::logic_error,
                 "Cannot write ROOT output file !");

    // This is very important to make sure new allocated ROOT
    // objects are attached to the new ROOT file :
    _hfile_->cd ();

    _genbb_tree_ = new TTree ("GenData", "GenData");
    _genbb_tree_->SetDirectory (_hfile_);
    _genbb_tree_->Branch ("primary_energy", &_primary_energy_, "prim_energy/D");

    _calo_tree_ = new TTree ("SimuData", "SimuData");
    _calo_tree_->SetDirectory (_hfile_);
    _calo_tree_->Branch ("total_energy", &_total_energy_, "tot_energy/D");

    /*************************************
     *  end of the initialization step   *
     *************************************/

    // Tag the module as initialized :
    _set_initialized (true);
    return;
  }

  // Reset :
  void basic_analysis_module::reset()
  {
    DT_THROW_IF(! is_initialized (),
                std::logic_error,
                "Module '" << get_name () << "' is not initialized !");

    _calo_tree_->Write ();
    _genbb_tree_->Write ();
    _hfile_->Close ();

    // Destroy internal resources :
    _hfile_      = 0;
    _calo_tree_  = 0;
    _genbb_tree_ = 0;

    /****************************
     *  revert to some defaults *
     ****************************/

    this->_set_defaults ();

    /****************************
     *  end of the reset step   *
     ****************************/

    // Tag the module as un-initialized :
    _set_initialized (false);
    return;
  }

  // Processing :
  dpp::base_module::process_status basic_analysis_module::process(datatools::things & data_record_)
  {
    DT_LOG_TRACE (get_logging_priority (), "Entering...");

    DT_THROW_IF(! is_initialized (),
                std::logic_error,
                "Module '" << get_name () << "' is not initialized !");

    DT_THROW_IF (! data_record_.has (_SD_bank_label_), std::logic_error,
                 "Missing simulated data to be processed !");
    const mctools::simulated_data & sd = data_record_.get<mctools::simulated_data> (_SD_bank_label_);
    DT_LOG_DEBUG (get_logging_priority (),  "Found the bank '" << _SD_bank_label_ << "' !");

    DT_LOG_TRACE (get_logging_priority (),"Simulated data:");
    if (get_logging_priority () >= datatools::logger::PRIO_TRACE)
      {
        sd.tree_dump (std::clog);
      }

    const mctools::simulated_data::primary_event_type & pevent = sd.get_primary_event ();
    const genbb::primary_event::particles_col_type & particles = pevent.get_particles ();

    for (genbb::primary_event::particles_col_type::const_iterator
           ip = particles.begin ();
         ip != particles.end (); ++ip)
      {
        _primary_energy_ = ip->get_kinetic_energy () / CLHEP::keV;
        _genbb_tree_->Fill ();
        DT_LOG_TRACE (get_logging_priority (), ip->get_particle_label () << " particle: "
                      << "E = " << ip->get_kinetic_energy () / CLHEP::keV << " keV, "
                      << "primary energy = " << _primary_energy_ << " keV "
                      << "t = " << ip->get_time () / CLHEP::ms << " ms");
      }

    const std::string hit_category = "crystal.hit";
    if (! sd.has_step_hits (hit_category))
      {
        DT_LOG_TRACE (get_logging_priority (), "No '" << hit_category << "' found!");
        return dpp::base_module::PROCESS_STOP;
      }

    const mctools::simulated_data::hit_handle_collection_type &
      my_hit_collection = sd.get_step_hits (hit_category);

    // 2013-10-22 XG: Reset total energy. Maybe it is better to invalidate the
    // total energy than assigning a zero value : We can imagine than a hit
    // deposit zero energy which is different to no hit at all
    _total_energy_ = 0.0 * CLHEP::keV;
    for (mctools::simulated_data::hit_handle_collection_type::const_iterator
           ihit = my_hit_collection.begin ();
         ihit != my_hit_collection.end (); ++ihit)
      {
        const mctools::base_step_hit & a_hit = ihit->get ();
        const double the_energy = a_hit.get_energy_deposit ();
        _total_energy_ += the_energy;

        DT_LOG_TRACE (get_logging_priority (), "The current energy deposit is "
                      << the_energy / CLHEP::keV << " keV");
      }

    DT_LOG_TRACE (get_logging_priority (), "The total energy deposit is "
                  << _total_energy_ / CLHEP::keV << " keV");

    _total_energy_ /= CLHEP::keV;
    if (_total_energy_ != 0.0 * CLHEP::keV) _calo_tree_->Fill ();

    DT_LOG_TRACE (get_logging_priority (), "Exiting.");
    return dpp::base_module::PROCESS_SUCCESS;
  }

} // namespace hpge

// end of basic_analysis_module.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
