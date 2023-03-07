// Copyright (C) 2004-2023 Robert Griebl
// SPDX-License-Identifier: GPL-3.0-only

#include <QPainter>

#include "betteritemdelegate.h"


BetterItemDelegate::BetterItemDelegate(Options options, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_options(options)
{ }

void BetterItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    extendedPaint(painter, option, index);
}

QSize BetterItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize ext(0, 2); // height + 2 looks better
    if (m_options & MoreSpacing)
        ext.rwidth() += option.fontMetrics.height();

    return QStyledItemDelegate::sizeHint(option, index) + ext;
}

void BetterItemDelegate::extendedPaint(QPainter *painter, const QStyleOptionViewItem &option,
                                       const QModelIndex &index,
                                       const std::function<void()> &paintCallback) const
{
    QStyleOptionViewItem myoption(option);

    bool firstColumnImageOnly = (m_options & FirstColumnImageOnly) && (index.column() == 0);

    bool useFrameHover = false;
    bool useFrameSelection = false;

    if (firstColumnImageOnly) {
        useFrameSelection = (option.state & QStyle::State_Selected);
        useFrameHover = (option.state & QStyle::State_MouseOver);
        if (useFrameSelection)
            myoption.state &= ~QStyle::State_Selected;
    }

    if ((m_options & AlwaysShowSelection) && (option.state & QStyle::State_Enabled))
        myoption.state |= QStyle::State_Active;

    QStyledItemDelegate::paint(painter, myoption, index);

    if (paintCallback)
        paintCallback();

    if (firstColumnImageOnly && (useFrameSelection || useFrameHover)) {
        painter->save();
        QColor c = option.palette.color(QPalette::Highlight);
        for (int i = 0; i < 6; ++i) {
            c.setAlphaF((useFrameHover && !useFrameSelection ? 0.6f : 1.f) - float(i) * .1f);
            painter->setPen(c);
            painter->drawRect(option.rect.adjusted(i, i, -i - 1, -i - 1));
        }
        painter->restore();
    }
}

#include "moc_betteritemdelegate.cpp"
