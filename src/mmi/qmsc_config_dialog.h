#pragma once
#include <QAbstractButton>
#include <QDialog>
#include <QFontComboBox>
#include <QSpinBox>
#include "qmsc_settings.h"

namespace Ui {
class QMscConfigDialog;
}

namespace qmsc {

class MscConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MscConfigDialog(QWidget *parent, MscSettings::Config &config);
    ~MscConfigDialog();

private slots:
    void accept() override;

private:
    void initFont(QFontComboBox *cbFont, QSpinBox *sbFontSize, const QFont &font, int szFont);
    void initColorButton(QAbstractButton *colorButton,
                         QAbstractButton *defaultButton,
                         const QColor &color,
                         const QColor &defaultColor);
    void setButtonColor(QAbstractButton *button, const QColor &color);
    QColor getButtonColor(QAbstractButton *button);

private:
    Ui::QMscConfigDialog *_ui;
    MscSettings::Config &_config;
    MscSettings::Config _defaultConfig;
};

} // namespace qmsc
