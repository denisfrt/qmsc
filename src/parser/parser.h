#pragma once
#include "ast.h"
#include "lexer.h"
#include <functional>
#include <list>
#include <string>

namespace qmsc {

bool compareNoCase(const std::string_view &a, const std::string_view &b);
std::size_t startsWithNoCase(const std::string_view &a, const std::string_view &b);

class MscParser
{
public:
    struct Error
    {
        unsigned int _startPos = 0;
        unsigned int _endPos = 0;
        std::string _description;
        ast::Block *_cur_block = nullptr;
        ast::Item *_cur_item = nullptr;
    };
    using TokenCB = std::function<void(const qmsc::MscLexer::Token &token)>;

public:
    explicit MscParser();
    virtual ~MscParser();
    const ast::MscList &ast() const;
    ast::MscList &mutable_ast();
    const std::list<Error> &errors() const;

    bool parse(const std::string_view &input);
    bool parse(const std::string_view &input, const TokenCB &callback);
    void error_register(const std::string &description, int start = -1, int end = -1);

    void start(const MscLexer::Token *token);
    void end(const MscLexer::Token *token = nullptr);
    void addProperty(const MscLexer::Token *tokp, const MscLexer::Token *tokv);
    void addConditionProperty(const MscLexer::Token *tokv);

private:
    void init(const std::string_view &input);
    void setItem(ast::Item &item, ast::eTokenType type, const MscLexer::Token *token);
    void addItem(const MscLexer::Token *token, std::list<ast::Instance> &elements);
    void addItem(ast::eTokenType type,
                 const MscLexer::Token *token,
                 std::list<ast::Element> &elements);
    ast::Block *handleCondition(ast::eTokenType type,
                                const MscLexer::Token *token,
                                std::list<ast::Element> &elements);
    bool checkValidConditionBlock(ast::eTokenType type, std::list<ast::Element> &elements);
    ast::Property &addProperty(ast::eTokenType type,
                               const MscLexer::Token *tokv,
                               std::list<ast::Property> &properties);
    void handleAnnoFlag(ast::Property &property,
                        std::list<ast::Property> &properties,
                        const MscLexer::Token *tokp,
                        const MscLexer::Token *tokv);
    void handleXform(ast::Item *item, ast::Property &property, const MscLexer::Token *tokv);
    int setParsedValue(const std::string_view &svalue, int defaultValue);

private:
    void *_parser;
    MscLexer _lexer;
    ast::MscList _ast;
    ast::Block *_cur_block;
    ast::Item *_cur_item;
    std::list<Error> _errors;
};

} // namespace qmsc
