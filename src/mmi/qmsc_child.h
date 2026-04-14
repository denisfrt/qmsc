#pragma once

#include <QScrollArea>
#include <QSplitter>
#include <QTextEdit>
#include "qmsc_document.h"
#include "qmsc_edit.h"
#include "qmsc_graphics.h"

namespace qmsc {

class MscDrawWidget : public QWidget
{
    Q_OBJECT

public:
    MscDrawWidget(MscDocument *doc, QScrollArea *parent);
    virtual ~MscDrawWidget();
    int getScale() const;
    void setScale(int factor);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    QSize sizeHint() const override;

public:
    void setSelectionFromEdit(const TextPosition &textPos);

signals:
    void textSelChanged(const qmsc::TextPosition &textPos);
    void viewSelChanged(const QPoint &selPos, int yOffset);

private:
    GraphicMsc _graphics;
    MscDocument *_doc;
    QScrollArea *_parent;
    QSize _size;
    int _scale;
};

class MscChild : public QSplitter
{
    Q_OBJECT

public:
    explicit MscChild(MscDocument *doc, QWidget *parent = nullptr);
    virtual ~MscChild();
    MscDocument *document() const;
    bool save(bool bForceSaveAs = false);
    void activated();

private slots:
    void onDocTextChanged(QObject *source);
    void onDocOptionChanged(int modifiedOptions);
    void onDocModificationChanged(bool changed);
    void onEditorTextChanged();
    void onCursorChanged(const qmsc::TextPosition &pos);
    void onTextSelChanged(const qmsc::TextPosition &pos);
    void onViewSelChanged(const QPoint &pos, int yOffset);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    bool _updatingText;
    bool _updatingTextSel;
    MscDocument *_doc;
    MscDrawWidget *_drawWidget;
    MscTextEdit *_textWidget;
    QScrollArea *_drawScroll;
};

} // namespace qmsc
