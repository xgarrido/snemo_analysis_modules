// /* nemo3_Y90_study_module.h
//  * Author(s)     : Xavier Garrido <garrido@lal.in2p3.fr>
//  * Creation date : 2012-10-28
//  * Last modified : 2012-10-28
//  *
//  * Copyright (C) 2012 Xavier Garrido <garrido@lal.in2p3.fr>
//  *
//  * This program is free software; you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License as published by
//  * the Free Software Foundation; either version 3 of the License, or (at
//  * your option) any later version.
//  *
//  * This program is distributed in the hope that it will be useful, but
//  * WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  * General Public License for more details.
//  *
//  * You should have received a copy of the GNU General Public License
//  * along with this program; if not, write to the Free Software
//  * Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  * Boston, MA 02110-1301, USA.
//  *
//  *
//  * Description:
//  *
//  * A very simple module to do plot using sncore::histogram_service
//  *
//  * History:
//  *
//  */

// #ifndef SNEMO_ANALYSIS_PROCESSING_NEMO3_Y90_STUDY_MODULE_H
// #define SNEMO_ANALYSIS_PROCESSING_NEMO3_Y90_STUDY_MODULE_H 1

// #include <sncore/processing/base_module.h>    // data processing module abstract base class
// #include <sncore/processing/module_macros.h>  // useful macros concerning data processing modules

// #include <string>

// namespace mygsl {
//   class histogram_pool;
// }

// namespace snemo {

//   namespace geometry {
//     class manager;
//   }

//   namespace analysis {

//     namespace model {
//       class particle_track_data;
//     }

//     namespace processing {

//       SNEMO_MODULE_CLASS_DECLARE (nemo3_Y90_study_module)
//       {
//       public:
//         static const std::string DEFAULT_PTD_LABEL;

//         void set_PTD_bank_label (const std::string & PTD_label);

//         const std::string & get_PTD_bank_label () const;

//         void set_Histo_service_label (const std::string & Histo_label);

//         const std::string & get_Histo_service_label () const;

//         void set_histogram_pool (mygsl::histogram_pool & pool_);

//         mygsl::histogram_pool & grab_histogram_pool ();

//         void set_Geo_service_label (const std::string & Geo_label);

//         const std::string & get_Geo_service_label () const;

//         bool has_geom_manager () const;

//         void set_geom_manager (const snemo::geometry::manager & gmgr_);

//         const snemo::geometry::manager & get_geom_manager () const;

//         // Macro to automate the public interface of the module (including ctor/dtor) :
//         SNEMO_MODULE_INTERFACE_CTOR_DTOR (nemo3_Y90_study_module);

//       private:

//         // Give default values to specific class members.
//         void _init_defaults_ ();

//         // Compute the energy distribution for electron and gamma
//         void _compute_energy_distribution_ (const model::particle_track_data & ptd_) const;

//         // Compute the angular distribution for electron and gamma
//         void _compute_angular_distribution_ (const model::particle_track_data & ptd_) const;

//       private:

//         // The label/name of the 'particle track' bank accessible from the event record :
//         std::string _PTD_bank_label_;

//         // The label/name of the 'Geo' service :
//         std::string _Geo_service_label_;

//         // The SuperNEMO geometry manager
//         const snemo::geometry::manager * _geom_manager_;

//         // The label/name of the 'Histogram' service :
//         std::string _Histo_service_label_;

//         // The histogram pool:
//         mygsl::histogram_pool * _histogram_pool_;

//         // Macro to automate the registration of the module :
//         SNEMO_MODULE_REGISTRATION_INTERFACE (nemo3_Y90_study_module);

//       };

//     } // namespace processing

//   } // namespace analysis

// } // namespace snemo

// #endif // SNEMO_ANALYSIS_PROCESSING_NEMO3_Y90_STUDY_MODULE_H

// // end of nemo3_Y90_study_module.h
// /*
// ** Local Variables: --
// ** mode: c++ --
// ** c-file-style: "gnu" --
// ** tab-width: 2 --
// ** End: --
// */
