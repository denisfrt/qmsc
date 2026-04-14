#include "qmsc_config_dialog.h"
#include <QColorDialog>
#include "qmsc_settings.h"
#include "ui_qmsc_config_dialog.h"

namespace qmsc {

MscConfigDialog::MscConfigDialog(QWidget *parent, MscSettings::Config &config)
    : QDialog(parent)
    , _ui(new Ui::QMscConfigDialog)
    , _config(config)
{
    _ui->setupUi(this);
    // clang-format off
    initColorButton(_ui->btColorSelection, _ui->btColorSelectionReset, _config._colorSelection, _defaultConfig._colorSelection);
    initColorButton(_ui->btColorBlock, _ui->btColorBlockReset, _config._colorBlock, _defaultConfig._colorBlock);
    initColorButton(_ui->btColorElement, _ui->btColorElementReset, _config._colorElement, _defaultConfig._colorElement);
    initColorButton(_ui->btColorProperty, _ui->btColorPropertyReset, _config._colorProperty, _defaultConfig._colorProperty);
    initColorButton(_ui->btColorString, _ui->btColorStringReset, _config._colorString, _defaultConfig._colorString);
    initColorButton(_ui->btColorComment, _ui->btColorCommentReset, _config._colorComment, _defaultConfig._colorComment);
    // clang-format on
    initFont(_ui->cbEditFont, _ui->sbEditFontSize, _config._editFont, _config._editFontSize);
    initFont(_ui->cbViewFont, _ui->sbViewFontSize, _config._mscFont, _config._mscFontSize);
}

void MscConfigDialog::initFont(QFontComboBox *cbFont,
                               QSpinBox *sbFontSize,
                               const QFont &font,
                               int szFont)
{
    int index = cbFont->findText(font.family(), Qt::MatchFixedString);
    if (index >= 0) {
        cbFont->setCurrentIndex(index);
    } else {
        cbFont->setCurrentFont(font);
    }
    sbFontSize->setValue(szFont);
}

void MscConfigDialog::initColorButton(QAbstractButton *colorButton,
                                      QAbstractButton *defaultButton,
                                      const QColor &color,
                                      const QColor &defaultColor)
{
    // init button
    setButtonColor(colorButton, color);
    defaultButton->setDisabled(color == defaultColor);
    // Set color callback
    auto onColorButton = [this, colorButton, defaultButton, &defaultColor]() {
        QColor curColor = getButtonColor(colorButton);
        QColor newColor = QColorDialog::getColor(curColor, this, tr("Choose Color"));
        if (newColor.isValid()) {
            setButtonColor(colorButton, newColor);
            defaultButton->setDisabled(curColor != defaultColor);
        }
    };
    connect(colorButton, &QAbstractButton::clicked, this, onColorButton);
    // Reset to default color callback
    auto onColorReset = [this, colorButton, defaultButton, &defaultColor]() {
        setButtonColor(colorButton, defaultColor);
        defaultButton->setDisabled(true);
    };
    connect(defaultButton, &QAbstractButton::clicked, this, onColorReset);
}

MscConfigDialog::~MscConfigDialog()
{
    delete _ui;
}

void MscConfigDialog::accept()
{
    _config._colorSelection = getButtonColor(_ui->btColorSelection);
    _config._colorBlock = getButtonColor(_ui->btColorBlock);
    _config._colorElement = getButtonColor(_ui->btColorElement);
    _config._colorProperty = getButtonColor(_ui->btColorProperty);
    _config._colorString = getButtonColor(_ui->btColorString);
    _config._colorComment = getButtonColor(_ui->btColorComment);
    _config._editFontName = _ui->cbEditFont->currentText();
    _config._editFontSize = _ui->sbEditFontSize->value();
    _config._editFont = QFont(_config._editFontName, _config._editFontSize);
    _config._mscFontName = _ui->cbViewFont->currentText();
    _config._mscFontSize = _ui->sbViewFontSize->value();
    _config._mscFont = QFont(_config._mscFontName, _config._mscFontSize);
    QDialog::accept();
}

void MscConfigDialog::setButtonColor(QAbstractButton *button, const QColor &color)
{
    QPalette palette = button->palette();
    palette.setColor(QPalette::Button, color);
    button->setAutoFillBackground(true);
    button->setPalette(palette);
    button->update();
}

QColor MscConfigDialog::getButtonColor(QAbstractButton *button)
{
    return button->palette().color(QPalette::Button);
}

} // namespace qmsc