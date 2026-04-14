#pragma once
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QPolygon>
#include <QRect>
#include <QWidget>
#include "ast.h"
#include "qmsc_document.h"
#include "qmsc_settings.h"
#include "qmsc_utils.h"

namespace qmsc {

enum eGraphicFlags : int {
    eFlagNone = 0,
    eAlignLeft = 0x01,
    eAlignRight = 0x02,
    eFontBold = 0x04,
    eFontItalic = 0x08,
    eFontUnderline = 0x10,
    eVCenter = 0x20,
    eDrawRect = 0x40,
    eFillBackground = 0x80,
    eFillRect = 0x100,
    eRoundRect = 0x200,
    eClipHeader = 0x400,
    eClipFifo = 0x800
};

struct GraphicLine
{
    QColor _textColor;
    std::string_view _sv;
    int _flags = eGraphicFlags::eFlagNone;
};

struct GraphicText
{
    std::list<GraphicLine> _text;
    QColor _backgroundColor;
    QSize _size{0, 0};
};

struct GraphicSubBlock
{
    ast::Item *_ast = nullptr;
    GraphicText _title;
    std::string _textPrefix;
    int _height = 0;
};

struct GraphicComponent
{
    ast::Item *_ast = nullptr;
    QRect _boundOut;
};

struct GraphicInstance : GraphicComponent
{
    GraphicText _main;
    int _xPos = 0;
};

struct GraphicElement : GraphicComponent
{
    int _xFrom = 0;
    int _xTo = 0;
    int _yPos = 0;
    QRect _boundIn;
    GraphicText _main;
    GraphicText _sub;
    GraphicText _anno[2];
};

struct GraphicBlock : GraphicComponent
{
    int _depth = 0;
    std::list<GraphicSubBlock> _sub_blocks;
};

class GraphicMsc
{
public:
    using Span = std::pair<std::optional<int>, std::optional<int>>;

public:
    explicit GraphicMsc(MscDocument *doc);
    void reset();
    void test() const;
    void setOptions(const QSize &viewSize, const QSize &scrollPos, int scale);
    void visitAndDraw(QWidget *view);

public:
    const QRect &bounds() const;
    const MscSettings::Config &cfg() const;
    Selection &selection();
    bool setSelection(const QPoint &p, bool bDblClick = false);
    bool setSelection(const TextPosition &textPos);
    int getLockInstanceY() const;
    QSize getComputedSize() const;

private:
    void visit(ast::Msc &);
    void visit(ast::Instance &);

    QRect visitElements(std::list<ast::Element> &);
    QRect visitElement(ast::Element &);
    QRect visitReference(ast::Element &,
                         std::optional<std::string_view> ref_name,
                         std::optional<std::reference_wrapper<ast::Msc>> ref_msc);
    QRect visitReferenceBlock(ast::Element &,
                              std::optional<std::string_view> ref_name,
                              std::optional<std::reference_wrapper<ast::Msc>> ref_msc);
    QRect visitBlock(ast::Element &);
    enum eSBlockFlag : int {
        eFlagFirst = 0x01,
        eFlagOpt = 0x02,
        eFlagAlt = 0x04,
        eFlagDefault = 0x08,
        eFlagWhile = 0x10,
        eFlagReference = 0x20,
        eFlagRefInline = 0x40
    };
    QRect visitSubBlock(GraphicBlock &gblock,
                        const std::string_view &title,
                        ast::Block &sblock,
                        int flag);

private:
    enum eExpandFlag : int { eExpandWidth = 0x01, eExpandHeight = 0x02 };
    QRect &expandTo(QRect &r, const QSize &s, int flag = eExpandWidth | eExpandHeight);
    static int getTextFlag(const GraphicLine &line);
    bool parseMultiline(const std::string_view &text, GraphicText &gtext) const;
    void parseLine(const std::string_view &line, GraphicText &gtext) const;
    bool parseLineCommand(const std::string_view &line,
                          size_t &pos,
                          GraphicText &gtext,
                          GraphicLine &gline) const;
    bool parseLineColor(const std::string_view &line, size_t &pos, QColor &color) const;
    void skipLineSpaces(const std::string_view &line, size_t &pos) const;
    QSize computeTextSize(std::list<GraphicLine> &lines, int minHeight = 0);
    QRect componentBoundsWithYMargin(const GraphicComponent &gcomp);
    Span getElementSpan(ast::Element &element, bool bKeepOrder = false);
    Span getElementSpan(std::list<ast::Element> &elements);
    std::optional<std::reference_wrapper<GraphicInstance>> findInstanceByName(
        const std::string_view &name);
    std::optional<std::reference_wrapper<ast::Msc>> //
    findMscByName(const std::string_view &name, bool clean = false);
    std::optional<std::reference_wrapper<GraphicBlock>> findBlock(const QPoint &, int depth = 1);
    void computeElementXBoundWithAnnos();
    ast::Item *findSelected(const TextPosition &textPos, QRect &rMatchSelection, bool bInstanceOnly);
    ast::Item *findSelected(const QPoint &p, QRect &rMatchSelection, bool bInstanceOnly);
    ast::Reference *findReference(const QPoint &p, bool bCheckRedux);
    void drawArrow(int x1, int x2, int y, bool isDash = false);
    void drawText(const GraphicText &gtext,
                  const QRect &bounds,
                  int flags = eGraphicFlags::eFlagNone);
    QRect getRedux(const QPoint &p, const QSize &margin = QSize(0, 0)) const;
    void drawRedux(const QPoint &p, bool isPlus);
    void drawRect(const QRect &bounds,
                  const DrawOptions &options = DrawOptions(),
                  bool bRound = false);
    void drawLine(const QPolygon &points, const DrawOptions &options = DrawOptions());
    void drawInstance(const GraphicInstance &instance, bool drawRect);
    void drawAnno(const GraphicElement &element, int index);
    void drawElement(const GraphicElement &element);
    void drawBlock(const GraphicBlock &block);
    QTransform getTransform(bool bLock = false) const;
    void draw();

private:
    std::list<GraphicInstance> _instances;
    std::list<GraphicElement> _elements;
    std::list<GraphicBlock> _blocks;
    MscDocument *_doc;
    QPainter _painter;
    int _current_depth;
    int _currentY;
    // view options
    QSize _viewSize;
    QSize _scrollBarPos;
    qreal _scale;
    // msc bounds
    QRect _bounds;
    QRect _instanceBounds;
    // selection
    Selection _selection;
    QRect _selectedRegion;
};

} // namespace qmsc
