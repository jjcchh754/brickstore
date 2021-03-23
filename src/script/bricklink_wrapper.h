/* Copyright (C) 2004-2021 Robert Griebl. All rights reserved.
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

#include <QQmlParserStatus>

#include "bricklink.h"
#include "document.h"
#include "lot.h"

QT_FORWARD_DECLARE_CLASS(QHeaderView)
class Window;

class QmlColor;
class QmlCategory;
class QmlItemType;
class QmlItem;
class QmlLot;
class QmlPriceGuide;
class QmlPicture;
class QmlDocument;


template <typename T> class QmlWrapperBase
{
public:
    inline T *wrappedObject() const
    {
        return (wrapped == wrappedNull()) ? nullptr : wrapped;
    }

protected:
    QmlWrapperBase(T *_wrappedObject)
        : wrapped(_wrappedObject ? _wrappedObject : wrappedNull())
    { }

    static T *wrappedNull()
    {
        static T t_null(nullptr);
        return &t_null;
    }

    bool isNull() const
    {
        return !wrappedObject();
    }

    T *wrapped;
};


class QmlBrickLink : public QObject
{
    Q_OBJECT
    Q_PRIVATE_PROPERTY(d, QString cachePath READ dataPath CONSTANT)
    Q_PRIVATE_PROPERTY(d, QSize standardPictureSize READ standardPictureSize CONSTANT)
    Q_PRIVATE_PROPERTY(d, bool ldrawEnabled READ isLDrawEnabled CONSTANT)
    Q_PRIVATE_PROPERTY(d, QString ldrawPath READ ldrawDataPath)
    Q_PRIVATE_PROPERTY(d, bool online READ onlineStatus)

    // const QImage noImage(const QSize &s) const;
    // const QImage colorImage(const Color *col, int w, int h) const;

    Q_PROPERTY(QmlItem noItem READ noItem CONSTANT)
    Q_PROPERTY(QmlColor noColor READ noColor CONSTANT)

public:
    QmlBrickLink(BrickLink::Core *core);

    // copied from namespace BrickLink
    enum class Time          { PastSix, Current };
    enum class Price         { Lowest, Average, WAverage, Highest };
    enum class Condition     { New, Used };
    enum class SubCondition  { None, Complete, Incomplete, Sealed };
    enum class Stockroom     { None, A, B, C };
    enum class Status        { Include, Exclude, Extra };
    enum class UpdateStatus  { Ok, Loading, Updating, UpdateFailed };

    enum class OrderType     { Received, Placed, Any };
    enum class OrderStatus   { Unknown, Pending, Updated, Processing, Ready, Paid, Packed, Shipped,
                               Received, Completed, OCR, NPB, NPX, NRS, NSS, Cancelled, Count };

    Q_ENUM(Time)
    Q_ENUM(Price)
    Q_ENUM(Condition)
    Q_ENUM(SubCondition)
    Q_ENUM(Stockroom)
    Q_ENUM(Status)
    Q_ENUM(UpdateStatus)
    Q_ENUM(OrderType)
    Q_ENUM(OrderStatus)

    QmlItem noItem() const;
    QmlColor noColor() const;

    Q_INVOKABLE QmlColor color(int id) const;
    Q_INVOKABLE QmlColor colorFromName(const QString &name) const;
    Q_INVOKABLE QmlColor colorFromLDrawId(int ldrawId) const;
    Q_INVOKABLE QmlCategory category(int id) const;
    Q_INVOKABLE QmlItemType itemType(const QString &itemTypeId) const;
    Q_INVOKABLE QmlItem item(const QString &itemTypeId, const QString &itemId) const;

    Q_INVOKABLE QmlPriceGuide priceGuide(QmlItem item, QmlColor color, bool highPriority = false);

    Q_INVOKABLE QmlPicture picture(QmlItem item, QmlColor color, bool highPriority = false);
    Q_INVOKABLE QmlPicture largePicture(QmlItem item, bool highPriority = false);

signals:
    void priceGuideUpdated(QmlPriceGuide pg);
    void pictureUpdated(QmlPicture inv);

private:
    static char firstCharInString(const QString &str);

    BrickLink::Core *d;
};


class QmlCategory : public QmlWrapperBase<const BrickLink::Category>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PRIVATE_PROPERTY(wrapped, int id READ id CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString name READ name CONSTANT)

public:
    QmlCategory(const BrickLink::Category *cat = nullptr);

    friend class QmlBrickLink;
};


class QmlItemType : public QmlWrapperBase<const BrickLink::ItemType>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PRIVATE_PROPERTY(wrapped, int id READ id CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString name READ name CONSTANT)
    Q_PROPERTY(QVariantList categories READ categories CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool hasInventories READ hasInventories CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool hasColors READ hasColors CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool hasYearReleased READ hasYearReleased CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool hasWeight READ hasWeight CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool hasSubConditions READ hasSubConditions CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, char pictureId READ pictureId CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QSize pictureSize READ pictureSize CONSTANT)

public:
    QmlItemType(const BrickLink::ItemType *itt = nullptr);

    QVariantList categories() const;

    friend class QmlBrickLink;
};


class QmlColor : public QmlWrapperBase<const BrickLink::Color>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PRIVATE_PROPERTY(wrapped, int id READ id CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString name READ name CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QColor color READ color CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, int ldrawId READ ldrawId CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool solid READ isSolid CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool transparent READ isTransparent CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool glitter READ isGlitter CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool speckle READ isSpeckle CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool metallic READ isMetallic CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool chrome READ isChrome CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool milky READ isMilky CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool modulex READ isModulex CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double popularity READ popularity CONSTANT)

    Q_PROPERTY(QImage image READ image)

public:
    QmlColor(const BrickLink::Color *col = nullptr);

    QImage image() const;

    friend class QmlBrickLink;
    friend class QmlDocument;
    friend class QmlLot;
};


class QmlItem : public QmlWrapperBase<const BrickLink::Item>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString name READ name CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QmlItemType itemType READ itemType CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QmlCategory category READ category CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool hasInventory READ hasInventory CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QDateTime inventoryUpdated READ inventoryUpdated CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QmlColor defaultColor READ defaultColor CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double weight READ weight CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, int yearReleased READ yearReleased CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool hasKnownColors READ hasKnownColors CONSTANT)
    Q_PROPERTY(QVariantList knownColors READ knownColors CONSTANT)

public:
    QmlItem(const BrickLink::Item *item = nullptr);

    QString id() const;
    QVariantList knownColors() const;

    Q_INVOKABLE QVariantList consistsOf() const;

    // tough .. BrickLink::AppearsIn appearsIn(const Color *color = nullptr) const;

    friend class QmlBrickLink;
    friend class QmlDocument;
};


class QmlPicture : public QmlWrapperBase<BrickLink::Picture>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PRIVATE_PROPERTY(wrapped, QmlItem item READ item CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QmlColor color READ color CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QDateTime lastUpdate READ lastUpdate)
    Q_PROPERTY(QmlBrickLink::UpdateStatus updateStatus READ updateStatus)
    Q_PRIVATE_PROPERTY(wrapped, bool isValid READ isValid)
    Q_PRIVATE_PROPERTY(wrapped, QImage image READ image)

public:
    explicit QmlPicture(BrickLink::Picture *pic = nullptr);
    QmlPicture(const QmlPicture &copy);
    QmlPicture &operator=(const QmlPicture &assign);
    virtual ~QmlPicture();

    QmlBrickLink::UpdateStatus updateStatus() const;

    Q_INVOKABLE void update(bool highPriority = false);

    friend class QmlBrickLink;
};


class QmlPriceGuide : public QmlWrapperBase<BrickLink::PriceGuide>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PRIVATE_PROPERTY(wrapped, QmlItem item READ item CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QmlColor color READ color CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QDateTime lastUpdate READ lastUpdate)
    Q_PROPERTY(QmlBrickLink::UpdateStatus updateStatus READ updateStatus)
    Q_PRIVATE_PROPERTY(wrapped, bool isValid READ isValid)

public:
    explicit QmlPriceGuide(BrickLink::PriceGuide *pg = nullptr);
    QmlPriceGuide(const QmlPriceGuide &copy);
    QmlPriceGuide &operator=(const QmlPriceGuide &assign);
    virtual ~QmlPriceGuide();

    QmlBrickLink::UpdateStatus updateStatus() const;

    Q_INVOKABLE void update(bool highPriority = false);

    Q_INVOKABLE int quantity(QmlBrickLink::Time time, QmlBrickLink::Condition condition) const;
    Q_INVOKABLE int lots(QmlBrickLink::Time time, QmlBrickLink::Condition condition) const;
    Q_INVOKABLE double price(QmlBrickLink::Time time, QmlBrickLink::Condition condition,
                             QmlBrickLink::Price price) const;

    friend class QmlBrickLink;
};


class QmlLot : QmlWrapperBase<Lot>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PROPERTY(QmlItem item READ item WRITE setItem)
    Q_PROPERTY(QmlColor color READ color WRITE setColor)
    Q_PROPERTY(QmlCategory category READ category)
    Q_PROPERTY(QmlItemType itemType READ itemType)
    Q_PROPERTY(QString itemId READ itemId)
    Q_PROPERTY(QString id READ itemId)
    Q_PROPERTY(QString itemName READ itemName)
    Q_PROPERTY(QString name READ itemName)
    Q_PROPERTY(QString colorName READ colorName)
    Q_PROPERTY(QString categoryName READ categoryName)
    Q_PROPERTY(QString itemTypeName READ itemTypeName)
    Q_PROPERTY(int itemYearReleased READ itemYearReleased)

    Q_PROPERTY(QmlBrickLink::Status status READ status WRITE setStatus)
    Q_PROPERTY(QmlBrickLink::Condition condition READ condition WRITE setCondition)
    Q_PROPERTY(QmlBrickLink::SubCondition subCondition READ subCondition WRITE setSubCondition)

    Q_PROPERTY(QString comments READ comments WRITE setComments)
    Q_PROPERTY(QString remarks READ remarks WRITE setRemarks)

    Q_PROPERTY(int quantity READ quantity WRITE setQuantity)
    Q_PROPERTY(int bulkQuantity READ bulkQuantity WRITE setBulkQuantity)
    Q_PROPERTY(int tier1Quantity READ tier1Quantity WRITE setTier1Quantity)
    Q_PROPERTY(int tier2Quantity READ tier2Quantity WRITE setTier2Quantity)
    Q_PROPERTY(int tier3Quantity READ tier3Quantity WRITE setTier3Quantity)

    Q_PROPERTY(double price READ price WRITE setPrice)
    Q_PROPERTY(double tier1Price READ tier1Price WRITE setTier1Price)
    Q_PROPERTY(double tier2Price READ tier2Price WRITE setTier2Price)
    Q_PROPERTY(double tier3Price READ tier3Price WRITE setTier3Price)

    Q_PROPERTY(int sale READ sale WRITE setSale)
    Q_PROPERTY(double total READ total)

    Q_PROPERTY(uint lotId READ lotId WRITE setLotId)
    Q_PROPERTY(bool retain READ retain WRITE setRetain)
    Q_PROPERTY(QmlBrickLink::Stockroom stockroom READ stockroom WRITE setStockroom)

    Q_PROPERTY(double totalWeight READ totalWeight WRITE setTotalWeight)
    Q_PROPERTY(QString reserved READ reserved WRITE setReserved)
    Q_PROPERTY(bool alternate READ alternate WRITE setAlternate)
    Q_PROPERTY(uint alternateId READ alternateId WRITE setAlternateId)
    Q_PROPERTY(bool counterPart READ counterPart WRITE setCounterPart)

    Q_PROPERTY(bool incomplete READ incomplete)

    Q_PROPERTY(QImage image READ image)

public:
    QmlLot(Lot *lot = nullptr, QmlDocument *document = nullptr);

    QmlItem item() const               { return get()->item(); }
    void setItem(QmlItem item)         { set().to()->setItem(item.wrappedObject()); }
    QmlColor color() const             { return get()->color(); }
    void setColor(QmlColor color)      { set().to()->setColor(color.wrappedObject()); }
    QmlCategory category() const       { return get()->category(); }
    QmlItemType itemType() const       { return get()->itemType(); }

    QString itemId() const             { return QString::fromLatin1(get()->itemId()); }
    QString itemName() const           { return get()->itemName(); }
    QString colorName() const          { return get()->colorName(); }
    QString categoryName() const       { return get()->categoryName(); }
    QString itemTypeName() const       { return get()->itemTypeName(); }
    int itemYearReleased() const       { return get()->itemYearReleased(); }

    QmlBrickLink::Status status() const                { return static_cast<QmlBrickLink::Status>(get()->status()); }
    void setStatus(QmlBrickLink::Status s)             { set().to()->setStatus(static_cast<BrickLink::Status>(s)); }
    QmlBrickLink::Condition condition() const          { return static_cast<QmlBrickLink::Condition>(get()->condition()); }
    void setCondition(QmlBrickLink::Condition c)       { set().to()->setCondition(static_cast<BrickLink::Condition>(c)); }
    QmlBrickLink::SubCondition subCondition() const    { return static_cast<QmlBrickLink::SubCondition>(get()->subCondition()); }
    void setSubCondition(QmlBrickLink::SubCondition c) { set().to()->setSubCondition(static_cast<BrickLink::SubCondition>(c)); }
    QString comments() const           { return get()->comments(); }
    void setComments(const QString &n) { set().to()->setComments(n); }
    QString remarks() const            { return get()->remarks(); }
    void setRemarks(const QString &r)  { set().to()->setComments(r); }

    int quantity() const               { return get()->quantity(); }
    void setQuantity(int q)            { set().to()->setQuantity(q); }
    int bulkQuantity() const           { return get()->bulkQuantity(); }
    void setBulkQuantity(int q)        { set().to()->setBulkQuantity(q); }
    int tier1Quantity() const          { return get()->tierQuantity(0); }
    void setTier1Quantity(int q)       { set().to()->setTierQuantity(0, q); }
    int tier2Quantity() const          { return get()->tierQuantity(1); }
    void setTier2Quantity(int q)       { set().to()->setTierQuantity(1, q); }
    int tier3Quantity() const          { return get()->tierQuantity(2); }
    void setTier3Quantity(int q)       { set().to()->setTierQuantity(2, q); }

    double price() const               { return get()->price(); }
    void setPrice(double p)            { set().to()->setPrice(p); }
    double tier1Price() const          { return get()->tierPrice(0); }
    void setTier1Price(double p)       { set().to()->setTierPrice(0, p); }
    double tier2Price() const          { return get()->tierPrice(1); }
    void setTier2Price(double p)       { set().to()->setTierPrice(1, p); }
    double tier3Price() const          { return get()->tierPrice(2); }
    void setTier3Price(double p)       { set().to()->setTierPrice(2, p); }

    int sale() const                   { return get()->sale(); }
    void setSale(int s)                { set().to()->setSale(s); }
    double total() const               { return get()->total(); }

    uint lotId() const                 { return get()->lotId(); }
    void setLotId(uint lid)            { set().to()->setLotId(lid); }

    bool retain() const                { return get()->retain(); }
    void setRetain(bool r)             { set().to()->setRetain(r); }
    QmlBrickLink::Stockroom stockroom() const     { return static_cast<QmlBrickLink::Stockroom>(get()->stockroom()); }
    void setStockroom(QmlBrickLink::Stockroom sr) { set().to()->setStockroom(static_cast<BrickLink::Stockroom>(sr)); }

    double totalWeight() const         { return get()->totalWeight(); }
    void setTotalWeight(double w)      { set().to()->setTotalWeight(w); }

    QString reserved() const           { return get()->reserved(); }
    void setReserved(const QString &r) { set().to()->setReserved(r); }

    bool alternate() const             { return get()->alternate(); }
    void setAlternate(bool a)          { set().to()->setAlternate(a); }
    uint alternateId() const           { return get()->alternateId(); }
    void setAlternateId(uint aid)      { set().to()->setAlternateId(aid); }

    bool counterPart() const           { return get()->counterPart(); }
    void setCounterPart(bool b)        { set().to()->setCounterPart(b); }

    bool incomplete() const            { return get()->isIncomplete(); }

    QImage image() const;

private:
    class Setter
    {
    public:
        Setter(QmlLot *lot);
        Lot *to();
        ~Setter();

    private:
        QmlLot *m_lot;
        Lot m_to;
    };
    Setter set();
    Lot *get() const;

    QmlDocument *doc = nullptr;

    friend class QmlDocument;
    friend class Setter;
};


class QmlOrder : public QmlWrapperBase<const BrickLink::Order>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PRIVATE_PROPERTY(wrapped, QString id READ id CONSTANT)
    Q_PROPERTY(QmlBrickLink::OrderType type READ type CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QDate date READ date CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QDate statusChange READ statusChange CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString otherParty READ otherParty CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double shipping READ shipping CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double insurance READ insurance CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double additionalCharges1 READ additionalCharges1 CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double additionalCharges2 READ additionalCharges2 CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double credit READ credit CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double creditCoupon READ creditCoupon CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double orderTotal READ orderTotal CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double salesTax READ salesTax CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double grandTotal READ grandTotal CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double vatCharges READ vatCharges CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString currencyCode READ currencyCode CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString paymentCurrencyCode READ paymentCurrencyCode CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, int itemCount READ itemCount CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, int lotCount READ lotCount CONSTANT)
    Q_PROPERTY(QmlBrickLink::OrderStatus status READ status CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString paymentType READ paymentType CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString trackingNumber READ trackingNumber CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString address READ address CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString countryCode READ countryCode CONSTANT)

public:
    QmlOrder(const BrickLink::Order *order = nullptr);

    QmlBrickLink::OrderType type() const;
    QmlBrickLink::OrderStatus status() const;

    friend class QmlBrickLink;
    friend class QmlDocument;
};

class QmlDocument : public QObject
{
    Q_OBJECT
    Q_PRIVATE_PROPERTY(d, QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PRIVATE_PROPERTY(d, QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PRIVATE_PROPERTY(d, QString currencyCode READ currencyCode NOTIFY currencyCodeChanged)
    Q_PRIVATE_PROPERTY(d, QString filter READ filter NOTIFY filterChanged)
    Q_PRIVATE_PROPERTY(d, QmlOrder order READ order CONSTANT)
    Q_PRIVATE_PROPERTY(d, int lotCount READ lotCount NOTIFY lotCountChanged)
    //TODO: missing: statistics

public:
    Q_ENUMS(Document::Field)

    QmlDocument(Window *win);
    bool isWrapperFor(Window *win) const;

    bool changeLot(QmlLot *from, Lot &to);

    Q_INVOKABLE QmlLot lot(int index);
    Q_INVOKABLE void deleteLot(QmlLot ii);
    Q_INVOKABLE QmlLot addLot(QmlItem item, QmlColor color);

//    Q_INVOKABLE Lot addItem(Lot lot, Flags consolidate)
//    {
//        if (m_lot.doc != this) {
//            ...
//        }
//    }

    inline Document *document() { return d; }
    inline Window *window() { return win; }

signals:
    void titleChanged(const QString &title);
    void fileNameChanged(const QString &fileName);
    void lotCountChanged(int count);
    void currencyCodeChanged(const QString &currencyCode);
    void filterChanged(const QString &filter);

private:
    Document *d;
    Window *win;
};


class QmlBrickStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVector<QmlDocument *> documents READ documents NOTIFY documentsChanged)
    Q_PROPERTY(QmlDocument *currentDocument READ currentDocument NOTIFY currentDocumentChanged)
    Q_PROPERTY(QString defaultCurrencyCode READ defaultCurrencyCode NOTIFY defaultCurrencyCodeChanged)

public:
    QmlBrickStore();

    QVector<QmlDocument *> documents() const;
    QmlDocument *currentDocument() const;

    QmlDocument *documentForWindow(Window *win) const;

    Q_INVOKABLE QmlDocument *newDocument(const QString &title);
    Q_INVOKABLE QmlDocument *openDocument(const QString &fileName);
    Q_INVOKABLE QmlDocument *importBrickLinkStore(const QString &title = { });

    QString defaultCurrencyCode() const;
    Q_INVOKABLE QString symbolForCurrencyCode(const QString &currencyCode) const;
    Q_INVOKABLE QString toCurrencyString(double value, const QString &symbol = { }, int precision = 3) const;
    Q_INVOKABLE QString toWeightString(double value, bool showUnit = false) const;

signals:
    void documentsChanged(QVector<QmlDocument *> documents);
    void currentDocumentChanged(QmlDocument *currentDocument);
    void defaultCurrencyCodeChanged(const QString &defaultCurrencyCode);

private:
    QmlDocument *setupDocument(Window *win, Document *doc, const QString &title = { });

    QVector<QmlDocument *> m_documents;
    QmlDocument *m_currentDocument = nullptr;
};

Q_DECLARE_METATYPE(QmlColor)
Q_DECLARE_METATYPE(QmlCategory)
Q_DECLARE_METATYPE(QmlItemType)
Q_DECLARE_METATYPE(QmlItem)
Q_DECLARE_METATYPE(QmlLot)
Q_DECLARE_METATYPE(QmlPicture)
Q_DECLARE_METATYPE(QmlPriceGuide)
Q_DECLARE_METATYPE(QmlOrder)
Q_DECLARE_METATYPE(QmlBrickLink::Time)
Q_DECLARE_METATYPE(QmlBrickLink::Price)
Q_DECLARE_METATYPE(QmlBrickLink::Condition)
Q_DECLARE_METATYPE(QmlBrickLink::Stockroom)
Q_DECLARE_METATYPE(QmlBrickLink::Status)
Q_DECLARE_METATYPE(QmlBrickLink::UpdateStatus)
Q_DECLARE_METATYPE(QmlBrickLink::OrderType)
Q_DECLARE_METATYPE(QmlBrickLink *)
Q_DECLARE_METATYPE(QmlDocument *)
