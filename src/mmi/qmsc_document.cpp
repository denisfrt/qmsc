#include "qmsc_document.h"
#include <QFileInfo>
#include <QFontDatabase>
#include "parser.h"
#include "qmsc_settings.h"

namespace qmsc {

MscDocument::MscDocument(QObject *parent, MscSettings::Config &config)
    : QTextDocument(parent)
    , _nativeFile(false)
    , _parseOk(false)
    , _config(config)
{}

std::string_view MscDocument::TokenPos::value() const
{
    if (_type == ast::eTokenType::eString) {
        return _value.substr(1, _value.size() - 2);
    }
    return _value;
}

const MscSettings::Config &MscDocument::config() const
{
    return _config;
}

void MscDocument::updateConfig(MscSettings::Config &config)
{
    int modified = _config.updateConfig(config);
    if (modified) {
        emit optionChanged(modified);
    }
}

const QString &MscDocument::text() const
{
    return _text;
}

const qmsc::ast::MscList &MscDocument::ast() const
{
    return _parser.ast();
}

qmsc::ast::MscList &MscDocument::mutable_ast()
{
    return _parser.mutable_ast();
}

QString MscDocument::title() const
{
    return _title + "[*]";
}

const QString &MscDocument::filename() const
{
    return _filename;
}

bool MscDocument::isNativeFile() const
{
    return _nativeFile;
}

const std::vector<MscDocument::TokenPos> &MscDocument::tokens() const
{
    return _tokens;
}

const std::list<MscParser::Error> &MscDocument::errors() const
{
    return _parser.errors();
}

void MscDocument::setFilename(const QString &filename)
{
    QFileInfo info(filename);
    _filename = filename;
    _nativeFile = info.isNativePath() && info.exists();
    if (_nativeFile || _title.isEmpty()) {
        _title = info.fileName();
    }
}

void MscDocument::setText(const QString &newText, QObject *source, bool isModified)
{
    if (_text == newText)
        return;
    // update source text
    _text = newText;

    // parse msc and save token position for highlighting
    _tokens.clear();
    auto cb = [this](const qmsc::MscLexer::Token &token) {
        auto cat = ast::getTokenCategory(token._id);
        if (cat == ast::eTokenCategory::eUnknown)
            return;
        auto type = ast::getTokenType(token._id);
        if ((cat == ast::eTokenCategory::eSymbol) && (type != ast::eTokenType::eOB))
            return;
        auto &tok = _tokens.emplace_back();
        tok._cat = cat;
        tok._type = type;
        tok._pos = token.startPos();
        tok._value = token.rawValue();
    };
    _buffer = _text.toStdString();
    _parseOk = _parser.parse(_buffer, cb);
    // qDebug() << "MscDocument:parse:" << _parseOk << "(emit docTextChanged)";
    if (!_parseOk && _config._debug) {
        for (auto const &e : _parser.errors()) {
            qDebug() << QString("%1-%2: %3")
                            .arg(e._startPos)
                            .arg(e._endPos)
                            .arg(getQString(e._description));
        }
    }

    // update config from msc
    if (!_parser.ast()._mscs.empty()) {
        auto const &msc = _parser.ast()._mscs.front();
        _config._instanceXSpace = msc._xinst;
        _config._elementMargin.setHeight(msc._yitem);
        if (!msc._fontName.empty()) {
            _config._mscFont = QFont(getQString(msc._fontName));
        }
        if (msc._fontSize != ast::defs::invalidFontSize) {
            _config._mscFont.setPointSize(msc._fontSize);
        }
    }

    // notify view
    setModified(isModified);
    emit docTextChanged(source);
}

MscDocument *MscDocument::create(const QString &filename,
                                 const QString &title,
                                 MscSettings::Config &config)
{
    auto *doc = new MscDocument(nullptr, config);
    doc->_title = title;
    // Read file content
    QFile f(filename.isEmpty() ? ":/msc/default.msc" : filename);
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&f);
        doc->setFilename(filename);
        doc->setText(in.readAll(), nullptr, filename.isEmpty());
        f.close();
    }
    return doc;
}

bool MscDocument::save(const QString &filename)
{
    bool r = false;
    // Write file content
    QFile f(filename.isEmpty() ? _filename : filename);
    if (f.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        QTextStream ou(&f);
        ou << _text;
        _title = f.fileName();
        setModified(false);
        setFilename(f.fileName());
        f.close();
        r = true;
    }
    return r;
}

} // namespace qmsc
