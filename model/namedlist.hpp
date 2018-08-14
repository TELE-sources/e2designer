#ifndef NAMEDLIST_H
#define NAMEDLIST_H

#include <QVector>

/**
 * @brief Stores list of (key, value) pairs
 * the order of keys is preserved
 * only unique key names are allowed
 * provides list like and map like interfaces
 * This is a helper class, mutating methods declared as protected
 */
template <class T>
class NamedList
{
public:
    explicit NamedList() {}
    virtual ~NamedList() {}
    static_assert(std::is_default_constructible<T>(), "T must be default constructible");

protected:
    // list like interface
    bool isValidIndex(int i) const { return 0 <= i && i < mItems.size(); }
    bool setItemName(int i, const QString& name);
    bool setItemValue(int i, const typename T::Value& value);

    bool canInsertItem(const T &item);
    bool insertItem(int i, const T &item);
    bool appendItem(T item) { return insertItem(mItems.count(), item); }
    bool canRemoveItems(int position, int count);
    bool removeItems(int position, int count);

public:
    bool contains(const QString& name) const;
    const T &getValue(const QString& name, T defaultValue = T()) const;
    QString getName(const typename T::Value value) const;

    inline int itemsCount() const { return mItems.count(); }
    inline const T& itemAt(int i) const { return mItems[i]; }
    int getIndex(const QString& name) const;

    typename QVector<T>::const_iterator begin() const noexcept { return mItems.cbegin(); }
    typename QVector<T>::const_iterator end() const noexcept { return mItems.cend(); }

protected:
    virtual void emitValueChanged(const QString& name, const T& value) const = 0;

private:
    QVector<T> mItems;
};

// Implementation

template <typename T>
bool NamedList<T>::setItemName(int i, const QString& name)
{
    if (!isValidIndex(i)) {
        return false;
    }
    if (contains(name)) {
        return false;
    }
    emitValueChanged(mItems[i].name(), T());
    mItems[i] = T(name, mItems[i].value());
    emitValueChanged(name, mItems[i]);
    return true;
}

template <typename T>
bool NamedList<T>::setItemValue(int i, const typename T::Value& value)
{
    if (!isValidIndex(i)) {
        return false;
    }
    mItems[i] = T(mItems[i].name(), value);
    emitValueChanged(mItems[i].name(), mItems[i]);
    return true;
}

template <typename T>
bool NamedList<T>::canInsertItem(const T &item)
{
    if (contains(item.name())) {
        return false;
    }
    return true;
}

template <typename T>
bool NamedList<T>::insertItem(int i, const T &item)
{
    if (canInsertItem(item)) {
        mItems.insert(i, item);
        emitValueChanged(item.name(), item);
        return true;
    }
    return false;
}

template <typename T>
bool NamedList<T>::canRemoveItems(int position, int count)
{
    if (position < 0 || position + count > mItems.size()) {
        return false;
    }
    return true;
}

template <typename T>
bool NamedList<T>::removeItems(int position, int count)
{
    if (canRemoveItems(position, count)) {
        for (int i = 0; i < count; ++i) {
            emitValueChanged(mItems[position].name(), T());
            mItems.removeAt(position);
        }
        return true;
    }
    return false;
}

template <typename T>
bool NamedList<T>::contains(const QString& name) const
{
    // TODO: optimization with QHash?
    for (const T& item : mItems) {
        if (item.name() == name) {
            return true;
        }
    }
    return false;
}

template <typename T>
const T& NamedList<T>::getValue(const QString& name, T defaultValue) const
{
    // TODO: optimization with QHash?
    for (const T& item : mItems) {
        if (item.name() == name)
            return item;
    }
    return defaultValue;
}

template <class T>
QString NamedList<T>::getName(const typename T::Value value) const
{
    for (const T& item : mItems) {
        if (item.value() == value)
            return item.name();
    }
    return QString();
}

#endif // NAMEDLIST_H
