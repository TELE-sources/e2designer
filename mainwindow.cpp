#include "mainwindow.hpp"
#include "skindelegate.hpp"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QColorDialog>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsView>
#include <QItemEditorFactory>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QRgb>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>

#include "colorlistbox.hpp"
#include "colorlistwindow.hpp"
#include "editor/xmlhighlighter.hpp"
#include "fontlistwindow.hpp"
#include "listbox.hpp"
#include "model/colorsmodel.hpp"
#include "repository/skinrepository.hpp"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mView(new ScreenView(SkinRepository::screens()))
    , mPropertiesModel(new PropertiesModel(this))
{
    ui->setupUi(this);
    readSettings();
    createActions();

    // hide editor by default
    ui->splitterR->setSizes(QList<int>{ 0, 1 });

    //
    QFont monoFont("Monospace");
    monoFont.setStyleHint(QFont::TypeWriter);
    ui->textEdit->setFont(monoFont);
    auto highlighter = new XMLHighlighter(ui->textEdit->document());
    Q_UNUSED(highlighter);

    ui->treeView->setModel(SkinRepository::screens());
    //	ui->treeView->setDragEnabled(true);
    //	ui->treeView->setDropIndicatorShown(true);
    //	ui->treeView->setAcceptDrops(true);
    //	ui->treeView->setDefaultDropAction(Qt::MoveAction);
    //	// accept dnd actions only from it self
    //	ui->treeView->setDragDropMode(QAbstractItemView::InternalMove);

    ui->propView->setModel(mPropertiesModel);
    ui->propView->setIndentation(5);

    //	SkinDelegate *delegate = new SkinDelegate(this);
    //	ui->propView->setItemDelegate(delegate);

    ui->graphicsView->setScene(mView->scene());

    // Setup default editors
    QItemEditorFactory* factory = new QItemEditorFactory();

    auto* colorCreator = new QStandardItemEditorCreator<ColorListBox>();
    factory->registerEditor(QVariant::Color, colorCreator);

    auto* stringCreator = new QStandardItemEditorCreator<QLineEdit>();
    factory->registerEditor(QVariant::String, stringCreator);

    auto* intCreator = new QStandardItemEditorCreator<QSpinBox>();
    factory->registerEditor(QVariant::Int, intCreator);

    auto* enumCreator = new QStandardItemEditorCreator<ListBox>();
    factory->registerEditor(qMetaTypeId<SkinEnumList>(), enumCreator);

    QItemEditorFactory::setDefaultFactory(factory);

    connect(ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &MainWindow::updateScene);

    connect(mView, &ScreenView::selectionChanged, this, &MainWindow::updateTreeSelection);

    // TEST!!!

    //    editColors();

    //	SkinRepository::instance().colors()->append(Color(QString("red"), 0));
    //	SkinRepository::instance().colors()->append(Color("green", qRgb(0, 255,
    // 0)));

    /*
    quintptr p;

    WidgetData *d = new WidgetData();
    p = quintptr(d);
    qDebug() << p;
    p = quintptr(static_cast<MixinTreeNode<WidgetData>*>(d));
    qDebug() << p;

    AttrAdapter *a = new AttrGroupAdapater("lol");
    p = quintptr(a);
    qDebug() << p;
    p = quintptr(static_cast<AttrAdapter*>(a));
    qDebug() << p;
    p = quintptr(static_cast<MixinTreeNode<AttrAdapter>*>(a));
    qDebug() << p;

    void *ptr = reinterpret_cast<void*>(p);
    p = quintptr(static_cast<AttrAdapter*>(ptr));
    qDebug() << p;
    p = quintptr(static_cast<AttrAdapter*>(
                      static_cast<MixinTreeNode<AttrAdapter>*>(ptr)
                      ));
    qDebug() << p;
    */
}

MainWindow::~MainWindow()
{
    if (mView)
        delete mView;
    delete ui;
}

void MainWindow::openFile(const QString& dirname)
{
    SkinRepository::instance().loadFile(dirname);
    //	m_model->loadFile(dirname);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (confirmClose()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

// TODO: somehow multiple selection?

void MainWindow::updateTreeSelection(QModelIndex index)
{
    //    ui->treeView->selectionModel()->select(index,
    //    QItemSelectionModel::SelectCurrent);
    ui->treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
    ui->treeView->scrollTo(index);

    qDebug() << "is true?" << (ui->treeView->selectionModel()->currentIndex() == index);

    mPropertiesModel->setWidget(index);
    ui->propView->expandAll();
}

void MainWindow::updateScene(const QModelIndex& index, const QModelIndex& previndex)
{
    qDebug() << "current changed" << index;
    mPropertiesModel->setWidget(index);
    ui->propView->expandAll();

    QModelIndex idx = index;

    while (idx.isValid() && idx.data(ScreensModel::TypeRole).toInt() != WidgetData::Screen) {
        idx = idx.parent();
    }
    if (idx.isValid()) {
        mView->setScreen(idx);
    }
}

void MainWindow::newSkin()
{
    if (confirmClose()) {
        // TODO: create new skin
    }
}

void MainWindow::open()
{
    if (confirmClose()) {
        QSettings settings(QCoreApplication::organizationName(),
                           QCoreApplication::applicationName());
        QString startdir = settings.value("lastdir").toString();
        //		if (startdir)
        //		QDir::home()
        const QString fname = QFileDialog::getOpenFileName(this, tr("Open skin"), startdir,
                                                           tr("Skin file (skin.xml)"));
        if (!fname.isNull()) {
            QString dir = QFileInfo(fname).absoluteDir().path();
            qDebug() << "opening" << dir;
            settings.setValue("lastdir", dir);
            openFile(dir);
        }
    }
}

bool MainWindow::save()
{
    // TODO: exceptions vs return codes?
    bool saved = SkinRepository::instance().save();
    if (!saved) {
    }
    return saved;
}

bool MainWindow::saveAs()
{
    // TODO
    return false;
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("e2designer"),
                       tr("The <b>e2designer</b> allows you to edit enigma2 "
                          "skins in a nice way"
                          "<br>Usefull hints:"
                          "<ul>"
                          "<li>Arrow keys - move object<li>"
                          "<li>Shift + Arrow keys - presize move object<li>"
                          "<li>Ctrl + Left mouse click - multiple selection<li>"
                          "</ul>"));
}

void MainWindow::skinWasModified()
{
    //??? Add fact that we modified in status bar
}

void MainWindow::addSkinItem(int type)
{
    // FIXME: is m_view always athorative?
    // TODO: should model provide more friendly interface?
    auto model = SkinRepository::screens();
    QModelIndex screen = mView->currentIndex();
    model->insertRow(model->rowCount(screen), screen);
    QModelIndex widget = model->index(model->rowCount(screen) - 1, 0, screen);
    model->setData(widget, type, ScreensModel::TypeRole);
    model->setWidgetAttr(widget, Property::size, QSize(100, 100), Roles::GraphicsRole);
    if (type == WidgetData::Widget) {
        model->setWidgetAttr(widget, Property::name, "Untitled", Qt::EditRole);
    } else if (type == WidgetData::Label) {
        model->setWidgetAttr(widget, Property::text, "Default text", Qt::EditRole);
    }
}

void MainWindow::addWidget()
{
    addSkinItem(WidgetData::Widget);
}
void MainWindow::addPixmap()
{
    addSkinItem(WidgetData::Pixmap);
}
void MainWindow::addLabel()
{
    addSkinItem(WidgetData::Label);
}
void MainWindow::addScreen()
{
    auto model = SkinRepository::instance().screens();
    // TODO: should model provide more friendly interface?
    QModelIndex root;
    model->insertRow(model->rowCount(root), root);
    QModelIndex screen = model->index(model->rowCount(root) - 1, 0, root);
    model->setData(screen, WidgetData::Screen, ScreensModel::TypeRole);
    model->setWidgetAttr(screen, Property::position, "center,center", Qt::EditRole);
    model->setWidgetAttr(screen, Property::size, QSize(300, 200), Roles::GraphicsRole);
    model->setWidgetAttr(screen, Property::name, "UntitledScreen", Qt::EditRole);
}

void MainWindow::delWidget()
{
    if (!mView)
        return;
    QModelIndex idx = ui->treeView->selectionModel()->currentIndex();
    SkinRepository::screens()->removeRow(idx.row(), idx.parent());
    //    mView->deleteSelected();
    // FIXME: is m_view always athorative?
}

void MainWindow::editColors()
{
    auto colorsWindow = new ColorListWindow(this);
    colorsWindow->setFloating(true);
    colorsWindow->show();
}

void MainWindow::editFonts()
{
    auto fontsWindow = new FontListWindow(this);
    fontsWindow->show();
}

void MainWindow::createActions()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    QToolBar* mainToolBar = ui->mainToolBar;

    const QIcon newIcon = QIcon::fromTheme("document-new");
    QAction* newAct = new QAction(newIcon, tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new skin"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newSkin);
    fileMenu->addAction(newAct);
    mainToolBar->addAction(newAct);

    const QIcon openIcon = QIcon::fromTheme("document-open");
    QAction* openAct = new QAction(openIcon, tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);
    fileMenu->addAction(openAct);
    mainToolBar->addAction(openAct);

    const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":/images/save.png"));
    QAction* saveAct = new QAction(saveIcon, tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);
    fileMenu->addAction(saveAct);
    mainToolBar->addAction(saveAct);

    const QIcon saveAsIcon = QIcon::fromTheme("document-save-as");
    QAction* saveAsAct =
        fileMenu->addAction(saveAsIcon, tr("Save &As..."), this, &MainWindow::saveAs);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));

    fileMenu->addSeparator();

    const QIcon exitIcon = QIcon::fromTheme("application-exit");
    QAction* exitAct = fileMenu->addAction(exitIcon, tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));

    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));

    QAction* addWidgetAct = editMenu->addAction(tr("Add widget"), this, &MainWindow::addWidget);
    mainToolBar->addAction(addWidgetAct);
    editMenu->addAction(tr("Add Pixmap"), this, &MainWindow::addPixmap);
    editMenu->addAction(tr("Add Label"), this, &MainWindow::addLabel);
    editMenu->addAction(tr("Add Screen"), this, &MainWindow::addScreen);
    QAction* delWidgetAct = editMenu->addAction(tr("Delete widget"), this, &MainWindow::delWidget);
    mainToolBar->addAction(delWidgetAct);
    editMenu->addAction(tr("Edit colors"), this, &MainWindow::editColors);
    editMenu->addAction(tr("Edit Fonts"), this, &MainWindow::editFonts);

    fileMenu->addSeparator();

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction* aboutAct = helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    aboutAct->setStatusTip(tr("Show the application's About box"));

    QAction* aboutQtAct = helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
}

void MainWindow::readSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        setWindowState(Qt::WindowMaximized);
    } else {
        restoreGeometry(geometry);
    }
}

void MainWindow::writeSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("geometry", saveGeometry());
}

bool MainWindow::confirmClose()
{
    if (!isModified())
        return true;

    const QMessageBox::StandardButton ret =
        QMessageBox::warning(this, tr("Application"), tr("The skin has been modified.\n"
                                                         "Do you want to save your changes?"),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (ret) {
    case QMessageBox::Save:
        return save();
    case QMessageBox::Cancel:
        return false;
    default:
        return true;
    }
}

bool MainWindow::isModified()
{
    // TODO
    return false;
}
