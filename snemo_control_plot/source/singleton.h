// -*- mode: c++ ; -*-
/* singleton.h
 * Author (s) :   Xavier Garrido <garrido@lal.in2p3.fr>
 * Creation date: 2016-03-24
 * Last modified: 2016-03-24
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
 *
 * Description:
 *
 * \class singleton singleton.h utils/singleton.h
 *
 * \brief Curiously Recurring Template Pattern (CRTP) for Meyers singleton
 *
 * The singleton class is implemented as follows
 * \code
 * #include <falaise/snemo/utils/singleton.h>
 *
 * class some_class : public utils::singleton<some_class> {
 *   ...
 * private:
 *   // prevent creation, destruction
 *   some_class() { }
 *   ~some_class() { }
 *
 *   friend class utils::singleton<some_class>;
 * };
 * \endcode
 *
 * Singleton automatically prevents copying of the derived class.
 *
 * made by Darko Veberic for Pierre Auger Observatory software
 *
 * History:
 *
 */

#ifndef SNEMO_UTILS_SINGLETON_H
#define SNEMO_UTILS_SINGLETON_H 1

namespace snemo {
namespace utils {

  template<typename T>
  class singleton {

  public:
    static T & get_instance() { static T instance; return instance; }

  protected:
    // derived class can call ctor and dtor
    singleton()
    {
    }

    ~singleton()
    {
    }

  private:
    // no one should do copies
    singleton(const singleton&);
    singleton& operator=(const singleton&);

  };

} // end of namespace utils
} // end of namespace snemo

#endif // SNEMO_UTILS_SINGLETON_H

// end of singleton.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
