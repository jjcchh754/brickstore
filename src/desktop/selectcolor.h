// Copyright (C) 2004-2023 Robert Griebl
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QDialog>

#include "bricklink/global.h"

QT_FORWARD_DECLARE_CLASS(QTreeView)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QToolButton)


class SelectColor : public QWidget
{
    Q_OBJECT
public:
    SelectColor(QWidget *parent = nullptr);

    void setWidthToContents(bool b);

    void setCurrentColor(const BrickLink::Color *color);
    void setCurrentColorAndItem(const BrickLink::Color *color, const BrickLink::Item *item);
    const BrickLink::Color *currentColor() const;

    bool colorLock() const;
    void setColorLock(bool locked);

    QByteArray saveState() const;
    bool restoreState(const QByteArray &ba);
    static QByteArray defaultState();

signals:
    void colorSelected(const BrickLink::Color *color, bool confirmed);
    void colorLockChanged(const BrickLink::Color *color);

protected slots:
    void colorConfirmed();
    void updateColorFilter(int filter);
    void languageChange();

protected:
    void changeEvent(QEvent *) override;
    void showEvent(QShowEvent *) override;

    void populateFilter(const BrickLink::Color *color);

protected:
    QComboBox *w_filter;
    QTreeView *w_colors;
    QToolButton *w_lock;
    BrickLink::ColorModel *m_colorModel;
    const BrickLink::Item *m_item = nullptr;
    bool m_locked = false;
};
