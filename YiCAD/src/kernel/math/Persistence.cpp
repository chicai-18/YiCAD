// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "Persistence.h"

#include <cassert>

#include "Base64.h"
//**************************************************************************
// Construction/Destruction
TYPESYSTEM_SOURCE_ABSTRACT(Persistence, MetaType, 0) //use abstract macro means this class cannot be initialized independent

//**************************************************************************
// separator for other implementation aspects
unsigned int Persistence::getMemSize () const
{
    // you have to implement this method in all descending classes!
    assert(0);
    return 0;
}

void Persistence::saveXML (Writer &/*writer*/) const
{
    // you have to implement this method in all descending classes!
    assert(0);
}

void Persistence::restoreXML(XMLReader &/*reader*/)
{
    // you have to implement this method in all descending classes!
    assert(0);
}

void Persistence::saveStream (OutputStream& /*str*/) const
{
}

void Persistence::restoreStream(InputStream& rdr, const std::vector<PAIR>& revs)
{
}

void Persistence::restoreStream(InputStream& rdr)
{
}

void Persistence::restoreStreamWithRev(InputStream& rdr, int rev)
{
}

std::string Persistence::encodeAttribute(const std::string& str)
{
    std::string tmp;
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if (*it == '<')
            tmp += "&lt;";
        else if (*it == '\"')
            tmp += "&quot;";
        else if (*it == '\'')
            tmp += "&apos;";
        else if (*it == '&')
            tmp += "&amp;";
        else if (*it == '>')
            tmp += "&gt;";
        else if (*it == '\r')
            tmp += "&#13;";
        else if (*it == '\n')
            tmp += "&#10;";
        else if (*it == '\t')
            tmp += "&#9;";
        else
            tmp += *it;
    }

    return tmp;
}

int Persistence::getRevisionId(const std::string& type, const std::vector<PAIR>& revs)
{
    for (auto& rev : revs)
    {
        if (rev.first == type)
        {
            return rev.second;
        }
    }

    return 0;
}

std::string Persistence::encode(const std::string& str) const
{
    if (isBase64)
    {
        return PersistentTools::base64_encode(reinterpret_cast<unsigned char const*>(str.c_str()), str.length());
    }
    else
    {
        return str;
    }
}

std::string Persistence::decode(const std::string& str) const
{
    if (isBase64)
    {
        return PersistentTools::base64_decode(str);
    }
    else
    {
        return str;
    }
}
