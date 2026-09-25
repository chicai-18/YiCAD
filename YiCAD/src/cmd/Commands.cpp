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

/// @file Commands.cpp
/// @brief 命令行命令管理器实现

#include <vector>
#include <QObject>
#include <QTextStream>
#include <QDomDocument>
#include <QStandardPaths>
#include "Commands.h"
#include "DmSettings.h"

#include "DmSystem.h"
#include "GuiDialogFactory.h"
#include "Debug.h"

Commands* Commands::m_pUniqueInstance = nullptr;

const char* Commands::FnPrefix = "Fn";
const char* Commands::AltPrefix = "Alt-";
const char* Commands::MetaPrefix = "Meta-";

namespace
{
/// @brief 旧格式里一条的属性名
const char* const kLegacyAttribute = "action";
/// @brief 新格式里一条的属性名
const char* const kCommandAttribute = "command";

/// @brief 读取 XML 配置文件
/// @return 文件不存在或打不开时返回空文档
QDomDocument readDocument(const QString& file)
{
    QFile f(file);
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
    {
        return QDomDocument();
    }
    QDomDocument doc(file);
    doc.setContent(&f);
    f.close();
    return doc;
}

/// @brief 一条的命令 ID：新格式直接取 command 属性，旧格式按原映射表转换
/// @return 注释节点、没有命令属性或旧格式里没有对应命令时返回空串
QString itemCommandId(const QDomNode& item)
{
    if (item.isComment())
    {
        return QString();
    }
    const QString command = item.attributes().namedItem(kCommandAttribute).nodeValue();
    if (!command.isEmpty())
    {
        return command;
    }
    const QString action = item.attributes().namedItem(kLegacyAttribute).nodeValue();
    return action.isEmpty() ? QString() : Commands::legacyCommandId(action);
}
}  // namespace

/// @brief 获取命令管理器单例实例
/// @return 唯一实例指针
Commands* Commands::instance()
{
    if (!m_pUniqueInstance)
    {
        m_pUniqueInstance = new Commands();
    }
    return m_pUniqueInstance;
}

/// @brief 构造函数，初始化命令字典并加载配置文件
Commands::Commands()
{
    m_strConfigFile = QDir::cleanPath(DMSYSTEM->getAppDir()
                      + QDir::separator() + "keyconfig.xml");
    m_strUserConfig = QDir::cleanPath(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QDir::separator() + "keyconfig.xml");
    load();
}

/// @brief 析构函数
Commands::~Commands()
{
}

QString Commands::legacyCommandId(const QString& actionName)
{
    // 原 Commands::initStrActionMap() 登记的枚举名，逐个对到现在的命令 ID。原映射表
    // 里有、但一直没有实现的（ActionFileExport/Print/Quit、ActionView*、ActionSelect*、
    // 椭圆的另三种画法、ActionScript* 等）不在这里，转换时丢弃；已搬进扩展的命令对到
    // 扩展的 ID。*NoSelect 是原先先选后建命令"已有选择集"的入口，现在与原命令合一。
    static const std::map<QString, QString> table = {
        {"ActionFileNew", "ext.file.new"},
        {"ActionFileOpen", "ext.file.open"},
        {"ActionFileSave", "ext.file.save"},
        {"ActionFileSaveAs", "ext.file.save_as"},

        {"ActionEditKillAllActions", "edit.kill_all"},
        {"ActionEditUndo", "ext.edit.undo"},
        {"ActionEditRedo", "ext.edit.redo"},
        {"ActionEditCut", "ext.edit.cut"},
        {"ActionEditCutNoSelect", "ext.edit.cut"},
        {"ActionEditCopy", "ext.edit.copy"},
        {"ActionEditCopyNoSelect", "ext.edit.copy"},
        {"ActionEditPaste", "ext.edit.paste"},

        {"ActionZoomIn", "ext.view.zoom_in"},
        {"ActionZoomOut", "ext.view.zoom_out"},
        {"ActionZoomPan", "ext.view.pan"},

        {"ActionDrawArc", "ext.draw.arc"},
        {"ActionDrawArc3P", "ext.draw.arc_3p"},
        {"ActionDrawArcTangential", "ext.draw.arc_tangential"},
        {"ActionDrawCircle", "ext.draw.circle"},
        {"ActionDrawCircle2P", "ext.draw.circle_2p"},
        {"ActionDrawCircle3P", "ext.draw.circle_3p"},
        {"ActionDrawCircleTan2", "ext.draw.circle_tan2"},
        {"ActionDrawCircleTan3", "ext.draw.circle_tan3"},
        {"ActionDrawEllipseArcAxis", "ext.draw.ellipse_arc_axis"},
        {"ActionDrawEllipseAxis", "ext.draw.ellipse_axis"},
        {"ActionDrawEllipseInscribe", "ext.draw.ellipse_inscribe"},
        {"ActionDrawHatch", "ext.hatch.draw"},
        {"ActionDrawHatchNoSelect", "ext.hatch.draw"},
        {"ActionDrawImage", "ext.draw.image"},
        {"ActionDrawLine", "ext.draw.line"},
        {"ActionDrawLineBisector", "ext.draw.line_bisector"},
        {"ActionDrawLineFree", "ext.draw.line_free"},
        {"ActionDrawLineOrthTan", "ext.draw.line_orth_tan"},
        {"ActionDrawLinePolygonCenCor", "ext.draw.line_polygon_cen_cor"},
        {"ActionDrawLinePolygonCenTan", "ext.draw.line_polygon_cen_tan"},
        {"ActionDrawLineRectangle", "ext.draw.line_rectangle"},
        {"ActionDrawLineTangent1", "ext.draw.line_tangent1"},
        {"ActionDrawLineTangent2", "ext.draw.line_tangent2"},
        {"ActionDrawMText", "ext.text.mtext"},
        {"ActionDrawPoint", "ext.draw.point"},
        {"ActionDrawRay", "ext.draw.ray"},
        {"ActionDrawSpline", "ext.draw.spline"},
        {"ActionDrawSplinePoints", "ext.draw.spline_points"},
        {"ActionDrawPolyline", "ext.draw.polyline"},
        {"ActionDrawText", "ext.text.draw"},
        {"ActionDrawXline", "ext.draw.xline"},

        {"ActionPolylineAdd", "ext.modify.polyline_add"},
        {"ActionPolylineAppend", "ext.modify.polyline_append"},
        {"ActionPolylineDel", "ext.modify.polyline_del"},
        {"ActionCloudLineRectangle", "ext.draw.cloud_line_rectangle"},
        {"ActionCloudLinePolygon", "ext.draw.cloud_line_polygon"},
        {"ActionCloudLineFree", "ext.draw.cloud_line_free"},

        {"ActionDimAligned", "ext.dim.aligned"},
        {"ActionDimLinear", "ext.dim.linear"},
        {"ActionDimRadial", "ext.dim.radial"},
        {"ActionDimDiametric", "ext.dim.diametric"},
        {"ActionDimAngular", "ext.dim.angular"},
        {"ActionDimLeader", "ext.dim.leader"},

        {"ActionModifyDelete", "ext.modify.delete"},
        {"ActionModifyDeleteNoSelect", "ext.modify.delete_no_select"},
        {"ActionModifyMove", "ext.modify.move"},
        {"ActionModifyMoveNoSelect", "ext.modify.move"},
        {"ActionModifyRotate", "ext.modify.rotate"},
        {"ActionModifyRotateNoSelect", "ext.modify.rotate"},
        {"ActionModifyScale", "ext.modify.scale"},
        {"ActionModifyScaleNoSelect", "ext.modify.scale"},
        {"ActionModifyMirror", "ext.modify.mirror"},
        {"ActionModifyMirrorNoSelect", "ext.modify.mirror"},
        {"ActionModifyEntity", "ext.modify.entity"},
        {"ActionModifyTrim", "ext.modify.trim"},
        {"ActionModifyCut", "ext.modify.cut"},
        {"ActionModifyCut2P", "ext.modify.cut_2p"},
        {"ActionModifyBevel", "ext.modify.bevel"},
        {"ActionModifyRound", "ext.modify.round"},
        {"ActionModifySingleOffset", "ext.modify.single_offset"},
        {"ActionModifyExtend", "ext.modify.extend"},
        {"ActionModifyExplode", "ext.modify.explode"},
        {"ActionModifyExplodeNoSelect", "ext.modify.explode"},
        {"ActionModifyReverse", "ext.modify.reverse"},
        {"ActionModifyReverseNoSelect", "ext.modify.reverse"},

        {"ActionSnapFree", "snap.free"},
        {"ActionSnapGrid", "snap.grid"},
        {"ActionSnapEndpoint", "snap.endpoint"},
        {"ActionSnapOnEntity", "snap.on_entity"},
        {"ActionSnapCenter", "snap.center"},
        {"ActionSnapMiddle", "snap.middle"},
        {"ActionSnapIntersection", "snap.intersection"},

        {"ActionRestrictNothing", "restrict.nothing"},
        {"ActionRestrictOrthogonal", "restrict.orthogonal"},
        {"ActionRestrictHorizontal", "restrict.horizontal"},
        {"ActionRestrictVertical", "restrict.vertical"},

        {"ActionInfoDist", "ext.measure.dist"},
        {"ActionInfoAngle", "ext.measure.angle"},
        {"ActionInfoTotalLength", "ext.measure.total_length"},
        {"ActionInfoTotalLengthNoSelect", "ext.measure.total_length"},
        {"ActionInfoArea", "ext.measure.area"},
        {"ActionInfoSelected", "ext.measure.selected"},

        {"ActionLayersDefreezeAll", "ext.layer.defreeze_all"},
        {"ActionLayersFreezeAll", "ext.layer.freeze_all"},
        {"ActionLayersUnlockAll", "ext.layer.unlock_all"},
        {"ActionLayersLockAll", "ext.layer.lock_all"},
        {"ActionLayersAdd", "ext.layer.add"},

        {"ActionBlocksSave", "ext.block.save"},
        {"ActionBlocksInsert", "ext.block.insert"},
        {"ActionBlocksCreate", "ext.block.create"},
        {"ActionBlocksCreateNoSelect", "ext.block.create"},
        {"ActionBlocksDelete", "ext.block.delete"},
        {"ActionBlocksImport", "ext.block.import"},

        {"ActionOptionsGeneral", "ext.options.general"},
        {"ActionOptionsDrawing", "ext.options.drawing"},
    };
    auto it = table.find(actionName);
    return it == table.end() ? QString() : it->second;
}

/// @brief 删除命令管理器单例实例
void Commands::deleteCommands()
{
    if (m_pUniqueInstance)
    {
        delete m_pUniqueInstance;
        m_pUniqueInstance = nullptr;
    }
}

/// @brief 将命令字符串转换为对应的命令 ID
/// @param [in] cmd 命令字符串（自动转为小写）
/// @return 对应的命令 ID，未找到返回空串
QString Commands::cmdToCommand(const QString& cmd) const
{
    auto it = m_keyCommandMap.find(cmd.toLower());
    return it == m_keyCommandMap.end() ? QString() : it->second;
}

/// @brief 将快捷键编码转换为对应的命令 ID
/// @param [in] code 按键编码字符串
/// @return 对应的命令 ID，未找到返回空串
QString Commands::keycodeToCommand(const QString& code) const
{
    if (code.size() < 1)
    {
        return QString();
    }

    QString c;

    if (!(code.startsWith(FnPrefix) || code.startsWith(AltPrefix)
          || code.startsWith(MetaPrefix)))
    {
        if (code.size() < 1
            || code.contains(QRegExp("^[a-z].*", Qt::CaseInsensitive))
               == false)
        {
            return QString();
        }
        c = code.toLower();
    }
    else
    {
        c = code;
    }
    auto it = m_keyCommandMap.find(c);

    if (it == m_keyCommandMap.end())
    {
        return QString();
    }
    // 找到
    GUIDIALOGFACTORY->commandMessage(
        QObject::tr("Accepted keycode: %1").arg(c));
    // fixme, need to handle multiple hits
    return it->second;
}

/// @brief 从程序配置文件及用户配置文件加载命令映射
/// @return true表示加载成功
bool Commands::load()
{
    // 用户目录下的旧格式文件先改写为新格式（保留备份），之后只读写新格式
    migrateLegacyConfig(m_strUserConfig);

    // 读取当前的快捷键组
    m_curGroup = DMSETTINGS->readEntry("/DefaultKeyboard", "");
    m_data = readConfigFile(m_strConfigFile, m_curGroup, false);
    loadFromData(m_data, true);
    auto data = readConfigFile(m_strUserConfig, m_curGroup, true);
    loadFromData(data, false);
    return true;
}

/// @brief 通过XML读取的数据加载到映射表中
/// @param [in] data 命令数据列表
/// @param [in] clearOld 是否清除旧数据
void Commands::loadFromData(const std::vector<CommandKeys>& data, bool clearOld)
{
    if (clearOld)
    {
        m_keyCommandMap.clear();
    }
    for (const CommandKeys& item : data)
    {
        for (const QString& key : item.keys)
        {
            m_keyCommandMap[key] = item.commandId;
        }
        auto it = std::find_if(m_data.begin(), m_data.end(),
            [&item](const CommandKeys& t)
            {
                return t.commandId == item.commandId;
            });
        if (it != m_data.end())
        {
            *it = item;
        }
        else
        {
            m_data.push_back(item);
        }
    }
}

/// @brief 读取配置文件获得所有组名
/// @return 组名列表
QStringList Commands::getGroups() const
{
    QStringList groups;
    QDomDocument doc = readDocument(m_strConfigFile);
    QDomElement groupsElem = doc.documentElement(); // groups
    QDomNodeList groupNodes = groupsElem.childNodes(); // group
    for (int i = 0; i < groupNodes.size(); i++)
    {
        QDomNode group = groupNodes.at(i);
        QString name = group.attributes().namedItem("name").nodeValue();
        if (!name.isEmpty())
        {
            groups.append(name);
        }
    }
    return groups;
}

/// @brief 获取命令对应的描述文本（翻译后的命令名）
/// @param [in] commandId 命令 ID
/// @return 描述文本，未找到返回空字符串
QString Commands::description(const QString& commandId) const
{
    if (commandId.isEmpty())
    {
        return QString();
    }
    for (const CommandKeys& item : m_data)
    {
        if (item.commandId == commandId)
        {
            return item.description;
        }
    }
    return QString();
}

/// @brief 获取命令的翻译文本（当前直接返回原命令）
/// @param [in] cmd 英文命令
/// @return 翻译后的命令
QString Commands::command(const QString& cmd)
{
    // todo ：不知道原来是想干啥
    return cmd;

    // 此处翻译啥？
    //auto it = instance()->m_cmdTranslation.find(cmd);
    //if (it != instance()->m_cmdTranslation.end())
    //{
    //	return instance()->m_cmdTranslation[cmd];
    //}
    //GUIDIALOGFACTORY->commandMessage(
    //    QObject::tr("Command not found: %1").arg(cmd));
    //return "";
}

/// @brief 检查给定字符串是否匹配指定命令
/// @param [in] cmd 要检查的命令（如 "angle"）
/// @param [in] str 用户输入的字符串
/// @return true表示匹配
bool Commands::checkCommand(const QString& cmd, const QString& str)
{
    // todo ：
    // todo ：简单改造了一下解决707bug 不知是否满足需求后续重写再考虑
    if (cmd == str)
    {
        return true;
    }
    else if (cmd == "help" || cmd == "close" || cmd == "undo")
    {
        return false;
    }

    return true;
}

/// @brief 获取"可用命令"的本地化文本
/// @return 本地化文本
QString Commands::msgAvailableCommands()
{
    return QObject::tr("Available commands:");
}

/// @brief 从命令行字符串中提取CLI计算器数学表达式
/// @param [in] cmd 命令行字符串
/// @return 数学表达式字符串，用于Math::eval()；未找到返回空字符串
QString Commands::filterCliCal(const QString& cmd)
{
    QString str = cmd.trimmed();
    const QRegExp calCmd(R"(^(cal|calculate))");
    if (!(str.contains(calCmd)
          || str.startsWith(
              QObject::tr("cal", "command to trigger cli calculator"),
              Qt::CaseInsensitive)
          || str.startsWith(
              QObject::tr("calculate",
                          "command to trigger cli calculator"),
              Qt::CaseInsensitive)))
    {
        return QString();
    }
    int index = str.indexOf(QRegExp(R"(\s)"));
    bool spaceFound = (index >= 0);
    str = str.mid(index);
    index = str.indexOf(QRegExp(R"(\S)"));
    if (!(spaceFound && index >= 0))
    {
        return QString();
    }
    str = str.mid(index);
    return str;
}

/// @brief 从配置文件查找指定名字的组，返回组内的数据
/// @param [in] configFile 配置文件路径
/// @param [in,out] group 组名，精确匹配或返回第一个组
/// @param [in] restrictMatch 是否精确匹配组
/// @return 组内的命令数据列表
std::vector<CommandKeys> Commands::readConfigFile(const QString& configFile, QString& group,
                                                  const bool restrictMatch)
{
    std::vector<CommandKeys> res;

    // 从配置文件查找指定名字的组
    QDomDocument doc = readDocument(configFile);
    QDomElement groupsElem = doc.documentElement(); // groups
    QDomNodeList groupNodes = groupsElem.childNodes(); // group
    if (groupNodes.size() == 0)
    {
        return res;
    }
    QDomNode theGroup;
    if (!restrictMatch)
    {
        theGroup = groupNodes.at(0);
    }

    for (int i = 0; i < groupNodes.size(); i++)
    {
        QDomNode curGroup = groupNodes.at(i);
        QString groupName = curGroup.attributes()
                            .namedItem("name").nodeValue();
        if (!groupName.isEmpty() && group == groupName)
        {
            theGroup = curGroup;
            break;
        }
    }
    if (theGroup.isNull())
    {
        return res;
    }

    group = theGroup.attributes().namedItem("name").nodeValue();

    // 从组获得所有快捷键
    QDomNodeList items = theGroup.childNodes();
    for (int i = 0; i < items.size(); i++)
    {
        QDomNode item = items.at(i);
        const QString commandId = itemCommandId(item);
        if (commandId.isEmpty())
        {
            continue;
        }
        QString descr = item.attributes()
                        .namedItem("description").nodeValue();
        QString keys = item.attributes().namedItem("keys").nodeValue();
        QStringList keysList;
        if (!keys.isEmpty())
        {
            QStringList list = keys.split(",");
            for (auto key : list)
            {
                QString trimedKey = key.trimmed().toLower();
                keysList.append(trimedKey);
            }
        }
        res.push_back(CommandKeys{commandId, descr, keysList});
    }
    return res;
}

/// @brief 获取当前命令数据
/// @return 命令数据列表
std::vector<CommandKeys> Commands::getData() const
{
    return m_data;
}

/// @brief 保存命令数据到文件
/// @param [in] data 命令数据列表
/// @param [in] group 组名
/// @param [in] file 文件路径
/// @return true表示保存成功
bool Commands::saveToFile(const std::vector<CommandKeys>& data, const QString& group, const QString& file)
{
    QFileInfo fi(file);
    if (!fi.dir().exists())
    {
        QDir().mkpath(fi.dir().path());
    }
    QDomDocument doc = readDocument(file);

    // 删除原group
    QDomElement groupsElem = doc.documentElement(); // groups
    if (groupsElem.isNull())
    {
        groupsElem = doc.createElement("groups");
        QDomProcessingInstruction pro =
            doc.createProcessingInstruction("xml",
                "version=\"1.0\" encoding=\"utf-8\"");
        doc.appendChild(pro);
    }
    QDomNodeList groupNodes = groupsElem.childNodes(); // group
    if (groupNodes.size() > 0)
    {
        // 删除原group
        QDomNode oldGroup;
        for (int i = 0; i < groupNodes.size(); i++)
        {
            QString name = groupNodes.at(i).attributes()
                           .namedItem("name").nodeValue();
            if (name == group)
            {
                oldGroup = groupNodes.at(i);
                break;
            }
        }
        if (!oldGroup.isNull())
        {
            groupsElem.removeChild(oldGroup);
        }
    }

    // 填充数据
    QDomElement groupNode = doc.createElement("group");
    groupNode.setAttribute("name", group);
    for (const CommandKeys& item : data)
    {
        QDomElement newElem = doc.createElement("item");
        newElem.setAttribute(kCommandAttribute, item.commandId);
        newElem.setAttribute("description", item.description);
        newElem.setAttribute("keys", item.keys.join(","));
        groupNode.appendChild(newElem);
    }
    groupsElem.appendChild(groupNode);
    doc.appendChild(groupsElem);

    // 写入
    QFile f(file);
    if (!f.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
    {
        return false;
    }
    QTextStream out(&f);
    //QString docStr = doc.toString();
    //out << docStr; // 用这个容易乱码
    doc.save(out, 4);
    f.close();
    return true;
}

bool Commands::migrateLegacyConfig(const QString& file)
{
    QDomDocument doc = readDocument(file);
    QDomElement groupsElem = doc.documentElement();
    if (groupsElem.isNull())
    {
        return false;
    }

    // 只要有一条旧格式的条目就整份改写
    bool legacy = false;
    QDomNodeList groupNodes = groupsElem.childNodes();
    for (int i = 0; i < groupNodes.size() && !legacy; i++)
    {
        QDomNodeList items = groupNodes.at(i).childNodes();
        for (int j = 0; j < items.size(); j++)
        {
            if (!items.at(j).isComment() && items.at(j).attributes().contains(kLegacyAttribute))
            {
                legacy = true;
                break;
            }
        }
    }
    if (!legacy)
    {
        return false;
    }

    // 备份原文件；已有备份（更早一次迁移留下的）不覆盖
    const QString backup = file + QStringLiteral(".bak");
    if (!QFile::exists(backup) && !QFile::copy(file, backup))
    {
        return false;
    }

    // 逐组转换：saveToFile 每次替换一组，按原顺序依次写入。先清空文件里的组，
    // 否则第一次写入读到的仍是旧格式的其它组
    std::vector<std::pair<QString, std::vector<CommandKeys>>> groups;
    for (int i = 0; i < groupNodes.size(); i++)
    {
        QString name = groupNodes.at(i).attributes().namedItem("name").nodeValue();
        if (name.isEmpty())
        {
            continue;
        }
        groups.emplace_back(name, readConfigFile(file, name, true));
    }
    if (!QFile::remove(file))
    {
        return false;
    }
    for (const auto& [name, data] : groups)
    {
        if (!saveToFile(data, name, file))
        {
            return false;
        }
    }
    return true;
}

/// @brief 获取命令行别名到命令 ID 的映射
/// @return 命令映射表
std::map<QString, QString> Commands::getKeyCommands() const
{
    return m_keyCommandMap;
}

/// @brief 获取配置文件路径
/// @return 配置文件全路径
QString Commands::getConfigFile() const
{
    return m_strConfigFile;
}

/// @brief 获取用户配置文件路径
/// @return 用户配置文件全路径
QString Commands::getUserConfig() const
{
    return m_strUserConfig;
}
