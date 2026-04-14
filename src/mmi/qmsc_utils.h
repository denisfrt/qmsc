#pragma once

#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QPoint>
#include <QRgb>
#include <QSize>
#include <QString>

namespace qmsc {

QString getQString(const std::string_view &value, const std::string_view &defaultValue = "");

struct TextPosition
{
    int _pos = -1;
    int _line = -1;
    int _col = -1;
    bool isValid() const;
    QString value() const;
    void set(int pos, int line, int col);
    void set(unsigned int pos, unsigned int line, unsigned int col);
};

class DrawOptions
{
    friend class DrawContext;

public:
    explicit DrawOptions();
    static QColor currentColor(const QPainter &painter);
    static QColor currentBkColor(const QPainter &painter);
    DrawOptions &setBrushStyle(Qt::BrushStyle brushStyle);
    DrawOptions &setPenStyle(Qt::PenStyle penStyle);
    DrawOptions &setColor(QColor color);
    DrawOptions &setBkColor(QColor bkColor);
    DrawOptions &setBkMode(Qt::BGMode mode);
    DrawOptions &setClipHeader(const QRect &r);
    DrawOptions &setClipFifo(const QRect &r);
    DrawOptions &setFont(bool bBold, bool bUnderline);

private:
    bool _colorSet;
    bool _bkColorSet;
    bool _bFontBold;
    bool _bFontUnderline;
    Qt::PenStyle _penStyle;
    Qt::BrushStyle _brushStyle;
    Qt::BGMode _bgMode;
    QColor _color;
    QColor _bkColor;
    QRect _roundClip;
    QRect _fifoClip;
};

// Change current QPainter pen and brush style
// original pen and brush style is restored on destruction
class DrawContext
{
public:
    explicit DrawContext(QPainter &painter, const DrawOptions &options);
    virtual ~DrawContext();

private:
    QPainter &_painter;
};

struct Selection
{
    enum class eSource : int { eNone = 0, eFromView, eFromEdit };
    eSource _source = eSource::eNone;
    QPoint _selectedPoint{-1, -1};
    TextPosition _selectedCursorPosition;
};

} // namespace qmsc