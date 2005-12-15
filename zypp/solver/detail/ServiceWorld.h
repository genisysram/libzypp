/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ServiceWorld.h
 *
 * Copyright (C) 2000-2002 Ximian, Inc.
 * Copyright (C) 2005 SUSE Linux Products GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _ServiceWorld_h
#define _ServiceWorld_h

#include <iosfwd>
#include <string.h>

#include <zypp/solver/detail/ServiceWorldPtr.h>
#include <zypp/solver/detail/StoreWorld.h>
#include <zypp/solver/detail/Channel.h>

///////////////////////////////////////////////////////////////////
namespace zypp {
//////////////////////////////////////////////////////////////////

typedef bool (*ServiceWorldAssembleFn) (ServiceWorldPtr service, void *error);	// GError **error

///////////////////////////////////////////////////////////////////
//
//	CLASS NAME : ServiceWorld

class ServiceWorld : public StoreWorld {
    REP_BODY(ServiceWorld);

  private:

    char *_url;
    char *_name;
    char *_unique_id;

    bool _is_sticky;		// if true, can't be unmounted
    bool _is_invisible;		// ... to users
    bool _is_unsaved;		// Never save into the services.xml file
    bool _is_singleton;		// only one such service at a time.  FIXME: broken

    ServiceWorldAssembleFn _assemble_fn;

  public:

    ServiceWorld ();
    virtual ~ServiceWorld();

    // ---------------------------------- I/O

    static std::string toString (const ServiceWorld & section);

    virtual std::ostream & dumpOn(std::ostream & str ) const;

    friend std::ostream& operator<<(std::ostream&, const ServiceWorld & section);

    std::string asString (void ) const;

    // ---------------------------------- accessors

    char *url () const { return _url; }
    char *name () const { return _name; }
    void setName (const char *name) { _name = strdup (name); }
    char *unique_id () const { return _unique_id; }

    // ---------------------------------- methods

};
    
///////////////////////////////////////////////////////////////////
}; // namespace zypp
///////////////////////////////////////////////////////////////////

#endif // _ServiceWorld_h
