#include "qmsc_utils.h"
#include <QColor>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QSize>
#include <QStyleHints>
#include <QWidget>

namespace qmsc {

// TextPosition
bool TextPosition::isValid() const
{
    return _pos != -1 && _line != -1 && _col != -1;
}

QString TextPosition::value() const
{
    return QString("pos=%1 (%2-%3)").arg(_pos).arg(_line).arg(_col);
}

void TextPosition::set(int pos, int line, int col)
{
    _pos = pos;
    _line = line;
    _col = col;
}

void TextPosition::set(unsigned int pos, unsigned int line, unsigned int col)
{
    set((int) pos, (int) line, (int) col);
}

// DrawOptions
DrawOptions::DrawOptions()
    : _colorSet(false)
    , _bkColorSet(false)
    , _bFontBold(false)
    , _bFontUnderline(false)
    , _penStyle(Qt::PenStyle::SolidLine)
    , _brushStyle(Qt::BrushStyle::NoBrush)
    , _bgMode(Qt::BGMode::TransparentMode)
{
}

QColor DrawOptions::currentColor(const QPainter& painter)
{
    return painter.pen().color();
}

QColor DrawOptions::currentBkColor(const QPainter& painter)
{
    return painter.brush().color();
}

// clang-format off
DrawOptions& DrawOptions::setBrushStyle(Qt::BrushStyle brushStyle) { _brushStyle = brushStyle; return *this; }
DrawOptions& DrawOptions::setPenStyle(Qt::PenStyle penStyle) { _penStyle = penStyle; return *this; }
DrawOptions& DrawOptions::setColor(QColor color) { _color = color; _colorSet = true; return *this; }
DrawOptions& DrawOptions::setBkColor(QColor bkColor) { _bkColor = bkColor; _bkColorSet = true; return *this; }
DrawOptions& DrawOptions::setBkMode(Qt::BGMode mode) { _bgMode = mode; return *this; }
DrawOptions& DrawOptions::setClipHeader(const QRect &r) { _roundClip = r.adjusted(0, 0, 0, r.height()); return *this; }
DrawOptions& DrawOptions::setClipFifo(const QRect &r) { _fifoClip = r; return *this; }
DrawOptions& DrawOptions::setFont(bool bBold, bool bUnderline) { _bFontBold = bBold; _bFontUnderline = bUnderline; return *this; }
// clang-format on

// DrawContext
DrawContext::DrawContext(QPainter& painter, const DrawOptions& options)
    : _painter(painter)
{
    QPen pen = _painter.pen();
    pen.setStyle(options._penStyle);
    if (options._colorSet) {
        pen.setColor(options._color);
    }
    QBrush brush = _painter.brush();
    brush.setStyle(options._brushStyle);
    if (options._bkColorSet) {
        brush.setColor(options._bkColor);
    }
    QFont font = _painter.font();
    if (options._bFontBold) {
        font.setBold(options._bFontBold);
    }
    if (options._bFontUnderline) {
        font.setUnderline(options._bFontUnderline);
    }
    _painter.save();
    if (!options._roundClip.isEmpty()) {
        QPainterPath path;
        path.addRoundedRect(options._roundClip, 10, 10);
        _painter.setClipPath(path);
    } else if (!options._fifoClip.isEmpty()) {
        QPainterPath path;
        path.addRect(options._fifoClip.adjusted(0, 5, -1, -5));
        path.addEllipse(options._fifoClip.left(),
                        options._fifoClip.top(),
                        options._fifoClip.width() - 1,
                        10);
        path.addEllipse(options._fifoClip.left(),
                        options._fifoClip.bottom() - 10,
                        options._fifoClip.width() - 1,
                        10);
        _painter.setClipPath(path);
    }
    _painter.setBackgroundMode(options._bgMode);
    _painter.setPen(pen);
    _painter.setBrush(brush);
    _painter.setFont(font);
}

DrawContext::~DrawContext()
{
    _painter.restore();
}

QString getQString(const std::string_view& value, const std::string_view& defaultValue)
{
    std::string_view sv = value;
    if (sv.empty()) {
        sv = defaultValue;
    }
    return QString::fromUtf8(sv.data(), sv.size());
}

} // namespace qmsc