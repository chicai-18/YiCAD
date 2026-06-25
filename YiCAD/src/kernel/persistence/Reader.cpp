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

#include <locale>
#include <exception>

#include "Reader.h"
#include "Archive.h"
#include "Base64.h"
#include "Persistence.h"
#include "Stream.h"
#include <Tools.h>

using namespace std;


// ---------------------------------------------------------------------------
//  XMLReader: Constructors and Destructor
// ---------------------------------------------------------------------------

XMLReader::XMLReader(const char* FileName, std::istream& str)
  : DocumentSchema(0), ProgramVersion(""), FileVersion(0), Level(0),
    CharacterCount(0), ReadType(None), _File(FileName), _valid(false),
    _verbose(true)
{
#ifdef _MSC_VER
    str.imbue(std::locale::empty());
#else
    str.imbue(std::locale::classic());
#endif

    // Parse the entire XML document into memory using pugixml.
    // pugixml reads UTF-8 natively, no transcoding needed.
    pugi::xml_parse_result result = doc.load(str,
        pugi::parse_default | pugi::parse_trim_pcdata);

    if (result) {
        _valid = true;
        cursor = doc.first_child();
        ReadType = StartDocument;
    }
    else {
        cerr << "XML parse error: " << result.description()
             << " at offset " << result.offset << "\n";
    }
}

XMLReader::~XMLReader()
{
}

const char* XMLReader::localName() const
{
    return LocalName.c_str();
}

unsigned int XMLReader::getAttributeCount() const
{
    return static_cast<unsigned int>(AttrMap.size());
}

long XMLReader::getAttributeAsInteger(const char* AttrName) const
{
    AttrMapType::const_iterator pos = AttrMap.find(AttrName);

    if (pos != AttrMap.end()) {
        return atol(pos->second.c_str());
    }
    else {
        // wrong name, use hasAttribute if not sure!
        std::ostringstream msg;
        msg << "XML Attribute: \"" << AttrName << "\" not found";
        throw OneException(msg.str().c_str());
    }
}

unsigned long XMLReader::getAttributeAsUnsigned(const char* AttrName) const
{
    AttrMapType::const_iterator pos = AttrMap.find(AttrName);

    if (pos != AttrMap.end()) {
        return strtoul(pos->second.c_str(),nullptr,10);
    }
    else {
        // wrong name, use hasAttribute if not sure!
        std::ostringstream msg;
        msg << "XML Attribute: \"" << AttrName << "\" not found";
        throw OneException(msg.str().c_str());
    }
}

double XMLReader::getAttributeAsFloat  (const char* AttrName) const
{
    AttrMapType::const_iterator pos = AttrMap.find(AttrName);

    if (pos != AttrMap.end()) {
        return atof(pos->second.c_str());
    }
    else {
        // wrong name, use hasAttribute if not sure!
        std::ostringstream msg;
        msg << "XML Attribute: \"" << AttrName << "\" not found";
        throw OneException(msg.str().c_str());
    }
}

const char*  XMLReader::getAttribute (const char* AttrName) const
{
    AttrMapType::const_iterator pos = AttrMap.find(AttrName);

    if (pos != AttrMap.end()) {
        return pos->second.c_str();
    }
    else {
        // wrong name, use hasAttribute if not sure!
        std::ostringstream msg;
        msg << "XML Attribute: \"" << AttrName << "\" not found";
        throw OneException(msg.str().c_str());
    }
}

bool XMLReader::hasAttribute (const char* AttrName) const
{
    return AttrMap.find(AttrName) != AttrMap.end();
}

// ---------------------------------------------------------------------------
//  DOM traversal helpers
// ---------------------------------------------------------------------------

bool XMLReader::advance()
{
    // Depth-first traversal in document order.
    // Prefer first child, then next sibling, then parent's next sibling, etc.
    if (!cursor)
        return false;

    pugi::xml_node next = cursor.first_child();
    if (next) {
        cursor = next;
        Level++;
        return true;
    }

    next = cursor.next_sibling();
    if (next) {
        cursor = next;
        return true;
    }

    // Walk up the tree looking for a parent's next sibling
    pugi::xml_node parent = cursor.parent();
    while (parent && parent != doc) {
        pugi::xml_node ps = parent.next_sibling();
        if (ps) {
            cursor = ps;
            Level--;
            return true;
        }
        parent = parent.parent();
        Level--;
    }

    // Reached end of document
    cursor = pugi::xml_node();
    Level = 0;
    ReadType = EndDocument;
    return false;
}

void XMLReader::readAttributes(pugi::xml_node node)
{
    AttrMap.clear();
    for (pugi::xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute()) {
        AttrMap[attr.name()] = attr.value();
    }
}

void XMLReader::readElement(const char* ElementName)
{
    if (!cursor)
        return;

    bool first = true;
    while (true) {
        // For the starting position, check it first; for subsequent positions,
        // advance first then check.
        if (!first) {
            if (!advance())
                break;
        }
        first = false;

        if (cursor.type() != pugi::node_element)
            continue;

        if (ElementName && strcmp(cursor.name(), ElementName) != 0)
            continue;

        // Found a matching element
        LocalName = cursor.name();
        readAttributes(cursor);

        // Check if this is a self-closing element (no children)
        if (cursor.first_child()) {
            ReadType = StartElement;
        }
        else {
            ReadType = StartEndElement;
        }

        lastStartElement = cursor;
        return;
    }
}

int XMLReader::level() const {
    return Level;
}

void XMLReader::readEndElement(const char* ElementName, int level)
{
    // if we are already at the end of the current element
    // (self-closing element from previous readElement)
    if (ReadType == StartEndElement
            && ElementName
            && LocalName == ElementName
            && (level<0 || level==Level))
    {
        // Move past the self-closing element
        advance();
        return;
    }
    else if (ReadType == EndDocument) {
        // the end of the document has been reached but we still try to continue on reading
        throw OneException("End of document reached");
    }

    while (true) {
        if (!advance()) break;
        if (ReadType == EndDocument)
            break;

        // For pugixml DOM traversal, after advancing past the last child
        // of an element, we end up at the element itself (parent).
        // Check if current cursor matches the target closing element.
        if (cursor.type() == pugi::node_element) {
            // If we're looking at an element that is a sibling or above,
            // check if it matches the target
            if (ElementName && strcmp(cursor.name(), ElementName) == 0) {
                if (level < 0 || level == Level) {
                    ReadType = EndElement;
                    LocalName = cursor.name();
                    return;
                }
            }
        }
    }
}

void XMLReader::readCharacters()
{
    // No-op: character data is read on demand via pugixml when needed.
    // The Characters field can be populated by readBinFile for CDATA content.
}

void XMLReader::readBinFile(const char* filename)
{
    Oofstream to(filename, std::ios::out | std::ios::binary);
    if (!to)
        throw OneException("XMLReader::readBinFile() Could not open file!");

    // With pugixml DOM, CDATA content is available as PCData child nodes.
    // Collect all text content from the current element's children.
    std::string cdata;
    if (cursor) {
        for (pugi::xml_node child = cursor.first_child(); child; child = child.next_sibling()) {
            if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata) {
                cdata += child.value();
            }
        }
    }

    to << PersistentTools::base64_decode(cdata);
    to.close();
}

void XMLReader::readFiles(ArchiveReader &archive) const
{
    // It's possible that not all objects inside the document could be created, e.g. if a module
    // is missing that would know these object types. So, there may be data files inside the zip
    // file that cannot be read. We simply ignore these files.
    // On the other hand, however, it could happen that a file should be read that is not part of
    // the zip file. This happens e.g. if a document is written without GUI up but is read with GUI
    // up. In this case the associated GUI document asks for its file which is not part of the ZIP
    // file, then.
    // In either case it's guaranteed that the order of the files is kept.
    if (!archive.nextEntry()) {
        // There is no further file at all. This can happen if the
        // project file was created without GUI
        return;
    }
    std::vector<FileEntry>::const_iterator it = FileList.begin();
    while (archive.entryValid() && it != FileList.end()) {
        std::vector<FileEntry>::const_iterator jt = it;
        // Check if the current entry is registered, otherwise check the next registered files as soon as
        // both file names match
        while (jt != FileList.end() && archive.entryName() != jt->FileName)
            ++jt;
        // If this condition is true both file names match and we can read-in the data, otherwise
        // no file name for the current entry in the zip was registered.
        if (jt != FileList.end()) {
            try {
                Reader reader(archive.stream(), jt->FileName, FileVersion);
                InputStream str(archive.stream());
                jt->Object->restoreStream(str);
                if (reader.getLocalReader())
                    reader.getLocalReader()->readFiles(archive);
            }
            catch(...) {
                // For any exception we just continue with the next file.
                // It doesn't matter if the last reader has read more or
                // less data than the file size would allow.
                // All what we need to do is to notify the user about the
                // failure.
                std::string msg = "Reading failed from embedded file: "+ archive.entryToString() +"\n";

                throw OneException(msg.c_str());
            }
            // Go to the next registered file name
            it = jt + 1;
        }

        // In either case we must go to the next entry
        if (!archive.nextEntry()) {
            // there is no further entry
            break;
        }
    }
}

const char *XMLReader::addFile(const char* Name, Persistence *Object)
{
    FileEntry temp;
    temp.FileName = Name;
    temp.Object = Object;

    FileList.push_back(temp);
    FileNames.push_back( temp.FileName );

    return Name;
}

const std::vector<std::string>& XMLReader::getFilenames() const
{
    return FileNames;
}

bool XMLReader::isRegistered(Persistence *Object) const
{
    if (Object) {
        for (std::vector<FileEntry>::const_iterator it = FileList.begin(); it != FileList.end(); ++it) {
            if (it->Object == Object)
                return true;
        }
    }

    return false;
}

void XMLReader::addName(const char*, const char*)
{
}

const char* XMLReader::getName(const char* name) const
{
    return name;
}

bool XMLReader::doNameMapping() const
{
    return false;
}


bool XMLReader::testStatus(ReaderStatus pos) const
{
    return StatusBits.test(static_cast<size_t>(pos));
}

void XMLReader::setStatus(ReaderStatus pos, bool on)
{
    StatusBits.set(static_cast<size_t>(pos), on);
}

void XMLReader::setPartialRestore(bool on)
{
    setStatus(PartialRestore, on);
    setStatus(PartialRestoreInDocumentObject, on);
    setStatus(PartialRestoreInProperty, on);
    setStatus(PartialRestoreInObject, on);
}

void XMLReader::clearPartialRestoreDocumentObject()
{
    setStatus(PartialRestoreInDocumentObject, false);
    setStatus(PartialRestoreInProperty, false);
    setStatus(PartialRestoreInObject, false);
}

void XMLReader::clearPartialRestoreProperty()
{
    setStatus(PartialRestoreInProperty, false);
    setStatus(PartialRestoreInObject, false);
}

void XMLReader::clearPartialRestoreObject()
{
    setStatus(PartialRestoreInObject, false);
}

// ----------------------------------------------------------

Reader::Reader(std::istream& str, const std::string& name, int version)
  : std::istream(str.rdbuf()), _str(str), _name(name), fileVersion(version)
{
}

std::string Reader::getFileName() const
{
    return this->_name;
}

int Reader::getFileVersion() const
{
    return fileVersion;
}

std::istream& Reader::getStream()
{
    return this->_str;
}

void Reader::initLocalReader(std::shared_ptr<XMLReader> reader)
{
    this->localreader = reader;
}

std::shared_ptr<XMLReader> Reader::getLocalReader() const
{
    return(this->localreader);
}
