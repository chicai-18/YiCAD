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

/// @file XmlWriterInterface.h
/// @brief XML写入器抽象接口类头文件

#ifndef XMLWRITERINTERFACE_H
#define XMLWRITERINTERFACE_H

#include <string>

class XMLWriterInterface
{
public:
    virtual void createRootElement(const std::string& name, const std::string& default_namespace_uri = "") = 0;

    virtual void addElement(const std::string& name, const std::string& namespace_uri = "") = 0;

    virtual void addAttribute(const std::string& name, const std::string& value, const std::string& namespace_uri = "") = 0;

    virtual void addNamespaceDeclaration(const std::string& prefix, const std::string& namespace_uri) = 0;

    virtual void closeElement() = 0;

    virtual std::string documentAsString() = 0;

    XMLWriterInterface() = default;
    virtual ~XMLWriterInterface() = default;
};

#endif
