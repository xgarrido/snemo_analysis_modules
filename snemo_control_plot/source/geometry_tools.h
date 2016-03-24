/// \file geometry_tools.h
/* Author(s)     : Xavier Garrido <garrido@lal.in2p3.fr>
 * Creation date : 2016-03-24
 * Last modified : 2016-03-24
 *
 * Copyright (C) 2016 Xavier Garrido <garrido@lal.in2p3.fr>
 *
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
 * Description:
 *
 *   Geometry tools singleton.
 *
 * History:
 *
 */

#ifndef SNEMO_UTILS_GEOMETRY_TOOLS_H
#define SNEMO_UTILS_GEOMETRY_TOOLS_H 1

// This project:
#include <singleton.h>

// Forward declaration
namespace datatools
{
  class properties;
  class service_manager;
}

namespace snemo {
namespace utils {

  /// \brief Geometry toolbox
  class geometry_tools : public ::snemo::utils::singleton<geometry_tools>
  {
  public:

    /// The main initialization method (post-construction):
    void initialize(const datatools::properties & config_,
                    const datatools::service_manager & service_manager_);

    /// The main termination method
    void reset();

  private:

    /// Forbid default constructor
    geometry_tools();

    /// Forbid destructor (done within singleton object)
    virtual ~geometry_tools();

    /// Non copyable constructor
    geometry_tools(const geometry_tools&);

    /// Non-copyable assignation
    geometry_tools & operator=(const geometry_tools&);

    /// Make the class singleton friend
    friend class utils::singleton<geometry_tools>;

  };

} // end of namespace utils
} // end of namespace snemo

#endif // SNEMO_UTILS_GEOMETRY_TOOLS_H

/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
