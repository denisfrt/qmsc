#pragma once
#include <QDialog>

namespace Ui {
class QMscAboutDialog;
}

namespace qmsc {

class MscAboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MscAboutDialog(QWidget *parent);
    ~MscAboutDialog();

private:
    Ui::QMscAboutDialog *_ui;
};

} // namespace qmsc
