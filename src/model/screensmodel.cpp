#include "screensmodel.hpp"
#include "commands/attrcommand.hpp"
#include "repository/skinrepository.hpp"
#include <QUndoStack>

// ScreensTree

void ScreensTree::loadPreviews(const QString &path)
{
    mPreviews.clear();

    QFile file(path);
    bool ok = file.open(QIODevice::ReadOnly);
    if (!ok)
        return;

    QXmlStreamReader xml(&file);

    while (xml.readNextStartElement()) {
        if (xml.name() == "screens")
            break;
        xml.skipCurrentElement();
    }
    // we are inside screens
    while (xml.readNextStartElement()) {
        if (xml.name() != "screen") {
            xml.skipCurrentElement();
            continue;
        }

        // we are inside screen
        QString screenName;
        QMap<QString, Preview> map;

        while (xml.readNextStartElement()) {
            if (xml.name() == "name") {
                screenName = xml.readElementText();

            } else if (xml.name() == "entries") {
                for (; xml.readNextStartElement(); xml.skipCurrentElement()) {
                    if (xml.name() != "entry")
                        continue;

                    // we are inside entry
                    const QString widgetName = xml.attributes().value("name").toString();
                    const QString value = xml.attributes().value("value").toString();
                    QString str = xml.attributes().value("render").toString();
                    auto render = EnumAttr<Property::Render>(str).value();
                    if (!widgetName.isEmpty()) {
                        map.insert(widgetName, Preview(value, render));
                    }
                }
            } else {
                xml.skipCurrentElement();
            }
        }
        if (!screenName.isEmpty()) {
            mPreviews.insert(screenName, map);
        }
    }
}

void ScreensTree::savePreviews(const QString &path)
{
    QFile file(path);
    bool ok = file.open(QIODevice::WriteOnly);
    if (!ok)
        return;
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);

    xml.writeStartDocument();
    xml.writeStartElement("screens");
    // iterate over screens
    for (auto s = mPreviews.begin(); s != mPreviews.end(); ++s) {
        if (s.value().empty())
            continue;
        xml.writeStartElement("screen");
        xml.writeTextElement("name", s.key());
        xml.writeStartElement("entries");
        // iterate over widgets
        for (auto w = s.value().begin(); w != s.value().end(); ++w) {
            auto valueStr = w.value().value.toString();
            auto renderStr = EnumAttr<Property::Render>(w.value().render).toStr();
            if (valueStr.isEmpty())
                continue;
            xml.writeStartElement("entry");
            xml.writeAttribute("name", w.key());
            xml.writeAttribute("value", valueStr);
            xml.writeAttribute("render", renderStr);
            xml.writeAttribute("type", "string");  // compatibility
            xml.writeEndElement();
        }
        xml.writeEndElement();
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndDocument();
}

Preview ScreensTree::getPreview(const QString& screen, const QString& widget) const
{
    auto screen_it = mPreviews.find(screen);
    if (screen_it == mPreviews.end())
        return Preview();
    auto widget_it = screen_it.value().find(widget);
    if (widget_it == screen_it.value().end())
        return Preview();
    return widget_it.value();
}

// ScreensModel

ScreensModel::ScreensModel(ColorsModel& colors, FontsModel& fonts, QObject* parent)
    : QAbstractItemModel(parent)
    , m_colorsModel(colors)
    , m_fontsModel(fonts)
    , mRoot(new WidgetData())
    , mCommander(new QUndoStack(this))
{
    mCommander->setUndoLimit(100);
    mRoot->setModel(this);
    connect(&colors, &ColorsModel::valueChanged, this, &ScreensModel::onColorChanged);
    connect(&fonts, &FontsModel::valueChanged, this, &ScreensModel::onFontChanged);
}

ScreensModel::~ScreensModel()
{
    delete mRoot;
}
QVariant ScreensModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case ColumnElement:
            return "Element";
        case ColumnName:
            return "Name";
        }
    }
    return QVariant();
}

QModelIndex ScreensModel::index(int row, int column, const QModelIndex& parent) const
{
    // check that row and column are in range
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    Item* parentItem;
    if (!parent.isValid()) {
        parentItem = mRoot;
    } else {
        parentItem = castItem(parent);
    }

    Item* childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ScreensModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    Item* item = castItem(index)->parent()->self();
    if (item == mRoot)
        return QModelIndex();
    return createIndex(item->myIndex(), ColumnElement, item);
}

int ScreensModel::rowCount(const QModelIndex& parent) const
{
    Item* parentItem;
    if (!parent.isValid())
        parentItem = mRoot;
    else
        parentItem = castItem(parent);

    return parentItem->childCount();
}

int ScreensModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return ColumnsCount;
}

QVariant ScreensModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    WidgetData* widget = static_cast<WidgetData*>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (index.column()) {
        case ColumnElement:
            return widget->typeStr();
        case ColumnName:
            switch (widget->type()) {
            case WidgetData::Label:
                return widget->text();
            case WidgetData::Pixmap:
                return widget->getAttr(Property::pixmap).toString();
            case WidgetData::Screen:
            case WidgetData::Widget:
                if (widget->name().isNull() && !widget->source().isEmpty()) {
                    return widget->source();
                }
                return widget->name();
            }
        }
    case ScreensModel::TypeRole:
        return widget->type();
    default:
        return QVariant();
    }
}

bool ScreensModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    WidgetData* widget = static_cast<WidgetData*>(index.internalPointer());

    switch (role) {
    case Qt::EditRole:
        switch (index.column()) {
        case ColumnName:
            switch (widget->type()) {
            case WidgetData::Label:
                return widget->setAttr(Property::text, value);
            case WidgetData::Pixmap:
                return widget->setAttr(Property::pixmap, value);
            case WidgetData::Screen:
            case WidgetData::Widget:
                return widget->setAttr(Property::name, value);
            }
        default:
            return false;
        }
    case ScreensModel::TypeRole:
        return widget->setType(value.toInt());
    default:
        return false;
    }
}

Qt::ItemFlags ScreensModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if (index.column() == ColumnElement) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
            | Qt::ItemIsDropEnabled;
    } else {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled
            | Qt::ItemIsDropEnabled;
    }
}

bool ScreensModel::insertRows(int row, int count, const QModelIndex& parent)
{
    Item* parentItem;
    if (!parent.isValid())
        parentItem = mRoot;
    else
        parentItem = castItem(parent);

    if (row < 0 || row > rowCount(parent)) {
        return false;
    }

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        parentItem->insertChild(row, new WidgetData());
    }
    endInsertRows();
    return true;
}

bool ScreensModel::removeRows(int row, int count, const QModelIndex& parent)
{
    Item* parentItem;
    if (!parent.isValid())
        parentItem = mRoot;
    else
        parentItem = castItem(parent);

    if (count <= 0 || row < 0 || row + count > parentItem->childCount()) {
        return false;
    }
    beginRemoveRows(parent, row, row + count - 1);
    parentItem->removeChildren(row, count);
    endRemoveRows();
    return true;
}

void ScreensModel::clear()
{
    beginResetModel();
    QModelIndex rootIndex;
    removeRows(0, rowCount(rootIndex), rootIndex);
    endResetModel();
    mCommander->clear();
}

void ScreensModel::appendFromXml(QXmlStreamReader& xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "screen");

    beginInsertRows(QModelIndex(), mRoot->childCount(), mRoot->childCount());

    WidgetData* w = new WidgetData();
    w->setModel(this);
    w->fromXml(xml);
    mRoot->appendChild(w);

    endInsertRows();
}

void ScreensModel::toXml(QXmlStreamWriter& xml)
{
    for (int i = 0; i < mRoot->childCount(); ++i) {
        mRoot->child(i)->toXml(xml);
    }
}

const WidgetData &ScreensModel::widget(const QModelIndex &index) const
{
    if (!index.isValid())
        qWarning() << "You asked for a widget at bad index!";
    return *indexToItem(index);
}

QVariant ScreensModel::widgetAttr(const QModelIndex& index, int key) const
{
    if (!index.isValid())
        return QVariant();

    WidgetData* widget = static_cast<WidgetData*>(index.internalPointer());
    return widget->getAttr(key);
}

bool ScreensModel::setWidgetAttr(const QModelIndex& index, int key, const QVariant& value)
{
    auto *widget = indexToItem(index);
    if (widget) {
        mCommander->push(new AttrCommand(widget, key, value));
        return true;
    }
    return false;
}

void ScreensModel::resizeWidget(const QModelIndex &index, const QSize &size)
{
    auto *widget = indexToItem(index);
    if (widget) {
        mCommander->push(new ResizeWidgetCommand(widget, size));
    }
}

void ScreensModel::moveWidget(const QModelIndex &index, const QPoint &pos)
{
    auto *widget = indexToItem(index);
    if (widget) {
        mCommander->push(new MoveWidgetCommand(widget, pos));
    }
}

void ScreensModel::registerObserver(const QModelIndex& index) {
    if (!index.isValid())
        return;
    if (m_observers[index]++ == 0) {
        indexToItem(index)->updateCache();
    }
}

void ScreensModel::unregisterObserver(const QModelIndex& index) {
    if (!index.isValid())
        return;
	auto it = m_observers.find(index);
    if (it != m_observers.end()) {
        it.value()--;
        if (it.value() == 0) {
            m_observers.erase(it);
		}
    } else {
        qWarning() << "observers map corrupted";
    }
}

void ScreensModel::widgetAttrHasChanged(const WidgetData* widget, int attrKey)
{
    QModelIndex index = createIndex(widget->myIndex(), ColumnElement,
                                    const_cast<WidgetData *>(widget));
    emit widgetChanged(index, attrKey);

//    switch (widget->type()) {
//    case WidgetData::Label:
//        if (attrKey != Property::text)
//            return;
//        break;
//    case WidgetData::Pixmap:
//        if (attrKey != Property::pixmap)
//            return;
//        break;
//    case WidgetData::Screen:
//    case WidgetData::Widget:
//        if (attrKey != Property::name)
//            return;
//        break;
//    }
    switch (attrKey) {
    case Property::text:
    case Property::name:
    case Property::source:
    case Property::pixmap:
        auto nameIndex = index.sibling(index.row(), ColumnName);
        emit dataChanged(nameIndex, nameIndex);
    }
}

void ScreensModel::onColorChanged(const QString &name, QRgb value)
{
    for (auto it = m_observers.begin(); it != m_observers.end(); ++it) {
        auto* w = indexToItem(it.key());
        w->onColorChanged(name, value);
    }
}

void ScreensModel::onFontChanged(const QString &name, const Font &value)
{
    for (auto it = m_observers.begin(); it != m_observers.end(); ++it) {
        auto* w = indexToItem(it.key());
        w->onFontChanged(name, value);
    }
}

ScreensModel::Item *ScreensModel::indexToItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        Q_ASSERT(index.model() == this);
        return static_cast<Item*>(index.internalPointer());
    } else {
        return mRoot;
    }
}

ScreensModel::Item* ScreensModel::castItem(const QModelIndex& index)
{
    Q_ASSERT(index.isValid());
    return static_cast<Item*>(index.internalPointer());
}

void ScreensModel::updatePreviewMap()
{
    mPreviews.clear();
    for (int i = 0; i < mRoot->childCount(); ++i) {
        const auto* screen = mRoot->child(i);
        QMap<QString, Preview> map;
        for (int j = 0; j < screen->childCount(); ++j) {
            const auto* widget = screen->child(j);
            if (!widget->previewValue().isNull())
                map[widget->name()] = Preview(widget->previewValue(), widget->previewRender());
        }
        if (!map.empty())
            mPreviews[screen->name()] = map;
    }
}

void ScreensModel::savePreviewTree(const QString &path)
{
    // When iterating over QMap order is sorted by name
    // Here walk over the tree structure

    QFile file(path);
    bool ok = file.open(QIODevice::WriteOnly);
    if (!ok)
        return;
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);

    xml.writeStartDocument();
    xml.writeStartElement("screens");
    // iterate over screens
    for (int i = 0; i < mRoot->childCount(); ++i) {
        const auto* screen = mRoot->child(i);
        xml.writeStartElement("screen");
        xml.writeTextElement("name", screen->name());
        xml.writeStartElement("entries");
        // iterate over widgets
        for (int j = 0; j < screen->childCount(); ++j) {
            auto const* widget = screen->child(j);
            auto valueStr = widget->previewValue().toString();
            auto renderStr = EnumAttr<Property::Render>(widget->previewRender()).toStr();
            if (valueStr.isEmpty())
                continue;
            xml.writeStartElement("entry");
            xml.writeAttribute("name", widget->name());
            xml.writeAttribute("value", valueStr);
            xml.writeAttribute("render", renderStr);
            xml.writeAttribute("type", "string");  // compatibility
            xml.writeEndElement();
        }
        xml.writeEndElement();
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndDocument();
}

WidgetObserverRegistrator::WidgetObserverRegistrator(ScreensModel* model, const QModelIndex& index)
    : m_model(model)
    , m_index(index)
{
    m_model->registerObserver(index);
}

void WidgetObserverRegistrator::setIndex(const QModelIndex& index)
{
    m_model->unregisterObserver(m_index);
    m_index = index;
    m_model->registerObserver(m_index);
}

WidgetObserverRegistrator::~WidgetObserverRegistrator()
{
    m_model->unregisterObserver(m_index);
}
