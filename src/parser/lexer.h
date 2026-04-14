#pragma once
#include <string_view>

#define TK_NONE 0
#define TK_UNKNOWN -1
#define TK_LCOMMENT -2
#define TK_BCOMMENT -3

namespace qmsc {

class MscLexer
{
public:
    struct Token
    {
        int _id;
        const char *_start;
        const char *_end;
        unsigned int _pos;
        unsigned int _line;
        unsigned int _col;
        unsigned int startPos() const;
        unsigned int endPos() const;
        unsigned int len() const;
        unsigned int offset() const;
        std::string_view value() const;
        std::string_view rawValue() const;
    };

public:
    explicit MscLexer() = default;
    void init(const std::string_view &input);
    unsigned int pos() const;
    unsigned int lastPos() const;
    unsigned int line() const;
    unsigned int col() const;
    bool next(Token &res);
    static void resetToken(Token &res);

private:
    void startToken(Token &res, int id);
    void endToken(Token &res);
    static void showToken(const Token &res);

private:
    unsigned int _pos = 0;
    unsigned int _lastPos = 0;
    unsigned int _line = 0;
    unsigned int _col = 0;
    const char *_token = nullptr;
    const char *_cursor = nullptr;
    const char *_marker = nullptr;
    const char *_input = nullptr;
    const char *_buf = nullptr;
};

} // namespace qmsc
