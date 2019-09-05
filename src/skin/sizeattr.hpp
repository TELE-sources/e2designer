#pragma once

#include <QSize>
#include <QString>
#include <QMetaType>

class Dimension
{
public:
    Dimension();
    Dimension(int px);
    Dimension(const QString& str) { parseStr(str); }
    enum class Type
    {
        Fill,    // TODO: implement this
        Percent, // TODO: implement this
        Number,
    };

    inline Type type() const { return m_type; }
    inline int value() const { return m_value; }

    /// Load and save text
    void parseStr(const QString& str);
    QString toStr() const;

    /// Convert to numerical value
    int getInt(int parent_size) const;
    /// Force set numerical value
    void setInt(int value);
    /// Try to convert back to percent if needed
    void parseInt(int value, int parent_size);

    /// Depends on parent or on size
    bool isRelative() const;

    inline bool operator==(const Dimension& other)
    {
        return m_type == other.m_type && m_value == other.m_value;
    }

private:
    Type m_type;
    int m_value;
};

class WidgetData;

class SizeAttr
{
public:
    SizeAttr(Dimension w = Dimension(), Dimension h = Dimension());
    inline const Dimension& w() const { return m_width; }
    inline const Dimension& h() const { return m_height; }

    SizeAttr(const QString& str) { fromStr(str); }
    bool isRelative() const { return m_width.isRelative() || m_height.isRelative(); }
    QSize getSize(const WidgetData& widget) const;
    void setSize(const WidgetData& widget, const QSize size);

    QString toStr() const;
    void fromStr(const QString& str);

    inline bool operator==(const SizeAttr& other)
    {
        return m_width == other.m_width && m_height == other.m_height;
    }

private:
    Dimension m_width, m_height;
};
Q_DECLARE_METATYPE(SizeAttr);

inline QString serialize(const SizeAttr& size)
{
    return size.toStr();
}
inline void deserialize(const QString& str, SizeAttr& value)
{
    value = SizeAttr(str);
}
