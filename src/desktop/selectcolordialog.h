// Copyright (C) 2004-2023 Robert Griebl
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QDialog>
#include "bricklink/global.h"
#include "ui_selectcolordialog.h"


class SelectColorDialog : public QDialog, private Ui::SelectColorDialog
{
    Q_OBJECT
public:
    SelectColorDialog(bool popupMode, QWidget *parent = nullptr);
    ~SelectColorDialog() override;

    void setColor(const BrickLink::Color *color);
    void setColorAndItem(const BrickLink::Color *color, const BrickLink::Item *item);
    const BrickLink::Color *color() const;

    void setPopupPosition(const QRect &pos = QRect());

protected:
    void moveEvent(QMoveEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *e) override;
    QSize sizeHint() const override;

private slots:
    void checkColor(const BrickLink::Color *, bool);

private:
    void setPopupGeometryChanged(bool b);
    bool isPopupGeometryChanged() const;

    bool m_popupMode = false;
    // only relevant when in popupMode and execAtPosition was called:
    QRect m_popupPos;
    QString m_geometryConfigKey;
    QAction *m_resetGeometryAction = nullptr;
};
