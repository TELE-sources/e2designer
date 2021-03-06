#pragma once

#include "widgetdata.hpp"

class IncludeFile : public WidgetData
{
public:
    static constexpr char tag[] = "include";

    IncludeFile();

    bool fromXml(QXmlStreamReader& xml) override;
    void toXml(XmlStreamWriter& xml) const override;

private:
    QString m_fileName;
    void read(const QString& fileName);
};
