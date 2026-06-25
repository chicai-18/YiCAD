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

#ifndef YICAD_PERSISTENCE_H
#define YICAD_PERSISTENCE_H

#include <MetaType.h>
#include <Stream.h>

class Reader;
class Writer;
class XMLReader;

/// Persistence class and root of the type system
class Persistence : public MetaType
{
	TYPESYSTEM_HEADER();

public:
	/** This method is used to get the size of objects
	 * It is not meant to have the exact size, it is more or less an estimation
	 * which runs fast! Is it two bytes or a GB?
	 */
	virtual unsigned int getMemSize() const;

	/** This method is used to save properties to an XML document.
	 * The programmer has to take care that a valid XML document
	 * is written. This means closing tags and writing UTF-8.
	 * @see Writer
	 */
	virtual void saveXML(Writer&/*writer*/) const;

	/** This method is used to restore properties from an XML document.
	 */
	virtual void restoreXML(XMLReader&/*reader*/);
	
	/** This method is used to save large amounts of data to a binary file.
	 * Sometimes it makes no sense to write property data as XML. In case the
	 * amount of data is too big or the data type has a more effective way to
	 * save itself. In this cases to write the data in a separate file inside
	 * the document archive is better way to take. 
	 */
	virtual void saveStream(OutputStream&/*str*/) const;

	/** This method is used to restore large amounts of data from a file
	*/
	virtual void restoreStream(InputStream& rdr, const std::vector<PAIR>& revs);
	virtual void restoreStream(InputStream& rdr);

	/** This method is used to restore object with revision from binary file
	 *
	 */
	virtual void restoreStreamWithRev(InputStream& rdr, int rev);

	/// Encodes an attribute upon saving.
	static std::string encodeAttribute(const std::string&);

	//dump the binary persistence data into into the stream
	void dumpToStream(std::ostream& stream, int compression);

	//restore the binary persistence data from a stream. Must have the format used by dumpToStream
	void restoreFromStream(std::istream& stream);

	/// @brief 获取指定类型保存时的版本号
	int getRevisionId(const std::string& type, const std::vector<PAIR>& revs);

	// ==========base64==============

	/// @brief base64编码
	std::string encode(const std::string& str) const;
	/// @brief base64解码
	std::string decode(const std::string& str) const;

private:
	/** This method is used at the end of restoreFromStream()
	 * after all data files have been read in.
	 * A subclass can set up some internals. The default
	 * implementation does nothing.
	 */
	virtual void restoreFinished() {}

private:
	bool isBase64 = true;
};



#endif // YICAD_PERSISTENCE_H
