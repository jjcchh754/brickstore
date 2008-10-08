/* Copyright (C) 2004-2008 Robert Griebl. All rights reserved.
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
#include <cfloat>
#include <cmath>

#include <QToolButton>
#include <QMenu>
#include <QPainter>
#include <QLayout>
#include <QLabel>
#include <QApplication>
#include <QFileDialog>
#include <QLineEdit>
#include <QTextLayout>
#include <QValidator>
#include <QClipboard>
#include <QCursor>
#include <QToolTip>
#include <QIcon>
#include <QTableView>
#include <QHeaderView>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QStyledItemDelegate>
#include <QStyleOptionFrameV2>
#include <QStyle>

#include "messagebox.h"
//#include "filteredit.h"
#include "config.h"
#include "framework.h"
#include "utility.h"
#include "money.h"
#include "document.h"
#include "disableupdates.h"
#include "window.h"

#include "selectitemdialog.h"
#include "selectcolordialog.h"

namespace {

// should be a function, but MSVC.Net doesn't accept default
// template arguments for template functions...

template <typename TG, typename TS = TG> class setOrToggle {
public:
    static void toggle(Window *w, const QString &actiontext, TG(Document::Item::* getter)() const, void (Document::Item::* setter)(TS t), TS val1, TS val2)
    { set_or_toggle(w, true, actiontext, getter, setter, val1, val2); }
    static void set(Window *w, const QString &actiontext, TG(Document::Item::* getter)() const, void (Document::Item::* setter)(TS t), TS val)
    { set_or_toggle(w, false, actiontext, getter, setter, val, TG()); }

private:
    static void set_or_toggle(Window *w, bool toggle, const QString &actiontext, TG(Document::Item::* getter)() const, void (Document::Item::* setter)(TS t), TS val1, TS val2 = TG())
    {
        Document *doc = w->document();

        if (w->document()->selection().isEmpty())
            return;

        //DisableUpdates disupd ( w_list );
        doc->beginMacro();
        uint count = 0;

        // Apples gcc4 has problems compiling this line (internal error)
        //foreach ( Document::Item *pos, doc->selection ( )) {
        for (Document::ItemList::const_iterator it = doc->selection().begin() ; it != doc->selection().end(); ++it) {
            Document::Item *pos = *it;
            if (toggle) {
                TG val = (pos->* getter)();

                if (val == val1)
                    val = val2;
                else if (val == val2)
                    val = val1;

                if (val != (pos->* getter)()) {
                    Document::Item item = *pos;

                    (item.* setter)(val);
                    doc->changeItem(pos, item);

                    count++;
                }
            }
            else {
                if ((pos->* getter)() != val1) {
                    Document::Item item = *pos;

                    (item.* setter)(val1);
                    doc->changeItem(pos, item);

                    count++;
                }
            }
        }
        doc->endMacro(actiontext.arg(count));
    }
};

} // namespace

class DocumentDelegate : public QStyledItemDelegate {
public:
    DocumentDelegate(Document *doc, QTableView *view);

    int defaultItemHeight(const QWidget *w = 0) const;

    virtual QSize sizeHint(const QStyleOptionViewItem &option1, const QModelIndex &idx) const;
    virtual void paint(QPainter *p, const QStyleOptionViewItem &option1, const QModelIndex &idx) const;
    virtual bool editorEvent(QEvent *e, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &idx);
    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

protected:
    bool nonInlineEdit(QEvent *e, Document::Item *it, const QStyleOptionViewItem &option, const QModelIndex &idx);

    QIcon::Mode iconMode(QStyle::State state) const;
    QIcon::State iconState(QStyle::State state) const;

    static QColor shadeColor(int idx, qreal alpha = 0);

protected:
    Document *   m_doc;
    QTableView *  m_view;
    SelectItemDialog * m_select_item;
    SelectColorDialog *m_select_color;
    mutable QPointer<QLineEdit> m_lineedit;

    static QVector<QColor> s_shades;
    static QHash<BrickLink::Status, QIcon> s_status_icons;
};

DocumentDelegate::DocumentDelegate(Document *doc, QTableView *view)
    : QStyledItemDelegate(view), m_doc(doc), m_view(view),
      m_select_item(0), m_select_color(0)
{
}

QColor DocumentDelegate::shadeColor(int idx, qreal alpha)
{
    if (s_shades.isEmpty()) {
        s_shades.resize(13);
        for (int i = 0; i < 13; i++)
            s_shades[i] = QColor::fromHsv(i == 0 ? -1 : (i - 1) * 30, 255, 255);
    }
    QColor c = s_shades[idx % s_shades.size()];
    if (alpha)
        c.setAlphaF(alpha);
    return c;
}

QIcon::Mode DocumentDelegate::iconMode(QStyle::State state) const
{
    if (!(state & QStyle::State_Enabled)) return QIcon::Disabled;
    if (state & QStyle::State_Selected) return QIcon::Selected;
    return QIcon::Normal;
}

QIcon::State DocumentDelegate::iconState(QStyle::State state) const
{
    return state & QStyle::State_Open ? QIcon::On : QIcon::Off;
}

int DocumentDelegate::defaultItemHeight(const QWidget *w) const
{
    static QSize picsize = BrickLink::core()->itemType('P')->pictureSize();
    QFontMetrics fm(w ? w->font() : QApplication::font("QTableView"));

    return 4 + qMax(fm.height() * 2, picsize.height() / 2);
}

QSize DocumentDelegate::sizeHint(const QStyleOptionViewItem &option1, const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QSize();

    static QSize picsize = BrickLink::core()->itemType('P')->pictureSize();
    int w = -1;

    if (idx.column() == Document::Picture)
        w = picsize.width() / 2 + 4;
    else
        w = QStyledItemDelegate::sizeHint(option1, idx).width();

    QStyleOptionViewItemV4 option(option1);
    return QSize(w, defaultItemHeight(option.widget));
}

void DocumentDelegate::paint(QPainter *p, const QStyleOptionViewItem &option1, const QModelIndex &idx) const
{
    if (!idx.isValid())
        return;

    Document::Item *it = m_doc->item(idx);
    if (!it)
        return;

    QStyleOptionViewItemV4 option(option1);

    QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
//    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
//        cg = QPalette::Inactive;

    int x = option.rect.x(), y = option.rect.y();
    int w = option.rect.width();
    int h = option.rect.height();
    int margin = 2;
    int align = (m_doc->data(idx, Qt::TextAlignmentRole).toInt() & ~Qt::AlignVertical_Mask) | Qt::AlignVCenter;
    quint64 colmask = 1ULL << idx.column();
    QString has_inv_tag;


    QPixmap pix;
    QIcon ico;
    QString str = idx.model()->data(idx, Qt::DisplayRole).toString();

    QColor bg;
    QColor fg;
    int checkmark = 0;

    bg = option.palette.color(cg, option.features & QStyleOptionViewItemV2::Alternate ? QPalette::AlternateBase : QPalette::Base);
    fg = option.palette.color(cg, QPalette::Text);

    switch (idx.column()) {
    case Document::Status:
        ico = s_status_icons[it->status()];
        if (ico.isNull()) {
            switch (it->status()) {
            case BrickLink::Exclude: ico = QIcon(":/images/status_exclude"); break;
            case BrickLink::Extra  : ico = QIcon(":/images/status_extra"); break;
            default                :
            case BrickLink::Include: ico = QIcon(":/images/status_include"); break;
            }
            s_status_icons.insert(it->status(), ico);
        }
        break;

    case Document::Description:
        if (it->item()->hasInventory())
            has_inv_tag = tr("Inv");
        break;

    case Document::Picture: {
        QImage img = it->image();
        pix = QPixmap::fromImage(img.scaled(img.size() / 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        break;
    }
    case Document::Color:
        if (const QPixmap *pixptr = BrickLink::core()->colorImage(it->color(), option.decorationSize.width(), option.rect.height()))
            pix = *pixptr;
        break;

    case Document::ItemType:
        bg = shadeColor(it->itemType()->id(), 0.1f);
        break;

    case Document::Category:
        bg = shadeColor(it->category()->id(), 0.2f);
        break;

    case Document::Quantity:
        if (it->quantity() <= 0)
            bg = (it->quantity() == 0) ? QColor::fromRgbF(1, 1, 0, 0.4f)
                 : QColor::fromRgbF(1, 0, 0, 0.4f);
        break;

    case Document::QuantityDiff:
        if (it->origQuantity() < it->quantity())
            bg = QColor::fromRgbF(0, 1, 0, 0.3f);
        else if (it->origQuantity() > it->quantity())
            bg = QColor::fromRgbF(1, 0, 0, 0.3f);
        break;

    case Document::PriceOrig:
    case Document::QuantityOrig:
        fg.setAlphaF(0.5f);
        break;

    case Document::PriceDiff:
        if (it->origPrice() < it->price())
            bg = QColor::fromRgbF(0, 1, 0, 0.3f);
        else if (it->origPrice() > it->price())
            bg = QColor::fromRgbF(1, 0, 0, 0.3f);
        break;

    case Document::Total:
        bg = QColor::fromRgbF(1, 1, 0, 0.1f);
        break;

    case Document::Condition:
        if (it->condition() != BrickLink::New) {
            bg = fg;
            bg.setAlphaF(0.3f);
        }
        break;

    case Document::TierP1:
    case Document::TierQ1:
        bg = fg;
        bg.setAlphaF(0.06f);
        break;

    case Document::TierP2:
    case Document::TierQ2:
        bg = fg;
        bg.setAlphaF(0.12f);
        break;

    case Document::TierP3:
    case Document::TierQ3:
        bg = fg;
        bg.setAlphaF(0.18f);
        break;

    case Document::Retain:
        checkmark = it->retain() ? 1 : -1;
        break;

    case Document::Stockroom:
        checkmark = it->stockroom() ? 1 : -1;
        break;
    }

    if (option.state & QStyle::State_Selected) {
        bg = option.palette.color(cg, QPalette::Highlight);
        if (!(option.state & QStyle::State_HasFocus))
            bg.setAlphaF(0.7f);
        fg = option.palette.color(cg, QPalette::HighlightedText);
    }

    if (!has_inv_tag.isEmpty()) {
        int itw = option.fontMetrics.width(has_inv_tag) + 2;
        int ith = option.fontMetrics.height() + 2;

        QRadialGradient grad(option.rect.bottomRight(), itw + ith);
        QColor col = fg;
        col.setAlphaF(0.2f);
        grad.setColorAt(0, col);
        grad.setColorAt(0.5, col);
        grad.setColorAt(1, bg);

        p->fillRect(option.rect, grad);

        p->setPen(bg);
        p->drawText(option.rect, Qt::AlignRight | Qt::AlignBottom, has_inv_tag);
    }
    else
        p->fillRect(option.rect, bg);


    if ((it->errors() & m_doc->errorMask() & (1ULL << idx.column()))) {
        p->setPen(QColor::fromRgbF(1, 0, 0, 0.75f));
        p->drawRect(x+.5, y+.5, w-1, h-1);
        p->setPen(QColor::fromRgbF(1, 0, 0, 0.50f));
        p->drawRect(x+1.5, y+1.5, w-3, h-3);
    }

    p->setPen(fg);

    x++; // extra spacing
    w -=2;

    if (checkmark != 0) {
        QStyleOptionViewItem opt(option);
        opt.state &= ~QStyle::State_HasFocus;
        opt.state |= ((checkmark > 0) ? QStyle::State_On : QStyle::State_Off);
        QStyle *style = option.widget ? option.widget->style() : QApplication::style();
        style->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &opt, p, option.widget);
    }
    else if (!pix.isNull()) {
        // clip the pixmap here ..this is cheaper than a cliprect

        int rw = w - 2 * margin;
        int rh = h; // - 2 * margin;

        int sw, sh;

        if (pix.height() <= rh) {
            sw = qMin(rw, pix.width());
            sh = qMin(rh, pix.height());
        }
        else {
            sw = pix.width() * rh / pix.height();
            sh = rh;
        }

        int px = x + margin;
        int py = y + /*margin +*/ (rh - sh) / 2;

        if (align == Qt::AlignCenter)
            px += (rw - sw) / 2;   // center if there is enough room

        if (pix.height() <= rh)
            p->drawPixmap(px, py, pix, 0, 0, sw, sh);
        else
            p->drawPixmap(QRect(px, py, sw, sh), pix);

        w -= (margin + sw);
        x += (margin + sw);
    }
    else if (!ico.isNull()) {
        ico.paint(p, x, y, w, h, Qt::AlignCenter, iconMode(option.state), iconState(option.state));
    }

    if (!str.isEmpty()) {
        int rw = w - 2 * margin;

        if (!(align & Qt::AlignVertical_Mask))
            align |= Qt::AlignVCenter;

        const QFontMetrics &fm = p->fontMetrics();


        bool do_elide = false;
        int lcount = (h + fm.leading()) / fm.lineSpacing();
        int height = 0;
        qreal widthUsed = 0;

        QTextLayout tl(str, option.font, const_cast<QWidget *>(option.widget));
        QTextOption to = tl.textOption();
        to.setAlignment(Qt::Alignment(align));
        tl.setTextOption(to);
        tl.beginLayout();

        for (int i = 0; i < lcount; i++) {
            QTextLine line = tl.createLine();
            if (!line.isValid())
                break;

            line.setLineWidth(rw);
            height += fm.leading();
            line.setPosition(QPoint(0, height));
            height += line.height();
            widthUsed = line.naturalTextWidth();

            if ((i == (lcount - 1)) && ((line.textStart() + line.textLength()) < str.length())) {
                do_elide = true;
                QString elide = QLatin1String("...");
                int elide_width = fm.width(elide) + 2;

                line.setLineWidth(rw - elide_width);
                widthUsed = line.naturalTextWidth();
            }
        }
        tl.endLayout();

        tl.draw(p, QPoint(x + margin, y + (h - height)/2));
        if (do_elide)
            p->drawText(QPoint(x + margin + widthUsed, y + (h - height)/2 + (lcount - 1) * fm.lineSpacing() + fm.ascent()), QLatin1String("..."));
    }
}

bool DocumentDelegate::editorEvent(QEvent *e, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &idx)
{
    if (!e || !model || !idx.isValid())
        return false;

    Document::Item *it = m_doc->item(idx);
    if (!it)
        return false;

    switch (e->type()) {
    case QEvent::KeyPress: {
        //no break
    }
    case QEvent::MouseButtonDblClick: {
        if (nonInlineEdit(e, it, option, idx))
            return true;
        break;
    }
    default: break;
    }

    return QStyledItemDelegate::editorEvent(e, model, option, idx);
}

bool DocumentDelegate::nonInlineEdit(QEvent *e, Document::Item *it, const QStyleOptionViewItem &option, const QModelIndex &idx)
{
    bool accept = true;

    bool dblclick = (e->type() == QEvent::MouseButtonDblClick);
    bool keypress = (e->type() == QEvent::KeyPress);
    bool editkey = false;
    int key = -1;

    if (keypress) {
        key = static_cast<QKeyEvent *>(e)->key();

        if (key == Qt::Key_Space ||
            key == Qt::Key_Return ||
#if defined( Q_WS_MAC )
            (key == Qt::Key_O && static_cast<QKeyEvent *>(e)->modifiers() & Qt::ControlModifier)
#else
            key == Qt::Key_F2
#endif
           ) {
            editkey = true;
        }
    }


    switch (idx.column()) {
    case Document::Retain:
        if (dblclick || (keypress && editkey)) {
            Document::Item item = *it;
            item.setRetain(!it->retain());
            m_doc->changeItem(it, item);
        }
        break;

    case Document::Stockroom:
        if (dblclick || (keypress && editkey)) {
            Document::Item item = *it;
            item.setStockroom(!it->stockroom());
            m_doc->changeItem(it, item);
        }
        break;

    case Document::Condition:
        if (dblclick || (keypress && (editkey || key == Qt::Key_N || key == Qt::Key_U))) {
            BrickLink::Condition cond;
            if (key == Qt::Key_N)
                cond = BrickLink::New;
            else if (key == Qt::Key_U)
                cond = BrickLink::Used;
            else
                cond = (it->condition() == BrickLink::New) ? BrickLink::Used : BrickLink::New;

            Document::Item item = *it;
            item.setCondition(cond);
            m_doc->changeItem(it, item);
        }
        break;

    case Document::Status:
        if (dblclick || (keypress && (editkey || key == Qt::Key_I || key == Qt::Key_E || key == Qt::Key_X))) {
            BrickLink::Status st = it->status();
            if (key == Qt::Key_I)
                st = BrickLink::Include;
            else if (key == Qt::Key_E)
                st = BrickLink::Exclude;
            else if (key == Qt::Key_X)
                st = BrickLink::Extra;
            else
                switch (st) {
                        case BrickLink::Include: st = BrickLink::Exclude; break;
                        case BrickLink::Exclude: st = BrickLink::Extra; break;
                        case BrickLink::Extra  :
                        default                : st = BrickLink::Include; break;
                }

            Document::Item item = *it;
            item.setStatus(st);
            m_doc->changeItem(it, item);
        }
        break;

    case Document::Picture:
    case Document::Description:
        if (dblclick || (keypress && editkey)) {
            if (!m_select_item) {
                m_select_item = new SelectItemDialog(false, m_view, Qt::Tool);
                m_select_item->setWindowTitle(tr("Modify Item"));
            }
            m_select_item->setItem(it->item());

            if (m_select_item->exec(QRect(m_view->viewport()->mapToGlobal(option.rect.topLeft()), option.rect.size())) == QDialog::Accepted) {
                Document::Item item = *it;
                item.setItem(m_select_item->item());
                m_doc->changeItem(it, item);
            }
        }
        break;

    case Document::Color:
        if (dblclick || (keypress && editkey)) {
            if (!m_select_color) {
                m_select_color = new SelectColorDialog(m_view, Qt::Tool);
                m_select_color->setWindowTitle(tr("Modify Color"));
            }
            m_select_color->setColor(it->color());

            if (m_select_color->exec(QRect(m_view->viewport()->mapToGlobal(option.rect.topLeft()), option.rect.size())) == QDialog::Accepted) {
                Document::Item item = *it;
                item.setColor(m_select_color->color());
                m_doc->changeItem(it, item);
            }
        }
        break;

    default:
        accept = false;
        break;
    }
    return accept;
}

QWidget *DocumentDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &idx) const
{
    Document::Item *it = m_doc->item(idx);
    if (!it)
        return false;

    QValidator *valid = 0;
    switch (idx.column()) {
    case Document::Sale        : valid = new QIntValidator(-1000, 99, 0); break;
    case Document::Quantity    :
    case Document::QuantityDiff: valid = new QIntValidator(-99999, 99999, 0); break;
    case Document::Bulk        : valid = new QIntValidator(1, 99999, 0); break;
    case Document::TierQ1      :
    case Document::TierQ2      :
    case Document::TierQ3      : valid = new QIntValidator(0, 99999, 0); break;
    case Document::Price       :
    case Document::TierP1      :
    case Document::TierP2      :
    case Document::TierP3      : valid = new MoneyValidator(0, 10000, 3, 0); break;
    case Document::PriceDiff   : valid = new MoneyValidator(-10000, 10000, 3, 0); break;
    case Document::Weight      : valid = new QDoubleValidator(0., 100000., 4, 0); break;
    default                     : break;
    }

    if (!m_lineedit)
        m_lineedit = new QLineEdit(parent);

    m_lineedit->setAlignment(Qt::Alignment(idx.data(Qt::TextAlignmentRole).toInt()));
    if (valid)
        m_lineedit->setValidator(valid);

    return m_lineedit;
}

void DocumentDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    if (qobject_cast<QLineEdit *>(editor))
        editor->setGeometry(option.rect);
}


QVector<QColor> DocumentDelegate::s_shades;
QHash<BrickLink::Status, QIcon> DocumentDelegate::s_status_icons;

Window::Window(Document *doc, QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_doc = doc;
    m_docmodel = new DocumentProxyModel(doc);
// m_ignore_selection_update = false;

//    m_filter_field = All;

    m_settopg_failcnt = 0;
    m_settopg_list = 0;
    m_settopg_time = BrickLink::AllTime;
    m_settopg_price = BrickLink::Average;

    w_list = new QTableView(this);
    w_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    w_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    w_list->setAlternatingRowColors(true);
    w_list->setTabKeyNavigation(true);
    w_list->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    w_list->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    w_list->setContextMenuPolicy(Qt::CustomContextMenu);
    w_list->verticalHeader()->setResizeMode(QHeaderView::Fixed);
    w_list->verticalHeader()->hide();
    w_list->horizontalHeader()->setHighlightSections(false);
    w_list->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
    setFocusProxy(w_list);

    w_list->setModel(m_docmodel);
    w_list->setSelectionModel(doc->selectionModel());
    DocumentDelegate *dd = new DocumentDelegate(doc, w_list);
    w_list->setItemDelegate(dd);
    w_list->verticalHeader()->setDefaultSectionSize(dd->defaultItemHeight(w_list));

    int em = w_list->fontMetrics().width(QChar('m'));
    for (int i = 0; i < w_list->model()->columnCount(); i++) {
        int width = w_list->model()->headerData(i, Qt::Horizontal, Qt::UserRole).toInt();
        if (width)
            w_list->horizontalHeader()->resizeSection(i, (width < 0 ? -width : width * em) + 8);
    }
    /*
    w_list->setShowSortIndicator ( true );
    w_list->setColumnsHideable ( true );
    w_list->setDifferenceMode ( false );
    w_list->setSimpleMode ( Config::inst ( )->simpleMode ( ));

    if ( doc->doNotSortItems ( ))
     w_list->setSorting ( w_list->columns ( ) + 1 );
*/
//    connect(w_list->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(updateSelectionFromView()));


    QBoxLayout *toplay = new QVBoxLayout(this);
    toplay->setSpacing(0);
    toplay->setMargin(0);
    toplay->addWidget(w_list, 10);

    connect(BrickLink::core(), SIGNAL(priceGuideUpdated(BrickLink::PriceGuide *)), this, SLOT(priceGuideUpdated(BrickLink::PriceGuide *)));

    connect(w_list, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
    //connect(w_list, SIGNAL(activated(const QModelIndex &)), w_list, SLOT(edit(const QModelIndex &)));
//    connect(w_filter_clear, SIGNAL(clicked()), w_filter_expression, SLOT(clearEditText()));
//    connect(w_filter_expression, SIGNAL(editTextChanged(const QString &)), this, SLOT(applyFilter()));
//    connect(w_filter_field, SIGNAL(activated(int)), this, SLOT(applyFilter()));

//    connect(w_filter, SIGNAL(textChanged(const QString &)), this, SLOT(applyFilter()));
//    connect(w_filter->menu(), SIGNAL(triggered(QAction *)), this, SLOT(applyFilterField(QAction *)));

    connect(Config::inst(), SIGNAL(simpleModeChanged(bool)), w_list, SLOT(setSimpleMode(bool)));
    connect(Config::inst(), SIGNAL(simpleModeChanged(bool)), this, SLOT(updateErrorMask()));
    connect(Config::inst(), SIGNAL(showInputErrorsChanged(bool)), this, SLOT(updateErrorMask()));

    connect(Config::inst(), SIGNAL(weightSystemChanged(Config::WeightSystem)), w_list, SLOT(triggerUpdate()));
    connect(Money::inst(), SIGNAL(monetarySettingsChanged()), w_list, SLOT(triggerUpdate()));

////////////////////////////////////////////////////////////////////////////////

    connect(m_doc, SIGNAL(titleChanged(const QString &)), this, SLOT(updateCaption()));
    connect(m_doc, SIGNAL(modificationChanged(bool)), this, SLOT(updateCaption()));

    connect(m_doc, SIGNAL(itemsAdded(const Document::ItemList &)), this, SLOT(itemsAddedToDocument(const Document::ItemList &)));
    connect(m_doc, SIGNAL(itemsRemoved(const Document::ItemList &)), this, SLOT(itemsRemovedFromDocument(const Document::ItemList &)));
    connect(m_doc, SIGNAL(itemsChanged(const Document::ItemList &, bool)), this, SLOT(itemsChangedInDocument(const Document::ItemList &, bool)));

    itemsAddedToDocument(m_doc->items());

    updateErrorMask();
    updateCaption();

    setFocusProxy(w_list);
}

void Window::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        updateCaption();
    }
}

void Window::updateCaption()
{
    QString cap = m_doc->title();

    if (cap.isEmpty())
        cap = m_doc->fileName();
    if (cap.isEmpty())
        cap = tr("Untitled");

    setWindowTitle(cap);
}

Window::~Window()
{
    delete m_doc; //->deleteLater();
}

void Window::on_view_save_default_col_triggered()
{
    //TODO w_list->saveDefaultLayout ( );
}

void Window::itemsAddedToDocument(const Document::ItemList &items)
{
    /*
    QListViewItem *last_item = w_list->lastItem ( );

    foreach ( Document::Item *item, items ) {
     CItemViewItem *ivi = new CItemViewItem ( item, w_list, last_item );
     m_lvitems.insert ( item, ivi );
     last_item = ivi;
    }

    //TODO w_list->ensureItemVisible ( m_lvitems [items.front ( )] );
    */

}

void Window::itemsRemovedFromDocument(const Document::ItemList &items)
{
    /*
    foreach ( Document::Item *item, items )
     delete m_lvitems.take ( item );
     */
}

void Window::itemsChangedInDocument(const Document::ItemList &items, bool /*grave*/)
{
    /*
    foreach ( Document::Item *item, items ) {
     CItemViewItem *ivi = m_lvitems [item];

     if ( ivi ) {
      ivi->widthChanged ( );
      ivi->repaint ( );
     }
    }
    */
}


void Window::setFilter(const QString &str)
{
    m_docmodel->setFilterExpression(str);
}

QString Window::filter() const
{
    return m_docmodel->filterExpression();
}

uint Window::addItems(const BrickLink::InvItemList &items, int multiply, uint globalmergeflags, bool dont_change_sorting)
{
    bool waitcursor = (items.count() > 100);
    bool was_empty = (w_list->model()->rowCount() == 0);
    uint dropped = 0;
    int merge_action_yes_no_to_all = MergeAction_Ask;
    int mergecount = 0, addcount = 0;

    if (items.count() > 1)
        m_doc->beginMacro();

    // disable sorting: new items should appear at the end of the list
    /*
     if ( !dont_change_sorting )
    //TODO  w_list->setSorting ( w_list->columns ( ) + 1 ); // +1 because of Qt-bug !!
    */
    if (waitcursor)
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    DisableUpdates disupd(w_list);


    foreach(const BrickLink::InvItem *origitem, items) {
        uint mergeflags = globalmergeflags;

        Document::Item *newitem = new Document::Item(*origitem);

        if (newitem->isIncomplete()) {
//   DlgIncompleteItemImpl d ( newitem, this );

            if (waitcursor)
                QApplication::restoreOverrideCursor();

            bool drop_this = true; //TODO: ( d.exec ( ) != QDialog::Accepted );

            if (waitcursor)
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            if (drop_this) {
                dropped++;
                continue;
            }
        }

        Document::Item *olditem = 0;

        if (mergeflags != MergeAction_None) {
            foreach(Document::Item *item, m_doc->items()) {
                if ((newitem->item() == item->item()) &&
                    (newitem->color() == item->color()) &&
                    (newitem->status() == item->status()) &&
                    (newitem->condition() == item->condition())) {
                    olditem = item;
                    break;
                }
            }
            if (!olditem)
                mergeflags = MergeAction_None;
        }

        if ((mergeflags & MergeAction_Mask) == MergeAction_Ask) {
            if (merge_action_yes_no_to_all != MergeAction_Ask) {
                mergeflags = merge_action_yes_no_to_all;
            }
            else {
                //DlgMergeImpl d ( olditem, newitem, (( mergeflags & MergeKeep_Mask ) == MergeKeep_Old ), this );

                int res = MergeAction_None; //TODO: d.exec ( );

                mergeflags = (res ? MergeAction_Force : MergeAction_None);

                if (mergeflags != MergeAction_None)
                    mergeflags |= MergeKeep_Old; //TODO:( d.attributesFromExisting ( ) ? MergeKeep_Old : MergeKeep_New );

                if (true) //TODO: d.yesNoToAll ( ))
                    merge_action_yes_no_to_all = mergeflags;
            }
        }

        switch (mergeflags & MergeAction_Mask) {
        case MergeAction_Force: {
            newitem->mergeFrom(*olditem, ((mergeflags & MergeKeep_Mask) == MergeKeep_New));

            m_doc->changeItem(olditem, *newitem);
            delete newitem;
            mergecount++;
            break;
        }
        case MergeAction_None:
        default: {
            if (multiply > 1)
                newitem->setQuantity(newitem->quantity() * multiply);

            m_doc->insertItem(0, newitem);
            addcount++;
            break;
        }
        }
    }
    disupd.reenable();

    if (waitcursor)
        QApplication::restoreOverrideCursor();

    if (items.count() > 1)
        m_doc->endMacro(tr("Added %1, merged %2 items").arg(addcount).arg(mergecount));

    if (was_empty)
        w_list->selectRow(0);

    return items.count() - dropped;
}


void Window::addItem(BrickLink::InvItem *item, uint mergeflags)
{
    BrickLink::InvItemList tmplist;
    tmplist.append(item);

    addItems(tmplist, 1, mergeflags);

    delete item;
}


void Window::mergeItems(const Document::ItemList &items, int globalmergeflags)
{
    if ((items.count() < 2) || (globalmergeflags & MergeAction_Mask) == MergeAction_None)
        return;

    int merge_action_yes_no_to_all = MergeAction_Ask;
    uint mergecount = 0;

    m_doc->beginMacro();

    DisableUpdates disupd(w_list);

    foreach(Document::Item *from, items) {
        Document::Item *to = 0;

        foreach(Document::Item *find_to, items) {
            if ((from != find_to) &&
                (from->item() == find_to->item()) &&
                (from->color() == find_to->color())&&
                (from->condition() == find_to->condition()) &&
                ((from->status() == BrickLink::Exclude) == (find_to->status() == BrickLink::Exclude)) &&
                (m_doc->items().indexOf(find_to) != -1)) {
                to = find_to;
                break;
            }
        }
        if (!to)
            continue;

        uint mergeflags = globalmergeflags;

        if ((mergeflags & MergeAction_Mask) == MergeAction_Ask) {
            if (merge_action_yes_no_to_all != MergeAction_Ask) {
                mergeflags = merge_action_yes_no_to_all;
            }
            else {
                //TODO: DlgMergeImpl d ( to, from, (( mergeflags & MergeKeep_Mask ) == MergeKeep_Old ), this );

                int res = MergeAction_None; //d.exec ( );

                mergeflags = (res ? MergeAction_Force : MergeAction_None);

                if (mergeflags != MergeAction_None)
                    mergeflags |= MergeKeep_Old; //TODO:( d.attributesFromExisting ( ) ? MergeKeep_Old : MergeKeep_New );

                if (true) //TODO: d.yesNoToAll ( ))
                    merge_action_yes_no_to_all = mergeflags;
            }
        }

        if ((mergeflags & MergeAction_Mask) == MergeAction_Force) {
            bool prefer_from = ((mergeflags & MergeKeep_Mask) == MergeKeep_New);

            Document::Item newitem = *to;
            newitem.mergeFrom(*from, prefer_from);
            m_doc->changeItem(to, newitem);
            m_doc->removeItem(from);

            mergecount++;
        }
    }
    m_doc->endMacro(tr("Merged %1 items").arg(mergecount));
}

QDomElement Window::createGuiStateXML(QDomDocument doc)
{
    int version = 1;

    QDomElement root = doc.createElement("GuiState");
    root.setAttribute("Application", "BrickStore");
    root.setAttribute("Version", version);

    if (isDifferenceMode())
        root.appendChild(doc.createElement("DifferenceMode"));

    QMap <QString, QString> list_map; //TODO: = w_list->saveSettings ( );

    if (!list_map.isEmpty()) {
        QDomElement e = doc.createElement("ItemView");
        root.appendChild(e);

        for (QMap <QString, QString>::const_iterator it = list_map.begin(); it != list_map.end(); ++it)
            e.appendChild(doc.createElement(it.key()).appendChild(doc.createTextNode(it.value())).parentNode());
    }

    return root;
}

bool Window::parseGuiStateXML(QDomElement root)
{
    bool ok = true;

    if ((root.nodeName() == "GuiState") &&
        (root.attribute("Application") == "BrickStore") &&
        (root.attribute("Version").toInt() == 1)) {
        for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling()) {
            if (!n.isElement())
                continue;
            QString tag = n.toElement().tagName();

            if (tag == "DifferenceMode") {
                on_view_difference_mode_toggled(true);
            }
            else if (tag == "ItemView") {
                QMap <QString, QString> list_map;

                for (QDomNode niv = n.firstChild(); !niv.isNull(); niv = niv.nextSibling()) {
                    if (!niv.isElement())
                        continue;
                    list_map [niv.toElement().tagName()] = niv.toElement().text();
                }

                if (!list_map.isEmpty())
                    ; //TODO: w_list->loadSettings ( list_map );
            }
        }

    }

    return ok;
}


void Window::on_edit_cut_triggered()
{
    on_edit_copy_triggered();
    on_edit_delete_triggered();
}

void Window::on_edit_copy_triggered()
{
    if (!m_doc->selection().isEmpty()) {
        BrickLink::InvItemList bllist;

        foreach(Document::Item *item, m_doc->selection())
        bllist.append(item);

        QApplication::clipboard()->setMimeData(new BrickLink::InvItemMimeData(bllist));
    }
}

void Window::on_edit_paste_triggered()
{
    BrickLink::InvItemList bllist = BrickLink::InvItemMimeData::items(QApplication::clipboard()->mimeData());

    if (bllist.count()) {
        if (!m_doc->selection().isEmpty()) {
            if (MessageBox::question(this, tr("Overwrite the currently selected items?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
                m_doc->removeItems(m_doc->selection());
        }
        addItems(bllist, 1, MergeAction_Ask | MergeKeep_New);
        qDeleteAll(bllist);
    }
}

void Window::on_edit_delete_triggered()
{
    if (!m_doc->selection().isEmpty())
        m_doc->removeItems(m_doc->selection());
}

void Window::on_edit_select_all_triggered()
{
    w_list->selectAll();
}

void Window::on_edit_select_none_triggered()
{
    w_list->clearSelection();
}

void Window::on_edit_reset_diffs_triggered()
{
    DisableUpdates disupd(w_list);

    m_doc->resetDifferences(m_doc->selection());
}


void Window::on_edit_status_include_triggered()
{
    setOrToggle<BrickLink::Status>::set(this, tr("Set 'include' status on %1 items"), &Document::Item::status, &Document::Item::setStatus, BrickLink::Include);
}

void Window::on_edit_status_exclude_triggered()
{
    setOrToggle<BrickLink::Status>::set(this, tr("Set 'exclude' status on %1 items"), &Document::Item::status, &Document::Item::setStatus, BrickLink::Exclude);
}

void Window::on_edit_status_extra_triggered()
{
    setOrToggle<BrickLink::Status>::set(this, tr("Set 'extra' status on %1 items"), &Document::Item::status, &Document::Item::setStatus, BrickLink::Extra);
}

void Window::on_edit_status_toggle_triggered()
{
    setOrToggle<BrickLink::Status>::toggle(this, tr("Toggled status on %1 items"), &Document::Item::status, &Document::Item::setStatus, BrickLink::Include, BrickLink::Exclude);
}

void Window::on_edit_cond_new_triggered()
{
    setOrToggle<BrickLink::Condition>::set(this, tr("Set 'new' condition on %1 items"), &Document::Item::condition, &Document::Item::setCondition, BrickLink::New);
}

void Window::on_edit_cond_used_triggered()
{
    setOrToggle<BrickLink::Condition>::set(this, tr("Set 'used' condition on %1 items"), &Document::Item::condition, &Document::Item::setCondition, BrickLink::Used);
}

void Window::on_edit_cond_toggle_triggered()
{
    setOrToggle<BrickLink::Condition>::toggle(this, tr("Toggled condition on %1 items"), &Document::Item::condition, &Document::Item::setCondition, BrickLink::New, BrickLink::Used);
}

void Window::on_edit_retain_yes_triggered()
{
    setOrToggle<bool>::set(this, tr("Set 'retain' flag on %1 items"), &Document::Item::retain, &Document::Item::setRetain, true);
}

void Window::on_edit_retain_no_triggered()
{
    setOrToggle<bool>::set(this, tr("Cleared 'retain' flag on %1 items"), &Document::Item::retain, &Document::Item::setRetain, false);
}

void Window::on_edit_retain_toggle_triggered()
{
    setOrToggle<bool>::toggle(this, tr("Toggled 'retain' flag on %1 items"), &Document::Item::retain, &Document::Item::setRetain, true, false);
}

void Window::on_edit_stockroom_yes_triggered()
{
    setOrToggle<bool>::set(this, tr("Set 'stockroom' flag on %1 items"), &Document::Item::stockroom, &Document::Item::setStockroom, true);
}

void Window::on_edit_stockroom_no_triggered()
{
    setOrToggle<bool>::set(this, tr("Cleared 'stockroom' flag on %1 items"), &Document::Item::stockroom, &Document::Item::setStockroom, false);
}

void Window::on_edit_stockroom_toggle_triggered()
{
    setOrToggle<bool>::toggle(this, tr("Toggled 'stockroom' flag on %1 items"), &Document::Item::stockroom, &Document::Item::setStockroom, true, false);
}


void Window::on_edit_price_set_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    QString price = m_doc->selection().front()->price().toLocalizedString(false);

    if (MessageBox::getString(this, tr("Enter the new price for all selected items:"), Money::inst()->currencySymbol(), price, new MoneyValidator(0, 10000, 3, 0))) {
        setOrToggle<money_t>::set(this, tr("Set price on %1 items"), &Document::Item::price, &Document::Item::setPrice, money_t::fromLocalizedString(price));
    }
}

void Window::on_edit_price_round_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    uint roundcount = 0;
    m_doc->beginMacro();

    DisableUpdates disupd(w_list);

    foreach(Document::Item *pos, m_doc->selection()) {
        money_t p = ((pos->price() + money_t (0.005)) / 10) * 10;

        if (p != pos->price()) {
            Document::Item item = *pos;

            item.setPrice(p);
            m_doc->changeItem(pos, item);

            roundcount++;
        }
    }
    m_doc->endMacro(tr("Price change on %1 items").arg(roundcount));
}


void Window::on_edit_price_to_priceguide_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    if (m_settopg_list) {
        MessageBox::information(this, tr("Prices are currently updated to Price Guide values.<br /><br />Please wait until this operation has finished."));
        return;
    }

    //DlgSetToPGImpl dlg ( this );

    if (true) { //TODO: dlg.exec ( ) == QDialog::Accepted ) {
        DisableUpdates disupd(w_list);

        m_settopg_list    = new QMultiHash<BrickLink::PriceGuide *, Document::Item *>();
        m_settopg_failcnt = 0;
        m_settopg_time    = BrickLink::PastSix; //dlg.time ( );
        m_settopg_price   = BrickLink::Average; //dlg.price ( );
        bool force_update = false; //dlg.forceUpdate ( );

        foreach(Document::Item *item, m_doc->selection()) {
            BrickLink::PriceGuide *pg = BrickLink::core()->priceGuide(item->item(), item->color());

            if (force_update && pg && (pg->updateStatus() != BrickLink::Updating))
                pg->update();

            if (pg && (pg->updateStatus() == BrickLink::Updating)) {
                m_settopg_list->insert(pg, item);
                pg->addRef();
            }
            else if (pg && pg->valid()) {
                money_t p = pg->price(m_settopg_time, item->condition(), m_settopg_price);

                if (p != item->price()) {
                    Document::Item newitem = *item;
                    newitem.setPrice(p);
                    m_doc->changeItem(item, newitem);
                }
            }
            else {
                Document::Item newitem = *item;
                newitem.setPrice(0);
                m_doc->changeItem(item, newitem);

                m_settopg_failcnt++;
            }
        }

        if (m_settopg_list && m_settopg_list->isEmpty())
            priceGuideUpdated(0);
    }
}

void Window::priceGuideUpdated(BrickLink::PriceGuide *pg)
{
    if (m_settopg_list && pg) {
        foreach(Document::Item *item, m_settopg_list->values(pg)) {
            if (pg->valid()) {

                money_t p = pg->price(m_settopg_time, item->condition(), m_settopg_price);

                if (p != item->price()) {
                    Document::Item newitem = *item;
                    newitem.setPrice(p);
                    m_doc->changeItem(item, newitem);
                }
            }
            pg->release();
        }
        m_settopg_list->remove(pg);
    }

    if (m_settopg_list && m_settopg_list->isEmpty()) {
        QString s = tr("Prices of the selected items have been updated to Price Guide values.");

        if (m_settopg_failcnt)
            s += "<br /><br />" + tr("%1 have been skipped, because of missing Price Guide records and/or network errors.").arg(CMB_BOLD(QString::number(m_settopg_failcnt)));

        MessageBox::information(this, s);

        m_settopg_failcnt = 0;
        delete m_settopg_list;
        m_settopg_list = 0;
    }
}


void Window::on_edit_price_inc_dec_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    //DlgIncDecPriceImpl dlg ( this, "", true );

    if (true) { //TODO: dlg.exec ( ) == QDialog::Accepted ) {
        money_t fixed   = money_t(1); //dlg.fixed ( );
        double percent = 0; //dlg.percent ( );
        double factor  = (1.+ percent / 100.);
        bool tiers     = true; //dlg.applyToTiers ( );
        uint incdeccount = 0;

        m_doc->beginMacro();

        DisableUpdates disupd(w_list);

        foreach(Document::Item *pos, m_doc->selection()) {
            Document::Item item = *pos;

            money_t p = item.price();

            if (percent != 0)     p *= factor;
            else if (fixed != 0)  p += fixed;

            item.setPrice(p);

            if (tiers) {
                for (uint i = 0; i < 3; i++) {
                    p = item.tierPrice(i);

                    if (percent != 0)     p *= factor;
                    else if (fixed != 0)  p += fixed;

                    item.setTierPrice(i, p);
                }
            }
            m_doc->changeItem(pos, item);
            incdeccount++;
        }

        m_doc->endMacro(tr("Price change on %1 items").arg(incdeccount));
    }
}

void Window::on_edit_qty_divide_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    int divisor = 1;

    if (MessageBox::getInteger(this, tr("Divide the quantities of all selected items by this number.<br /><br />(A check is made if all quantites are exactly divisible without reminder, before this operation is performed.)"), QString::null, divisor, new QIntValidator(1, 1000, 0))) {
        if (divisor > 1) {
            int lots_with_errors = 0;

            foreach(Document::Item *item, m_doc->selection()) {
                if (qAbs(item->quantity()) % divisor)
                    lots_with_errors++;
            }

            if (lots_with_errors) {
                MessageBox::information(this, tr("The quantities of %1 lots are not divisible without remainder by %2.<br /><br />Nothing has been modified.").arg(lots_with_errors).arg(divisor));
            }
            else {
                uint divcount = 0;
                m_doc->beginMacro();

                DisableUpdates disupd(w_list);

                foreach(Document::Item *pos, m_doc->selection()) {
                    Document::Item item = *pos;

                    item.setQuantity(item.quantity() / divisor);
                    m_doc->changeItem(pos, item);

                    divcount++;
                }
                m_doc->endMacro(tr("Quantity divide by %1 on %2 Items").arg(divisor).arg(divcount));
            }
        }
    }
}

void Window::on_edit_qty_multiply_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    int factor = 1;

    if (MessageBox::getInteger(this, tr("Multiply the quantities of all selected items with this factor."), tr("x"), factor, new QIntValidator(-1000, 1000, 0))) {
        if ((factor <= 1) || (factor > 1)) {
            uint mulcount = 0;
            m_doc->beginMacro();

            DisableUpdates disupd(w_list);

            foreach(Document::Item *pos, m_doc->selection()) {
                Document::Item item = *pos;

                item.setQuantity(item.quantity() * factor);
                m_doc->changeItem(pos, item);

                mulcount++;
            }
            m_doc->endMacro(tr("Quantity multiply by %1 on %2 Items").arg(factor).arg(mulcount));
        }
    }
}

void Window::on_edit_sale_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    int sale = m_doc->selection().front()->sale();

    if (MessageBox::getInteger(this, tr("Set sale in percent for the selected items (this will <u>not</u> change any prices).<br />Negative values are also allowed."), tr("%"), sale, new QIntValidator(-1000, 99, 0)))
        setOrToggle<int>::set(this, tr("Set sale on %1 items"), &Document::Item::sale, &Document::Item::setSale, sale);
}

void Window::on_edit_bulk_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    int bulk = m_doc->selection().front()->bulkQuantity();

    if (MessageBox::getInteger(this, tr("Set bulk quantity for the selected items:"), QString(), bulk, new QIntValidator(1, 99999, 0)))
        setOrToggle<int>::set(this, tr("Set bulk quantity on %1 items"), &Document::Item::bulkQuantity, &Document::Item::setBulkQuantity, bulk);
}

void Window::on_edit_color_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    SelectColorDialog d(this);
    d.setWindowTitle(tr("Modify Color"));
    d.setColor(m_doc->selection().front()->color());

    if (d.exec() == QDialog::Accepted)
        setOrToggle<const BrickLink::Color *>::set(this, tr( "Set color on %1 items" ), &Document::Item::color, &Document::Item::setColor, d.color());
}

void Window::on_edit_remark_set_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    QString remarks = m_doc->selection().front()->remarks();

    if (MessageBox::getString(this, tr("Enter the new remark for all selected items:"), remarks))
        setOrToggle<QString, const QString &>::set(this, tr("Set remark on %1 items"), &Document::Item::remarks, &Document::Item::setRemarks, remarks);
}

void Window::on_edit_remark_add_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    QString addremarks;

    if (MessageBox::getString(this, tr("Enter the text, that should be added to the remarks of all selected items:"), addremarks)) {
        uint remarkcount = 0;
        m_doc->beginMacro();

        QRegExp regexp("\\b" + QRegExp::escape(addremarks) + "\\b");

        DisableUpdates disupd(w_list);

        foreach(Document::Item *pos, m_doc->selection()) {
            QString str = pos->remarks();

            if (str.isEmpty())
                str = addremarks;
            else if (str.indexOf(regexp) != -1)
                ;
            else if (addremarks.indexOf(QRegExp("\\b" + QRegExp::escape(str) + "\\b")) != -1)
                str = addremarks;
            else
                str = str + " " + addremarks;

            if (str != pos->remarks()) {
                Document::Item item = *pos;

                item.setRemarks(str);
                m_doc->changeItem(pos, item);

                remarkcount++;
            }
        }
        m_doc->endMacro(tr("Modified remark on %1 items").arg(remarkcount));
    }
}

void Window::on_edit_remark_rem_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    QString remremarks;

    if (MessageBox::getString(this, tr("Enter the text, that should be removed from the remarks of all selected items:"), remremarks)) {
        uint remarkcount = 0;
        m_doc->beginMacro();

        QRegExp regexp("\\b" + QRegExp::escape(remremarks) + "\\b");

        DisableUpdates disupd(w_list);

        foreach(Document::Item *pos, m_doc->selection()) {
            QString str = pos->remarks();
            str.remove(regexp);
            str = str.simplified();

            if (str != pos->remarks()) {
                Document::Item item = *pos;

                item.setRemarks(str);
                m_doc->changeItem(pos, item);

                remarkcount++;
            }
        }
        m_doc->endMacro(tr("Modified remark on %1 items").arg(remarkcount));
    }
}


void Window::on_edit_comment_set_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    QString comments = m_doc->selection().front()->comments();

    if (MessageBox::getString(this, tr("Enter the new comment for all selected items:"), comments))
        setOrToggle<QString, const QString &>::set(this, tr("Set comment on %1 items"), &Document::Item::comments, &Document::Item::setComments, comments);
}

void Window::on_edit_comment_add_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    QString addcomments;

    if (MessageBox::getString(this, tr("Enter the text, that should be added to the comments of all selected items:"), addcomments)) {
        uint commentcount = 0;
        m_doc->beginMacro();

        QRegExp regexp("\\b" + QRegExp::escape(addcomments) + "\\b");

        DisableUpdates disupd(w_list);

        foreach(Document::Item *pos, m_doc->selection()) {
            QString str = pos->comments();

            if (str.isEmpty())
                str = addcomments;
            else if (str.indexOf(regexp) != -1)
                ;
            else if (addcomments.indexOf(QRegExp("\\b" + QRegExp::escape(str) + "\\b")) != -1)
                str = addcomments;
            else
                str = str + " " + addcomments;

            if (str != pos->comments()) {
                Document::Item item = *pos;

                item.setComments(str);
                m_doc->changeItem(pos, item);

                commentcount++;
            }
        }
        m_doc->endMacro(tr("Modified comment on %1 items").arg(commentcount));
    }
}

void Window::on_edit_comment_rem_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    QString remcomments;

    if (MessageBox::getString(this, tr("Enter the text, that should be removed from the comments of all selected items:"), remcomments)) {
        uint commentcount = 0;
        m_doc->beginMacro();

        QRegExp regexp("\\b" + QRegExp::escape(remcomments) + "\\b");

        DisableUpdates disupd(w_list);

        foreach(Document::Item *pos, m_doc->selection()) {
            QString str = pos->comments();
            str.remove(regexp);
            str = str.simplified();

            if (str != pos->comments()) {
                Document::Item item = *pos;

                item.setComments(str);
                m_doc->changeItem(pos, item);

                commentcount++;
            }
        }
        m_doc->endMacro(tr("Modified comment on %1 items").arg(commentcount));
    }
}


void Window::on_edit_reserved_triggered()
{
    if (m_doc->selection().isEmpty())
        return;

    QString reserved = m_doc->selection().front()->reserved();

    if (MessageBox::getString(this, tr("Reserve all selected items for this specific buyer (BrickLink username):"), reserved))
        setOrToggle<QString, const QString &>::set(this, tr("Set reservation on %1 items"), &Document::Item::reserved, &Document::Item::setReserved, reserved);
}

void Window::updateErrorMask()
{
    quint64 em = 0;

    if (Config::inst()->showInputErrors()) {
        if (Config::inst()->simpleMode()) {
            em = 1ULL << Document::Status     | \
                 1ULL << Document::Picture     | \
                 1ULL << Document::PartNo      | \
                 1ULL << Document::Description | \
                 1ULL << Document::Condition   | \
                 1ULL << Document::Color       | \
                 1ULL << Document::Quantity    | \
                 1ULL << Document::Remarks     | \
                 1ULL << Document::Category    | \
                 1ULL << Document::ItemType    | \
                 1ULL << Document::Weight      | \
                 1ULL << Document::YearReleased;
        }
        else {
            em = (1ULL << Document::FieldCount) - 1;
        }
    }

    m_doc->setErrorMask(em);
}

void Window::on_edit_copyremarks_triggered()
{
// DlgSubtractItemImpl d ( tr( "Please choose the document that should serve as a source to fill in the remarks fields of the current document:" ), this, "CopyRemarkDlg" );

    if (true) { //TODO: d.exec ( ) == QDialog::Accepted ) {
        BrickLink::InvItemList list; // = d.items ( );

        if (!list.isEmpty())
            copyRemarks(list);

        qDeleteAll(list);
    }
}

void Window::copyRemarks(const BrickLink::InvItemList &items)
{
    if (items.isEmpty())
        return;

    m_doc->beginMacro();

    DisableUpdates disupd(w_list);

    int copy_count = 0;

    foreach(Document::Item *pos, m_doc->items()) {
        BrickLink::InvItem *match = 0;

        foreach(BrickLink::InvItem *ii, items) {
            const BrickLink::Item *item   = ii->item();
            const BrickLink::Color *color = ii->color();
            BrickLink::Condition cond     = ii->condition();
            int qty                       = ii->quantity();

            if (!item || !color || !qty)
                continue;

            if ((pos->item() == item) && (pos->condition() == cond)) {
                if (pos->color() == color)
                    match = ii;
                else if (!match)
                    match = ii;
            }
        }

        if (match && !match->remarks().isEmpty()) {
            Document::Item newitem = *pos;
            newitem.setRemarks(match->remarks());
            m_doc->changeItem(pos, newitem);

            copy_count++;
        }
    }
    m_doc->endMacro(tr("Copied Remarks for %1 Items").arg(copy_count));
}

void Window::on_edit_subtractitems_triggered()
{
    //DlgSubtractItemImpl d ( tr( "Which items should be subtracted from the current document:" ), this, "SubtractItemDlg" );

    if (true) { //TODO: d.exec ( ) == QDialog::Accepted ) {
        BrickLink::InvItemList list; // = d.items ( );

        if (!list.isEmpty())
            subtractItems(list);

        qDeleteAll(list);
    }
}

void Window::subtractItems(const BrickLink::InvItemList &items)
{
    if (items.isEmpty())
        return;

    m_doc->beginMacro();

    DisableUpdates disupd(w_list);

    foreach(BrickLink::InvItem *ii, items) {
        const BrickLink::Item *item   = ii->item();
        const BrickLink::Color *color = ii->color();
        BrickLink::Condition cond     = ii->condition();
        int qty                       = ii->quantity();

        if (!item || !color || !qty)
            continue;

        Document::Item *last_match = 0;

        foreach(Document::Item *pos, m_doc->items()) {
            if ((pos->item() == item) && (pos->color() == color) && (pos->condition() == cond)) {
                Document::Item newitem = *pos;

                if (pos->quantity() >= qty) {
                    newitem.setQuantity(pos->quantity() - qty);
                    qty = 0;
                }
                else {
                    qty -= pos->quantity();
                    newitem.setQuantity(0);
                }
                m_doc->changeItem(pos, newitem);
                last_match = pos;
            }
        }

        if (qty) {   // still a qty left
            if (last_match) {
                Document::Item newitem = *last_match;
                newitem.setQuantity(last_match->quantity() - qty);
                m_doc->changeItem(last_match, newitem);
            }
            else {
                Document::Item *newitem = new Document::Item();
                newitem->setItem(item);
                newitem->setColor(color);
                newitem->setCondition(cond);
                newitem->setOrigQuantity(0);
                newitem->setQuantity(-qty);

                m_doc->insertItem(0, newitem);
            }
        }
    }
    m_doc->endMacro(tr("Subtracted %1 Items").arg(items.count()));
}

void Window::on_edit_mergeitems_triggered()
{
    if (!m_doc->selection().isEmpty())
        mergeItems(m_doc->selection());
    else
        mergeItems(m_doc->items());
}

void Window::on_edit_partoutitems_triggered()
{
    if (m_doc->selection().count() >= 1) {
        foreach(Document::Item *item, m_doc->selection())
        FrameWork::inst()->fileImportBrickLinkInventory(item->item());
    }
    else
        QApplication::beep();
}

void Window::setPrice(money_t d)
{
    if (m_doc->selection().count() == 1) {
        Document::Item *pos = m_doc->selection().front();
        Document::Item item = *pos;

        item.setPrice(d);
        m_doc->changeItem(pos, item);
    }
    else
        QApplication::beep();
}

void Window::contextMenu(const QPoint &p)
{
    FrameWork::inst()->showContextMenu(/*TODO: */ true, w_list->viewport()->mapToGlobal(p));
}

void Window::on_file_close_triggered()
{
    close();
}

void Window::closeEvent(QCloseEvent *e)
{
    bool close_empty = (m_doc->items().isEmpty() && Config::inst()->closeEmptyDocuments());

    if (m_doc->isModified() && !close_empty) {
        switch (MessageBox::warning(this, tr("Save changes to %1?").arg(CMB_BOLD(windowTitle())), MessageBox::Yes | MessageBox::No | MessageBox::Cancel, MessageBox::Yes)) {
        case MessageBox::Yes:
            m_doc->fileSave(sortedItems());

            if (!m_doc->isModified())
                e->accept();
            else
                e->ignore();
            break;

        case MessageBox::No:
            e->accept();
            break;

        default:
            e->ignore();
            break;
        }
    }
    else {
        e->accept();
    }

}

void Window::on_edit_bl_catalog_triggered()
{
    if (!m_doc->selection().isEmpty())
        QDesktopServices::openUrl(BrickLink::core()->url(BrickLink::URL_CatalogInfo, (*m_doc->selection().front()).item()));
}

void Window::on_edit_bl_priceguide_triggered()
{
    if (!m_doc->selection().isEmpty())
        QDesktopServices::openUrl(BrickLink::core()->url(BrickLink::URL_PriceGuideInfo, (*m_doc->selection().front()).item(), (*m_doc->selection().front()).color()));
}

void Window::on_edit_bl_lotsforsale_triggered()
{
    if (!m_doc->selection().isEmpty())
        QDesktopServices::openUrl(BrickLink::core()->url(BrickLink::URL_LotsForSale, (*m_doc->selection().front()).item(), (*m_doc->selection().front()).color()));
}

void Window::on_edit_bl_myinventory_triggered()
{
    if (!m_doc->selection().isEmpty()) {
        uint lotid = (*m_doc->selection().front()).lotId();
        QDesktopServices::openUrl(BrickLink::core()->url(BrickLink::URL_StoreItemDetail, &lotid));
    }
}

void Window::on_view_difference_mode_toggled(bool b)
{
//TODO: w_list->setDifferenceMode ( b );
}

bool Window::isDifferenceMode() const
{
    return false; //TODO: return w_list->isDifferenceMode ( );
}

void Window::on_file_print_triggered()
{
    if (m_doc->items().isEmpty())
        return;
    /*
     if ( CReportManager::inst ( )->reports ( ).isEmpty ( )) {
      CReportManager::inst ( )->reload ( );

      if ( CReportManager::inst ( )->reports ( ).isEmpty ( ))
       return;
     }

     QPrinter *prt = CReportManager::inst ( )->printer ( );

     if ( !prt )
      return;

     //prt->setOptionEnabled ( QPrinter::PrintToFile, false );
     prt->setOptionEnabled ( QPrinter::PrintSelection, !m_doc->selection ( ).isEmpty ( ));
     prt->setOptionEnabled ( QPrinter::PrintPageRange, false );
     prt->setPrintRange ( m_doc->selection ( ).isEmpty ( ) ? QPrinter::AllPages : QPrinter::Selection );

     QString doctitle = m_doc->title();
     if (doctitle == m_doc->fileName())
         doctitle = QFileInfo(doctitle).baseName();

     prt->setDocName(doctitle);

     prt->setFullPage ( true );


     if ( !prt->setup ( FrameWork::inst ( )))
      return;

     DlgSelectReportImpl d ( this );

     if ( d.exec ( ) != QDialog::Accepted )
      return;

     const CReport *rep = d.report ( );

     if ( !rep )
      return;

     rep->print ( prt, m_doc, sortedItems ( prt->printRange ( ) == QPrinter::Selection ));
     */
}

void Window::on_file_print_pdf_triggered()
{
    qWarning("Print to PDF is not implemented yet");
}

void Window::on_file_save_triggered()
{
    m_doc->fileSave(sortedItems());
}

void Window::on_file_saveas_triggered()
{
    m_doc->fileSaveAs(sortedItems());
}

void Window::on_file_export_bl_xml_triggered()
{
    Document::ItemList items = exportCheck();

    if (!items.isEmpty())
        m_doc->fileExportBrickLinkXML(items);
}

void Window::on_file_export_bl_xml_clip_triggered()
{
    Document::ItemList items = exportCheck();

    if (!items.isEmpty())
        m_doc->fileExportBrickLinkXMLClipboard(items);
}

void Window::on_file_export_bl_update_clip_triggered()
{
    Document::ItemList items = exportCheck();

    if (!items.isEmpty())
        m_doc->fileExportBrickLinkUpdateClipboard(items);
}

void Window::on_file_export_bl_invreq_clip_triggered()
{
    Document::ItemList items = exportCheck();

    if (!items.isEmpty())
        m_doc->fileExportBrickLinkInvReqClipboard(items);
}

void Window::on_file_export_bl_wantedlist_clip_triggered()
{
    Document::ItemList items = exportCheck();

    if (!items.isEmpty())
        m_doc->fileExportBrickLinkWantedListClipboard(items);
}

void Window::on_file_export_briktrak_triggered()
{
    Document::ItemList items = exportCheck();

    if (!items.isEmpty())
        m_doc->fileExportBrikTrakInventory(items);
}

Document::ItemList Window::exportCheck() const
{
    Document::ItemList items;
    bool selection_only = false;

    if (!m_doc->selection().isEmpty() && (m_doc->selection().count() != m_doc->items().count())) {
        if (MessageBox::question(FrameWork::inst(), tr("There are %1 items selected.<br /><br />Do you want to export only these items?").arg(m_doc->selection().count()), MessageBox::Yes, MessageBox::No) == MessageBox::Yes) {
            selection_only = true;
        }
    }

    if (m_doc->statistics(selection_only ? m_doc->selection() : m_doc->items()).errors()) {
        if (MessageBox::warning(FrameWork::inst(), tr("This list contains items with errors.<br /><br />Do you really want to export this list?"), MessageBox::Yes, MessageBox::No) != MessageBox::Yes)
            return Document::ItemList();
    }

    return sortedItems(selection_only);
}

Document::ItemList Window::sortedItems(bool selection_only) const
{
    Document::ItemList sorted;
    /*
     for ( QListViewItemIterator it ( w_list ); it.current ( ); ++it ) {
      CItemViewItem *ivi = static_cast <CItemViewItem *> ( it.current ( ));

      if ( selection_only && ( m_doc->selection ( ).find ( ivi->item ( )) == m_doc->selection ( ).end ( )))
       continue;

      sorted.append ( ivi->item ( ));
     }
    */
    return sorted;
}
