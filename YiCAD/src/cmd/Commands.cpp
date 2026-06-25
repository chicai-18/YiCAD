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

bool Commands::g_strActMapInilized = false;
std::map<QString, DM::ActionType> Commands::m_strActMap;
std::map<DM::ActionType, QString> Commands::m_actStrMap;

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
    initStrActionMap();
    load();
}

/// @brief 析构函数
Commands::~Commands()
{
}

/// @brief 初始化字符串到ActionType的静态映射表
void Commands::initStrActionMap()
{
    if (g_strActMapInilized)
    {
        return;
    }
    g_strActMapInilized = true;

    // 全部DM::ActionType的翻译映射
    m_strActMap["ActionNone"] = DM::ActionNone;
    m_strActMap["ActionDefault"] = DM::ActionDefault;

    m_strActMap["ActionFileNew"] = DM::ActionFileNew;
    m_strActMap["ActionFileOpen"] = DM::ActionFileOpen;
    m_strActMap["ActionFileSave"] = DM::ActionFileSave;
    m_strActMap["ActionFileSaveAs"] = DM::ActionFileSaveAs;
    m_strActMap["ActionFileExport"] = DM::ActionFileExport;
    m_strActMap["ActionFileClose"] = DM::ActionFileClose;
    m_strActMap["ActionFilePrint"] = DM::ActionFilePrint;
    m_strActMap["ActionFilePrintPDF"] = DM::ActionFilePrintPDF;
    m_strActMap["ActionFilePrintPreview"] = DM::ActionFilePrintPreview;

    m_strActMap["ActionFileQuit"] = DM::ActionFileQuit;

    m_strActMap["ActionEditKillAllActions"] = DM::ActionEditKillAllActions;
    m_strActMap["ActionEditUndo"] = DM::ActionEditUndo;
    m_strActMap["ActionEditRedo"] = DM::ActionEditRedo;
    m_strActMap["ActionEditCut"] = DM::ActionEditCut;
    m_strActMap["ActionEditCutNoSelect"] = DM::ActionEditCutNoSelect;
    m_strActMap["ActionEditCopy"] = DM::ActionEditCopy;
    m_strActMap["ActionEditCopyNoSelect"] = DM::ActionEditCopyNoSelect;
    m_strActMap["ActionEditPaste"] = DM::ActionEditPaste;

    m_strActMap["ActionViewStatusBar"] = DM::ActionViewStatusBar;
    m_strActMap["ActionViewLayerTable"] = DM::ActionViewLayerTable;
    m_strActMap["ActionViewBlockList"] = DM::ActionViewBlockList;
    m_strActMap["ActionViewCommandLine"] = DM::ActionViewCommandLine;
    m_strActMap["ActionViewLibrary"] = DM::ActionViewLibrary;

    m_strActMap["ActionViewPenToolbar"] = DM::ActionViewPenToolbar;
    m_strActMap["ActionViewOptionToolbar"] = DM::ActionViewOptionToolbar;
    m_strActMap["ActionViewCadToolbar"] = DM::ActionViewCadToolbar;
    m_strActMap["ActionViewFileToolbar"] = DM::ActionViewFileToolbar;
    m_strActMap["ActionViewEditToolbar"] = DM::ActionViewEditToolbar;
    m_strActMap["ActionViewSnapToolbar"] = DM::ActionViewSnapToolbar;

    m_strActMap["ActionViewGrid"] = DM::ActionViewGrid;
    m_strActMap["ActionViewDraft"] = DM::ActionViewDraft;

    m_strActMap["ActionZoomIn"] = DM::ActionZoomIn;
    m_strActMap["ActionZoomOut"] = DM::ActionZoomOut;
    m_strActMap["ActionZoomPan"] = DM::ActionZoomPan;

    m_strActMap["ActionSelect"] = DM::ActionSelect;
    m_strActMap["ActionSelectSingle"] = DM::ActionSelectSingle;
    m_strActMap["ActionSelectMultiple"] = DM::ActionSelectMultiple;

    m_strActMap["ActionDrawArc"] = DM::ActionDrawArc;
    m_strActMap["ActionDrawArc3P"] = DM::ActionDrawArc3P;
    m_strActMap["ActionDrawArcTangential"] = DM::ActionDrawArcTangential;
    m_strActMap["ActionDrawCircle"] = DM::ActionDrawCircle;
    m_strActMap["ActionDrawCircle2P"] = DM::ActionDrawCircle2P;
    m_strActMap["ActionDrawCircle3P"] = DM::ActionDrawCircle3P;
    m_strActMap["ActionDrawCircleTan2"] = DM::ActionDrawCircleTan2;
    m_strActMap["ActionDrawCircleTan3"] = DM::ActionDrawCircleTan3;

    m_strActMap["ActionDrawEllipseArcAxis"] = DM::ActionDrawEllipseArcAxis;
    m_strActMap["ActionDrawEllipseAxis"] = DM::ActionDrawEllipseAxis;
    m_strActMap["ActionDrawEllipseFociPoint"] = DM::ActionDrawEllipseFociPoint;
    m_strActMap["ActionDrawEllipse4Points"] = DM::ActionDrawEllipse4Points;
    m_strActMap["ActionDrawEllipseCenter3Points"] =
        DM::ActionDrawEllipseCenter3Points;
    m_strActMap["ActionDrawEllipseInscribe"] = DM::ActionDrawEllipseInscribe;

    m_strActMap["ActionDrawHatch"] = DM::ActionDrawHatch;
    m_strActMap["ActionDrawHatchNoSelect"] = DM::ActionDrawHatchNoSelect;
    m_strActMap["ActionDrawImage"] = DM::ActionDrawImage;
    m_strActMap["ActionDrawLine"] = DM::ActionDrawLine;
    m_strActMap["ActionDrawLineBisector"] = DM::ActionDrawLineBisector;
    m_strActMap["ActionDrawLineFree"] = DM::ActionDrawLineFree;
    m_strActMap["ActionDrawLineOrthTan"] = DM::ActionDrawLineOrthTan;
    m_strActMap["ActionDrawLinePolygonCenCor"] =
        DM::ActionDrawLinePolygonCenCor;
    m_strActMap["ActionDrawLinePolygonCenTan"] =
        DM::ActionDrawLinePolygonCenTan;
    m_strActMap["ActionDrawLineRectangle"] = DM::ActionDrawLineRectangle;
    m_strActMap["ActionDrawLineTangent1"] = DM::ActionDrawLineTangent1;
    m_strActMap["ActionDrawLineTangent2"] = DM::ActionDrawLineTangent2;
    m_strActMap["ActionDrawMText"] = DM::ActionDrawMText;
    m_strActMap["ActionDrawPoint"] = DM::ActionDrawPoint;
    m_strActMap["ActionDrawRay"] = DM::ActionDrawRay;
    m_strActMap["ActionDrawSpline"] = DM::ActionDrawSpline;
    m_strActMap["ActionDrawSplinePoints"] = DM::ActionDrawSplinePoints;
    m_strActMap["ActionDrawPolyline"] = DM::ActionDrawPolyline;
    m_strActMap["ActionDrawText"] = DM::ActionDrawText;
    m_strActMap["ActionDrawXline"] = DM::ActionDrawXline;

    m_strActMap["ActionPolylineAdd"] = DM::ActionPolylineAdd;
    m_strActMap["ActionPolylineAppend"] = DM::ActionPolylineAppend;
    m_strActMap["ActionPolylineDel"] = DM::ActionPolylineDel;
    m_strActMap["ActionCloudLineRectangle"] = DM::ActionCloudLineRectangle;
    m_strActMap["ActionCloudLinePolygon"] = DM::ActionCloudLinePolygon;
    m_strActMap["ActionCloudLineFree"] = DM::ActionCloudLineFree;

    m_strActMap["ActionDimAligned"] = DM::ActionDimAligned;
    m_strActMap["ActionDimLinear"] = DM::ActionDimLinear;
    m_strActMap["ActionDimRadial"] = DM::ActionDimRadial;
    m_strActMap["ActionDimDiametric"] = DM::ActionDimDiametric;
    m_strActMap["ActionDimAngular"] = DM::ActionDimAngular;
    m_strActMap["ActionDimLeader"] = DM::ActionDimLeader;

    m_strActMap["ActionModifyDelete"] = DM::ActionModifyDelete;
    m_strActMap["ActionModifyDeleteNoSelect"] =
        DM::ActionModifyDeleteNoSelect;
    m_strActMap["ActionModifyMove"] = DM::ActionModifyMove;
    m_strActMap["ActionModifyMoveNoSelect"] = DM::ActionModifyMoveNoSelect;
    m_strActMap["ActionModifyRotate"] = DM::ActionModifyRotate;
    m_strActMap["ActionModifyRotateNoSelect"] =
        DM::ActionModifyRotateNoSelect;
    m_strActMap["ActionModifyScale"] = DM::ActionModifyScale;
    m_strActMap["ActionModifyScaleNoSelect"] =
        DM::ActionModifyScaleNoSelect;
    m_strActMap["ActionModifyMirror"] = DM::ActionModifyMirror;
    m_strActMap["ActionModifyMirrorNoSelect"] =
        DM::ActionModifyMirrorNoSelect;
    m_strActMap["ActionModifyEntity"] = DM::ActionModifyEntity;
    m_strActMap["ActionModifyTrim"] = DM::ActionModifyTrim;
    m_strActMap["ActionModifyCut"] = DM::ActionModifyCut;
    m_strActMap["ActionModifyCut2P"] = DM::ActionModifyCut2P;
    m_strActMap["ActionModifyBevel"] = DM::ActionModifyBevel;
    m_strActMap["ActionModifyRound"] = DM::ActionModifyRound;
    m_strActMap["ActionModifySingleOffset"] = DM::ActionModifySingleOffset;
    m_strActMap["ActionModifyExtend"] = DM::ActionModifyExtend;

    m_strActMap["ActionSnapFree"] = DM::ActionSnapFree;
    m_strActMap["ActionSnapGrid"] = DM::ActionSnapGrid;
    m_strActMap["ActionSnapEndpoint"] = DM::ActionSnapEndpoint;
    m_strActMap["ActionSnapOnEntity"] = DM::ActionSnapOnEntity;
    m_strActMap["ActionSnapCenter"] = DM::ActionSnapCenter;
    m_strActMap["ActionSnapMiddle"] = DM::ActionSnapMiddle;
    m_strActMap["ActionSnapIntersection"] = DM::ActionSnapIntersection;

    m_strActMap["ActionRestrictNothing"] = DM::ActionRestrictNothing;
    m_strActMap["ActionRestrictOrthogonal"] = DM::ActionRestrictOrthogonal;
    m_strActMap["ActionRestrictHorizontal"] = DM::ActionRestrictHorizontal;
    m_strActMap["ActionRestrictVertical"] = DM::ActionRestrictVertical;

    m_strActMap["ActionInfoDist"] = DM::ActionInfoDist;
    m_strActMap["ActionInfoAngle"] = DM::ActionInfoAngle;
    m_strActMap["ActionInfoTotalLength"] = DM::ActionInfoTotalLength;
    m_strActMap["ActionInfoTotalLengthNoSelect"] =
        DM::ActionInfoTotalLengthNoSelect;
    m_strActMap["ActionInfoArea"] = DM::ActionInfoArea;
    m_strActMap["ActionInfoSelected"] = DM::ActionInfoSelected;

    m_strActMap["ActionLayersDefreezeAll"] = DM::ActionLayersDefreezeAll;
    m_strActMap["ActionLayersFreezeAll"] = DM::ActionLayersFreezeAll;
    m_strActMap["ActionLayersUnlockAll"] = DM::ActionLayersUnlockAll;
    m_strActMap["ActionLayersLockAll"] = DM::ActionLayersLockAll;
    m_strActMap["ActionLayersAdd"] = DM::ActionLayersAdd;

    m_strActMap["ActionBlocksSave"] = DM::ActionBlocksSave;
    m_strActMap["ActionBlocksInsert"] = DM::ActionBlocksInsert;
    m_strActMap["ActionBlocksCreate"] = DM::ActionBlocksCreate;
    m_strActMap["ActionBlocksCreateNoSelect"] =
        DM::ActionBlocksCreateNoSelect;
    m_strActMap["ActionBlocksDelete"] = DM::ActionBlocksDelete;
    m_strActMap["ActionModifyExplode"] = DM::ActionModifyExplode;
    m_strActMap["ActionModifyExplodeNoSelect"] =
        DM::ActionModifyExplodeNoSelect;
    m_strActMap["ActionModifyReverse"] = DM::ActionModifyReverse;
    m_strActMap["ActionModifyReverseNoSelect"] =
        DM::ActionModifyReverseNoSelect;
    m_strActMap["ActionBlocksImport"] = DM::ActionBlocksImport;

    m_strActMap["ActionOptionsGeneral"] = DM::ActionOptionsGeneral;
    m_strActMap["ActionOptionsDrawing"] = DM::ActionOptionsDrawing;

    m_strActMap["ActionScriptOpenIDE"] = DM::ActionScriptOpenIDE;
    m_strActMap["ActionScriptRun"] = DM::ActionScriptRun;

    m_strActMap["ActionLast"] = DM::ActionLast;

    // 反向的映射
    for (auto& kv : m_strActMap)
    {
        DM::ActionType type = kv.second;
        QString name = kv.first;
        m_actStrMap[type] = name;
    }
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

/// @brief 将命令字符串转换为对应的ActionType
/// @param [in] cmd 命令字符串（自动转为小写）
/// @param [in] verbose 是否输出详细信息
/// @return 对应的ActionType，未找到返回ActionNone
DM::ActionType Commands::cmdToAction(const QString& cmd, bool verbose)
{
    QString full = cmd.toLower();
    DM::ActionType ret = DM::ActionNone;

    // 查找命令:
    if (m_keyActMap.count(full))
    {
        ret = m_keyActMap[full];
    }
    else
    {
        return ret;
    }

    if (!verbose)
    {
        return ret;
    }
    return ret;
}

/// @brief 将快捷键编码转换为对应的ActionType
/// @param [in] code 按键编码字符串
/// @return 对应的ActionType，未找到返回ActionNone
DM::ActionType Commands::keycodeToAction(const QString& code)
{
    if (code.size() < 1)
    {
        return DM::ActionNone;
    }

    QString c;

    if (!(code.startsWith(FnPrefix) || code.startsWith(AltPrefix)
          || code.startsWith(MetaPrefix)))
    {
        if (code.size() < 1
            || code.contains(QRegExp("^[a-z].*", Qt::CaseInsensitive))
               == false)
        {
            return DM::ActionNone;
        }
        c = code.toLower();
    }
    else
    {
        c = code;
    }
    auto it = m_keyActMap.find(c);

    if (it == m_keyActMap.end())
    {
        return DM::ActionNone;
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
void Commands::loadFromData(
    const std::vector<std::tuple<DM::ActionType, QString, QStringList>>& data,
    bool clearOld)
{
    if (clearOld)
    {
        m_actKeysMap.clear();
        m_keyActMap.clear();
        //m_acts.clear();
    }
    for (auto& item : data)
    {
        DM::ActionType type = std::get<0>(item);
        QString desr = std::get<1>(item);
        QStringList keys = std::get<2>(item);
        m_actKeysMap[type] = keys;
        for (auto key : keys)
        {
            m_keyActMap[key] = type;
        }
        auto it = std::find_if(m_data.begin(), m_data.end(),
            [&type](const std::tuple<DM::ActionType, QString, QStringList>& t)
            {
                return std::get<0>(t) == type;
            });
        if (it != m_data.end())
        {
            *it = std::make_tuple(type, desr, keys);
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
    QFile configFile(m_strConfigFile);
    if (!configFile.exists())
    {
        return groups;
    }
    if (!configFile.open(QIODevice::ReadOnly))
    {
        return groups;
    }
    QDomDocument doc(m_strConfigFile);
    doc.setContent(&configFile);
    configFile.close();

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

/// @brief 获取ActionType对应的描述文本（翻译后的命令名）
/// @param [in] type ActionType
/// @return 描述文本，未找到返回空字符串
QString Commands::description(DM::ActionType type) const
{
    for (auto& item : m_data)
    {
        if (std::get<0>(item) == type)
        {
            return std::get<1>(item);
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
/// @param [in] action 相关的ActionType
/// @param [in] str 用户输入的字符串
/// @return true表示匹配
bool Commands::checkCommand(const QString& cmd, const QString& str,
                            DM::ActionType /*action*/)
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
std::vector<std::tuple<DM::ActionType, QString, QStringList>>
    Commands::readConfigFile(const QString& configFile, QString& group,
                             const bool restrictMatch)
{
    std::vector<std::tuple<DM::ActionType, QString, QStringList>> res;

    // 从配置文件查找指定名字的组
    QFile configFileF(configFile);
    if (!configFileF.exists())
    {
        return res;
    }
    if (!configFileF.open(QIODevice::ReadOnly))
    {
        return res;
    }
    QDomDocument doc(configFile);
    doc.setContent(&configFileF);
    configFileF.close();

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
        if (item.isComment())
        {
            continue;
        }
        QString action = item.attributes().namedItem("action").nodeValue();
        if (action.isEmpty())
        {
            continue;
        }
        auto it = m_strActMap.find(action);
        if (m_strActMap.end() == it)
        {
            continue;
        }
        DM::ActionType type = it->second;
        QString descr = item.attributes()
                        .namedItem("description").nodeValue();
        QString keys = item.attributes().namedItem("keys").nodeValue();
        QStringList keysList;
        if (keys.isEmpty())
        {
            keysList = QStringList();
        }
        else
        {
            QStringList list = keys.split(",");
            for (auto key : list)
            {
                QString trimedKey = key.trimmed().toLower();
                keysList.append(trimedKey);
                //m_keyActMap[trimedKey] = type;
            }
        }
        res.emplace_back(std::make_tuple(type, descr, keysList));
    }
    return res;
}

/// @brief 获取当前命令数据
/// @return 命令数据列表
std::vector<std::tuple<DM::ActionType, QString, QStringList>>
    Commands::getData() const
{
    return m_data;
}

/// @brief 保存命令数据到文件
/// @param [in] data 命令数据列表
/// @param [in] group 组名
/// @param [in] file 文件路径
/// @return true表示保存成功
bool Commands::saveToFile(
    const std::vector<std::tuple<DM::ActionType, QString, QStringList>>& data,
    const QString& group, const QString& file)
{
    QFileInfo fi(file);
    if (!fi.dir().exists())
    {
        QString path = fi.dir().path();
        bool res = QDir().mkpath(path);
    }
    QFile f(file);
    QDomDocument doc;
    if (f.exists())
    {
        f.open(QIODevice::ReadOnly);
        doc = QDomDocument(file);
        doc.setContent(&f);
        f.close();
    }

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
    for (int i = 0; i < data.size(); i++)
    {
        auto item = data.at(i);
        DM::ActionType act = std::get<0>(item);
        QString desr = std::get<1>(item);
        QStringList keys = std::get<2>(item);
        QString strKeys = keys.join(",");
        QDomElement newElem = doc.createElement("item");
        newElem.setAttribute("action", m_actStrMap[act]);
        newElem.setAttribute("description", desr);
        newElem.setAttribute("keys", strKeys);
        groupNode.appendChild(newElem);
    }
    groupsElem.appendChild(groupNode);
    doc.appendChild(groupsElem);

    // 写入
    f.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text);
    QTextStream out(&f);
    //QString docStr = doc.toString();
    //out << docStr; // 用这个容易乱码
    doc.save(out, 4);
    f.close();
    return true;
}

/// @brief 获取命令到ActionType的映射
/// @return 命令映射表
std::map<QString, DM::ActionType> Commands::getActionCommands()
{
    return m_keyActMap;
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
