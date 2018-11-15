#ifndef WINDOWSTYLE_H
#define WINDOWSTYLE_H

#include "skin/positionattr.hpp"
#include "skin/fontattr.hpp"
#include "skin/colorattr.hpp"
#include "borderset.hpp"
#include "repository/xmlnode.hpp"
#include "model/namedlist.hpp"

class WindowStyleTitle
{
public:
    void fromXml(QXmlStreamReader& xml);
    void toXml(QXmlStreamWriter& xml) const;

    PositionAttr position;
    FontAttr font;
};

class WindowStyleColor
{
public:
    void fromXml(QXmlStreamReader& xml);
    void toXml(QXmlStreamWriter& xml) const;

    QString name;
    ColorAttr color;
};

class WindowStyle : public XmlData
{
    Q_GADGET
public:
    WindowStyle();

    QString name() const { return QString::number(m_id); }
    using Value = WindowStyle;

    void fromXml(QXmlStreamReader& xml);
    void toXml(QXmlStreamWriter& xml) const;

    enum ColorRole  {
        Background,
        LabelForeground,
        ListboxBackground,
        ListboxForeground,
        ListboxSelectedBackground,
        ListboxSelectedForeground,
        ListboxMarkedBackground,
        ListboxMarkedForeground,
        ListboxMarkedAndSelectedBackground,
        ListboxMarkedAndSelectedForeground,
        WindowTitleForeground,
        WindowTitleBackground,
    };
    Q_ENUM(ColorRole)

    ColorAttr getColorAttr(ColorRole role) const;

private:
    QString m_type;
    int m_id;
    WindowStyleTitle m_title;
    QVector<WindowStyleColor> m_colors;
    BorderSet m_borderSet;
};

class WindowStylesList : public QObject, public NamedList<WindowStyle>
{
    Q_OBJECT
public:
    void append(WindowStyle style);
    // Xml:
    void toXml(QXmlStreamWriter& xml) const;
    inline const WindowStyle getStyle(int id) {
        return getValue(QString::number(id));
    }
    void clear() { removeItems(0, itemsCount()); }
signals:
    void styleChanged(const QString &name, const WindowStyle &value) const;
protected:
    void emitValueChanged(const QString &name, const WindowStyle &value) const final;
};

#endif // WINDOWSTYLE_H
