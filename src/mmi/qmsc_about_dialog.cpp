#include "qmsc_about_dialog.h"
#include "ui_qmsc_about_dialog.h"

namespace qmsc {

MscAboutDialog::MscAboutDialog(QWidget *parent)
    : QDialog(parent)
    , _ui(new Ui::QMscAboutDialog)
{
    _ui->setupUi(this);
    QPalette palLogo = palette();
    palLogo.setColor(QPalette::Window, Qt::black);
    setPalette(palLogo);
    setAutoFillBackground(true);

    QPixmap pixmap(":/images/logo.png");
    if (pixmap.isNull()) {
        _ui->labelLogo->setText(tr("Image not found"));
    } else {
        _ui->labelLogo->setPixmap(pixmap);
        _ui->labelLogo->setScaledContents(true);
        _ui->labelLogo->resize(pixmap.size());
    }

    QPalette palText = _ui->labelText->palette();
    palText.setColor(QPalette::WindowText, Qt::white);
    _ui->labelText->setPalette(palText);
    QString text = R"(<u>Author</u>: Denis F.<br>
<u>Libraries</u>:
<ul>
<li>Qt libraries (<a href="https://www.qt.io/development/qt-framework/qt-licensing">qt.io</a>)</li>
<li>RE2C - Regular Expressions to Code (<a href="https://re2c.org/">re2c.org</a>)</li>
<li>Lemon - LALR(1) Parser Generator (<a href="https://sqlite.org/lemon.html">sqlite.org</a>)</li>
</ul>)";
    _ui->labelText->setText(text);
    _ui->labelText->setTextFormat(Qt::RichText);
    _ui->labelText->setTextInteractionFlags(Qt::TextBrowserInteraction);
    _ui->labelText->setOpenExternalLinks(true);
}

MscAboutDialog::~MscAboutDialog()
{
    delete _ui;
}

} // namespace qmsc