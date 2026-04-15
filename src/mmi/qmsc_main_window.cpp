#include "qmsc_main_window.h"
#include <QCloseEvent>
#include <QFileDialog>
#include <QLabel>
#include <QMdiSubWindow>
#include <QTextEdit>
#include "qmsc_about_dialog.h"
#include "qmsc_child.h"
#include "qmsc_config_dialog.h"
#include "qmsc_settings.h"
#include "ui_qmsc_main_window.h"

namespace qmsc {

MscMainWindow *MscMainWindow::_window = nullptr;

MscMainWindow::MscMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , _ui(new Ui::QMscMainWindow)
{
    _ui->setupUi(this);
    _window = this;
    _settings = new MscSettings(this);
    _settings->load();

    _labelParseStatus = new QLabel(this);
    _labelParseStatus->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    _labelParseStatus->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    statusBar()->addPermanentWidget(_labelParseStatus);
    _labelDrawScale = new QLabel(this);
    _labelDrawScale->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    _labelDrawScale->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    statusBar()->addPermanentWidget(_labelDrawScale);
    _labelLinePos = new QLabel(this);
    _labelLinePos->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    _labelLinePos->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    statusBar()->addPermanentWidget(_labelLinePos);

    updateMenus();
    updateLineCol();
    updateParseStatus();
    updateDrawScale();
    updateRecentMenu();
    statusBar()->showMessage(tr("Ready"));

    // Connect Designer actions to slots
    connect(_ui->action_New, &QAction::triggered, this, &MscMainWindow::onNewDocument);
    connect(_ui->action_Open, &QAction::triggered, this, &MscMainWindow::onOpenDocument);
    connect(_ui->action_Save, &QAction::triggered, this, &MscMainWindow::onSaveCurrentDocument);
    connect(_ui->action_Save_As, &QAction::triggered, this, &MscMainWindow::onSaveCurrentDocument);
    connect(_ui->action_Tile, &QAction::triggered, this, &MscMainWindow::onTileWindows);
    connect(_ui->action_Cascade, &QAction::triggered, this, &MscMainWindow::onCascadeWindows);
    connect(_ui->action_Debug, &QAction::triggered, this, &MscMainWindow::onOptionUpdated);
    connect(_ui->action_Lock_Instances, &QAction::triggered, this, &MscMainWindow::onOptionUpdated);
    connect(_ui->action_Center_Msc, &QAction::triggered, this, &MscMainWindow::onOptionUpdated);
    connect(_ui->action_Settings, &QAction::triggered, this, &MscMainWindow::onSettings);
    connect(_ui->action_About, &QAction::triggered, this, &MscMainWindow::onAbout);
    connect(_ui->mdiArea, &QMdiArea::subWindowActivated, this, &MscMainWindow::onSubWindowActivated);
    //newDocument();
    //addDocument("/home/denis/DEV/qmsc/src/mmi/msc/test.msc");
}

MscMainWindow::~MscMainWindow()
{
    delete _ui;
}

MscMainWindow *MscMainWindow::ptr()
{
    return _window;
}

::Ui::QMscMainWindow *MscMainWindow::ui()
{
    return _ui;
}

void MscMainWindow::addDocument(const QString &fileName, const QString &title)
{
    MscDocument *doc = MscDocument::create(fileName, title, _settings->config());
    if (doc) {
        MscChild *child = new MscChild(doc, this);
        QMdiSubWindow *subWin = _ui->mdiArea->addSubWindow(child);
        subWin->setWindowIcon(QIcon(":/images/document.png"));
        subWin->show();
        // Add to recent if file exists
        if (doc->isNativeFile()) {
            _settings->addToRecent(fileName);
            updateRecentMenu();
        }
    }
}

void MscMainWindow::setStatus(const QString &status) const
{
    statusBar()->showMessage(status);
}

void MscMainWindow::updateLineCol(const TextPosition &pos) const
{
    QString text;
    if (pos.isValid()) {
        text = QString(tr("Ln %1, Col %2")).arg(pos._line + 1).arg(pos._col + 1);
        if (_ui->action_Debug->isChecked()) {
            text += QString(tr(", Pos %1").arg(pos._pos));
        }
    }
    _labelLinePos->setVisible(!text.isEmpty());
    _labelLinePos->setText(text);
}

void MscMainWindow::updateParseStatus(int errorCount)
{
    QString text;
    if (errorCount <= -1) {
        _labelParseStatus->setStyleSheet("");
    } else if (errorCount == 0) {
        _labelParseStatus->setStyleSheet("QLabel { background-color: green; }");
        text = QString(tr("OK"));
    } else {
        _labelParseStatus->setStyleSheet("QLabel { background-color: red; }");
        text = QString(tr("%n error(s)", "", errorCount));
    }
    _labelParseStatus->setVisible(!text.isEmpty());
    _labelParseStatus->setText(text);
}

void MscMainWindow::updateDrawScale(int scale)
{
    QString text;
    if (scale != 100) {
        text = QString("%1%").arg(scale);
    }
    _labelDrawScale->setVisible(!text.isEmpty());
    _labelDrawScale->setText(text);
}

void MscMainWindow::closeEvent(QCloseEvent *event)
{
    // sends QCloseEvent to each subwindow
    _ui->mdiArea->closeAllSubWindows();

    // If any subwindow ignored its closeEvent, it will still be in the list
    if (!_ui->mdiArea->subWindowList().isEmpty()) {
        event->ignore(); // keep main window open
        return;
    }
    _settings->save();
    event->accept(); // all children closed, OK to quit
    QMainWindow::closeEvent(event);
}

void MscMainWindow::onNewDocument()
{
    addDocument("", tr("Message Sequence Chart %1").arg(_ui->mdiArea->subWindowList().size() + 1));
}

void MscMainWindow::onOpenDocument()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Open Flowchart"),
                                                    QString(),
                                                    tr("Flowchart Files %1").arg("(*.msc)"));
    if (filename.isEmpty()) {
        return;
    }
    addDocument(filename);
}

void MscMainWindow::onSaveCurrentDocument()
{
    QAction *action = qobject_cast<QAction *>(sender());
    bool bForceSaveAs = (action == _ui->action_Save_As);
    if (auto *child = getMscChild(_ui->mdiArea->currentSubWindow())) {
        child->save(bForceSaveAs);
        // Add to recent files
        if (child->document()->isNativeFile()) {
            _settings->addToRecent(child->document()->filename());
            updateRecentMenu();
        }
    }
}

void MscMainWindow::onTileWindows()
{
    _ui->mdiArea->tileSubWindows();
}

void MscMainWindow::onCascadeWindows()
{
    _ui->mdiArea->cascadeSubWindows();
}

void MscMainWindow::onOptionUpdated()
{
    _settings->config()._debug = _ui->action_Debug->isChecked();
    _settings->config()._lockInstances = _ui->action_Lock_Instances->isChecked();
    _settings->config()._centerMsc = _ui->action_Center_Msc->isChecked();
    for (auto const *sub : _ui->mdiArea->subWindowList()) {
        if (auto *child = getMscChild(sub)) {
            child->document()->updateConfig(_settings->config());
            child->activated();
        }
    }
}

void MscMainWindow::onSettings()
{
    auto *dialog = new MscConfigDialog(this, _settings->config());
    connect(dialog, &QDialog::finished, this, [this, dialog](int result) {
        if (result == QDialog::Accepted) {
            onOptionUpdated();
        }
        delete dialog;
    });
    dialog->open();
}

void MscMainWindow::onAbout()
{
    auto *dialog = new MscAboutDialog(this);
    dialog->open();
}

void MscMainWindow::updateMenus(QMdiSubWindow *child)
{
    if (child == nullptr) {
        child = _ui->mdiArea->currentSubWindow();
    }
    bool childActivated = (child != nullptr);
    bool childModified = (child != nullptr && child->isWindowModified());
    //qDebug() << "act:" << childActivated << " mod:" << childModified;
    _ui->action_Save->setEnabled(childActivated && childModified);
    _ui->action_Save_As->setEnabled(childActivated);
    _ui->action_Debug->setEnabled(childActivated);
    _ui->action_Debug->setChecked(_settings->config()._debug);
    _ui->action_Lock_Instances->setEnabled(childActivated);
    _ui->action_Lock_Instances->setChecked(_settings->config()._lockInstances);
    _ui->action_Center_Msc->setEnabled(childActivated);
    _ui->action_Center_Msc->setChecked(_settings->config()._centerMsc);
}

void MscMainWindow::updateRecentMenu()
{
    QMenu *menu = _ui->menu_Recent_Files;
    QAction *sepExample = nullptr;
    QAction *sepRecent = nullptr;

    // Clear actions
    QList<QAction *> clearList;
    for (auto *action : menu->actions()) {
        if (action->isSeparator()) {
            if (sepExample == nullptr)
                sepExample = action;
            else if (sepRecent == nullptr)
                sepRecent = action;
        } else if (action->isEnabled()) {
            clearList.append(action);
        }
    }
    for (auto *action : clearList) {
        menu->removeAction(action);
    }

    // Add examples
    if (sepExample) {
        QDir dir(":/msc");
        const QFileInfoList examples = dir.entryInfoList(QDir::Files);
        for (auto const &example : examples) {
            auto *action = new QAction(menu);
            action->setText(example.fileName());
            action->setData(example.absoluteFilePath());
            connect(action, &QAction::triggered, this, [this]() {
                QAction *action = qobject_cast<QAction *>(sender());
                if (action)
                    addDocument(action->data().toString());
            });
            menu->insertAction(sepExample, action);
        }
    }
    // Add recents files
    if (sepRecent) {
        for (auto const &recentFile : _settings->recentFiles()) {
            auto *action = new QAction(menu);
            action->setText(recentFile);
            connect(action, &QAction::triggered, this, [this]() {
                QAction *action = qobject_cast<QAction *>(sender());
                if (action)
                    addDocument(action->text());
            });
            menu->insertAction(sepRecent, action);
        }
    }
}

void MscMainWindow::onSubWindowActivated(QMdiSubWindow *subWindow)
{
    updateMenus(subWindow);
    if (!subWindow) {
        updateLineCol();
        updateParseStatus();
    } else if (auto *child = getMscChild(subWindow)) {
        child->activated();
    }
}

MscChild *MscMainWindow::getMscChild(const QMdiSubWindow *subWindow)
{
    MscChild *r = nullptr;
    if (subWindow) {
        r = qobject_cast<MscChild *>(subWindow->widget());
    }
    return r;
}

} // namespace qmsc
