/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file XMLWriterQXmlStreamWriter.h
/// @brief 基于QXmlStreamWriter的XML写入器类头文件

#ifndef XMLWRITERQXMLSTREAMWRITER_H
#define XMLWRITERQXMLSTREAMWRITER_H

#include <QString>
#include <memory>

#include "XmlWriterInterface.h"

class QXmlStreamWriter;

class XMLWriterQXmlStreamWriter : public XMLWriterInterface
{
public:
    XMLWriterQXmlStreamWriter();

    ~XMLWriterQXmlStreamWriter();

    void createRootElement(const std::string& name, const std::string& namespace_uri = "");

    void addElement(const std::string& name, const std::string& namespace_uri = "");

    void addAttribute(const std::string& name, const std::string& value, const std::string& namespace_uri = "");

    void addNamespaceDeclaration(const std::string& prefix, const std::string& namespace_uri);

    void closeElement();

    std::string documentAsString();

private:
    std::unique_ptr<QXmlStreamWriter> xmlWriter;

    QString xml;
};

#endif
