#include "qmsc_edit.h"
#include <QAbstractItemView>
#include <QCompleter>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QScrollBar>
#include <QStringListModel>
#include <QSyntaxHighlighter>
#include <QToolTip>
#include <QUndoView>
#include <QWidget>
#include "qmsc_document.h"

namespace qmsc {

// MscHighlighter
MscHighlighter::MscHighlighter(MscDocument *doc, QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , _doc(doc)
    , _updating(false)
{
    init(MscSettings::eOptionFlag::eColorOptions);
}

void MscHighlighter::init(int modifiedOptions)
{
    if (modifiedOptions & MscSettings::eColorBlock) {
        _blockFormat.setForeground(_doc->config()._colorBlock);
        _blockFormat.setFontWeight(QFont::Weight::Bold);
    }
    if (modifiedOptions & MscSettings::eColorElement) {
        _elementFormat.setForeground(_doc->config()._colorElement);
        _elementFormat.setFontWeight(QFont::Weight::Bold);
    }
    if (modifiedOptions & MscSettings::eColorProperty) {
        _propertyFormat.setForeground(_doc->config()._colorProperty);
    }
    if (modifiedOptions & MscSettings::eColorString) {
        _stringFormat.setForeground(_doc->config()._colorString);
    }
    if (modifiedOptions & MscSettings::eColorComment) {
        _commentFormat.setForeground(_doc->config()._colorComment);
        _commentFormat.setFontItalic(true);
    }
}

bool MscHighlighter::tokenInBlock(const MscDocument::TokenPos &token,
                                  unsigned int &start,
                                  unsigned int &end)
{
    unsigned int tokStart = token._pos;
    unsigned int tokEnd = token._pos + token._value.size();
    if ((tokStart <= end) && (tokEnd >= start)) {
        start = std::max(start, tokStart);
        end = std::min(end, tokEnd);
        return true;
    }
    return false;
}

void MscHighlighter::highlightBlock(const QString &text)
{
    if (!_updating)
        return;
    //qDebug() << "highlightBlock:" << currentBlock().blockNumber() << currentBlock().position()
    //         << text;

    // Current block position
    int bStartPos = currentBlock().position();
    int bEndPos = bStartPos + text.length();

    // Search token overlapping current pos
    for (auto const &tok : _doc->tokens()) {
        unsigned int tStartPos = bStartPos;
        unsigned int tEndPos = bEndPos;
        if (!tokenInBlock(tok, tStartPos, tEndPos))
            continue;

        // clang-format off
        QTextCharFormat *tcFormat = nullptr;
        // Format from token category
        switch (tok._cat) {
        case ast::eTokenCategory::eString: tcFormat = &_stringFormat; break;
        case ast::eTokenCategory::eBlock: tcFormat = &_blockFormat; break;
        case ast::eTokenCategory::eElement: tcFormat = &_elementFormat; break;
        case ast::eTokenCategory::eProperty: tcFormat = &_propertyFormat; break;
        case ast::eTokenCategory::eComment: tcFormat = &_commentFormat; break;
        default: break;
        }
        // clang-format on
        if (tcFormat) {
            setFormat(tStartPos - bStartPos, tEndPos - tStartPos, *tcFormat);
        }
    }
}

void MscHighlighter::update(int modifiedOptions)
{
    _updating = true;
    init(modifiedOptions);
    rehighlight();
    _updating = false;
}

// MscTextEdit
MscTextEdit::MscTextEdit(MscDocument *doc, QWidget *parent)
    : QTextEdit(parent)
    , _doc(doc)
    , _highlighter(nullptr)
    , _completer(nullptr)
    , _completerModel(nullptr)
{
    setFont(_doc->config()._editFont);
    setLineWrapMode(QTextEdit::LineWrapMode::NoWrap);
    setTabChangesFocus(false);
    _errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    _errorFormat.setUnderlineColor(Qt::red);
    _selectionFormat.setBackground(_doc->config()._colorSelection);
    _selectionFormat.setProperty(QTextFormat::FullWidthSelection, true);

    connect(doc, &MscDocument::docTextChanged, this, &MscTextEdit::onDocTextChanged);
    connect(doc, &MscDocument::optionChanged, this, &MscTextEdit::onDocOptionChanged);
    connect(document(), &QTextDocument::contentsChange, this, [this](int, int, int) { //
        _updatingText = true;
    });
    connect(document(), &QTextDocument::contentsChanged, this, [this]() { //
        _updatingText = false;
    });
    connect(this, &MscTextEdit::cursorPositionChanged, this, [this] {
        if (!_updatingText) {
            TextPosition pos = getCursorPosition();
            setCursorPosition(pos);
            emit cursorChanged(pos);
        }
    });

    //highlighter
    _highlighter = new MscHighlighter(doc, document());
    // Completer
    _completerModel = new QStringListModel(this);
    _completer = new QCompleter(_completerModel, this);
    _completer->setWidget(this);
    _completer->setCompletionMode(QCompleter::PopupCompletion);
    _completer->setCaseSensitivity(Qt::CaseInsensitive);
    connect(_completer,
            QOverload<const QString &>::of(&QCompleter::activated),
            this,
            &MscTextEdit::insertCompletion);
}

const TextPosition MscTextEdit::getCursorPosition() const
{
    auto const &cursor = textCursor();
    return {cursor.position(), cursor.blockNumber(), cursor.positionInBlock()};
}

void MscTextEdit::setTextContent(const QString &text)
{
    setText(text);
    onDocTextChanged(nullptr);
}

void MscTextEdit::onDocTextChanged(QObject *)
{
    _highlighter->update();
    updateExtraSel();
}

void MscTextEdit::onDocOptionChanged(int modifiedOptions)
{
    if (modifiedOptions & MscSettings::eEditFont) {
        setFont(_doc->config()._editFont);
    }
    if (modifiedOptions & MscSettings::eColorOptions) {
        _highlighter->update(modifiedOptions);
        if (modifiedOptions & MscSettings::eColorSelection) {
            _selectionFormat.setBackground(_doc->config()._colorSelection);
            updateExtraSel();
        }
    }
}

void MscTextEdit::setCursorPosition(const TextPosition &pos, bool moveCaret)
{
    if (moveCaret) {
        auto cursor = textCursor();
        cursor.setPosition(pos._pos);
        setTextCursor(cursor);
        setFocus(); // need focus to show caret
    }
    updateExtraSel();
}

void MscTextEdit::keyPressEvent(QKeyEvent *event)
{
    const bool ctrlSpace = event->modifiers().testFlag(Qt::ControlModifier)
                           && event->key() == Qt::Key_Space;
    if (_completer && _completer->popup()->isVisible()) {
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            event->ignore();
            return;
        default:
            break;
        }
    }

    // Check if the key pressed is the Tab key
    if (event->key() == Qt::Key_Tab) {
        insertPlainText("  ");
        event->accept();
        return;
    }
    QTextEdit::keyPressEvent(event);

    // Completer
    checkCompleterStatus(ctrlSpace);
}

bool MscTextEdit::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        auto *helpEvent = static_cast<QHelpEvent *>(event);
        QTextCursor cursor = cursorForPosition(helpEvent->pos());
        QString tooltipText = findErrorDescription((unsigned int) cursor.position());
        if (!tooltipText.isNull()) {
            QToolTip::showText(helpEvent->globalPos(), tooltipText, this, cursorRect(cursor));
        } else {
            QToolTip::hideText();
        }
        return true;
    }
    return QTextEdit::event(event);
}

void MscTextEdit::updateExtraSel()
{
    QList<ExtraSelection> selections;
    ExtraSelection sel;
    // current selection line
    sel.cursor = textCursor();
    sel.cursor.clearSelection();
    sel.format = _selectionFormat;
    selections.append(sel);
    // parsing errors
    for (auto const &error : _doc->errors()) {
        sel.cursor = textCursor();
        sel.cursor.setPosition(error._startPos, QTextCursor::MoveAnchor);
        sel.cursor.setPosition(error._endPos, QTextCursor::KeepAnchor);
        sel.format = _errorFormat;
        selections.append(sel);
    }
    setExtraSelections(selections);
}

QString MscTextEdit::findErrorDescription(unsigned int pos)
{
    QString tooltipText;
    for (auto const &error : _doc->errors()) {
        if ((pos >= error._startPos) && (pos <= error._endPos)) {
            if (!tooltipText.isNull())
                tooltipText += "\n";
            tooltipText += error._description;
        }
    }
    return tooltipText;
}

void MscTextEdit::checkCompleterStatus(bool bInit)
{
    if (!bInit && !_completer->popup()->isVisible())
        return;

    updateCompleterFromCursor(bInit, textCursor().position());

#if 0
    qDebug() << "---------------------";
    qDebug() << "flags:" << Qt::hex << "0x" << _completerCtx._flags;
    qDebug() << "pos:" << _completerCtx._startPosition;
    qDebug() << "prefix:" << _completerCtx._prefix;
    qDebug() << "words:" << _completerCtx._words;
#endif

    if (bInit) {
        if (_completerCtx._words.isEmpty()) {
            _completer->popup()->hide();
            return;
        }
        _completerModel->setStringList(_completerCtx._words);
    }

    const QString prefix = _completerCtx._prefix;
    if (prefix != _completer->completionPrefix()) {
        _completer->setCompletionPrefix(prefix);
        _completer->popup()->setCurrentIndex(_completer->completionModel()->index(0, 0));
    }

    if (_completerCtx._flags) {
        QRect cr = cursorRect();
        cr.setWidth(_completer->popup()->sizeHintForColumn(0)
                    + _completer->popup()->verticalScrollBar()->sizeHint().width());
        _completer->complete(cr);
    } else {
        _completer->popup()->hide();
    }
}

void MscTextEdit::updateCompleterFromCursor(bool bInit, unsigned int pos)
{
    auto invalidToken = _doc->tokens().rend();
    auto curToken = invalidToken;
    auto prevToken = invalidToken;
    auto prevItemToken = invalidToken;

    _completerCtx._flags = 0;
    if (bInit) {
        _completerCtx._words.clear();
        _completerCtx._startPosition = pos;
    }

    // Get current item
    for (auto tok = _doc->tokens().rbegin(); tok != invalidToken; ++tok) {
        if (tok->_pos > pos)
            continue;
        if ((curToken == invalidToken) && ((tok->_pos + tok->_value.size()) > pos)) {
            curToken = tok;
        } else if (prevToken == invalidToken) {
            prevToken = tok;
        }
        if ((prevItemToken == invalidToken)
            && (tok->_cat == ast::eTokenCategory::eElement || tok->_type == ast::eTokenType::eMsc)) {
            prevItemToken = tok;
            break;
        }
        if (tok->_cat == ast::eTokenCategory::eBlock) {
            break;
        }
    }

    // Check cursor is in a valid item
    if (prevItemToken == invalidToken) {
        return;
    }
    auto *tcItem = ast::findItemByPos(_doc->ast(), prevItemToken->_pos);
    if (!(tcItem && tcItem->_pos < pos && pos < tcItem->_pos + tcItem->_len)) {
        return;
    }

#if 0
    qDebug() << "------------------";
    auto displayToken = [this](const std::string &title,
                               const std::vector<MscDocument::TokenPos>::const_reverse_iterator &t) {
        if (t != _doc->tokens().rend())
            qDebug() << title << t->_value << t->_pos;
    };
    displayToken("curTok  :", curToken);
    displayToken("prevTok :", prevToken);
    displayToken("prevItem:", prevItemToken);
#endif
    // Get possible choices for property value
    if ((curToken != invalidToken) && (curToken->_cat == ast::eTokenCategory::eProperty)) {
        auto nextToken = (curToken != invalidToken && curToken != _doc->tokens().rbegin())
                             ? std::prev(curToken)
                             : invalidToken;
        prevToken = curToken;
        curToken = nextToken;
    }

    auto cbInsertValue = [this](const std::string_view &name) {
        QString s = getQString(name);
        if (_completerCtx._flags & eCompleterFlag::ePropertyAdd) {
            s += ":\"\"";
        }
        _completerCtx._words.append(s);
    };
    _completerCtx._prefix.clear();

    if ((prevToken != invalidToken) && (prevToken->_cat == ast::eTokenCategory::eProperty)) {
        if (curToken == invalidToken) {
            auto nextToken = (prevToken != _doc->tokens().rbegin()) ? std::prev(prevToken)
                                                                    : invalidToken;
            curToken = nextToken;
        }
        int insertPos = -1;
        if ((curToken != invalidToken) && (curToken->_type == ast::eTokenType::eString)) {
            _completerCtx._flags |= eCompleterFlag::ePropertyEdit;
            _completerCtx._startPosition = curToken->_pos + 1;
            insertPos = curToken->_pos + curToken->_value.size() - 1;
            if (!curToken->value().empty())
                _completerCtx._prefix = getQString(curToken->value());
        } else {
            _completerCtx._flags |= eCompleterFlag::ePropertySet;
            insertPos = prevToken->_pos + prevToken->_value.size();
        }
        if (bInit) {
            ast::getPropertyValuesForItem(_doc->ast(), *tcItem, prevToken->_type, cbInsertValue);
            if (_completerCtx._flags & eCompleterFlag::ePropertyEdit) {
                setCursorPosition(insertPos);
            } else if (_completerCtx._flags & eCompleterFlag::ePropertySet) {
                insertSuffix(insertPos, ":\"\"");
                _completerCtx._startPosition = textCursor().position();
            }
        }
    }
    // Get possible choices for element property
    else if (curToken == invalidToken) {
        auto nextToken = (prevItemToken != _doc->tokens().rbegin()) ? std::prev(prevToken)
                                                                    : invalidToken;
        if (nextToken->_type != ast::eTokenType::eOB) {
            if (bInit) {
                _completerCtx._flags |= eCompleterFlag::ePropertyAdd;
                _completerCtx._startPosition = pos;
                ast::getPropertiesForItem(*tcItem, cbInsertValue);
            }
        }
    }
}

void MscTextEdit::insertCompletion(const QString &completion)
{
    QTextCursor tc = textCursor();
    const int prefixLen = _completerCtx._prefix.size();
    tc.setPosition(_completerCtx._startPosition, QTextCursor::MoveAnchor);
    tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, prefixLen);
    if (tc.selectedText() != completion) {
        tc.insertText(completion);
        if (completion.endsWith("\"")) {
            tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor);
        }
    } else {
        tc.clearSelection();
    }
    setTextCursor(tc);
}

void MscTextEdit::setCursorPosition(int pos)
{
    QTextCursor tc = textCursor();
    if (pos != tc.position()) {
        tc.setPosition(pos, QTextCursor::MoveAnchor);
        setTextCursor(tc);
    }
}

void MscTextEdit::insertSuffix(int insertPos, const QString &suffix)
{
    int curTextPos = insertPos;
    int curSuffixPos = 0;
    while ((curTextPos < document()->characterCount()) && (curSuffixPos < suffix.length())) {
        auto c = document()->characterAt(curTextPos);
        if (c == suffix.at(curSuffixPos)) {
            insertPos = curTextPos + 1;
            ++curSuffixPos;
        } else if (c != ' ' && c != '\t') {
            break;
        }
        ++curTextPos;
    }
    QString insertedSuffix = suffix.mid(curSuffixPos);
    if (!insertedSuffix.isEmpty()) {
        QTextCursor tc = textCursor();
        tc.setPosition(insertPos, QTextCursor::MoveAnchor);
        tc.insertText(insertedSuffix);
        tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor);
        setTextCursor(tc);
    }
}

} // namespace qmsc
