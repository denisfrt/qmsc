#pragma once
#include <QLabel>
#include <QMainWindow>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QSettings>
#include "qmsc_child.h"
#include "qmsc_settings.h"
#include "qmsc_utils.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class QMscMainWindow;
}
QT_END_NAMESPACE

namespace qmsc {

class MscMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MscMainWindow(QWidget *parent = nullptr);
    virtual ~MscMainWindow() override;
    static MscMainWindow *ptr();
    ::Ui::QMscMainWindow *ui();
    MscSettings &settings();

public:
    void addDocument(const QString &fileName, const QString &title = "");
    void setStatus(const QString &status) const;
    void updateLineCol(const TextPosition &pos = TextPosition()) const;
    void updateParseStatus(int errorCount = -1);
    void updateDrawScale(int scale = 100);
    void updateMenus(QMdiSubWindow *subWindow = nullptr);
    void updateRecentMenu();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNewDocument();
    void onOpenDocument();
    void onSaveCurrentDocument();
    void onTileWindows();
    void onCascadeWindows();
    void onOptionUpdated();
    void onSettings();
    void onAbout();
    void onSubWindowActivated(QMdiSubWindow *subWindow);

private:
    MscChild *getMscChild(const QMdiSubWindow *subWindow);

private:
    Ui::QMscMainWindow *_ui;
    MscSettings *_settings;
    QLabel *_labelLinePos;
    QLabel *_labelParseStatus;
    QLabel *_labelDrawScale;
    static MscMainWindow *_window;
};

} // namespace qmsc
