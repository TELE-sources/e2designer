#include "coloritem.hpp"
#include "adapter/attritemfactory.hpp"
#include "repository/skinrepository.hpp"
#include <QString>

// ColorAttr

ColorAttr::ColorAttr()
    : mName()
    , mValue(0)
    , mDefined(false)
{
}
ColorAttr::ColorAttr(const QColor& color)
    : ColorAttr()
{
    setColor(color);
}
ColorAttr::ColorAttr(const QString& str, bool invertAlpha)
    : ColorAttr()
{
    fromStr(str, invertAlpha);
}

QColor ColorAttr::getColor() const
{
    if (mDefined) {
        return QColor::fromRgba(mValue);
    } else {
        return QColor();
    }
}
void ColorAttr::setColor(const QColor& color)
{
    // FIXME!
    //    mName = SkinRepository::colors()->getName(color.rgba());
    mName = QString();
    mValue = color.rgba();
    mDefined = true;
}

void ColorAttr::setColorName(const QString& name)
{
    mName = name;
    mDefined = !name.isEmpty();
    reload();
}

QString ColorAttr::toStr(bool invertAlpha) const
{
    if (!mDefined) {
        return QString();
    }

    if (mName.isEmpty()) {
        QColor color = getColor();
        if (invertAlpha) {
            // enigma2 representation is inverted
            // 255=transparent, 0=opaque
            color.setAlpha(255 - color.alpha());
        }
        return color.name(QColor::HexArgb);
    } else {
        return mName;
    }
}

bool ColorAttr::fromStr(const QString& str, bool invertAlpha)
{
    if (str.length() > 0 && str[0] == '#') {
        mName = QString();
        QColor color(str);
        if (color.isValid()) {
            if (invertAlpha) {
                // enigma2 representation is inverted
                // 255=transparent, 0=opaque
                color.setAlpha(255 - color.alpha());
            }
            mValue = color.rgba();
            mDefined = true;
        } else {
            mValue = qRgb(0, 0, 0);
            mDefined = false;
        }
    } else {
        setColorName(str);
    }
    return mDefined;
}

void ColorAttr::reload()
{
    if (!mName.isEmpty()) {
        mValue = SkinRepository::colors()->getValue(mName).value();
    }
}

// ColorItem

QVariant ColorItem::data(int role) const
{
    switch (role) {
    case Roles::DataRole:
        return QVariant::fromValue(attr());
    case Qt::DecorationRole:
    case Roles::GraphicsRole:
    case Qt::EditRole:
        return attr().getColor();
    case Qt::DisplayRole:
        return attr().toStr(false);
    case Roles::XmlRole:
        return attr().toStr(true);
    default:
        return AttrItem::data(role);
    }
}

bool ColorItem::setData(const QVariant& value, int role)
{
    switch (role) {
    case Roles::DataRole:
        setAttr(value.value<ColorAttr>());
        return true;
    case Roles::GraphicsRole:
    case Qt::EditRole:
        setAttr(ColorAttr(value.value<QColor>()));
        return true;
    case Roles::XmlRole:
        setAttr(ColorAttr(value.toString(), true));
        return true;
    default:
        return AttrItem::setData(value, role);
    }
}

ColorAttr ColorItem::attr() const
{
    return pWidget->getAttr<ColorAttr>(key());
}
void ColorItem::setAttr(const ColorAttr& attr)
{
    pWidget->setAttr(key(), attr);
}
static AttrItemRegistrator<ColorAttr, ColorItem> registrator;
