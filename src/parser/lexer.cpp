#include "lexer.h"
#include "generated_parser.h"
#include <iostream>
#include <string_view>

namespace qmsc {

void MscLexer::init(const std::string_view &input)
{
    _input = _buf = input.data();
    _lastPos = _pos = 0;
    _line = 0;
    _col = 0;
    _token = nullptr;
    _cursor = nullptr;
    _marker = nullptr;
}

// clang-format off
unsigned int MscLexer::Token::startPos() const { return _pos - offset(); }
unsigned int MscLexer::Token::endPos() const { return startPos() + len(); }
unsigned int MscLexer::Token::len() const { return (_end - _start) + 2 * offset(); }
unsigned int MscLexer::Token::offset() const { return (_id == TK_STRING) ? 1 : 0;}
std::string_view MscLexer::Token::value() const { return {_start, _end}; }
std::string_view MscLexer::Token::rawValue() const { return {_start - offset(), _end + offset()}; }
unsigned int MscLexer::pos() const { return _pos; }
unsigned int MscLexer::lastPos() const { return _lastPos; }
unsigned int MscLexer::line() const { return _line; }
unsigned int MscLexer::col() const { return _col; }
// clang-format on

void MscLexer::startToken(Token &res, int id)
{
    // Ignore token start if last token was unknown
    // we'll reset cursor in endToken to rescan it
    if (res._id == TK_UNKNOWN) {
        return;
    }
    // Start new token
    res._id = id;
    res._pos = _token - _input;
    res._line = _line;
    res._col = _col + (_token - _buf);
    res._start = _token;
    if (res._id == TK_STRING) {
        ++res._pos;
        ++res._start;
    }
    res._end = res._start;
    _pos = res._pos;
}

void MscLexer::endToken(Token &res)
{
    _lastPos = _pos;

    // reset cursor if last token was unknown
    if (res._id == TK_UNKNOWN) {
        _cursor = _token;
        _buf = _token;
    }
    // End token
    int offset = 0;
    if ((res._id == TK_STRING) || (_cursor[-1] == '\0') || (_cursor[-1] == '\n')) {
        offset = -1;
    }
    res._end = _cursor + offset;
    _lastPos = _pos;
    _pos = _cursor + offset - _input;
}

void MscLexer::resetToken(Token &res)
{
    res._id = TK_NONE;
    res._pos = 0;
    res._start = nullptr;
    res._end = nullptr;
    res._line = 0;
    res._col = 0;
}

void MscLexer::showToken(const Token &res)
{
    std::cout << "pos:" << res._pos << " " << res.value() << "(" << res._line << ":" << res._col
              << ")" << std::endl;
}

} // namespace qmsc