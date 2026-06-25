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

/// @file XMLWriterQXmlStreamWriter.cpp
/// @brief 基于QXmlStreamWriter的XML写入器类实现

#include "XMLWriterQXmlStreamWriter.h"

#include <QXmlStreamWriter>

#include "XmlWriterInterface.h"

XMLWriterQXmlStreamWriter::XMLWriterQXmlStreamWriter()
    : xmlWriter(new QXmlStreamWriter(&xml))
{
	xmlWriter->setAutoFormatting(true);
	xmlWriter->setCodec("UTF-8");
}

XMLWriterQXmlStreamWriter::~XMLWriterQXmlStreamWriter() = default;

void XMLWriterQXmlStreamWriter::createRootElement(const std::string &name, const std::string &namespace_uri)
{
    xmlWriter->writeStartDocument();
    xmlWriter->writeDefaultNamespace(QString::fromStdString(namespace_uri));
    xmlWriter->writeStartElement(QString::fromStdString(namespace_uri), QString::fromStdString(name));
}

void XMLWriterQXmlStreamWriter::addElement(const std::string &name, const std::string &namespace_uri)
{
    xmlWriter->writeStartElement(QString::fromStdString(namespace_uri), QString::fromStdString(name));
}

void XMLWriterQXmlStreamWriter::addAttribute(const std::string &name, const std::string &value, const std::string &namespace_uri)
{
    xmlWriter->writeAttribute(QString::fromStdString(namespace_uri), QString::fromStdString(name), QString::fromStdString(value));
}

void XMLWriterQXmlStreamWriter::addNamespaceDeclaration(const std::string &prefix, const std::string &namespace_uri)
{
    xmlWriter->writeNamespace(QString::fromStdString(namespace_uri), QString::fromStdString(prefix));
}

void XMLWriterQXmlStreamWriter::closeElement()
{
    xmlWriter->writeEndElement();
}

std::string XMLWriterQXmlStreamWriter::documentAsString()
{
    xmlWriter->writeEndDocument();

    return xml.toStdString();
}
