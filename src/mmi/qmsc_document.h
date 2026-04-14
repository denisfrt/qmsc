// document.h
#pragma once
#include <QString>
#include <QTextDocument>
#include "ast.h"
#include "parser.h"
#include "qmsc_settings.h"
#include <vector>

namespace qmsc {

class MscDocument : public QTextDocument
{
    Q_OBJECT
public:
    struct TokenPos
    {
        ast::eTokenType _type = ast::eTokenType::eUnknown;
        ast::eTokenCategory _cat = ast::eTokenCategory::eUnknown;
        unsigned int _pos = 0;
        std::string_view _value;
        std::string_view value() const;
    };

public:
    explicit MscDocument(QObject *parent, MscSettings::Config &config);
    static MscDocument *create(const QString &filename,
                               const QString &title,
                               MscSettings::Config &config);
    bool save(const QString &filename = QString());

    const MscSettings::Config &config() const;
    void updateConfig(MscSettings::Config &config);
    const QString &text() const;
    const ast::MscList &ast() const;
    ast::MscList &mutable_ast();
    QString title() const;
    const QString &filename() const;
    bool isNativeFile() const;
    const std::vector<TokenPos> &tokens() const;
    const std::list<MscParser::Error> &errors() const;
    void setFilename(const QString &filename);
    void setText(const QString &newText, QObject *source = nullptr, bool isModified = true);

signals:
    void docTextChanged(QObject *source);
    void optionChanged(int modifiedOptions);

private:
    QString _text;
    std::string _buffer;
    QString _filename;
    bool _nativeFile;
    QString _title;
    bool _parseOk;
    MscParser _parser;
    MscSettings::Config _config;
    std::vector<TokenPos> _tokens;
};

} // namespace qmsc
