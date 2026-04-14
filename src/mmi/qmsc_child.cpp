#include "qmsc_child.h"
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include "qmsc_edit.h"
#include "qmsc_graphics.h"
#include "qmsc_main_window.h"

namespace qmsc {

// MscDrawWidget
MscDrawWidget::MscDrawWidget(MscDocument *doc, QScrollArea *parent)
    : QWidget(parent)
    , _graphics(doc)
    , _doc(doc)
    , _parent(parent)
    , _size(100000, 100000)
    , _scale(100)
{
    setMouseTracking(true);
}

MscDrawWidget::~MscDrawWidget() {}

int MscDrawWidget::getScale() const
{
    return _scale;
}

void MscDrawWidget::setScale(int factor)
{
    int scale = _scale;
    if (!factor) {
        scale = 100;
    } else {
        scale = std::max(10, scale + factor);
    }
    if (scale != _scale) {
        _scale = scale;
        MscMainWindow::ptr()->updateDrawScale(_scale);
        update();
    }
}

QSize MscDrawWidget::sizeHint() const
{
    return _size;
}

void MscDrawWidget::paintEvent(QPaintEvent *)
{
    //qDebug() << "MscDrawWidget::paintEvent ";
    QSize scrollPos{_parent->horizontalScrollBar()->value(), _parent->verticalScrollBar()->value()};
    _graphics.setOptions(parentWidget()->size(), scrollPos, _scale);
    _graphics.visitAndDraw(this);

    // update size
    QSize computedSize = _graphics.getComputedSize();
    if (computedSize != _size) {
        _size = computedSize;
        adjustSize();
    }

    // update text selection from the view selection
    if (_graphics.selection()._source == Selection::eSource::eFromView) {
        //qDebug() << "update text selection from view";
        _graphics.selection()._source = Selection::eSource::eNone;
        emit textSelChanged(_graphics.selection()._selectedCursorPosition);
    }
    // update view from the edit selection
    if (_graphics.selection()._source == Selection::eSource::eFromEdit) {
        //qDebug() << "update view selection from edit:" << _graphics.selection()._selectedPoint;
        _graphics.selection()._source = Selection::eSource::eNone;
        int yOffset = _graphics.cfg()._lockInstances ? _graphics.getLockInstanceY() : 0;
        emit viewSelChanged(_graphics.selection()._selectedPoint, yOffset);
    }
}

void MscDrawWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        int factor = (event->angleDelta().y() > 0) ? 10 : -10;
        setScale(factor);
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}

void MscDrawWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (_doc->config()._debug) {
        MscMainWindow::ptr()->setStatus(
            QString("%1-%2").arg(event->pos().x()).arg(event->pos().y()));
    }
    QWidget::mouseMoveEvent(event);
}

void MscDrawWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    //qDebug() << "dblclick: " << event->button() << " pos:" << event->pos();
    if (event->button() == Qt::MouseButton::LeftButton) {
        if (_graphics.setSelection(event->pos(), true)) {
            repaint();
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

void MscDrawWidget::mousePressEvent(QMouseEvent *event)
{
    //qDebug() << "click: " << event->button() << " pos:" << event->pos();
    if (event->button() == Qt::MouseButton::LeftButton) {
        if (_graphics.setSelection(event->pos())) {
            repaint();
        }
    }
    QWidget::mousePressEvent(event);
}

void MscDrawWidget::setSelectionFromEdit(const TextPosition &textPos)
{
    _graphics.setSelection(textPos);
}

// MscDrawScroll
class MscDrawScroll : public QScrollArea
{
public:
    explicit MscDrawScroll(QWidget *parent = nullptr)
        : QScrollArea(parent)
    {}

protected:
    void scrollContentsBy(int dx, int dy) override
    {
        QScrollArea::scrollContentsBy(dx, dy);
        widget()->update();
    }
};

// MscChild
MscChild::MscChild(MscDocument *doc, QWidget *parent)
    : QSplitter(Qt::Vertical, parent)
    , _updatingText(false)
    , _updatingTextSel(false)
    , _doc(doc)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(doc->title());
    setWindowModified(_doc->isModified());

    _drawScroll = new MscDrawScroll;
    _drawWidget = new MscDrawWidget(doc, _drawScroll);
    _drawScroll->setWidget(_drawWidget);
    _drawScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _drawScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    _textWidget = new MscTextEdit(doc, this);

    addWidget(_drawScroll);
    addWidget(_textWidget);

    // Initialize from document
    _textWidget->setText(_doc->text());

    // Connect document -> text view
    connect(_doc, &MscDocument::docTextChanged, this, &MscChild::onDocTextChanged);
    connect(_doc, &MscDocument::optionChanged, this, &MscChild::onDocOptionChanged);
    connect(_doc, &MscDocument::modificationChanged, this, &MscChild::onDocModificationChanged);
    // Connect text view -> document
    connect(_textWidget, &MscTextEdit::textChanged, this, &MscChild::onEditorTextChanged);
    connect(_textWidget, &MscTextEdit::cursorChanged, this, &MscChild::onCursorChanged);
    // Connect draw view -> text view
    connect(_drawWidget, &MscDrawWidget::textSelChanged, this, &MscChild::onTextSelChanged);
    connect(_drawWidget, &MscDrawWidget::viewSelChanged, this, &MscChild::onViewSelChanged);

    onDocTextChanged(nullptr);
}

MscChild::~MscChild()
{
    delete _doc;
}

MscDocument *MscChild::document() const
{
    return _doc;
}

bool MscChild::save(bool bForceSaveAs)
{
    QString filename = document()->filename();
    if (filename.isEmpty() || !document()->isNativeFile() || bForceSaveAs) {
        filename = QFileDialog::getSaveFileName(this,
                                                tr("Save Flowchart"),
                                                QString(),
                                                tr("Flowchart Files %1").arg("(*.msc)"));
        if (filename.isEmpty()) {
            return false;
        }
        if (!filename.endsWith(".msc")) {
            filename += ".msc";
        }
    }
    bool r = document()->save(filename);
    setWindowTitle(document()->title());
    setWindowModified(document()->isModified());
    return r;
}

void MscChild::activated()
{
    // update status bar
    MscMainWindow::ptr()->updateLineCol(_textWidget->getCursorPosition());
    MscMainWindow::ptr()->updateParseStatus(_doc->errors().size());
    MscMainWindow::ptr()->updateDrawScale(_drawWidget->getScale());
}

void MscChild::onDocTextChanged(QObject *source)
{
    //qDebug() << "MscChild:onDocTextChanged updating=" << _updatingText;
    _updatingText = true;
    if (source != this) {
        _textWidget->setTextContent(_doc->text());
    }
    _drawWidget->setSelectionFromEdit(_textWidget->getCursorPosition());
    _drawWidget->update();
    setWindowModified(document()->isModified());
    // update status bar
    MscMainWindow::ptr()->updateLineCol(_textWidget->getCursorPosition());
    MscMainWindow::ptr()->updateParseStatus(_doc->errors().size());
    _updatingText = false;
}

void MscChild::onDocOptionChanged(int options)
{
    //qDebug() << "options:" << options << " changed";
    if (options
        & (MscSettings::eDebug | MscSettings::eCenterMsc | MscSettings::eLockInstances
           | MscSettings::eColorSelection | MscSettings::eMscFont)) {
        _drawWidget->repaint();
    }
}

void MscChild::onDocModificationChanged(bool changed)
{
    //qDebug() << "onDocModificationChanged:" << changed;
    setWindowModified(changed);
    MscMainWindow::ptr()->updateMenus();
}

void MscChild::onEditorTextChanged()
{
    //qDebug() << "MscChild:onEditorTextChanged updating=" << _updatingText;
    if (_updatingText)
        return;
    _doc->setText(_textWidget->toPlainText(), this);
}

void MscChild::onCursorChanged(const TextPosition &pos)
{
    // update status bar
    MscMainWindow::ptr()->updateLineCol(pos);

    if (_updatingTextSel)
        return;
    _updatingTextSel = true;

    // Set current cursor selection in view
    _drawWidget->setSelectionFromEdit(pos);
    _drawWidget->update();

    _updatingTextSel = false;
}

void MscChild::onTextSelChanged(const TextPosition &pos)
{
    //qDebug() << "MscChild:onTextSelChanged updating=" << _updatingTextSel << pos.value();
    if (_updatingTextSel)
        return;
    if (pos.isValid()) {
        _updatingTextSel = true;
        _textWidget->setCursorPosition(pos, true);
        _updatingTextSel = false;
    }
}

void MscChild::onViewSelChanged(const QPoint &pos, int yOffset)
{
    // Scroll to make selection visible
    if (!pos.isNull()) {
        _drawScroll->ensureVisible(pos.x(), pos.y(), 50, 50 + yOffset);
    }
}

void MscChild::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        int factor = (event->angleDelta().y() > 0) ? 10 : -10;
        _drawWidget->setScale(factor);
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}

void MscChild::closeEvent(QCloseEvent *event)
{
    bool bAccept = true;
    if (document()->isModified()) {
        auto ret = QMessageBox::warning(
            this,
            tr("Unsaved changes"),
            tr("The document has been modified.\nDo you want to save your changes?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);
        switch (ret) {
        case QMessageBox::Save:
            bAccept = save();
            break;
        case QMessageBox::Discard:
            break;
        case QMessageBox::Cancel:
        default:
            bAccept = false;
            break;
        }
    }
    if (bAccept) {
        event->accept();
    } else {
        event->ignore();
    }
    //QSplitter::closeEvent(event);
}

} // namespace qmsc
