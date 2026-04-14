#include "qmsc_graphics.h"
#include <QDebug>
#include <QPolygon>
#include <QWidget>
#include "qmsc_document.h"
#include "qmsc_settings.h"
#include <limits>

namespace qmsc {

GraphicMsc::GraphicMsc(MscDocument *doc)
    : _doc(doc)
    , _current_depth(0)
    , _currentY(0)
    , _viewSize{0, 0}
    , _scrollBarPos{0, 0}
    , _scale(1.0)
{}

void GraphicMsc::reset()
{
    _current_depth = 0;
    _currentY = 0;
    _instances.clear();
    _elements.clear();
    _blocks.clear();
    _bounds = QRect();
    _instanceBounds = QRect();
    _selectedRegion = QRect();
}

const QRect &GraphicMsc::bounds() const
{
    return _bounds;
}

const MscSettings::Config &GraphicMsc::cfg() const
{
    return _doc->config();
}

Selection &GraphicMsc::selection()
{
    return _selection;
}

void GraphicMsc::setOptions(const QSize &viewSize, const QSize &scrollPos, int scale)
{
    _scale = scale / 100.;
    _viewSize = viewSize / _scale;
    _scrollBarPos = scrollPos / _scale;
}

void GraphicMsc::visitAndDraw(QWidget *view)
{
    _painter.begin(view);
    _painter.setPen(QPen(view->palette().color(view->foregroundRole())));
    _painter.setBrush(QBrush(view->palette().color(view->backgroundRole())));
    _painter.setRenderHint(QPainter::Antialiasing, true);
    _painter.setFont(cfg()._mscFont);

    reset();

    // Visit 1st msc only
    _currentY = 0;
    if (!_doc->mutable_ast()._mscs.empty()) {
        visit(_doc->mutable_ast()._mscs.front());
    }

    // Handle anno placement
    computeElementXBoundWithAnnos();

    // Compute Msc bounds with instances
    for (auto const &instance : _instances) {
        _bounds = _bounds.united(instance._boundOut);
    }
    _instanceBounds = _bounds;

    // Compute Msc bounds with elements/blocks
    for (auto const &element : _elements) {
        _bounds = _bounds.united(element._boundOut);
    }

    for (auto const &block : _blocks) {
        _bounds = _bounds.united(block._boundOut);
    }

    // add margin to bottom
    _bounds.setBottom(_bounds.bottom() + cfg()._instanceSize.height());

    // draw
    draw();
    _painter.end();
}

bool GraphicMsc::setSelection(const QPoint &p, bool bDblClick)
{
    bool bNeedRepaint = false;
    if (p != _selection._selectedPoint || bDblClick) {
        QPoint newSelection = getTransform().inverted().map(p);
        // Expand/Collapse reference
        if (auto *ref = findReference(newSelection, !bDblClick)) {
            //qDebug() << "found ref: " << ref->get().flag << " dbl:" << bDblClick;
            if (ref->_flags & ast::eFlag::eRefExpanded) {
                ref->_flags &= ~ast::eFlag::eRefExpanded;
                bNeedRepaint = true;
            } else if (ref->_flags & ast::eFlag::eRefFound) {
                ref->_flags |= ast::eFlag::eRefExpanded;
                bNeedRepaint = true;
            }
        }
        bNeedRepaint |= (_selection._selectedPoint != newSelection);
        _selection._selectedPoint = newSelection;
        _selection._source = Selection::eSource::eFromView;
    }
    return bNeedRepaint;
}

bool GraphicMsc::setSelection(const TextPosition &textPos)
{
    _selection._selectedCursorPosition = textPos;
    _selection._source = Selection::eSource::eFromEdit;
    return true;
}

int GraphicMsc::getLockInstanceY() const
{
    return _instanceBounds.bottom() + cfg()._elementMargin.height();
}

QSize GraphicMsc::getComputedSize() const
{
    QSize sz = _bounds.size();
    if (cfg()._centerMsc) {
        int xOffset = cfg()._offset.width();
        int xRightWidth = _instanceBounds.width() / 2 + _bounds.right() - _instanceBounds.right()
                          + xOffset;
        int xLeftWidth = _instanceBounds.width() / 2 - _bounds.left() + xOffset;
        if (xRightWidth + xLeftWidth < _viewSize.width()) {
            sz.setWidth(std::max(_viewSize.width() / 2, xLeftWidth)
                        + std::max(_viewSize.width() / 2, xRightWidth));
        } else {
            sz.setWidth(xLeftWidth + xRightWidth);
        }
        sz.setHeight(_bounds.height() + 2 * cfg()._offset.height());
    } else {
        sz += 2 * cfg()._offset;
    }
    return sz * _scale;
}

void GraphicMsc::visit(ast::Msc &msc)
{
    for (auto &inst : msc._instances) {
        visit(inst);
    }
    visitElements(msc._elements);
}

void GraphicMsc::visit(ast::Instance &inst)
{
    auto &g = _instances.emplace_back();
    g._ast = &inst;

    // default bound rect
    g._boundOut.setRect((_instances.size() - 1) * cfg()._instanceXSpace,
                        0,
                        cfg()._instanceSize.width(),
                        cfg()._instanceSize.height());

    // instance position (middle)
    g._xPos = g._boundOut.left() + g._boundOut.width() / 2;

    // adjust bounds to name content
    if (auto name = ast::getPropertyValueT(inst, ast::eTokenType::eName)) {
        parseMultiline(*name, g._main);
        g._main._size = computeTextSize(g._main._text);
        expandTo(g._boundOut, g._main._size);
    }
    if (inst._flags & ast::eFlag::eInstanceShapeFIFO) {
        g._boundOut.setBottom(g._boundOut.bottom() + 20);
    }

    // adjust current Y
    _currentY = std::max(_currentY, g._boundOut.bottom());
    _currentY += cfg()._elementMargin.height();
}

QRect GraphicMsc::visitElements(std::list<ast::Element> &elements)
{
    QRect bounds;
    ++_current_depth;
    for (auto &e : elements) {
        QRect b;
        if (std::holds_alternative<ast::Text>(e) || std::holds_alternative<ast::Message>(e)) {
            b = visitElement(e);
        } else if (auto *ref = std::get_if<ast::Reference>(&e)) {
            // get reference name and find referenced msc
            auto ref_name = ast::getPropertyValue(e, ast::eTokenType::eName);
            std::optional<std::reference_wrapper<ast::Msc>> ref_msc = std::nullopt;
            if (ref_name) {
                ref_msc = findMscByName(*ref_name, true);
                // note reference msc destination was not found
                if (ref_msc) {
                    ref->_flags |= ast::eFlag::eRefFound;
                }
            }
            // expanded reference treated as a block
            if ((ref->_flags & (ast::eFlag::eRefExpanded | ast::eFlag::eRefInline))
                && (ref->_flags & ast::eFlag::eRefFound)) {
                if (ref->_flags & ast::eFlag::eRefInline)
                    --_current_depth;
                b = visitReferenceBlock(e, ref_name, ref_msc);
                if (ref->_flags & ast::eFlag::eRefInline)
                    ++_current_depth;
            } else {
                b = visitReference(e, ref_name, ref_msc);
            }
        } else if (std::holds_alternative<ast::Block>(e)
                   || std::holds_alternative<ast::Condition>(e)) {
            b = visitBlock(e);
        }
        bounds = bounds.united(b);
    }
    --_current_depth;
    return bounds;
}

QRect GraphicMsc::visitElement(ast::Element &element)
{
    auto &g = _elements.emplace_back();
    g._ast = ast::getElement(&element);

    // default bound rect
    g._boundIn.setRect(0, _currentY, cfg()._instanceSize.width(), cfg()._instanceSize.height());

    // adapt to defined instances
    if (!_instances.empty()) {
        g._boundIn.setLeft(_instances.front()._xPos);
        g._boundIn.setRight(_instances.back()._xPos);
    }

    // adapt bound rect to from and to
    auto span = getElementSpan(element, true);
    g._xFrom = span.first ? *span.first : g._boundIn.left();
    g._xTo = span.second ? *span.second : g._boundIn.right();
    g._boundIn.setLeft(g._xTo < g._xFrom ? g._xTo : g._xFrom);
    g._boundIn.setRight(g._xTo < g._xFrom ? g._xFrom : g._xTo);

    // Enlarge width by 1/4 instance width
    if (std::holds_alternative<ast::Text>(element)) {
        g._boundIn.setLeft(g._boundIn.left() - cfg()._instanceSize.width() / 4);
        g._boundIn.setRight(g._boundIn.right() + cfg()._instanceSize.width() / 4);
    }

    // name content
    if (auto name = ast::getPropertyValue(element, ast::eTokenType::eName)) {
        parseMultiline(*name, g._main);
        g._main._size = computeTextSize(g._main._text);
    }

    // subtext content
    if (auto sub = ast::getPropertyValue(element, ast::eTokenType::eSubtext)) {
        parseMultiline(*sub, g._sub);
        g._sub._size = computeTextSize(g._sub._text);
    }

    // Take subtext size & arrow height into account for messages
    QSize textSize{g._main._size};
    int yArrowHeight = 0;
    if (std::holds_alternative<ast::Message>(element)) {
        if (g._sub._size.width()) {
            textSize.setWidth(g._sub._size.width());
        }
        textSize.setHeight(textSize.height() + g._sub._size.height());
        if (g._xFrom != g._xTo) {
            yArrowHeight = cfg()._yArrowHeight;
            textSize.setHeight(textSize.height() + yArrowHeight);
        }
    }
    expandTo(g._boundIn, textSize);
    g._boundOut = g._boundIn;

    // handle anno placement
    int annoIndex[2] = {0, 1};
    if (g._ast->_flags & (ast::eFlag::eAnno1 | ast::eFlag::eAnno2)) {
        bool bSwap = (g._ast->_flags & ast::eFlag::eAnno2Left)
                     || (g._ast->_flags & ast::eFlag::eAnno1Right)
                     || ((g._xFrom < g._xTo) && (g._ast->_flags & ast::eFlag::eAnno2From))
                     || ((g._xTo < g._xFrom) && (g._ast->_flags & ast::eFlag::eAnno2To))
                     || ((g._xFrom < g._xTo) && (g._ast->_flags & ast::eFlag::eAnno1To))
                     || ((g._xTo < g._xFrom) && (g._ast->_flags & ast::eFlag::eAnno1From));
        if (bSwap) {
            std::swap(annoIndex[0], annoIndex[1]);
        }
    }
    // adjust height bounds to anno content
    for (std::size_t i = 0; i < sizeof annoIndex / sizeof *annoIndex; ++i) {
        if (auto anno = ast::getPropertyValue(element, ast::eTokenType::eAnno, annoIndex[i])) {
            parseMultiline(*anno, g._anno[i]);
            g._anno[i]._size = computeTextSize(g._anno[i]._text);
            expandTo(g._boundOut, g._anno[i]._size, eExpandFlag::eExpandHeight);
        }
    }

    // set y-position of message arrow or text y-center
    g._yPos = _currentY;
    if (g._boundOut.height() > g._boundIn.height()) {
        // centered on max anno height
        g._yPos = _currentY + (g._boundOut.height() / 2) - (textSize.height() / 2);
    }
    g._boundIn.moveTop(g._yPos);
    if (std::holds_alternative<ast::Text>(element)) {
        g._yPos += g._boundIn.height() / 2;
    } else {
        // under main text
        g._yPos += g._main._size.height() + yArrowHeight / 2;
    }

    // adjust current Y
    _currentY = g._boundOut.bottom() + cfg()._elementMargin.height();
    return componentBoundsWithYMargin(g);
}

QRect GraphicMsc::visitReference(ast::Element &element,
                                 std::optional<std::string_view> ref_name,
                                 std::optional<std::reference_wrapper<ast::Msc>> ref_msc)
{
    auto &g = _elements.emplace_back();
    g._ast = ast::getElement(&element);

    // default bound rect
    g._boundOut.setRect(0, _currentY, cfg()._instanceXSpace, cfg()._instanceSize.height());

    // adapt to defined instances
    if (!_instances.empty()) {
        g._boundOut.setLeft(_instances.front()._xPos);
        g._boundOut.setRight(_instances.back()._xPos);
    }
    g._xFrom = g._boundOut.left();
    g._xTo = g._boundOut.right();

    // Get referenced msc name
    if (ref_name) {
        // adjust bounds to referenced msc tasks
        if (ref_msc) {
            auto span = getElementSpan(ref_msc->get()._elements);
            g._xFrom = span.first ? *span.first : g._xFrom;
            g._xTo = span.second ? *span.second : g._xTo;
            g._boundOut.setLeft(g._xFrom);
            g._boundOut.setRight(g._xTo);
        }

        // adjust bounds to name content
        parseMultiline(*ref_name, g._main);
        g._main._size = computeTextSize(g._main._text);
        // at least name content + redux width
        expandTo(g._boundOut, g._main._size + QSize(cfg()._refReduxMargin.width(), 2));
        // at least redux size
        expandTo(g._boundOut, cfg()._refReduxMargin);
    }

    // adjust reference bounds
    g._boundOut.setLeft(g._boundOut.left() - cfg()._elementMargin.width());
    g._boundOut.setRight(g._boundOut.right() + cfg()._elementMargin.width());
    g._boundOut.setBottom(g._boundOut.bottom() + cfg()._elementMargin.height());
    g._boundIn = g._boundOut;
    // adjust current Y
    _currentY = g._boundOut.bottom() + cfg()._elementMargin.height();
    return componentBoundsWithYMargin(g);
}

QRect GraphicMsc::visitReferenceBlock(ast::Element &element,
                                      std::optional<std::string_view> ref_name,
                                      std::optional<std::reference_wrapper<ast::Msc>> ref_msc)
{
    auto &gblock = _blocks.emplace_back();
    gblock._ast = ast::getElement(&element);
    gblock._depth = _current_depth;

    // default bound rect
    gblock._boundOut.setRect(0, _currentY, 0, 0);

    // visit referenced msc
    if (ref_msc) {
        int flag = eSBlockFlag::eFlagFirst | eSBlockFlag::eFlagReference;
        if (gblock._ast->_flags & ast::eFlag::eRefInline) {
            flag |= eSBlockFlag::eFlagRefInline;
        }
        visitSubBlock(gblock, *ref_name, ref_msc->get(), flag);
    }

    // adjust block bounds and update current Y
    gblock._boundOut.setBottom(_currentY);
    if (!(gblock._ast->_flags & ast::eFlag::eRefInline)) {
        gblock._boundOut.setLeft(gblock._boundOut.left() - cfg()._elementMargin.width());
        gblock._boundOut.setRight(gblock._boundOut.right() + cfg()._elementMargin.width());
        _currentY += cfg()._elementMargin.height();
        return componentBoundsWithYMargin(gblock);
    }
    return gblock._boundOut;
}

QRect GraphicMsc::visitBlock(ast::Element &element)
{
    auto &gblock = _blocks.emplace_back();
    gblock._ast = ast::getElement(&element);
    gblock._depth = _current_depth;

    // default bound rect
    gblock._boundOut.setRect(0, _currentY, 0, 0);

    // iterate over sub-blocks
    if (auto *cond = std::get_if<ast::Condition>(&element)) {
        int flag = eSBlockFlag::eFlagFirst;
        for (auto &sb : cond->_elements) {
            if (auto *block = std::get_if<ast::Block>(&sb)) {
                if (block->_type == ast::eTokenType::eIf) {
                    flag |= cond->_elements.size() == 1 ? eSBlockFlag::eFlagOpt
                                                        : eSBlockFlag::eFlagAlt;
                } else if (block->_type == ast::eTokenType::eElif) {
                    flag |= eSBlockFlag::eFlagAlt;
                } else if (block->_type == ast::eTokenType::eElse) {
                    flag |= eSBlockFlag::eFlagDefault;
                }
                visitSubBlock(gblock, block->_condition, *block, flag);
                flag = 0;
            }
        }
    } else if (auto *block = std::get_if<ast::Block>(&element)) {
        visitSubBlock(gblock,
                      block->_condition,
                      *block,
                      eSBlockFlag::eFlagWhile | eSBlockFlag::eFlagFirst);
    }

    // adjust block bounds and update current Y
    gblock._boundOut.setBottom(_currentY);
    gblock._boundOut.setLeft(gblock._boundOut.left() - cfg()._elementMargin.width());
    gblock._boundOut.setRight(gblock._boundOut.right() + cfg()._elementMargin.width());
    _currentY = gblock._boundOut.bottom() + cfg()._elementMargin.height();

    return componentBoundsWithYMargin(gblock);
}

QRect GraphicMsc::visitSubBlock(GraphicBlock &gblock,
                                const std::string_view &title,
                                ast::Block &sblock,
                                int flag)
{
    int currentAltY = _currentY;
    auto &gSubBlock = gblock._sub_blocks.emplace_back();
    gSubBlock._ast = &sblock;

    // Handle sub-block prefix
    if (flag & eSBlockFlag::eFlagOpt) {
        gSubBlock._textPrefix = "OPT: ";
    } else if (flag & eSBlockFlag::eFlagAlt) {
        gSubBlock._textPrefix = "ALT: ";
    } else if (flag & eSBlockFlag::eFlagWhile) {
        gSubBlock._textPrefix = "WHILE: ";
    } else if (flag & eSBlockFlag::eFlagDefault) {
        gSubBlock._textPrefix = "Default";
    }
    if (parseMultiline(title, gSubBlock._title)) {
        auto &line1 = gSubBlock._title._text.front();
        gSubBlock._textPrefix += line1._sv;
        line1._sv = gSubBlock._textPrefix;
    }

    // adjust y-bounds to title content (minimum height is _instanceSize/_refReduxMargin)
    gSubBlock._title._size = computeTextSize(gSubBlock._title._text);
    QSize szTitleText = gSubBlock._title._size;
    if (flag & eSBlockFlag::eFlagReference) {
        szTitleText.setWidth(szTitleText.width() + cfg()._refReduxMargin.width());
        szTitleText.setHeight(std::max(szTitleText.height(), cfg()._refReduxMargin.height()));
    } else {
        szTitleText.setHeight(std::max(szTitleText.height(), cfg()._instanceSize.height()));
    }
    gblock._boundOut.setBottom(gblock._boundOut.bottom() + szTitleText.height());

    // adjust current Y
    if (flag & eSBlockFlag::eFlagRefInline) {
        _currentY = currentAltY;
    } else {
        _currentY = gblock._boundOut.bottom() + cfg()._elementMargin.height();
    }

    // Visit condition sub-elements
    QRect bounds = visitElements(sblock._elements);

    // init block x/width to 1st condition elements
    if (flag & eSBlockFlag::eFlagFirst) {
        gblock._boundOut.setLeft(bounds.left());
        gblock._boundOut.setRight(bounds.right());
    }

    // adjust x-bounds to title content
    expandTo(gblock._boundOut, szTitleText, eExpandFlag::eExpandWidth);
    // adjust bounds to sub-elements
    if (flag & eSBlockFlag::eFlagRefInline) {
        gblock._boundOut = bounds;
        gblock._boundOut.setBottom(_currentY);
    } else {
        gblock._boundOut = gblock._boundOut.united(bounds);
        gblock._boundOut.setBottom(gblock._boundOut.bottom() + cfg()._elementMargin.height());
    }

    // set sub-block height
    gSubBlock._height = gblock._boundOut.bottom() - currentAltY;

    // adjust current Y
    _currentY = gblock._boundOut.bottom();
    return gblock._boundOut;
}

QRect &GraphicMsc::expandTo(QRect &r, const QSize &s, int flag)
{
    auto center = r.center();
    if (flag & eExpandFlag::eExpandWidth) {
        int dx = s.width();
        if (dx > r.width()) {
            r.setLeft(center.x() - dx / 2);
            r.setRight(center.x() + dx / 2);
        }
    }
    if (flag & eExpandFlag::eExpandHeight) {
        int dy = s.height();
        if (dy > r.height()) {
            r.setBottom(r.top() + dy);
        }
    }
    return r;
}

int GraphicMsc::getTextFlag(const GraphicLine &line)
{
    int r = Qt::TextFlag::TextSingleLine;
    if (line._flags & eAlignLeft) {
        r |= Qt::AlignLeft;
    } else if (line._flags & eAlignRight) {
        r |= Qt::AlignRight;
    } else {
        r |= Qt::AlignHCenter;
    }
    return r;
}

bool GraphicMsc::parseMultiline(const std::string_view &text, GraphicText &gtext) const
{
    std::size_t start = 0;
    gtext._backgroundColor = DrawOptions::currentBkColor(_painter);
    while (start <= text.size()) {
        std::string_view sub_str;
        size_t pos = text.find("\\n", start);
        if (pos == std::string_view::npos) {
            sub_str = text.substr(start);
            start = text.size() + 1;
        } else {
            sub_str = text.substr(start, pos - start);
            start = pos + 2;
        }
        parseLine(sub_str, gtext);
    }
    return !gtext._text.empty();
}

void GraphicMsc::parseLine(const std::string_view &line, GraphicText &gtext) const
{
    auto &gline = gtext._text.emplace_back();
    gline._textColor = DrawOptions::currentColor(_painter);
    std::size_t pos = 0;
    while (parseLineCommand(line, pos, gtext, gline)) {
        ;
    }
    gline._sv = line.substr(pos);
}

bool GraphicMsc::parseLineCommand(const std::string_view &line,
                                  size_t &pos,
                                  GraphicText &gtext,
                                  GraphicLine &gline) const
{
    int flag = eGraphicFlags::eFlagNone;
    int rflag = eGraphicFlags::eFlagNone;
    size_t p = pos;
    if (p < line.size() && (line[p] == '\\') && (++p < line.size())) {
        // clang-format off
        switch (line[p]) {
        case 'l': flag = eGraphicFlags::eAlignLeft; rflag = eGraphicFlags::eAlignRight; ++p; break;
        case 'r': flag = eGraphicFlags::eAlignRight; rflag = eGraphicFlags::eAlignLeft; ++p; break;
        case 'b': flag = eGraphicFlags::eFontBold; ++p; break;
        case 'i': flag = eGraphicFlags::eFontItalic; ++p; break;
        case 'u': flag = eGraphicFlags::eFontUnderline; ++p; break;
        case 'c': {
            if ((p + 1) < line.size() && ((line[p + 1] == 't') || (line[p + 1] == 'b'))) {
                pos = p + 2;
                skipLineSpaces(line, pos);
                if ((line[p + 1] == 'b')) {
                    parseLineColor(line, pos, gtext._backgroundColor);
                } else {
                    parseLineColor(line, pos, gline._textColor);
                }
                skipLineSpaces(line, pos);
                return true;
            }
            break;
        }
        default:
            break;
        }
        // clang-format on
        if (flag || rflag) {
            pos = p;
            gline._flags |= flag;
            gline._flags &= ~rflag;
            skipLineSpaces(line, pos);
            return true;
        }
    }
    return false;
}

bool GraphicMsc::parseLineColor(const std::string_view &line, size_t &pos, QColor &color) const
{
    bool status = false;
    int colorLen = 0, xdigit = 0;
    size_t p = pos;
    while (p < line.size()) {
        if (xdigit == 8) {
            break;
        } else if (std::isxdigit(line[p])) {
            ++xdigit;
        } else if (!std::isalpha(line[p])) {
            break;
        }
        ++p;
        ++colorLen;
    }
    if (colorLen) {
        std::string textColor;
        if (colorLen == xdigit) { // #AARRGGBB
            static constexpr std::string_view prefix = "#ff000000";
            if (colorLen <= 8) {
                textColor = prefix.substr(0, prefix.size() - colorLen);
            }
        }
        textColor += line.substr(pos, colorLen);
        QColor c = QColor::fromString(textColor);
        if (c != QColor::Invalid) {
            color = c;
            status = true;
        }
    }
    pos = p;
    return status;
}

void GraphicMsc::skipLineSpaces(const std::string_view &line, size_t &pos) const
{
    while ((pos < line.size()) && (line[pos] == ' ' || line[pos] == '\t')) {
        ++pos;
    }
}

QSize GraphicMsc::computeTextSize(std::list<GraphicLine> &lines, int minHeight)
{
    int textSizeX = 0, textSizeY = 0;
    for (auto &line : lines) {
        DrawContext ctx(_painter,
                        DrawOptions().setFont(line._flags & eFontBold,
                                              line._flags & eFontUnderline));
        QRect bounds = _painter.boundingRect(QRect(), Qt::TextSingleLine, getQString(line._sv, " "));
        textSizeX = std::max(bounds.width() + cfg()._textLineMargin.width() * 2, textSizeX);
        textSizeY += bounds.height() - 1 + cfg()._textLineMargin.height();
    }
    textSizeY += 2;
    return {textSizeX, std::max(textSizeY, minHeight)};
}

QRect GraphicMsc::componentBoundsWithYMargin(const GraphicComponent &gcomp)
{
    return gcomp._boundOut.adjusted(0, 0, 0, cfg()._elementMargin.height());
}

// returns span(min(a.first, b.first), max(a.second, b.second))
// we assume that first < second
GraphicMsc::Span &operator&=(GraphicMsc::Span &a, const GraphicMsc::Span &b)
{
    int max = std::max(a.second ? *a.second : std::numeric_limits<int>::min(),
                       b.second ? *b.second : std::numeric_limits<int>::min());
    int min = std::min(a.first ? *a.first : std::numeric_limits<int>::max(),
                       b.first ? *b.first : std::numeric_limits<int>::max());

    a.first.reset();
    a.second.reset();
    if (max != std::numeric_limits<int>::min()) {
        a.second = max;
    }
    if (min != std::numeric_limits<int>::max()) {
        a.first = min;
    }
    return a;
}

GraphicMsc::Span GraphicMsc::getElementSpan(ast::Element &element, bool bKeepOrder)
{
    GraphicMsc::Span r{std::nullopt, std::nullopt};
    if (std::holds_alternative<ast::Text>(element)
        || std::holds_alternative<ast::Message>(element)) {
        if (auto from = ast::getPropertyValue(element, ast::eTokenType::eFrom)) {
            if (auto fromI = findInstanceByName(*from)) {
                r.first = fromI->get()._xPos;
            }
        }
        if (auto to = ast::getPropertyValue(element, ast::eTokenType::eTo)) {
            if (auto toI = findInstanceByName(*to)) {
                r.second = toI->get()._xPos;
            }
        }
        if (!bKeepOrder && r.first && r.second && r.second < r.first) {
            std::swap(r.first, r.second);
        }
    } else if (auto *cond = std::get_if<ast::Condition>(&element)) {
        r &= getElementSpan(cond->_elements);
    } else if (auto *block = std::get_if<ast::Block>(&element)) {
        r &= getElementSpan(block->_elements);
    }
    return r;
}

GraphicMsc::Span GraphicMsc::getElementSpan(std::list<ast::Element> &elements)
{
    GraphicMsc::Span r{std::nullopt, std::nullopt};
    for (auto &element : elements) {
        r &= getElementSpan(element);
    }
    return r;
}

std::optional<std::reference_wrapper<GraphicInstance>> //
GraphicMsc::findInstanceByName(const std::string_view &name)
{
    std::optional<std::reference_wrapper<GraphicInstance>> r = std::nullopt;
    auto inst = std::find_if(_instances.begin(), _instances.end(), [&name](auto const &i) {
        return !i._main._text.empty() && i._main._text.front()._sv == name;
    });
    if (inst != _instances.end()) {
        r.emplace(*inst);
    }
    return r;
}

std::optional<std::reference_wrapper<ast::Msc>> //
GraphicMsc::findMscByName(const std::string_view &name, bool clean)
{
    std::optional<std::reference_wrapper<ast::Msc>> r = std::nullopt;
    std::string_view nameToFind{name};
    GraphicText gtext;
    if (clean) {
        // consider 1st line without commands for reference name
        parseLine(name.substr(0, name.find("\\n")), gtext);
        nameToFind = gtext._text.front()._sv;
    }
    auto &mscVector = _doc->mutable_ast()._mscs;
    for (auto msc = mscVector.begin(); msc != mscVector.end(); ++msc) {
        auto mscName = ast::getPropertyValueT(*msc, ast::eTokenType::eName);
        if (msc != mscVector.begin() && mscName && *mscName == nameToFind) {
            r.emplace(*msc);
            break;
        }
    }
    return r;
}

std::optional<std::reference_wrapper<GraphicBlock>> //
GraphicMsc::findBlock(const QPoint &pt, int depth)
{
    std::optional<std::reference_wrapper<GraphicBlock>> r = std::nullopt;
    auto blk = std::find_if(_blocks.begin(), _blocks.end(), [&pt, depth](auto const &b) {
        return b._depth == depth && b._boundOut.contains(pt);
    });
    if (blk != _blocks.end()) {
        r.emplace(*blk);
    }
    return r;
}

void GraphicMsc::computeElementXBoundWithAnnos()
{
    for (auto &element : _elements) {
        if (!element._anno[0]._size.isNull() || !element._anno[1]._size.isNull()) {
            int annoLPos = element._boundIn.left();
            int annoRPos = element._boundIn.right();
            if (auto blk = findBlock(element._boundIn.center())) {
                annoLPos = blk->get()._boundOut.left();
                annoRPos = blk->get()._boundOut.right();
            }
            if (!element._anno[0]._size.isNull()) {
                element._boundOut.setLeft(annoLPos - element._anno[0]._size.width()
                                          - cfg()._yAnnoMargin);
            }
            if (!element._anno[1]._size.isNull()) {
                element._boundOut.setRight(annoRPos + element._anno[1]._size.width()
                                           + cfg()._yAnnoMargin);
            }
        }
    }
}

static QRect getSelectionBounds(const GraphicComponent *gc, int y1 = -1, int y2 = -1)
{
    QRect r;
    if (gc) {
        if (gc->_ast->_type == ast::eTokenType::eInstance) {
            r = gc->_boundOut;
        } else {
            r = {QPoint(-100000, gc->_boundOut.top()), QPoint(100000, gc->_boundOut.bottom())};
        }
    } else {
        r = {QPoint(-100000, y1), QPoint(100000, y2)};
    }
    return r;
}

ast::Item *GraphicMsc::findSelected(const TextPosition &textPos,
                                    QRect &rMatchSelection,
                                    bool bInstanceOnly)
{
    QRect rDefaultSelection;
    ast::Item *matchItem = nullptr, *defaultItem = nullptr;

    // For elements, priorit)ize exact element match then line match
    auto checkSelection = [&](const GraphicComponent *gc) {
        if ((textPos._pos >= (int) gc->_ast->_pos)
            && (textPos._pos <= (int) (gc->_ast->_pos + gc->_ast->_len))) {
            rMatchSelection = getSelectionBounds(gc);
            matchItem = gc->_ast;
        } else if ((defaultItem == nullptr) && (textPos._line == (int) gc->_ast->_line)) {
            rDefaultSelection = getSelectionBounds(gc);
            defaultItem = gc->_ast;
        }
        return matchItem != nullptr;
    };
    // For block, prioritize line match then in block match
    auto checkBlockSelection = [&](ast::Item *ast, int y1, int y2) {
        if (textPos._line == (int) ast->_line) {
            rMatchSelection = getSelectionBounds(nullptr, y1, y2);
            matchItem = ast;
        } else if ((textPos._pos >= (int) ast->_pos)
                   && (textPos._pos <= (int) (ast->_pos + ast->_len))) {
            rDefaultSelection = getSelectionBounds(nullptr, y1, y2);
            defaultItem = ast;
        }
        return matchItem != nullptr;
    };

    // Search in graphic instances
    for (auto const &inst : _instances) {
        if (checkSelection(&inst)) {
            goto search_end;
        }
    }
    // Stop here if instance only search
    if (bInstanceOnly) {
        goto search_end;
    }

    // Search in graphic elements
    for (auto const &element : _elements) {
        if (checkSelection(&element)) {
            goto search_end;
        }
    }
    // Default to matching line component if found
    if (defaultItem != nullptr) {
        goto search_end;
    }

    // Finally search in blocks if nothing else matches
    for (auto const &block : _blocks) {
        int currentY = block._boundOut.top();
        if (checkBlockSelection(block._ast, currentY, block._boundOut.bottom())) {
            goto search_end;
        }
        std::size_t altIndex = 1;
        for (auto &sblk : block._sub_blocks) {
            int yBottom = altIndex == block._sub_blocks.size() ? block._boundOut.bottom()
                                                               : currentY + sblk._height - 1;
            if (checkBlockSelection(sblk._ast, currentY, yBottom)) {
                goto search_end;
            }
            currentY += sblk._height;
            ++altIndex;
        }
    }

search_end:
    // Default to matching line component if found
    if ((matchItem == nullptr) && (defaultItem != nullptr)) {
        rMatchSelection = rDefaultSelection;
        matchItem = defaultItem;
    }
    return matchItem;
}

ast::Item *GraphicMsc::findSelected(const QPoint &p, QRect &rMatchSelection, bool bInstanceOnly)
{
    ast::Item *item = nullptr;
    QRect rSelection;
    // search in instances
    for (auto const &instance : _instances) {
        if (instance._boundOut.contains(p)) {
            rSelection = getSelectionBounds(&instance, true);
            item = instance._ast;
            goto search_end;
        }
    }
    if (bInstanceOnly)
        goto search_end;

    // search in elements
    for (auto const &element : _elements) {
        if (element._boundOut.top() > p.y())
            break;
        if (p.y() >= element._boundOut.top() && p.y() <= element._boundOut.bottom()) {
            rSelection = getSelectionBounds(&element);
            item = element._ast;
            goto search_end;
        }
    }
    // search in block
    for (auto const &block : _blocks) {
        int currentY = block._boundOut.top();
        if (currentY > p.y())
            break;
        std::size_t altIndex = 1;
        for (auto const &sblk : block._sub_blocks) {
            int titleTextHeight = sblk._title._size.height();
            if (block._ast->_type != ast::eTokenType::eReference) {
                titleTextHeight = std::max(titleTextHeight, cfg()._instanceSize.height());
            }
            if (p.y() >= currentY && p.y() <= (currentY + titleTextHeight)) {
                int yBottom = altIndex == block._sub_blocks.size() ? block._boundOut.bottom()
                                                                   : currentY + sblk._height - 1;
                rSelection = getSelectionBounds(nullptr, currentY, yBottom);
                item = sblk._ast;
                goto search_end;
            }
            currentY += sblk._height;
            ++altIndex;
        }
    }
search_end:
    if (item) {
        rMatchSelection = rSelection;
    }
    return item;
}

ast::Reference *GraphicMsc::findReference(const QPoint &p, bool bCheckRedux)
{
    // search reference element
    for (auto const &element : _elements) {
        if (element._boundOut.top() > p.y())
            break;
        auto *ref = dynamic_cast<ast::Reference *>(element._ast);
        if (ref && (p.y() >= element._boundOut.top()) && (p.y() <= element._boundOut.bottom())) {
            // limit to redux rectangle if requested
            if (bCheckRedux) {
                QRect r = getRedux(element._boundOut.topLeft(), QSize(1, 1));
                if (!r.contains(p)) {
                    return nullptr;
                }
            }
            return ref;
        }
    }

    // search reference block
    for (auto const &block : _blocks) {
        int currentY = block._boundOut.top();
        if (currentY > p.y())
            break;
        if ((block._ast->_type == ast::eTokenType::eReference) && !block._sub_blocks.empty()) {
            int nextY = block._boundOut.bottom();
            // check p is in expanded reference title
            if (block._ast->_flags & ast::eFlag::eRefExpanded) {
                nextY = currentY + block._sub_blocks.front()._title._size.height();
            }
            if (p.y() >= currentY && p.y() <= nextY) {
                // limit to redux rectangle if requested
                if (bCheckRedux) {
                    QRect r = getRedux(block._boundOut.topLeft(), QSize(1, 1));
                    if (!r.contains(p)) {
                        return nullptr;
                    }
                }
                return dynamic_cast<ast::Reference *>(block._ast);
            }
        }
    }
    return nullptr;
}

void GraphicMsc::drawText(const GraphicText &gtext, const QRect &bounds, int flags)
{
    auto bkColor = gtext._backgroundColor;
    if (gtext._size.isNull())
        return;

    // Change background color if drawing in current selection and no custom background defined
    if ((flags & (eFillBackground | eDrawRect | eFillRect))
        && _selectedRegion.contains(bounds.center())
        && (bkColor == DrawOptions::currentBkColor(_painter))) {
        bkColor = cfg()._colorSelection;
    }

    QRect lBounds = bounds;
    // center in bounds
    QPoint mid = lBounds.center();
    if (flags & eGraphicFlags::eVCenter) {
        lBounds.setTop(mid.y() - gtext._size.height() / 2 + 1);
    }
    lBounds.setLeft(mid.x() - gtext._size.width() / 2 + cfg()._textLineMargin.width());
    lBounds.setRight(mid.x() + gtext._size.width() / 2 - cfg()._textLineMargin.width());

    // fill background text
    if (flags & eGraphicFlags::eFillBackground) {
        QRect background = lBounds;
        background.adjust(0, 1, 0, -1);
        drawRect(background,
                 DrawOptions()
                     .setBrushStyle(Qt::BrushStyle::SolidPattern)
                     .setBkColor(bkColor)
                     .setPenStyle(Qt::PenStyle::NoPen),
                 (flags & eRoundRect));
    }

    // draw bound rect
    if (flags & (eDrawRect | eFillRect)) {
        auto &options = DrawOptions() //
                            .setBrushStyle(Qt::BrushStyle::SolidPattern)
                            .setBkColor(bkColor);
        if (flags & eGraphicFlags::eClipHeader) {
            options.setClipHeader(bounds);
        } else if (flags & eGraphicFlags::eClipFifo) {
            options.setClipFifo(bounds);
        }
        if (flags & eGraphicFlags::eFillRect) {
            options.setPenStyle(Qt::PenStyle::NoPen);
        }
        drawRect(bounds, options, (flags & eRoundRect));
    }

    // draw lines
    for (auto &line : gtext._text) {
        auto &options = DrawOptions()
                            .setColor(line._textColor)
                            .setFont(line._flags & eFontBold, line._flags & eFontUnderline);
        DrawContext ctx(_painter, options);
        QRect computedLBounds;
        _painter.drawText(lBounds, getTextFlag(line), getQString(line._sv, " "), &computedLBounds);
        lBounds.setTop(computedLBounds.bottom() + cfg()._textLineMargin.height());
    }
}

void GraphicMsc::drawArrow(int x1, int x2, int y, bool isDash)
{
    QPolygon pts;
    if (x1 != x2) {
        int xWay = (x1 < x2) ? 1 : -1;
        pts << QPoint(x2 - xWay * 6, y - 4) << QPoint(x2 - xWay * 6, y - 4) //
            << QPoint(x2 - xWay, y) << QPoint(x2 - xWay * 6, y + 4);
        DrawContext ctx(_painter,
                        DrawOptions()
                            .setBrushStyle(Qt::BrushStyle::SolidPattern)
                            .setPenStyle(isDash ? Qt::DashLine : Qt::SolidLine));
        _painter.drawLine(x1, y, x2, y);
        if (isDash) {
            QPen pen = _painter.pen();
            pen.setStyle(Qt::SolidLine);
            _painter.setPen(pen);
        }
        _painter.drawPolygon(pts);
    }
}

QRect GraphicMsc::getRedux(const QPoint &p, const QSize &margin) const
{
    return {p.x() + 6, p.y() + 5, 8 + margin.width(), 8 + margin.height()};
}

void GraphicMsc::drawRedux(const QPoint &p, bool isPlus)
{
    QRect r = getRedux(p);
    _painter.drawRect(r);
    _painter.drawLine(p.x() + 8, p.y() + 9, p.x() + 12, p.y() + 9);
    if (isPlus) {
        _painter.drawLine(p.x() + 10, p.y() + 7, p.x() + 10, p.y() + 11);
    }
}

void GraphicMsc::drawRect(const QRect &bounds, const DrawOptions &options, bool bRound)
{
    DrawContext ctx(_painter, options);
    if (bRound) {
        _painter.drawRoundedRect(bounds, 10, 10);
    } else {
        _painter.drawRect(bounds);
    }
}

void GraphicMsc::drawLine(const QPolygon &points, const DrawOptions &options)
{
    DrawContext ctx(_painter, options);
    _painter.drawPolyline(points);
}

QTransform GraphicMsc::getTransform(bool bLock) const
{
    int xOffset = cfg()._offset.width();
    int xTranslation = xOffset - _bounds.left();
    int yTranslation = xOffset - _bounds.top();
    if (cfg()._centerMsc) {
        int xRightWidth = _instanceBounds.width() / 2 + _bounds.right() - _instanceBounds.right()
                          + xOffset;
        int xLeftWidth = _instanceBounds.width() / 2 - _bounds.left() + xOffset;
        if (xRightWidth + xLeftWidth < _viewSize.width()) {
            if (xRightWidth > _viewSize.width() / 2) {
                xTranslation = std::max(xTranslation,
                                        _viewSize.width() - _instanceBounds.width() / 2
                                            - xRightWidth);
            } else {
                xTranslation = std::max(xTranslation,
                                        _viewSize.width() / 2 - _instanceBounds.width() / 2);
            }
        }
    }
    if (bLock) {
        yTranslation += _scrollBarPos.height();
    }
    QTransform tr;
    tr.scale(_scale, _scale);
    tr.translate(xTranslation, yTranslation);
    return tr;
}

void GraphicMsc::draw()
{
    ast::Item *selectedItem = nullptr;
    // View transform (default and lock instance)
    QTransform tr(getTransform());
    QTransform trLock(getTransform(true));

    // Note if selection is in locked instances area
    bool selectedPointInLock = false;
    QRect lockInstanceBounds = getSelectionBounds(nullptr, 0, getLockInstanceY() - 1);

    // update selection and selected region
    _selectedRegion = {};

    // set selection from text cursor position
    if (_selection._source == Selection::eSource::eFromEdit) {
        selectedItem = findSelected(_selection._selectedCursorPosition, _selectedRegion, false);
        _selection._selectedPoint = {-1, -1};
        if (selectedItem) {
            _selection._selectedPoint = QPoint(_selectedRegion.center().x(),
                                               _selectedRegion.top() + 2);
        }
    } else {
        // Handle mouse selection in locked instances area
        if ((cfg()._lockInstances) && (_selection._source == Selection::eSource::eFromView)) {
            // check if selection in instances
            QPoint p = trLock.inverted().map(tr.map(_selection._selectedPoint));
            if (lockInstanceBounds.contains(p)) {
                //qDebug() << p << "<-" << _selection._selectedPoint;
                _selection._selectedPoint = p;
                selectedPointInLock = true;
            }
        }

        // Set selection for mouse cursor position
        _selection._selectedCursorPosition = {};
        selectedItem = findSelected(_selection._selectedPoint, _selectedRegion, false);
        if (selectedItem && _selection._source == Selection::eSource::eFromView) {
            _selection._selectedCursorPosition.set(selectedItem->_pos,
                                                   selectedItem->_line,
                                                   selectedItem->_col);
        }
    }

    // Reposition to origin
    _painter.setWorldTransform(tr);

    // draw selection background
    if (selectedItem && (selectedItem->_type != ast::eTokenType::eInstance)
        && (!cfg()._lockInstances || !selectedPointInLock)) {
        drawRect(_selectedRegion,
                 DrawOptions()
                     .setColor(cfg()._colorSelection)
                     .setBrushStyle(Qt::BrushStyle::SolidPattern)
                     .setBkColor(cfg()._colorSelection));
    }

    // draw instances (only draw vertical lines if locked)
    for (auto &instance : _instances) {
        drawInstance(instance, !cfg()._lockInstances);
    }

    // draw blocks
    for (auto &block : _blocks) {
        drawBlock(block);
    }

    // draw elements
    for (auto &element : _elements) {
        drawElement(element);
    }

    // draw global bounds
    if (cfg()._debug) {
        drawRect(_instanceBounds, DrawOptions().setColor(Qt::GlobalColor::green));
        drawRect(_bounds);
    }

    // simulate instance locked
    //   - change coord transformation to take vertical scrollbar into account
    //   - erase upper chart area for instances drawing
    if (cfg()._lockInstances) {
        lockInstanceBounds.setTop(lockInstanceBounds.top() - cfg()._offset.height() - 1);
        _painter.setWorldTransform(trLock);
        _painter.setClipRect(lockInstanceBounds);
        // erase background and draw instance separation line
        drawRect(lockInstanceBounds,
                 DrawOptions()
                     .setPenStyle(Qt::PenStyle::NoPen)
                     .setBrushStyle(Qt::BrushStyle::SolidPattern));
        if (_scrollBarPos.height()) {
            drawLine({lockInstanceBounds.bottomLeft(), lockInstanceBounds.bottomRight()});
        }

        // draw instances
        for (auto &instance : _instances) {
            drawInstance(instance, true);
        }
    }
}

void GraphicMsc::drawInstance(const GraphicInstance &instance, bool drawRect)
{
    _painter.drawLine(instance._xPos,
                      instance._boundOut.bottom() + 1,
                      instance._xPos,
                      _bounds.bottom());
    if (drawRect) {
        auto r = instance._boundOut;
        int flags = eGraphicFlags::eVCenter;
        if (instance._ast->_flags & ast::eFlag::eInstanceShapeFIFO) {
            flags |= eGraphicFlags::eFillRect | eGraphicFlags::eClipFifo;
        } else {
            flags |= eGraphicFlags::eDrawRect;
        }
        drawText(instance._main, r, flags);
        if (instance._ast->_flags & ast::eFlag::eInstanceShapeFIFO) {
            _painter.drawLine(r.left(), r.top() + 5, r.left(), r.bottom() - 5);
            _painter.drawLine(r.right(), r.top() + 5, r.right(), r.bottom() - 5);
            _painter.drawEllipse(r.left(), r.top(), r.width() - 1, 10);
            _painter.drawEllipse(r.left(), r.bottom() - 10, r.width() - 1, 10);
        }
    }
}

void GraphicMsc::drawAnno(const GraphicElement &element, int index)
{
    if (element._anno[index]._size.isNull())
        return;

    QSize textSize = element._anno[index]._size;
    int x = index ? element._boundOut.right() - textSize.width() : element._boundOut.left();
    QRect bounds(x, element._boundOut.top(), textSize.width(), textSize.height());
    // y-center on element by default
    bounds.setTop(element._yPos - textSize.height() / 2);
    bounds.setBottom(element._yPos + textSize.height() / 2);
    // adjust to bounds-out if necessary
    if (bounds.top() < element._boundOut.top()) {
        bounds.moveTop(element._boundOut.top());
    } else if (bounds.bottom() > element._boundOut.bottom()) {
        bounds.moveBottom(element._boundOut.bottom());
    }
    // draw text annotation
    if (index == 0) { // left
        drawLine({{bounds.right() + 3, bounds.top() + bounds.height() / 3},
                  {bounds.right() + cfg()._yAnnoMargin - 2, element._yPos}});
    } else { // right
        drawLine({{bounds.left() - 2, bounds.top() + bounds.height() / 3},
                  {bounds.left() - cfg()._yAnnoMargin + 3, element._yPos}});
    }
    drawText(element._anno[index], bounds, eGraphicFlags::eVCenter | eGraphicFlags::eDrawRect);
}

void GraphicMsc::drawElement(const GraphicElement &element)
{
    if (element._ast->_type == ast::eTokenType::eText) {
        int flags = eGraphicFlags::eVCenter | eGraphicFlags::eDrawRect;
        if (element._ast->_flags & ast::eFlag::eTextShapeRound) {
            flags |= eGraphicFlags::eRoundRect;
        }
        drawText(element._main, element._boundIn, flags);
    } else if (element._ast->_type == ast::eTokenType::eMessage) {
        int yArrowHeight = (element._xFrom != element._xTo) ? cfg()._yArrowHeight : 0;
        // main text
        QRect bounds1{element._boundIn.left(),
                      element._yPos - element._main._size.height() - yArrowHeight / 2,
                      element._boundIn.width(),
                      element._main._size.height()};
        drawText(element._main, bounds1, eGraphicFlags::eFillBackground);
        // Arrow
        bool isDash = (element._ast->_flags & ast::eMessageArrowDash);
        drawArrow(element._xFrom, element._xTo, element._yPos, isDash);
        // sub text
        QRect bounds2{element._boundIn.left(),
                      element._yPos + yArrowHeight / 2,
                      element._boundIn.width(),
                      element._sub._size.height()};
        drawText(element._sub, bounds2, eGraphicFlags::eFillBackground);
    } else if (element._ast->_type == ast::eTokenType::eReference) {
        // non-expanded reference
        drawText(element._main,
                 element._boundOut,
                 eGraphicFlags::eVCenter | eGraphicFlags::eDrawRect | eGraphicFlags::eRoundRect);
        if (element._ast->_flags & ast::eFlag::eRefFound) {
            drawRedux(element._boundOut.topLeft(), true);
        }
    }
    // draw left/right annos
    drawAnno(element, 0);
    drawAnno(element, 1);
    // debug
    if (cfg()._debug) {
        drawRect(element._boundIn, DrawOptions().setColor(Qt::GlobalColor::blue));
        if (element._boundIn != element._boundOut) {
            drawRect(element._boundOut, DrawOptions().setColor(Qt::GlobalColor::red));
        }
    }
}

void GraphicMsc::drawBlock(const GraphicBlock &block)
{
    int currentY = block._boundOut.top();
    int altIndex = 0;
    bool isReferenceBlock = false, isReferenceFound = false;
    DrawOptions options;

    // Condition block -> dash line
    if (block._ast->_type == ast::eTokenType::eCondition) {
        options.setPenStyle(Qt::PenStyle::DashLine);
    } else if (block._ast->_type == ast::eTokenType::eReference) {
        isReferenceBlock = true;
        isReferenceFound = block._ast->_flags & ast::eFlag::eRefFound;
        if (block._ast->_flags & ast::eFlag::eRefInline) {
            return;
        }
    }

    // Iterate over block sub-blocks
    for (auto const &sblk : block._sub_blocks) {
        int titleTextHeight = 0;
        QRect bounds(block._boundOut.left(), currentY, 0, 0);
        QPolygon pts;
        // sub-block title
        if (isReferenceBlock) {
            bounds.setWidth(block._boundOut.width());
            titleTextHeight = std::max(sblk._title._size.height(), cfg()._refReduxMargin.height());
        } else {
            bounds.setWidth(sblk._title._size.width());
            titleTextHeight = std::max(sblk._title._size.height(), cfg()._instanceSize.height());
        }
        bounds.setHeight(titleTextHeight);
        int headerFlags = eVCenter | eFillRect | ((isReferenceBlock) ? eClipHeader : eFlagNone);
        drawText(sblk._title, bounds, headerFlags);
        // lower/right condition dash line
        pts << QPoint(bounds.left(), bounds.bottom() + 1)
            << QPoint(bounds.right() + 1, bounds.bottom() + 1);
        if (!isReferenceBlock) {
            pts << QPoint(bounds.right() + 1, bounds.top());
        }
        drawLine(pts, options);
        // sub-block dash line separator (condition alternatives only)
        if (altIndex) {
            drawLine({{bounds.left(), currentY}, {block._boundOut.right(), currentY}}, options);
        }
        if (isReferenceFound) {
            drawRedux(bounds.topLeft(), false);
        }
        currentY += sblk._height;
        ++altIndex;
    }
    drawRect(block._boundOut, options, isReferenceBlock);
}

void GraphicMsc::test() const
{
    // text color #1 and #23
    GraphicText text1;
    parseLine(R"(\r\b\ct1\cb23 text1)", text1);
    assert(text1._text.size() == 1);
    assert(text1._text.front()._sv == "text1");
    assert(text1._text.front()._textColor == QColor(0, 0, 1));
    assert(text1._backgroundColor == QColor(0, 0, 0x23));
    assert(text1._text.front()._flags == (eAlignRight | eFontBold));
    // color #456 and #789A + \r overriden by \l
    GraphicText text2;
    parseLine(R"(\r\cb789A\l\u\ct456\i        my text2)", text2);
    assert(text2._text.size() == 1);
    assert(text2._text.front()._sv == "my text2");
    assert(text2._text.front()._textColor == QColor(0, 0x04, 0x56));
    assert(text2._backgroundColor == QColor(0, 0x78, 0x9A));
    assert(text2._text.front()._flags == (eAlignLeft | eFontItalic | eFontUnderline));
    // color by name + \l overriden by \r
    GraphicText text3;
    parseLine(R"(\cbblue\l\u\ctmagenta\rtext3)", text3);
    assert(text3._text.size() == 1);
    assert(text3._text.front()._sv == "text3");
    assert(text3._text.front()._textColor == QColor::fromString("magenta"));
    assert(text3._backgroundColor == QColor::fromString("blue"));
    assert(text3._text.front()._flags == (eAlignRight | eFontUnderline));
    // color #BCDE0 and #123456
    GraphicText text4;
    parseLine(R"(\ctBCDE0 \cb123456 text4)", text4);
    assert(text4._text.size() == 1);
    assert(text4._text.front()._sv == "text4");
    assert(text4._text.front()._textColor == QColor(0xb, 0xcd, 0xe0));
    assert(text4._backgroundColor == QColor(0x12, 0x34, 0x56));
    // color #789ABCD and #EF012345
    GraphicText text5;
    parseLine(R"(\ct789ABCD\cbEF012345text5)", text5);
    assert(text5._text.size() == 1);
    assert(text5._text.front()._sv == "text5");
    assert(text5._text.front()._textColor == QColor(0x89, 0xab, 0xcd, 0xf7));
    assert(text5._backgroundColor == QColor(0x01, 0x23, 0x45, 0xEF));
    // color too long
    GraphicText text6;
    parseLine(R"(\ct0123456789 text6)", text6);
    assert(text6._text.size() == 1);
    assert(text6._text.front()._sv == "89 text6");
    assert(text6._text.front()._textColor == QColor(0x23, 0x45, 0x67, 0x01));
}
} // namespace qmsc
