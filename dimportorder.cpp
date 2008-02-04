/* Copyright (C) 2004-2005 Robert Griebl.All rights reserved.
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
#include <QLineEdit>
#include <QValidator>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QStackedWidget>
#include <QLabel>
#include <QComboBox>
#include <QHeaderView>
#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QColorGroup>
#include <QVariant>

#include "cimport.h"
#include "cprogressdialog.h"
#include "cutility.h"

#include "dimportorder.h"

namespace {

template <typename T> static int cmp(const T &a, const T &b)
{
    if (a < b)
        return -1;
    else if (a == b)
        return 0;
    else
        return 1;
}

class OrderListModel : public QAbstractTableModel {
public:
    OrderListModel(const QList<QPair<BrickLink::Order *, BrickLink::InvItemList *> > &orderlist)
            : m_orderlist(orderlist)
    { }

    ~OrderListModel()
    {
        for (QList<QPair<BrickLink::Order *, BrickLink::InvItemList *> >::iterator it = m_orderlist.begin(); it != m_orderlist.end(); ++it) {
            delete it->first;
            delete it->second;
        }
    }

    virtual int rowCount(const QModelIndex & /*parent*/) const
    {
        return m_orderlist.size();
    }

    virtual int columnCount(const QModelIndex & /*parent*/) const
    {
        return 5;
    }

    bool isReceived(const QPair<BrickLink::Order *, BrickLink::InvItemList *> &order) const
    {
        return (order.first->type() == BrickLink::Order::Received);
    }

    virtual QVariant data(const QModelIndex &index, int role) const
    {
        QVariant res;
        const QPair<BrickLink::Order *, BrickLink::InvItemList *> &order = m_orderlist [index.row()];
        int col = index.column();

        if (role == Qt::DisplayRole) {
            switch (col) {
            case 0: res = isReceived(order) ? DImportOrder::tr("Received") : DImportOrder::tr("Placed"); break;
            case 1: res = order.first->id(); break;
            case 2: res = QLocale().toString(order.first->date().date(), QLocale::ShortFormat); break;
            case 3: res = isReceived(order) ? order.first->buyer() : order.first->seller(); break;
            case 4: res = order.first->grandTotal().toLocalizedString(); break;
            }
        }
        else if (role == Qt::TextAlignmentRole) {
            res = (col == 4) ? Qt::AlignRight : Qt::AlignLeft;
        }
        else if (role == Qt::BackgroundColorRole) {
            if (col == 0) {
                QColor c = isReceived(order) ? Qt::green : Qt::blue;
                c.setAlphaF(0.2);
                res = c;
            }
        }

        return res;
    }

    virtual QVariant headerData(int section, Qt::Orientation orient, int role) const
    {
        QVariant res;

        if (orient == Qt::Horizontal) {
            if (role == Qt::DisplayRole) {
                switch (section) {
                case  0: res = DImportOrder::tr("Type"); break;
                case  1: res = DImportOrder::tr("Order #"); break;
                case  2: res = DImportOrder::tr("Date"); break;
                case  3: res = DImportOrder::tr("Buyer/Seller"); break;
                case  4: res = DImportOrder::tr("Total"); break;
                }
            }
            else if (role == Qt::TextAlignmentRole) {
                res = (section == 4) ? Qt::AlignRight : Qt::AlignLeft;
            }
        }
        return res;
    }

    virtual void sort(int section, Qt::SortOrder so)
    {
        emit layoutAboutToBeChanged();
        qStableSort(m_orderlist.begin(), m_orderlist.end(), orderCompare(section, so));
        emit layoutChanged();
    }


    class orderCompare {
    public:
        orderCompare(int section, Qt::SortOrder so) : m_section(section), m_so(so) { }

        bool operator()(const QPair<BrickLink::Order *, BrickLink::InvItemList *> &o1, const QPair<BrickLink::Order *, BrickLink::InvItemList *> &o2)
        {
            bool d = false;

            switch (m_section) {
            case  0: d = (o1.first->type() < o2.first->type()); break;
            case  1: d = (o1.first->id() <  o2.first->id()); break;
            case  2: d = (o1.first->date() < o2.first->date()); break;
            case  3: d = (o1.first->buyer().toLower() < o2.first->buyer().toLower()); break;
            case  4: d = (o1.first->grandTotal() < o2.first->grandTotal()); break;
            }
            return m_so == Qt::DescendingOrder ? d : !d;
        }

    private:
        int m_section;
        Qt::SortOrder m_so;
    };


private:
    QList<QPair<BrickLink::Order *, BrickLink::InvItemList *> > m_orderlist;
};


class TransHighlightDelegate : public QItemDelegate {
public:
    TransHighlightDelegate(QObject *parent = 0)
            : QItemDelegate(parent)
    { }

    void makeTrans(QPalette &pal, QPalette::ColorGroup cg, QPalette::ColorRole cr, const QColor &bg, qreal factor) const
    {
        QColor hl = pal.color(cg, cr);
        QColor th;
        th.setRgbF(hl.redF()   * (1.0 - factor) + bg.redF()   * factor,
                   hl.greenF() * (1.0 - factor) + bg.greenF() * factor,
                   hl.blueF()  * (1.0 - factor) + bg.blueF()  * factor);

        pal.setColor(cg, cr, th);
    }

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItem myoption(option);

        if (option.state & QStyle::State_Selected) {
            QVariant v = index.data(Qt::BackgroundColorRole);
            QColor c = qvariant_cast<QColor> (v);
            if (v.isValid() && c.isValid()) {
                QStyleOptionViewItem myoption(option);

                makeTrans(myoption.palette, QPalette::Active,   QPalette::Highlight, c, 0.2);
                makeTrans(myoption.palette, QPalette::Inactive, QPalette::Highlight, c, 0.2);
                makeTrans(myoption.palette, QPalette::Disabled, QPalette::Highlight, c, 0.2);

                QItemDelegate::paint(painter, myoption, index);
                return;
            }
        }

        QItemDelegate::paint(painter, option, index);
    }
};

}


bool  DImportOrder::s_last_select   = true;
QDate DImportOrder::s_last_from     = QDate::currentDate().addMonths(-1);
QDate DImportOrder::s_last_to       = QDate::currentDate();
int   DImportOrder::s_last_type     = 0;


DImportOrder::DImportOrder(QWidget *parent, Qt::WindowFlags f)
        : QDialog(parent, f)
{
    setupUi(this);

    w_order_number->setValidator(new QIntValidator(1, 999999, w_order_number));

    w_order_list->setItemDelegate(new TransHighlightDelegate());
    w_order_list->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    w_order_list->horizontalHeader()->setMovable(false);
    w_order_list->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    w_order_list->verticalHeader()->hide();
    w_order_list->setShowGrid(false);
    w_order_list->setAlternatingRowColors(true);
    w_order_list->setSortingEnabled(true);

    connect(w_order_number, SIGNAL(textChanged(const QString &)), this, SLOT(checkId()));
    connect(w_by_number, SIGNAL(toggled(bool)), this, SLOT(checkId()));
    connect(w_order_from, SIGNAL(valueChanged(const QDate &)), this, SLOT(checkId()));
    connect(w_order_to, SIGNAL(valueChanged(const QDate &)), this, SLOT(checkId()));

    connect(w_order_list, SIGNAL(selectionChanged()), this, SLOT(checkSelected()));
    connect(w_order_list, SIGNAL(doubleClicked(QListViewItem *, const QPoint &, int)), this, SLOT(activateItem(QListViewItem *)));
    connect(w_order_list, SIGNAL(returnPressed(QListViewItem *)), this, SLOT(activateItem(QListViewItem *)));

    connect(w_next, SIGNAL(clicked()), this, SLOT(download()));
    connect(w_back, SIGNAL(clicked()), this, SLOT(start()));

    w_order_from->setDate(s_last_from);
    w_order_to->setDate(s_last_to);
    w_order_type->setCurrentIndex(s_last_type);
    w_by_number->setChecked(s_last_select);

    start();
    resize(sizeHint());
}

DImportOrder::~DImportOrder()
{ }

void DImportOrder::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        retranslateUi(this);
    else
        QDialog::changeEvent(e);
}

void DImportOrder::accept()
{
    s_last_select = w_by_number->isChecked();
    s_last_from   = w_order_from->date();
    s_last_to     = w_order_to->date();
    s_last_type   = w_order_type->currentIndex();

    QDialog::accept();
}

void DImportOrder::start()
{
    w_stack->setCurrentIndex(0);

    if (w_by_number->isChecked())
        w_order_number->setFocus();
    else
        w_order_from->setFocus();

    w_ok->hide();
    w_back->hide();
    w_next->show();
    w_next->setDefault(true);

    checkId();
}

void DImportOrder::download()
{
    CProgressDialog progress(this);
    CImportBLOrder *import;

    if (w_by_number->isChecked())
        import = new CImportBLOrder(w_order_number->text(), orderType(), &progress);
    else
        import = new CImportBLOrder(w_order_from->date(), w_order_to->date(), orderType(), &progress);

    bool ok = (progress.exec() == QDialog::Accepted);

    if (ok && !import->orders().isEmpty()) {
        w_stack->setCurrentIndex(2);

        w_order_list->setModel(new OrderListModel(import->orders()));
        w_order_list->horizontalHeader()->setResizeMode(2, QHeaderView::Stretch);
        w_order_list->sortByColumn(2);

        w_order_list->selectionModel()->select(w_order_list->model()->index(0, 0), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        w_order_list->scrollTo(w_order_list->model()->index(0, 0));
        w_order_list->setFocus();

        w_ok->show();
        w_ok->setDefault(true);
        w_next->hide();
        w_back->show();

        checkSelected();
    }
    else {
        w_message->setText(tr("There was a problem downloading the data for the specified order(s).This could have been caused by three things:<ul><li>a network error occured.</li><li>the order number and/or type you entered is invalid.</li><li>there are no orders of the specified type in the given time period.</li></ul>"));
        w_stack->setCurrentIndex(1);

        w_ok->hide();
        w_next->hide();
        w_back->show();
        w_back->setDefault(true);
    }

    delete import;
}

QPair<BrickLink::Order *, BrickLink::InvItemList *> DImportOrder::order() const
{
// OrderListItem *oli = static_cast<OrderListItem *> ( w_order_list->selectedItem ( ));

// if ( oli )
//  return oli->order ( );
// else
    return qMakePair<BrickLink::Order *, BrickLink::InvItemList *> (0, 0);
}

BrickLink::Order::Type DImportOrder::orderType() const
{
    switch (w_order_type->currentIndex()) {
    case 2 : return BrickLink::Order::Placed;
    case 1 : return BrickLink::Order::Received;
    case 0 :
    default: return BrickLink::Order::Any;
    }
}

void DImportOrder::checkId()
{
    bool ok = true;

    if (w_by_number->isChecked())
        ok = w_order_number->hasAcceptableInput() && (w_order_number->text().length() == 6);
    else
        ok = (w_order_from->date() <= w_order_to->date()) && (w_order_to->date() <= QDate::currentDate());

    w_next->setEnabled(ok);
}

void DImportOrder::checkSelected()
{
// w_ok->setEnabled ( w_order_list->selectedItem ( ) != 0 );
}

void DImportOrder::activateItem(/*QListViewItem **/)
{
    checkSelected();
    w_ok->animateClick();
}