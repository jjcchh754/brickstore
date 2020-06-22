/* Copyright (C) 2004-2020 Robert Griebl. All rights reserved.
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
#pragma once

#include "bricklinkfwd.h"

#include <QDateTime>
#include <QString>
#include <QColor>
#include <QObject>
#include <QImage>
#include <QtXml/QDomDocument>
#include <QLocale>
#include <QHash>
#include <QVector>
#include <QList>
#include <QMap>
#include <QPair>
#include <QUrl>
#include <QMimeData>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QMutex>
#include <QTimer>
#include <QPointer>
#include <QStyledItemDelegate>

#include <ctime>

#include "ref.h"
#include "currency.h"
#include "threadpool.h"
#include "transfer.h"
#include "staticpointermodel.h"
#include "q3cache.h"

class QIODevice;
class QFile;


namespace BrickLink {

class ItemType
{
public:
    char id() const                 { return m_id; }
    QString name() const            { return m_name; }
    QString apiName () const        { return QString(m_name).replace(" ", "_"); } //TODO5: brickstock has this

    const QVector<const Category *> categories() const  { return m_categories; }
    bool hasInventories() const     { return m_has_inventories; }
    bool hasColors() const          { return m_has_colors; }
    bool hasYearReleased() const    { return m_has_year; }
    bool hasWeight() const          { return m_has_weight; }
    bool hasSubConditions() const   { return m_has_subconditions; }
    char pictureId() const          { return m_picture_id; }
    QSize pictureSize() const;

private:
    char  m_id;
    char  m_picture_id;

    bool  m_has_inventories   : 1;
    bool  m_has_colors        : 1;
    bool  m_has_weight        : 1;
    bool  m_has_year          : 1;
    bool  m_has_subconditions : 1;

    QString m_name;

    QVector<const Category *> m_categories;

private:
    ItemType() = default;

    friend class Core;
    friend class TextImport;
    friend QDataStream &operator << (QDataStream &ds, const ItemType *itt);
    friend QDataStream &operator >> (QDataStream &ds, ItemType *itt);
};


class Category
{
public:
    uint id() const       { return m_id; }
    QString name() const  { return m_name; }

private:
    uint     m_id;
    QString  m_name;

private:
    Category() = default;

    friend class Core;
    friend class TextImport;
    friend QDataStream &operator << (QDataStream &ds, const Category *cat);
    friend QDataStream &operator >> (QDataStream &ds, Category *cat);
};

class Color
{
public:
    uint id() const           { return m_id; }
    QString name() const      { return m_name; }
    QColor color() const      { return m_color; }

    int ldrawId() const       { return m_ldraw_id; }

    enum TypeFlag {
        Solid        = 0x0001,
        Transparent  = 0x0002,
        Glitter      = 0x0004,
        Speckle      = 0x0008,
        Metallic     = 0x0010,
        Chrome       = 0x0020,
        Pearl        = 0x0040,
        Milky        = 0x0080,
        Modulex      = 0x0100,

        Mask         = 0x01ff
    };

    Q_DECLARE_FLAGS(Type, TypeFlag)
    Type type() const          { return m_type; }

    bool isSolid() const       { return m_type & Solid; }
    bool isTransparent() const { return m_type & Transparent; }
    bool isGlitter() const     { return m_type & Glitter; }
    bool isSpeckle() const     { return m_type & Speckle; }
    bool isMetallic() const    { return m_type & Metallic; }
    bool isChrome() const      { return m_type & Chrome; }
    bool isPearl() const       { return m_type & Pearl; }
    bool isMilky() const       { return m_type & Milky; }
    bool isModulex() const     { return m_type & Modulex; }

    qreal popularity() const   { return m_popularity < 0 ? 0 : m_popularity; }

    static QString typeName(TypeFlag t);

private:
    QString m_name;
    uint    m_id;
    int     m_ldraw_id;
    QColor  m_color;
    Type    m_type;
    qreal   m_popularity = 0;
    quint16 m_year_from = 0;
    quint16 m_year_to = 0;

private:
    Color() = default;

    friend class Core;
    friend class TextImport;
    friend QDataStream &operator << (QDataStream &ds, const Color *col);
    friend QDataStream &operator >> (QDataStream &ds, Color *col);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Color::Type)

class Item
{
public:
    QString id() const                     { return m_id; }
    QString name() const                   { return m_name; }
    const ItemType *itemType() const       { return m_item_type; }
    const Category *category() const       { return m_categories.isEmpty() ? nullptr : m_categories.constFirst(); }
    const QVector<const Category *> allCategories() const  { return m_categories; }
    bool hasCategory(const Category *cat) const;
    bool hasInventory() const              { return (m_last_inv_update >= 0); }
    QDateTime inventoryUpdated() const     { QDateTime dt; if (m_last_inv_update >= 0) dt.setTime_t (m_last_inv_update); return dt; }
    const Color *defaultColor() const      { return m_color; }
    double weight() const                  { return m_weight; }
    int yearReleased() const               { return m_year ? m_year + 1900 : 0; }

    ~Item();

    AppearsIn appearsIn(const Color *color = nullptr) const;
    InvItemList  consistsOf() const;

    uint index() const { return m_index; }   // only for internal use (picture/priceguide hashes)

private:
    QString           m_name;
    QString           m_id;
    const ItemType *  m_item_type = nullptr;
    QVector<const Category *> m_categories;
    const Color *     m_color;
    time_t            m_last_inv_update = -1;
    float             m_weight;
    quint32           m_index : 24;
    quint32           m_year  : 8;

    mutable quint32 * m_appears_in = nullptr;
    mutable quint64 * m_consists_of = nullptr;

private:
    Item() = default;
    Q_DISABLE_COPY(Item);

    void setAppearsIn(const AppearsIn &hash) const;
    void setConsistsOf(const InvItemList &items) const;

    struct appears_in_record {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        quint32  m12  : 12;
        quint32  m20  : 20;
#else
        quint32  m20  : 20;
        quint32  m12  : 12;
#endif
    };

    struct consists_of_record {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        quint64  m_qty      : 12;
        quint64  m_index    : 20;
        quint64  m_color    : 12;
        quint64  m_extra    : 1;
        quint64  m_isalt    : 1;
        quint64  m_altid    : 6;
        quint64  m_cpart    : 1;
        quint64  m_reserved : 11;
#else
        quint64  m_reserved : 11;
        quint64  m_cpart    : 1;
        quint64  m_altid    : 6;
        quint64  m_isalt    : 1;
        quint64  m_extra    : 1;
        quint64  m_color    : 12;
        quint64  m_index    : 20;
        quint64  m_qty      : 12;
#endif
    };

    static int compare(const Item **a, const Item **b);
    static bool lessThan(const Item *a, const Item *b);

    friend class Core;
    friend class TextImport;
    friend QDataStream &operator << (QDataStream &ds, const Item *item);
    friend QDataStream &operator >> (QDataStream &ds, Item *item);
};


class Picture : public Ref
{
public:
    const Item *item() const          { return m_item; }
    const Color *color() const        { return m_color; }

    void update(bool high_priority = false);
    QDateTime lastUpdate() const      { return m_fetched; }

    bool valid() const                { return m_valid; }
    int updateStatus() const          { return m_update_status; }

    const QPixmap pixmap() const;

    virtual ~Picture();

    const QImage image() const        { return m_image; }
    QString key() const               { return QString::number(m_image.cacheKey()); }

    int cost() const;

private:
    const Item *  m_item;
    const Color * m_color;

    QDateTime     m_fetched;

    bool          m_valid         : 1;
    int           m_update_status : 7;

    QImage        m_image;

private:
    Picture(const Item *item, const Color *color);

    void load_from_disk();
    void save_to_disk();

    void parse(const char *data, uint size);

    friend class Core;
    friend class PictureLoaderJob;
};

class InvItem
{
public:
    InvItem(const Color *color = nullptr, const Item *item = nullptr);
    InvItem(const InvItem &copy);
    ~InvItem();

    InvItem &operator = (const InvItem &copy);
    bool operator == (const InvItem &cmp) const;

    const Item *item() const           { return m_item; }
    void setItem(const Item *i)        { /*can be 0*/ m_item = i; }
    const Category *category() const   { return m_item ? m_item->category() : 0; }
    const ItemType *itemType() const   { return m_item ? m_item->itemType() : 0; }
    const Color *color() const         { return m_color; }
    void setColor(const Color *c)      { m_color = c; }

    QString itemId() const             { return m_item ? m_item->id() : (m_incomplete ? m_incomplete->m_item_id : QString()); }
    QString itemName() const           { return m_item ? m_item->name() : (m_incomplete ? m_incomplete->m_item_name : QString()); }
    QString colorName() const          { return m_color ? m_color->name() : (m_incomplete ? m_incomplete->m_color_name : QString()); }
    QString categoryName() const       { return category() ? category()->name() : (m_incomplete ? m_incomplete->m_category_name : QString()); }
    QString itemTypeName() const       { return itemType() ? itemType()->name() : (m_incomplete ? m_incomplete->m_itemtype_name : QString()); }
    int itemYearReleased() const       { return m_item ? m_item->yearReleased() : 0; }

    Status status() const              { return m_status; }
    void setStatus(Status s)           { m_status = s; }
    Condition condition() const        { return m_condition; }
    void setCondition(Condition c)     { m_condition = c; }
    SubCondition subCondition() const  { return m_scondition; }
    void setSubCondition(SubCondition c) { m_scondition = c; }
    QString comments() const           { return m_comments; }
    void setComments(const QString &n) { m_comments = n; }
    QString remarks() const            { return m_remarks; }
    void setRemarks(const QString &r)  { m_remarks = r; }

    int quantity() const               { return m_quantity; }
    void setQuantity(int q)            { m_quantity = q; }
    int origQuantity() const           { return m_orig_quantity; }
    void setOrigQuantity(int q)        { m_orig_quantity = q; }
    int bulkQuantity() const           { return m_bulk_quantity; }
    void setBulkQuantity(int q)        { m_bulk_quantity = qMax(1, q); }
    int tierQuantity(uint i) const     { return m_tier_quantity [i < 3 ? i : 0]; }
    void setTierQuantity(uint i, int q){ m_tier_quantity [i < 3 ? i : 0] = q; }
    double price() const               { return m_price; }
    void setPrice(double p)            { m_price = p; }
    double origPrice() const           { return m_orig_price; }
    void setOrigPrice(double p)        { m_orig_price = p; }
    double tierPrice(uint i) const     { return m_tier_price [i < 3 ? i : 0]; }
    bool setTierPrice(uint i, double p){ if (p < 0) return false; m_tier_price [i < 3 ? i : 0] = p; return true; }
    int sale() const                   { return m_sale; }
    void setSale(int s)                { m_sale = qMax(-99, qMin(100, s)); }
    double total() const               { return m_price * m_quantity; }

    uint lotId() const                 { return m_lot_id; }
    void setLotId(uint lid)            { m_lot_id = lid; }

    bool retain() const                { return m_retain; }
    void setRetain(bool r)             { m_retain = r; }
    bool stockroom() const             { return m_stockroom; }
    void setStockroom(bool s)          { m_stockroom = s; }

    double weight() const              { return m_weight ? m_weight : (m_item ? m_item->weight() * m_quantity : 0); }
    void setWeight(double w)           { m_weight = (float) w; }

    QString reserved() const           { return m_reserved; }
    void setReserved(const QString &r) { m_reserved = r; }

    bool alternate() const             { return m_alternate; }
    void setAlternate(bool a)          { m_alternate = a; }
    uint alternateId() const           { return m_alt_id; }
    void setAlternateId(uint aid)      { m_alt_id = aid; }

    bool counterPart() const           { return m_cpart; }
    void setCounterPart(bool b)        { m_cpart = b; }

    struct Incomplete {
        QString m_item_id;
        QString m_item_name;
        QString m_itemtype_name;
        QString m_color_name;
        QString m_category_name;
    };

    const Incomplete *isIncomplete() const { return m_incomplete; }
    void setIncomplete(Incomplete *inc)    { delete m_incomplete; m_incomplete = inc; }

    bool mergeFrom(const InvItem &merge, bool prefer_from = false);

    typedef void *Diff; // opaque handle

    Diff *createDiff(const InvItem &diffto) const;
    bool applyDiff(Diff *diff);

private:
    const Item *     m_item;
    const Color *    m_color;

    Incomplete *     m_incomplete;

    Status           m_status    : 3;
    Condition        m_condition : 2;
    SubCondition     m_scondition: 3;
    bool             m_alternate : 1;
    bool             m_cpart     : 1;
    uint             m_alt_id    : 6;
    bool             m_retain    : 1;
    bool             m_stockroom : 1;
    int              m_xreserved : 14;

    QString          m_comments;
    QString          m_remarks;
    QString          m_reserved;

    int              m_quantity;
    int              m_bulk_quantity;
    int              m_tier_quantity [3];
    int              m_sale;

    double           m_price;
    double           m_tier_price [3];

    float            m_weight;
    uint             m_lot_id;

    double           m_orig_price;
    int              m_orig_quantity;

    friend QDataStream &operator << (QDataStream &ds, const InvItem &ii);
    friend QDataStream &operator >> (QDataStream &ds, InvItem &ii);

    friend class Core;
};

class InvItemMimeData : public QMimeData
{
    Q_OBJECT
public:
    InvItemMimeData(const InvItemList &items);

    QStringList formats() const override;
    bool hasFormat(const QString & mimeType) const override;

    void setItems(const InvItemList &items);
    static InvItemList items(const QMimeData *md);

private:
    static const char *s_mimetype;
};

class Order
{
public:
    Order(const QString &id, OrderType type);

    QString id() const          { return m_id; }
    OrderType type() const      { return m_type; }
    QDateTime date() const      { return m_date; }
    QDateTime statusChange() const  { return m_status_change; }
    //QString buyer() const     { return m_type == Received ? m_other : QString(); }
    //QString seller() const    { return m_type == Placed ? m_other : QString(); }
    QString other() const       { return m_other; }
    double shipping() const     { return m_shipping; }
    double insurance() const    { return m_insurance; }
    double delivery() const     { return m_delivery; }
    double credit() const       { return m_credit; }
    double grandTotal() const   { return m_grand_total; }
    QString status() const      { return m_status; }
    QString payment() const     { return m_payment; }
    QString remarks() const     { return m_remarks; }
    QString address() const     { return m_address; }
    QString countryName() const;
    QString countryCode() const;
    QString currencyCode() const { return m_currencycode; }

    void setId(const QString &id)             { m_id = id; }
    void setDate(const QDateTime &dt)         { m_date = dt; }
    void setStatusChange(const QDateTime &dt) { m_status_change = dt; }
    void setBuyer(const QString &str)         { m_other = str; m_type = Received; }
    void setSeller(const QString &str)        { m_other = str; m_type = Placed; }
    void setShipping(double m)                { m_shipping = m; }
    void setInsurance(double m)               { m_insurance = m; }
    void setDelivery(double m)                { m_delivery = m; }
    void setCredit(double m)                  { m_credit = m; }
    void setGrandTotal(double m)              { m_grand_total = m; }
    void setStatus(const QString &str)        { m_status = str; }
    void setPayment(const QString &str)       { m_payment = str; }
    void setRemarks(const QString &str)       { m_remarks = str; }
    void setAddress(const QString &str)       { m_address = str; }
    void setCountryName(const QString &str);
    void setCountryCode(const QString &str);
    void setCurrencyCode(const QString &str)  { m_currencycode = str; }

private:
    QString   m_id;
    OrderType m_type;
    QDateTime m_date;
    QDateTime m_status_change;
    QString   m_other;
    double    m_shipping;
    double    m_insurance;
    double    m_delivery;
    double    m_credit;
    double    m_grand_total;
    QString   m_status;
    QString   m_payment;
    QString   m_remarks;
    QString   m_address;
    QString   m_currencycode;
    QChar     m_countryCode[2];
};

class PriceGuide : public Ref
{
public:
    const Item *item() const          { return m_item; }
    const Color *color() const        { return m_color; }

    void update(bool high_priority = false);
    QDateTime lastUpdate() const      { return m_fetched; }

    bool valid() const                { return m_valid; }
    int updateStatus() const          { return m_update_status; }

    int quantity(Time t, Condition c) const            { return m_quantities [t < TimeCount ? t : 0][c < ConditionCount ? c : 0]; }
    int lots(Time t, Condition c) const                { return m_lots [t < TimeCount ? t : 0][c < ConditionCount ? c : 0]; }
    double price(Time t, Condition c, Price p) const { return m_prices [t < TimeCount ? t : 0][c < ConditionCount ? c : 0][p < PriceCount ? p : 0]; }

private:
    const Item *  m_item;
    const Color * m_color;

    QDateTime     m_fetched;

    bool          m_valid         : 1;
    int           m_update_status : 7;

    int           m_quantities [TimeCount][ConditionCount];
    int           m_lots       [TimeCount][ConditionCount];
    double        m_prices     [TimeCount][ConditionCount][PriceCount];

private:
    PriceGuide(const Item *item, const Color *color);

    void load_from_disk();
    void save_to_disk();

    void parse(const QByteArray &ba);

    friend class Core;
};

class ChangeLogEntry : public QByteArray
{
public:
    enum Type {
        Invalid,
        ItemId,
        ItemType,
        ItemMerge,
        CategoryName,
        CategoryMerge,
        ColorName,
        ColorMerge
    };

    ChangeLogEntry(const char *data) { m_data = QByteArray::fromRawData(data, qstrlen(data)); }
    ~ChangeLogEntry() = default;

    Type type() const              { return m_data.isEmpty() ? Invalid : Type(m_data.at(0)); }
    QByteArray from(int idx) const { return (idx < 0 || idx >= 2) ? QByteArray() : m_data.split('\t')[idx+1]; }
    QByteArray to(int idx) const   { return (idx < 0 || idx >= 2) ? QByteArray() : m_data.split('\t')[idx+3]; }

private:
    Q_DISABLE_COPY(ChangeLogEntry)

    QByteArray m_data;

    friend class Core;
};

class TextImport
{
public:
    TextImport();

    bool import(const QString &path);
    void exportTo(Core *);

    bool importInventories(QVector<const Item *> &items);
    void exportInventoriesTo(Core *);

    const QHash<int, const Color *>    &colors() const     { return m_colors; }
    const QHash<int, const Category *> &categories() const { return m_categories; }
    const QHash<int, const ItemType *> &itemTypes() const  { return m_item_types; }
    const QVector<const Item *>       &items() const       { return m_items; }

private:
    template <typename T> T *parse(uint count, const char **strs);

    template <typename C> bool readDB(const QString &name, C &container, bool skip_header = false);
    template <typename T> bool readDB_processLine(QHash<int, const T *> &d, uint tokencount, const char **tokens);
    template <typename T> bool readDB_processLine(QVector<const T *> &v, uint tokencount, const char **tokens);

    struct btinvlist_dummy { };
    bool readDB_processLine(btinvlist_dummy &, uint count, const char **strs);
    struct btpriceguide_dummy { };
    bool readDB_processLine(btpriceguide_dummy &, uint count, const char **strs);
    struct btchglog_dummy { };
    bool readDB_processLine(btchglog_dummy &, uint count, const char **strs);

    bool readInventory(const Item *item);

    const Category *findCategoryByName(const QStringRef &name) const;
    const Item *findItem(char type, const QString &id);

    void calculateColorPopularity();

private:
    QHash<int, const Color *>    m_colors;
    QHash<int, const ItemType *> m_item_types;
    QHash<int, const Category *> m_categories;
    QVector<const Item *>       m_items;

    QHash<const Item *, AppearsIn>   m_appears_in_hash;
    QHash<const Item *, InvItemList> m_consists_of_hash;
    QVector<const char *>            m_changelog;

    const ItemType *m_current_item_type;
};


class ColorModel : public StaticPointerModel
{
    Q_OBJECT
public:
    ColorModel(QObject *parent);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orient, int role) const override;

    using StaticPointerModel::index;
    QModelIndex index(const Color *color) const;
    const Color *color(const QModelIndex &index) const;

    bool isFiltered() const override;
    void setFilterItemType(const ItemType *it);
    void setFilterType(Color::Type type);
    void unsetFilterType();
    void setFilterPopularity(qreal p);

protected:
    int pointerCount() const override;
    const void *pointerAt(int index) const override;
    int pointerIndexOf(const void *pointer) const override;

    bool filterAccepts(const void *pointer) const override;
    bool lessThan(const void *pointer1, const void *pointer2, int column) const override;

private:
    const ItemType *m_itemtype_filter;
    Color::Type m_type_filter;
    qreal m_popularity_filter;

    friend class Core;
};


class CategoryModel : public StaticPointerModel
{
    Q_OBJECT
public:
    CategoryModel(QObject *parent);

    static const Category *AllCategories;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orient, int role) const override;

    using StaticPointerModel::index;
    QModelIndex index(const Category *category) const;
    const Category *category(const QModelIndex &index) const;

    bool isFiltered() const override;
    void setFilterItemType(const ItemType *it);
    void setFilterAllCategories(bool);

protected:
    int pointerCount() const override;
    const void *pointerAt(int index) const override;
    int pointerIndexOf(const void *pointer) const override;

    bool filterAccepts(const void *pointer) const override;
    bool lessThan(const void *pointer1, const void *pointer2, int column) const override;

private:
    const ItemType *m_itemtype_filter;
    bool m_all_filter;

    friend class Core;
};


class ItemTypeModel : public StaticPointerModel
{
    Q_OBJECT
public:
    ItemTypeModel(QObject *parent);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orient, int role) const override;

    using StaticPointerModel::index;
    QModelIndex index(const ItemType *itemtype) const;
    const ItemType *itemType(const QModelIndex &index) const;

    bool isFiltered() const override;
    void setFilterWithoutInventory(bool on);

protected:
    int pointerCount() const override;
    const void *pointerAt(int index) const override;
    int pointerIndexOf(const void *pointer) const override;

    bool filterAccepts(const void *pointer) const override;
    bool lessThan(const void *pointer1, const void *pointer2, int column) const override;

private:
    bool m_inv_filter;

    friend class Core;
};


class ItemModel : public StaticPointerModel
{
    Q_OBJECT
public:
    ItemModel(QObject *parent);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orient, int role) const override;

    using StaticPointerModel::index;
    QModelIndex index(const Item *item) const;
    const Item *item(const QModelIndex &index) const;

    bool isFiltered() const override;
    void setFilterItemType(const ItemType *it);
    void setFilterCategory(const Category *cat);
    void setFilterText(const QString &str);
    void setFilterWithoutInventory(bool on);

protected slots:
    void pictureUpdated(BrickLink::Picture *);

protected:
    int pointerCount() const override;
    const void *pointerAt(int index) const override;
    int pointerIndexOf(const void *pointer) const override;

    bool filterAccepts(const void *pointer) const override;
    bool lessThan(const void *pointer1, const void *pointer2, int column) const override;

private:
    const ItemType *m_itemtype_filter;
    const Category *m_category_filter;
    QString         m_text_filter;
    bool            m_inv_filter;

    friend class Core;
};


class AppearsInsModel;

class InternalAppearsInModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ~InternalAppearsInModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orient, int role) const override;

    const AppearsInItem *appearsIn(const QModelIndex &idx) const;
    QModelIndex index(const AppearsInItem *const_ai) const;

protected:
    InternalAppearsInModel(const BrickLink::InvItemList &list, QObject *parent);
    InternalAppearsInModel(const Item *item, const Color *color, QObject *parent);

    void init(const InvItemList &list);

    const Item *        m_item;
    const Color *       m_color;
    AppearsIn     m_appearsin;
    QList<AppearsInItem *> m_items;

    friend class AppearsInModel;
};

class AppearsInModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    AppearsInModel(const BrickLink::InvItemList &list, QObject *parent);
    AppearsInModel(const Item *item, const Color *color, QObject *parent);

    using QSortFilterProxyModel::index;
    const AppearsInItem *appearsIn(const QModelIndex &idx) const;
    QModelIndex index(const AppearsInItem *const_ai) const;

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};


class ItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    enum Option {
        None,
        AlwaysShowSelection,
    };
    Q_DECLARE_FLAGS(Options, Option);

    ItemDelegate(QObject *parent = nullptr, Options options = None);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

public slots:
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option,
                   const QModelIndex &index) override;

private:
    QString createToolTip(const BrickLink::Item *item, BrickLink::Picture *pic) const;

private slots:
    void pictureUpdated(BrickLink::Picture *pic);

private:
    Options m_options;
    BrickLink::Picture *m_tooltip_pic;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ItemDelegate::Options)



class Core : public QObject
{
    Q_OBJECT
public:
    ~Core();

    QUrl url(UrlList u, const void *opt = nullptr, const void *opt2 = nullptr);

    enum DatabaseVersion {
        BrickStore_1_1,
        BrickStore_2_0,

        Default = BrickStore_2_0
    };

    QString defaultDatabaseName(DatabaseVersion version = Default) const;

    QString dataPath() const;
    QString dataPath(const ItemType *) const;
    QString dataPath(const Item *) const;
    QString dataPath(const Item *, const Color *) const;

    const QHash<int, const Color *>    &colors() const;
    const QHash<int, const Category *> &categories() const;
    const QHash<int, const ItemType *> &itemTypes() const;
    const QVector<const Item *>        &items() const;

    const QImage noImage(const QSize &s) const;

    const QImage colorImage(const Color *col, int w, int h) const;

    const Color *color(uint id) const;
    const Color *colorFromName(const QString &name) const;
    const Color *colorFromLDrawId(int ldraw_id) const;
    const Category *category(uint id) const;
    const Category *categoryFromName(const char *name, int len = -1) const;
    const ItemType *itemType(char id) const;
    const Item *item(char tid, const QString &id) const;

    PriceGuide *priceGuide(const Item *item, const Color *color, bool high_priority = false);
    void flushPriceGuidesToUpdate(); //TODO5: brickstock has this

    QSize pictureSize(const ItemType *itt) const;
    Picture *picture(const Item *item, const Color *color, bool high_priority = false);
    Picture *largePicture(const Item *item, bool high_priority = false);

    struct ParseItemListXMLResult {
        ParseItemListXMLResult()
            : items(0), invalidItemCount(0)
        { }

        InvItemList *items;
        uint invalidItemCount;
        QString currencyCode;
    };

    ParseItemListXMLResult parseItemListXML(const QDomElement &root, ItemListXMLHint hint);
    QDomElement createItemListXML(QDomDocument doc, ItemListXMLHint hint, const InvItemList &items, const QString &currencyCode = QString(), QMap<QString, QString> *extra = nullptr);

    bool parseLDrawModel(QFile &file, InvItemList &items, uint *invalid_items = nullptr);

    int applyChangeLogToItems(BrickLink::InvItemList &bllist);

    bool onlineStatus() const;

    Transfer *transfer() const;
    void setTransfer(Transfer *trans);

    bool readDatabase(QString *infoText = nullptr, const QString &filename = QString());
    bool writeDatabase(const QString &filename, BrickLink::Core::DatabaseVersion version,
                       const QString &infoText = QString()) const;

public slots:
    void updatePriceGuide(BrickLink::PriceGuide *pg, bool high_priority = false);
    void updatePicture(BrickLink::Picture *pic, bool high_priority = false);

    void setOnlineStatus(bool on);
    void setUpdateIntervals(const QMap<QByteArray, int> &intervals);

    void cancelPictureTransfers();
    void cancelPriceGuideTransfers();

signals:
    void priceGuideUpdated(BrickLink::PriceGuide *pg);
    void pictureUpdated(BrickLink::Picture *inv);

    void transferJobProgress(int, int);

private:
    Core(const QString &datadir);

    static Core *create(const QString &datadir, QString *errstring);
    static inline Core *inst() { return s_inst; }
    static Core *s_inst;

    friend Core *core();
    friend Core *create(const QString &, QString *);

private:
    bool updateNeeded(bool valid, const QDateTime &last, int iv);
    bool parseLDrawModelInternal(QFile &file, const QString &model_name, InvItemList &items, uint *invalid_items, QHash<QString, InvItem *> &mergehash, QStringList &recursion_detection);

    void setDatabase_ConsistsOf(const QHash<const Item *, InvItemList> &hash);
    void setDatabase_AppearsIn(const QHash<const Item *, AppearsIn> &hash);
    void setDatabase_Basics(const QHash<int, const Color *> &colors,
                            const QHash<int, const Category *> &categories,
                            const QHash<int, const ItemType *> &item_types,
                            const QVector<const Item *> &items);
    void setDatabase_ChangeLog(const QVector<const char *> &changelog);

    friend class TextImport;

private slots:
    void pictureJobFinished(ThreadPoolJob *); //TODO5: timeout handling in brickstock updatePicturesTimeOut
    void priceGuideJobFinished(ThreadPoolJob *);

    void pictureLoaded(ThreadPoolJob *);

private:
    QString  m_datadir;
    bool     m_online;
    QLocale  m_c_locale;
    mutable QMutex m_corelock;

    mutable QHash<QString, QImage>  m_noimages;
    mutable QHash<QString, QImage>  m_colimages;

    QHash<int, const Color *>       m_colors;      // id ->Color *
    QHash<int, const Category *>    m_categories;  // id ->Category *
    QHash<int, const ItemType *>    m_item_types;  // id ->ItemType *
    QVector<const Item *>           m_items;       // sorted array of Item *
    QVector<const char *>           m_changelog;

    QPointer<Transfer>  m_transfer;

    //Transfer                   m_pg_transfer;
    int                          m_pg_update_iv;
    Q3Cache<quint64, PriceGuide> m_pg_cache;

    //Transfer                   m_pic_transfer;
    int                          m_pic_update_iv;
    ThreadPool                   m_pic_diskload;
    Q3Cache<quint64, Picture>    m_pic_cache;
};

inline Core *core() { return Core::inst(); }

inline Core *create(const QString &datadir, QString *errstring) { return Core::create(datadir, errstring); }

} // namespace BrickLink

Q_DECLARE_METATYPE(const BrickLink::Color *)
Q_DECLARE_METATYPE(const BrickLink::Category *)
Q_DECLARE_METATYPE(const BrickLink::ItemType *)
Q_DECLARE_METATYPE(const BrickLink::Item *)
Q_DECLARE_METATYPE(const BrickLink::AppearsInItem *)


// tell Qt that Pictures and PriceGuides are shared and can't simply be deleted
// (QCache will use that function to determine what can really be purged from the cache)

template<> inline bool q3IsDetached<BrickLink::Picture>(BrickLink::Picture &c) { return c.refCount() == 0; }
template<> inline bool q3IsDetached<BrickLink::PriceGuide>(BrickLink::PriceGuide &c) { return c.refCount() == 0; }