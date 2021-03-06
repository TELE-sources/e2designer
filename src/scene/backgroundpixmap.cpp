#include "backgroundpixmap.hpp"
#include <QPainter>

void BackgroundPixmap::paint(QPainter* painter,
                             const QStyleOptionGraphicsItem* option,
                             QWidget* widget)
{
    // BackgroundPixmap emulates the video layer of hardware blitter
    // our zValue is the greatest,
    // so with DestinationOver we actually put pixmap below all other
    // WidgetViews.
    painter->setCompositionMode(QPainter::CompositionMode_DestinationOver);
    QGraphicsPixmapItem::paint(painter, option, widget);
}

void BackgroundRect::paint(QPainter* painter,
                           const QStyleOptionGraphicsItem* option,
                           QWidget* widget)
{
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    QGraphicsRectItem::paint(painter, option, widget);
}
