#include "colorlistbox.h"
#include "repository/skinrepository.h"

ColorListBox::ColorListBox(QWidget *parent)
    : QComboBox(parent)
{
    populateList();
}

QColor ColorListBox::color() const
{
    return qvariant_cast<QColor>(itemData(currentIndex(), Qt::DecorationRole));
}

void ColorListBox::setColor(QColor color)
{
    setCurrentIndex(findData(color, Qt::DecorationRole));
}

void ColorListBox::populateList()
{
    auto *colors = SkinRepository::colors();
    for (int i = 0; i < colors->rowCount(); ++i) {
        Color color = colors->itemAt(i);
        insertItem(i, color.name());
        setItemData(i, QColor(color.value()), Qt::DecorationRole);
    }
}
