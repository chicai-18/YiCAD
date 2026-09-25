/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file DrawImageCommand.cpp
/// @brief 插入图片命令与工具的实现

#include "DrawImageCommand.h"

#include <cmath>

#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "DrawCommands.h"
#include "Commands.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmImage.h"
#include "DmLine.h"
#include "DmSettings.h"
#include "DmSystem.h"
#include "DmUnits.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Math2d.h"
#include "Transaction.h"
#include "UIDialogRunner.h"

namespace
{
constexpr int IMAGE_DEFAULT_WIDTH = 50.0;  ///< 默认图片宽度（像素）
constexpr int IMAGE_DEFAULT_HEIGHT = 50.0; ///< 默认图片高度（像素）
constexpr double IMAGE_ZERO_COORD = 0.0;   ///< 默认坐标零值
constexpr double IMAGE_UNIT_SCALE = 1.0;   ///< 默认单位缩放
constexpr int IMAGE_DEFAULT_DPI = 0;       ///< 默认 DPI（0 表示未设置）

/// @brief 选择要插入的图片文件；上次的目录与过滤器记在设置的 /Paths 组
/// @return 图片的完整路径；取消时返回空串
QString chooseImageFile()
{
    DMSETTINGS->beginGroup("/Paths");
    QString defDir = DMSETTINGS->readEntry("/OpenImage", DMSYSTEM->getHomeDir());
    QString defFilter = DMSETTINGS->readEntry("/ImageFilter", "");
    DMSETTINGS->endGroup();

    QStringList filters;
    QString all;
    bool haveJpeg = false;
    for (const QByteArray& format : QImageReader::supportedImageFormats())
    {
        if (format.toUpper() == "JPG" || format.toUpper() == "JPEG")
        {
            if (!haveJpeg)
            {
                haveJpeg = true;
                filters.append("jpeg (*.jpeg *.jpg)");
                all += " *.jpeg *.jpg";
            }
        }
        else
        {
            filters.append(QString("%1 (*.%1)").arg(QString(format)));
            all += QString(" *.%1").arg(QString(format));
        }
    }
    const QString allImageFiles = DrawImageCommand::tr("All Image Files (%1)").arg(all);
    filters.append(allImageFiles);
    filters.append(DrawImageCommand::tr("All Files (*.*)"));

    QFileDialog fileDlg(nullptr, "");
    fileDlg.setModal(true);
    fileDlg.setFileMode(QFileDialog::ExistingFile);
    fileDlg.setWindowTitle(DrawImageCommand::tr("Open Image"));
    fileDlg.setDirectory(defDir);
    fileDlg.setNameFilters(filters);
    if (defFilter.isEmpty())
    {
        defFilter = allImageFiles;
    }
    fileDlg.selectNameFilter(defFilter);

    QString fileName;
    if (UIDialogRunner::exec(fileDlg) == QDialog::Accepted)
    {
        const QStringList selectedFiles = fileDlg.selectedFiles();
        if (!selectedFiles.isEmpty())
        {
            fileName = selectedFiles.first();
        }

        DMSETTINGS->beginGroup("/Paths");
        DMSETTINGS->writeEntry("/OpenImage", QFileInfo(fileName).absolutePath());
        DMSETTINGS->writeEntry("/ImageFilter", fileDlg.selectedNameFilter());
        DMSETTINGS->endGroup();
    }
    return fileName;
}

/// @brief 插入图片工具：指定插入点；命令行可改角度、缩放、DPI
class DrawImageTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        ShowDialog,     ///< 显示图片选择对话框（由命令在启动时完成）
        SetTargetPoint, ///< 设置插入参考点
        SetAngle,       ///< 在命令行设置旋转角度
        SetFactor,      ///< 在命令行设置缩放因子
        SetDPI          ///< 在命令行设置 DPI
    };

    DrawImageTool(DrawImageCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
        setStatus(SetTargetPoint);
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetTargetPoint:
            GUIDIALOGFACTORY->updateMouseWidget(DrawImageCommand::tr("Specify reference point"),
                                                DrawImageCommand::tr("Cancel"));
            break;
        case SetAngle:
            GUIDIALOGFACTORY->updateMouseWidget(DrawImageCommand::tr("Enter angle:"), "");
            break;
        case SetFactor:
            GUIDIALOGFACTORY->updateMouseWidget(DrawImageCommand::tr("Enter factor:"), "");
            break;
        case SetDPI:
            GUIDIALOGFACTORY->updateMouseWidget(DrawImageCommand::tr("Enter dpi:"), "");
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        if (status() == SetTargetPoint)
        {
            m_command.previewFrame(snapper()->snapPoint(e));
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            onCoordinate(snapper()->snapPoint(e));
        }
        else if (e->button() == Qt::RightButton)
        {
            command().finish();
        }
    }

    void onCoordinate(const DmVector& pos) override
    {
        m_command.commitImage(pos);
    }

    void onCommand(GuiCommandEvent* e) override
    {
        QString c = e->getCommand().toLower();
        if (Commands::checkCommand("help", c))
        {
            // 与原 Action 一致：列出命令后不接受
            GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands() + availableCommands().join(", "));
            return;
        }

        switch (status())
        {
        case SetTargetPoint:
            // 与原 Action 一致（既有缺陷）：Commands::checkCommand 对 "angle" 一律返回真，
            // 任何文字都进入输入角度；这段文字不被接受
            if (Commands::checkCommand("angle", c))
            {
                enterInput(SetAngle);
            }
            else if (Commands::checkCommand("factor", c))
            {
                enterInput(SetFactor);
            }
            else if (Commands::checkCommand("dpi", c))
            {
                enterInput(SetDPI);
            }
            break;
        case SetAngle:
        case SetFactor:
        case SetDPI:
        {
            bool ok = false;
            double value = Math2d::eval(c, &ok);
            if (ok)
            {
                if (status() == SetAngle)
                {
                    m_command.setAngle(Math2d::deg2rad(value));
                }
                else if (status() == SetFactor)
                {
                    m_command.setFactor(value);
                }
                else
                {
                    m_command.setFactor(m_command.dpiToScale(value));
                }
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(DrawImageCommand::tr("Not a valid expression"));
            }
            // 与原 Action 一致：数值不接受
            GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
            setStatus(m_lastStatus);
            break;
        }
        default:
            break;
        }
    }

private:
    void enterInput(Status s)
    {
        m_command.preview().clear();
        m_lastStatus = static_cast<Status>(status());
        setStatus(s);
    }

    QStringList availableCommands() const
    {
        QStringList cmd;
        if (status() == SetTargetPoint)
        {
            cmd += Commands::command("angle");
            cmd += Commands::command("factor");
            cmd += Commands::command("dpi");
        }
        return cmd;
    }

    DrawImageCommand& m_command;
    Status m_lastStatus = ShowDialog; ///< 进入输入前的状态
};
}  // namespace

/// @brief 要插入的图片
struct DrawImageCommand::Image
{
    ImageData data; ///< 图片实体的数据（插入点、方向、缩放、路径）
    QImage img;     ///< 像素，预览外框用它的宽高
};

DrawImageCommand::DrawImageCommand()
    : m_image(std::make_unique<Image>())
{
}

DrawImageCommand::~DrawImageCommand() = default;

std::unique_ptr<BasePlaceTool> DrawImageCommand::createTool()
{
    m_image->data = {IMAGE_DEFAULT_DPI,
                     {IMAGE_ZERO_COORD, IMAGE_ZERO_COORD},
                     {IMAGE_UNIT_SCALE, IMAGE_ZERO_COORD},
                     {IMAGE_ZERO_COORD, IMAGE_UNIT_SCALE},
                     {IMAGE_UNIT_SCALE, IMAGE_UNIT_SCALE},
                     "",
                     IMAGE_DEFAULT_WIDTH,
                     IMAGE_DEFAULT_HEIGHT,
                     IMAGE_DEFAULT_DPI};
    m_image->data.setPath(chooseImageFile().toStdString());
    if (QString::fromStdString(m_image->data.getPath()).isEmpty())
    {
        return nullptr;
    }
    m_image->img = QImage(QString::fromStdString(m_image->data.getPath()));
    return std::make_unique<DrawImageTool>(*this, document(), view());
}

double DrawImageCommand::getAngle() const
{
    return m_image->data.getUVector().angle();
}

void DrawImageCommand::setAngle(double a)
{
    double l = m_image->data.getUVector().magnitude();
    m_image->data.getUVector().setPolar(l, a);
    m_image->data.getVVector().setPolar(l, a + M_PI_2);
}

double DrawImageCommand::getFactor() const
{
    return m_image->data.getScale().x;
}

void DrawImageCommand::setFactor(double f)
{
    m_image->data.setScale(DmVector(f, f));
}

double DrawImageCommand::dpiToScale(double dpi) const
{
    return DmUnits::dpiToScale(dpi, document()->getUnit());
}

double DrawImageCommand::scaleToDpi(double scale) const
{
    return DmUnits::scaleToDpi(scale, document()->getUnit());
}

void DrawImageCommand::previewFrame(const DmVector& insertionPoint)
{
    ImageData& data = m_image->data;
    data.setInsertionPoint(insertionPoint);
    preview().clear();
    Preview& entities = preview().entities();
    const double w = m_image->img.width();
    const double h = m_image->img.height();
    entities.addEntity(new DmLine(nullptr, {IMAGE_ZERO_COORD, IMAGE_ZERO_COORD}, {w, IMAGE_ZERO_COORD}));
    entities.addEntity(new DmLine(nullptr, {w, IMAGE_ZERO_COORD}, {w, h}));
    entities.addEntity(new DmLine(nullptr, {w, h}, {IMAGE_ZERO_COORD, h}));
    entities.addEntity(new DmLine(nullptr, {IMAGE_ZERO_COORD, h}, {IMAGE_ZERO_COORD, IMAGE_ZERO_COORD}));
    entities.getEntityContainer()->scale({IMAGE_ZERO_COORD, IMAGE_ZERO_COORD}, {data.getScale().x, data.getScale().y});
    entities.getEntityContainer()->rotate({IMAGE_ZERO_COORD, IMAGE_ZERO_COORD}, data.getUVector());
    entities.move(data.getInsertionPoint());
    preview().draw();
}

void DrawImageCommand::commitImage(const DmVector& insertionPoint)
{
    m_image->data.setInsertionPoint(insertionPoint);
    preview().clear();
    if (!QString::fromStdString(m_image->data.getPath()).isEmpty())
    {
        Transaction t(tr("Draw Image").toStdString(), document());
        t.start();
        DmImage* img = new DmImage(nullptr, m_image->data);
        img->setDocument(document());
        img->update();
        document()->getEntityTable()->add(img);
        t.commit();
    }
    view()->redraw();
    finish();
}

void DrawImageCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void DrawImageCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

ExclusiveCommandFactory DrawCommands::image()
{
    return exclusiveCommandFactory<DrawImageCommand>();
}
