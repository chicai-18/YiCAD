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

/// @file TextStyleTableCmd.h
/// @brief 文字样式表相关的 Undo/Redo 命令类

#ifndef TEXTSTYLETABLECMD_H
#define TEXTSTYLETABLECMD_H

#include "Cmd.h"
#include "DmTextStyle.h"
#include <string>

class DmTextStyleTable;
class DmTextStyle;

/// @brief 文字样式表添加命令
class TextStyleTableAddCmd : public ICmd
{
public:
    /// @brief 构造函数
    /// @param [in] table 文字样式表指针
    /// @param [in] addedTextStyle 被添加的文字样式指针
    TextStyleTableAddCmd(DmTextStyleTable* table, DmTextStyle* addedTextStyle);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

    DmObject* getObject() override
    {
        return m_addedTextStyle;
    }

    CmdType getCmdType() const override
    {
        return CmdType::TextStyleTableAddCmd;
    }

protected:
    DmTextStyleTable* m_table = nullptr;          ///< 文字样式表指针
    DmTextStyle* m_addedTextStyle = nullptr;      ///< 被添加的文字样式指针
    bool m_isOwnByCommand = false;                ///< 是否由该命令所有
};


/// @brief 文字样式表修改命令
class TextStyleTableModifyCmd : public ICmd
{
public:
    TextStyleTableModifyCmd()
        : m_table(nullptr)
        , m_modifiedTextStyle(nullptr)
    {
    }

    /// @brief 构造函数
    /// @param [in] table 文字样式表指针
    /// @param [in] modifiedTextStyle 被修改的文字样式指针
    TextStyleTableModifyCmd(DmTextStyleTable* table, DmTextStyle* modifiedTextStyle);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

    DmObject* getObject() override
    {
        return m_modifiedTextStyle;
    }

    CmdType getCmdType() const override
    {
        return CmdType::TextStyleTableModifyCmd;
    }

protected:
    std::string m_originData;                     ///< 原始序列化数据
    std::string m_newData;                        ///< 新序列化数据
    DmTextStyle* m_modifiedTextStyle = nullptr;   ///< 被修改的文字样式指针
    DmTextStyleTable* m_table = nullptr;          ///< 文字样式表指针
};


/// @brief 文字样式表移除命令
class TextStyleTableRemoveCmd : public ICmd
{
public:
    /// @brief 构造函数
    /// @param [in] table 文字样式表指针
    /// @param [in] removedTextStyle 被移除的文字样式指针
    TextStyleTableRemoveCmd(DmTextStyleTable* table, DmTextStyle* removedTextStyle);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

    DmObject* getObject() override
    {
        return m_removedTextStyle;
    }

    CmdType getCmdType() const override
    {
        return CmdType::TextStyleTableRemoveCmd;
    }

protected:
    DmTextStyleTable* m_table = nullptr;          ///< 文字样式表指针
    DmTextStyle* m_removedTextStyle = nullptr;    ///< 被移除的文字样式指针
};


/// @brief 文字样式表激活命令
class TextStyleTableActivateCmd : public ICmd
{
public:
    /// @brief 构造函数
    /// @param [in] table 文字样式表指针
    /// @param [in] activatedTextStyle 被激活的文字样式指针
    TextStyleTableActivateCmd(DmTextStyleTable* table, DmTextStyle* activatedTextStyle);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

    CmdType getCmdType() const override
    {
        return CmdType::TextStyleTableActivateCmd;
    }

protected:
    DmTextStyleTable* m_table = nullptr;              ///< 文字样式表指针
    DmTextStyle* m_activatedTextStyle = nullptr;      ///< 被激活的文字样式指针
    DmTextStyle* m_originActiveTextStyle = nullptr;   ///< 原始激活的文字样式指针
};

#endif //TEXTSTYLETABLECMD_H
