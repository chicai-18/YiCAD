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

#ifndef BASE_READER_H
#define BASE_READER_H

#include <bitset>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <pugixml.hpp>

#if defined(_WIN32) && defined(ZIPIOS_UTF8)
#include <FileInfo.h>
#endif

class ArchiveReader;
class Persistence;

/** The XML reader class
 * This is an important helper class for the store and retrieval system
 * of objects. These classes mainly inherit the Persitance
 * base class and implement the Restore() method.
 * The reader gets mainly initialized by the Document on retrieving a
 * document out of a file. From there subsequently the Restore() method will
 * by called on all object stored.
 *  \par
 * A simple example is the Restore:
 *  \code
void String::Save (short indent,std::ostream &str)
{
    str << "<String value=\"" <<  _cValue.c_str() <<"\"/>" ;
}

void PropertyString::Restore(Base::Reader &reader)
{
    // read my Element
    reader.readElement("String");
    // get the value of my Attribute
    _cValue = reader.getAttribute("value");
}
 *  \endcode
 */

class XMLReader
{
public:
    enum ReaderStatus {
        PartialRestore = 0,                     // This bit indicates that a partial restore took place somewhere in this Document
        PartialRestoreInDocumentObject = 1,     // This bit is local to the DocumentObject being read indicating a partial restore therein
        PartialRestoreInProperty = 2,           // Local to the Property
        PartialRestoreInObject = 3              // Local to the object partially restored itself
    };
    /// open the file and read the first element
    XMLReader(const char* FileName, std::istream&);
    ~XMLReader();

    bool isValid() const { return _valid; }
    bool isVerbose() const { return _verbose; }
    void setVerbose(bool on) { _verbose = on; }

    /** @name Parser handling */
    //@{
    /// get the local name of the current Element
    const char* localName() const;
    /// get the current element level
    int level() const;
    /// read until a start element is found (\<name\>) or start-end element (\<name/\>) (with special name if given)
    void readElement   (const char* ElementName=nullptr);

    /** read until an end element is found
     *
     * @param ElementName: optional end element name to look for. If given, then
     * the parser will read until this name is found.
     *
     * @param level: optional level to look for. If given, then the parser will
     * read until this level. Note that the parse only increase the level when
     * finding a start element, not start-end element, and decrease the level
     * after finding an end element. So, if you obtain the parser level after
     * calling readElement(), you should specify a level minus one when calling
     * this function. This \c level parameter is only useful if you know the
     * child element may have the same name as its parent, otherwise, using \c
     * ElementName is enough.
     */
    void readEndElement(const char* ElementName=nullptr, int level=-1);
    /// read until characters are found
    void readCharacters();
    /// read binary file
    void readBinFile(const char*);
    //@}

    /** @name Attribute handling */
    //@{
    /// get the number of attributes of the current element
    unsigned int getAttributeCount() const;
    /// check if the read element has a special attribute
    bool hasAttribute(const char* AttrName) const;
    /// return the named attribute as an interer (does type checking)
    long getAttributeAsInteger(const char* AttrName) const;
    unsigned long getAttributeAsUnsigned(const char* AttrName) const;
    /// return the named attribute as a double floating point (does type checking)
    double getAttributeAsFloat(const char* AttrName) const;
    /// return the named attribute as a double floating point (does type checking)
    const char* getAttribute(const char* AttrName) const;
    //@}

    /** @name additional file reading */
    //@{
    /// add a read request of a persistent object
    const char *addFile(const char* Name, Persistence *Object);
    /// process the requested file reads from an archive
    void readFiles(ArchiveReader &archive) const;
    /// get all registered file names
    const std::vector<std::string>& getFilenames() const;
    bool isRegistered(Persistence *Object) const;
    virtual void addName(const char*, const char*);
    virtual const char* getName(const char*) const;
    virtual bool doNameMapping() const;
    //@}

    /// Schema Version of the document
    int DocumentSchema;
    /// Version of FreeCAD that wrote this document
    std::string ProgramVersion;
    /// Version of the file format
    int FileVersion;

    /// sets simultaneously the global and local PartialRestore bits
    void setPartialRestore(bool on);

    void clearPartialRestoreDocumentObject();
    void clearPartialRestoreProperty();
    void clearPartialRestoreObject();

    /// return the status bits
    bool testStatus(ReaderStatus pos) const;
    /// set the status bits
    void setStatus(ReaderStatus pos, bool on);
    struct FileEntry {
        std::string FileName;
        Persistence *Object;
    };
    std::vector<FileEntry> FileList;

protected:
    /// advance cursor to the next element in document order, return false if none
    bool advance();

    /// populate AttrMap from the given node's attributes
    void readAttributes(pugi::xml_node node);

    pugi::xml_document doc;
    pugi::xml_node cursor;          // current position in the DOM
    pugi::xml_node lastStartElement; // last element entered via readElement (for self-closing detection)

    int Level;
    std::string LocalName;
    std::string Characters;
    unsigned int CharacterCount;

    std::map<std::string,std::string> AttrMap;
    typedef std::map<std::string,std::string> AttrMapType;

    enum {
        None = 0,
        Chars,
        StartDocument,
        EndDocument,
        StartElement,
        StartEndElement,
        EndElement,
        StartCDATA,
        EndCDATA
    }   ReadType;

    std::string _File;
    bool _valid;
    bool _verbose;

    std::vector<std::string> FileNames;

    std::bitset<32> StatusBits;
};

class Reader : public std::istream
{
public:
    Reader(std::istream&, const std::string&, int version);
    std::istream& getStream();
    std::string getFileName() const;
    int getFileVersion() const;
    void initLocalReader(std::shared_ptr<XMLReader>);
    std::shared_ptr<XMLReader> getLocalReader() const;

private:
    std::istream& _str;
    std::string _name;
    int fileVersion;
    std::shared_ptr<XMLReader> localreader;
};

#endif
