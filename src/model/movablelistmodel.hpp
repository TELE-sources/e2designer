#ifndef MOVABLELISTMODEL_HPP
#define MOVABLELISTMODEL_HPP

#include <QAbstractTableModel>

/**
 * @brief Model for the list wich can reoder items by drag and drop
 * You must reimplement ::moveItems command to make it work
 */
class MovableListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit MovableListModel(QObject* parent = nullptr);

    /**
     * @brief Minimum set of flags required to enable rows ordering with drag and drop
     * @param index
     * @return
     */
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Drag and drop
    /// Returns ownership according to Qt documentation
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex& parent) override;
    Qt::DropActions supportedDropActions() const override;

private:
    void encodeRows(const QModelIndexList& indexes, QDataStream& stream) const;
    QList<int> decodeRows(QDataStream& stream) const;
};

#endif // MOVABLELISTMODEL_HPP
