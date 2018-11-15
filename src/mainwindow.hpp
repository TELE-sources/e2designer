#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QGraphicsScene>
#include <QMainWindow>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTreeView>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "model/propertiesmodel.hpp"
#include "scene/screenview.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = Q_NULLPTR);
    ~MainWindow() override;

    void openFile(const QString& dirname);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onCurrentSelectionChanged(const QModelIndex& index, const QModelIndex& previous);

    void newSkin();
    void open();
    bool save();
    bool saveAs();
    void about();

    void addSkinItem(int type);
    void addWidget();
    void addPixmap();
    void addLabel();
    void addScreen();

    void delWidget();
    void editColors();
    void editFonts();

private:
    // menu and toolbar
    void createActions();
    void readSettings();
    void writeSettings();
    // shows save before close dialog
    bool confirmClose();
    bool isModified();

    Ui::MainWindow* ui;

    //	SkinModel *m_model;

    ScreenView* mView;

    // QObject owned
    PropertiesModel* mPropertiesModel;

    //	QSortFilterProxyModel *m_topfilter;
    //	QSortFilterProxyModel *m_headerfilter;
};

#endif // MAINWINDOW_H
