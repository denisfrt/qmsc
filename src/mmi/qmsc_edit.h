#pragma once
#include <QCompleter>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QStringListModel>
#include <QSyntaxHighlighter>
#include "qmsc_document.h"
#include "qmsc_utils.h"

namespace qmsc {

// MscHighlighter
class MscHighlighter : public QSyntaxHighlighter
{
public:
    explicit MscHighlighter(MscDocument *doc, QTextDocument *parent = nullptr);
    void init(int modifiedOptions);
    void update(int modifiedOptions = 0);

protected:
    void highlightBlock(const QString &text) override;
    bool tokenInBlock(const MscDocument::TokenPos &token, unsigned int &start, unsigned int &end);

private:
    MscDocument *_doc;
    bool _updating;
    QTextCharFormat _blockFormat;
    QTextCharFormat _elementFormat;
    QTextCharFormat _propertyFormat;
    QTextCharFormat _stringFormat;
    QTextCharFormat _commentFormat;
};

// MscTextEdit
class MscTextEdit : public QTextEdit
{
    Q_OBJECT
private:
    enum eCompleterFlag : int { ePropertyEdit = 0x01, ePropertySet = 0x02, ePropertyAdd = 0x04 };
    struct CompleterCtx
    {
        int _flags = 0;
        QStringList _words;
        QString _prefix;
        int _startPosition = -1;
    };

public:
    explicit MscTextEdit(MscDocument *doc, QWidget *parent = nullptr);
    const TextPosition getCursorPosition() const;
    void setTextContent(const QString &text);
    void setCursorPosition(const TextPosition &pos, bool moveCaret = false);

signals:
    void cursorChanged(const qmsc::TextPosition &pos);

public slots:
    void onDocTextChanged(QObject *source);
    void onDocOptionChanged(int modifiedOptions);
private slots:
    void insertCompletion(const QString &completion);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;

private:
    void updateExtraSel();
    QString findErrorDescription(unsigned int pos);
    void checkCompleterStatus(bool bInit);
    void updateCompleterFromCursor(bool bInit, unsigned int pos);
    void setCursorPosition(int pos);
    void insertSuffix(int insertPos, const QString &suffix);

private:
    MscDocument *_doc;
    MscHighlighter *_highlighter;
    bool _updatingText;
    QTextCharFormat _errorFormat;
    QTextCharFormat _selectionFormat;
    // completer
    QCompleter *_completer;
    QString _currentPrefix;
    QStringListModel *_completerModel;
    CompleterCtx _completerCtx;
};

} // namespace qmsc
