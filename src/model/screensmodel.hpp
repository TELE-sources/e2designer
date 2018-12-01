#ifndef SCREENSMODEL_H
#define SCREENSMODEL_H

#include "skin/widgetdata.hpp"
#include <QAbstractItemModel>
#include <QUndoStack>

struct Preview
{
    Preview(QVariant value = QVariant(), Property::Render render = Property::Label)
        : value(value)
        , render(render)
    {
    }
    QVariant value;
    Property::Render render;
};

class ScreensTree
{
public:
    void loadPreviews(const QString &path);
    /**
     * @brief Save previews stored in QMap
     * @param path Xml file name
     */
    void savePreviews(const QString &path);
    Preview getPreview(const QString& screen, const QString& widget) const;

protected:
    QMap<QString, QMap<QString, Preview>> mPreviews;
};

class ColorsModel;
class FontsModel;

class ScreensModel : public QAbstractItemModel, public ScreensTree
{
    Q_OBJECT

public:
    explicit ScreensModel(ColorsModel& colors, FontsModel& fonts, QObject* parent = Q_NULLPTR);
    ~ScreensModel() override;

    //	typedef MixinTreeNode<WidgetData> Item;
    typedef WidgetData Item;

    enum { ColumnElement, ColumnName, ColumnsCount };
    enum { TypeRole = Qt::UserRole + 1 };

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Add data:
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

    // Remove data:
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    void clear();

    // Xml:
    void appendFromXml(QXmlStreamReader& xml);
    void toXml(QXmlStreamWriter& xml);

    // Read only access to widget:
    const WidgetData &widget(const QModelIndex &index) const;

    // Access widget attributes:
    QVariant widgetAttr(const QModelIndex& index, int key) const;
    bool setWidgetAttr(const QModelIndex& index, int key, const QVariant& value);
    // Syntax shugar
//    template<class T>
//    bool setWidgetAttr(const QModelIndex &index, int key, const T &value) {
//        return setWidgetAttr(index, key, QVariant::fromValue(value));
//    }

    // Access asosiated fonts and colors
    const ColorsModel& colors() const { return m_colorsModel; };
    const FontsModel& fonts() const { return m_fontsModel; }

    // Access undo model
    QUndoStack *undoStack() const { return mCommander; }

    // Move and resize widget
    void resizeWidget(const QModelIndex &index, const QSize &size);
    void moveWidget(const QModelIndex &index, const QPoint &pos);

	// widgetChanged only emmited for widgets being observed
    void registerObserver(const QModelIndex& index);
    void unregisterObserver(const QModelIndex& index);

    // Build preview map from widget tree
    void updatePreviewMap();
    void savePreviewTree(const QString &path);

signals:
    void widgetChanged(const QModelIndex &index, int attr);

public slots:
    // to be called from WidgetData
    void widgetAttrHasChanged(const WidgetData* widget, int attrKey);
    //
    void onColorChanged(const QString &name, QRgb value);
    void onFontChanged(const QString &name, const Font &value);

private:
    Item *indexToItem(const QModelIndex &index) const;
    static Item* castItem(const QModelIndex& index);

	QHash<QPersistentModelIndex, int> m_observers;

    // QTimer* m_timer;
    // QTime m_lastShot;

    // ref
    ColorsModel& m_colorsModel;
    FontsModel& m_fontsModel;

    // own
    WidgetData* mRoot;
    // QObject owned
    QUndoStack *mCommander;
};

class WidgetObserverRegistrator
{
    Q_DISABLE_COPY(WidgetObserverRegistrator)
public:
    WidgetObserverRegistrator(ScreensModel *model, const QModelIndex& index);
    void setIndex(const QModelIndex& index);
    ~WidgetObserverRegistrator();
private:
    ScreensModel* m_model;
    QPersistentModelIndex m_index;
};

#endif // SCREENSMODEL_H
