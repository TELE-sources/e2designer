#pragma once

#include <QGraphicsRectItem>

class ForegroundWidget : public QGraphicsRectItem
{
public:
    ForegroundWidget(const QRectF& rect, QGraphicsItem* parent = Q_NULLPTR)
        : QGraphicsRectItem(rect, parent)
    {}

    // QGraphicsItem interface
public:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};
