/* Copyright (C) 2004-2022 Robert Griebl. All rights reserved.
**
** This file is part of BrickStore.
**
** This file may be distributed and/or modified under the terms of the GNU
** General Public License version 2 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://fsf.org/licensing/licenses/gpl.html for GPL licensing information.
*/
#include <QtCore/QBuffer>
#include <QtCore/QStringBuilder>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QHelpEvent>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QToolTip>
#include <QtWidgets/QLabel>

#include "bricklink/core.h"
#include "bricklink/item.h"
#include "bricklink/picture.h"
#include "bricklink/item.h"
#include "bricklink/delegate.h"
#include "utility/utility.h"


namespace BrickLink {

/////////////////////////////////////////////////////////////
// ITEMDELEGATE
/////////////////////////////////////////////////////////////


ItemDelegate::ItemDelegate(Options options, QObject *parent)
    : BetterItemDelegate(options, parent)
{ }

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    BetterItemDelegate::extendedPaint(painter, option, index, [this, painter, option, index]() {
        bool firstColumnImageOnly = (m_options & FirstColumnImageOnly) && (index.column() == 0);

        if (firstColumnImageOnly) {
            if (auto *item = index.data(ItemPointerRole).value<const Item *>()) {
                QImage image;

                Picture *pic = core()->picture(item, item->defaultColor());
                if (pic && pic->isValid())
                    image = pic->image();
                else
                    image = core()->noImage(option.rect.size());

                painter->fillRect(option.rect, Qt::white);
                if (!image.isNull()) {
                    QSizeF s = image.size().scaled(option.rect.size(), Qt::KeepAspectRatio);
                    QPointF p = option.rect.center() - QRectF({ }, s).center() + QPointF(.5, .5);
                    // scale while drawing, so we don't have to deal with hi-dpi calculations
                    painter->drawImage({ p, s }, image, image.rect());
                }
            }
        }
    });
}

bool ItemDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() == QEvent::ToolTip && index.isValid()) {
        if (auto item = index.data(BrickLink::ItemPointerRole).value<const BrickLink::Item *>()) {
            ToolTip::inst()->show(item, nullptr, event->globalPos(), view);
            return true;
        }
    }
    return QStyledItemDelegate::helpEvent(event, view, option, index);
}



ToolTip *ToolTip::s_inst = nullptr;

ToolTip *ToolTip::inst()
{
    if (!s_inst) {
        s_inst = new ToolTip();
        connect(core(), &Core::pictureUpdated, s_inst, &ToolTip::pictureUpdated);

        // animated tooltips do not work for us, because the tooltip widget is not "visible"
        // until the animation is finished, effectively breaking pictureUpdated() below
        QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);

#if defined(Q_OS_WINDOWS)
        // on Windows we can at least use the fade animation, as this is done via the window
        // opacity (see qeffects.cpp, QAlphaWidget::run).
        QApplication::setEffectEnabled(Qt::UI_FadeTooltip, true);
#endif
    }
    return s_inst;
}

bool ToolTip::show(const Item *item, const Color *color, const QPoint &globalPos, QWidget *parent)
{
    QString tt;

    if (item) {
        if (Picture *pic = core()->picture(item, nullptr, true)) {
            m_tooltip_pic = ((pic->updateStatus() == UpdateStatus::Updating)
                             || (pic->updateStatus() == UpdateStatus::Loading)) ? pic : nullptr;

            // need to 'clear' to reset the image cache of the QTextDocument
            const auto tlwidgets = QApplication::topLevelWidgets();
            for (QWidget *w : tlwidgets) {
                if (w->inherits("QTipLabel")) {
                    qobject_cast<QLabel *>(w)->clear();
                    break;
                }
            }
            tt = createItemToolTip(pic->item(), pic);
        }
    } else if (color) {
        tt = createColorToolTip(color);
    }

    if (!tt.isEmpty()) {
        QToolTip::showText(globalPos, tt, parent);
        return true;
    }
    return false;
}

QString ToolTip::createItemToolTip(const Item *item, Picture *pic) const
{
    static const QString str = QLatin1String(R"(<table class="tooltip_picture" style="float: right;"><tr><td><i>%4</i></td></tr></table><div>%2<br><b>%3</b><br>%1</div>)");
    static const QString img_left = QLatin1String(R"(<center><img src="data:image/png;base64,%1" width="%2" height="%3"/></center>)");
    QString note_left = u"<i>" % ItemDelegate::tr("[Image is loading]") % u"</i>";
    QString yearStr;
    QString id = QString::fromLatin1(item->id());

    if (item->yearReleased())
        yearStr = QString::number(item->yearReleased());
    if (item->yearLastProduced() && (item->yearLastProduced() != item->yearReleased()))
        yearStr = yearStr % u'-' % QString::number(item->yearLastProduced());

    QColor color = qApp->palette().color(QPalette::Highlight);
    id = id % uR"(&nbsp;&nbsp;<i><font color=")" % Utility::textColor(color).name() %
            uR"(" style="background-color: )" % color.name() % uR"(;">&nbsp;)" %
            item->itemType()->name() % uR"(&nbsp;</font></i>)";

    if (pic && ((pic->updateStatus() == UpdateStatus::Updating)
                || (pic->updateStatus() == UpdateStatus::Loading))) {
        return str.arg(note_left, id, item->name(), yearStr);
    } else {
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        const QImage img = pic->image();
        img.save(&buffer, "PNG");

        return str.arg(img_left.arg(QString::fromLatin1(ba.toBase64())).arg(img.width()).arg(img.height()),
                       id, item->name(), yearStr);
    }
}

void BrickLink::ToolTip::pictureUpdated(BrickLink::Picture *pic)
{
    if (!pic || pic != m_tooltip_pic)
        return;

    if ((pic->updateStatus() != UpdateStatus::Updating)
            && (pic->updateStatus() != UpdateStatus::Loading)) {
        m_tooltip_pic = nullptr;
    }

    if (QToolTip::isVisible() && QToolTip::text().startsWith(uR"(<table class="tooltip_picture")")) {
        const auto tlwidgets = QApplication::topLevelWidgets();
        for (QWidget *w : tlwidgets) {
            if (w->inherits("QTipLabel")) {
                qobject_cast<QLabel *>(w)->clear();
                qobject_cast<QLabel *>(w)->setText(createToolTip(pic->item(), pic));

                QRect r(w->pos(), w->sizeHint());
                QRect desktop = w->window()->windowHandle()->screen()->availableGeometry();

                r.translate(r.right() > desktop.right() ? desktop.right() - r.right() : 0,
                            r.bottom() > desktop.bottom() ? desktop.bottom() - r.bottom() : 0);
                w->setGeometry(r);
                break;
            }
        }
    }
}

} // namespace BrickLink

#include "moc_delegate.cpp"
