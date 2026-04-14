#include "parser.h"
#include "generated_parser.h"
#include "lexer.h"
#include <algorithm>
#include <set>
#include <string>

extern void mscParser(void *, int, qmsc::MscLexer::Token, class qmsc::MscParser *);
extern void mscParserFree(void *, void (*freeProc)(void *));
extern void *mscParserAlloc(void *(*mallocProc)(size_t) );

namespace qmsc {

bool compareNoCase(const std::string_view &a, const std::string_view &b)
{
    return a.size() == b.size() &&                                        //
           std::equal(a.begin(), a.end(), b.begin(), [](auto a, auto b) { //
               return std::tolower(a) == std::tolower(b);
           });
};

std::size_t startsWithNoCase(const std::string_view &a, const std::string_view &b)
{
    if (a.size() && b.size() && (a.size() >= b.size()) && compareNoCase(a.substr(0, b.size()), b)) {
        return b.size();
    }
    return 0;
}

MscParser::MscParser()
    : _cur_block(nullptr)
    , _cur_item(nullptr)
{
    _parser = mscParserAlloc(malloc);
}

MscParser::~MscParser()
{
    if (_parser) {
        mscParserFree(_parser, free);
    }
}

const ast::MscList &MscParser::ast() const
{
    return _ast;
}

ast::MscList &MscParser::mutable_ast()
{
    return _ast;
}

const std::list<MscParser::Error> &MscParser::errors() const
{
    return _errors;
}

bool MscParser::parse(const std::string_view &input)
{
    return parse(input, nullptr);
}

bool MscParser::parse(const std::string_view &input, const TokenCB &callback)
{
    MscLexer::Token token;
    init(input);
    while (_lexer.next(token)) {
        if (token._id == TK_UNKNOWN) {
            error_register("invalid token");
        } else if ((token._id != TK_LCOMMENT) && (token._id != TK_BCOMMENT)) {
            mscParser(_parser, token._id, token, this);
        }
        if (callback) {
            callback(token);
        }
    }
    MscLexer::resetToken(token);
    mscParser(_parser, token._id, token, this);
    return _errors.empty();
}

void MscParser::error_register(const std::string &description, int start, int end)
{
    auto &e = _errors.emplace_back();
    e._endPos = (end >= 0) ? end : _lexer.pos();
    e._startPos = (start >= 0) ? start : _lexer.lastPos();
    e._description = description;
    e._cur_block = _cur_block;
    e._cur_item = _cur_item;
}

void MscParser::start(const MscLexer::Token *token)
{
    auto type = ast::getTokenType(token->_id);
    if (type == ast::eTokenType::eMsc) {
        auto &msc = _ast._mscs.emplace_back();
        setItem(msc, type, token);
        _cur_block = &msc;
    } else if (_cur_block) {
        switch (_cur_block->_type) {
        case ast::eTokenType::eMsc:
            if (type == ast::eTokenType::eInstance) {
                addItem(token, static_cast<ast::Msc *>(_cur_block)->_instances);
            } else {
                addItem(type, token, static_cast<ast::Msc *>(_cur_block)->_elements);
            }
            break;
        default:
            if (type != ast::eTokenType::eInstance) {
                addItem(type, token, _cur_block->_elements);
            }
            break;
        }
    }
}

void MscParser::end(const MscLexer::Token *token)
{
    auto curPos = _lexer.pos();
    if (token) {
        curPos = token->_pos + (token->_end - token->_start);
    }
    if (_cur_item) {
        _cur_item->_len = curPos - _cur_item->_pos;
        _cur_item = nullptr;
    } else if (_cur_block) {
        _cur_block->_len = curPos - _cur_block->_pos;
        _cur_block = _cur_block->_parent;
        if (_cur_block && (_cur_block->_type == ast::eTokenType::eCondition)) {
            _cur_block->_len = curPos - _cur_block->_pos;
            _cur_block = _cur_block->_parent;
        }
    }
}

void MscParser::init(const std::string_view &input)
{
    _lexer.init(input);
    _ast._mscs.clear();
    _cur_block = nullptr;
    _cur_item = nullptr;
    _errors.clear();
}

void MscParser::setItem(ast::Item &item, ast::eTokenType type, const MscLexer::Token *token)
{
    item._type = type;
    item._pos = token->_pos;
    item._line = token->_line;
    item._col = token->_col;
    item._len = token->_end - token->_start;
}

void MscParser::addItem(const MscLexer::Token *token, std::list<ast::Instance> &elements)
{
    auto &item = elements.emplace_back();
    setItem(item, ast::eTokenType::eInstance, token);
    _cur_item = &item;
}

void MscParser::addItem(ast::eTokenType type,
                        const MscLexer::Token *token,
                        std::list<ast::Element> &elements)
{
    ast::Item *item = nullptr;
    ast::Block *block = nullptr;
    switch (type) {
    case ast::eTokenType::eMessage: {
        item = std::get_if<ast::Message>(&elements.emplace_back(std::in_place_type<ast::Message>));
    } break;
    case ast::eTokenType::eText: {
        item = std::get_if<ast::Text>(&elements.emplace_back(std::in_place_type<ast::Text>));
    } break;
    case ast::eTokenType::eReference: {
        item = std::get_if<ast::Reference>(
            &elements.emplace_back(std::in_place_type<ast::Reference>));
    } break;
    case ast::eTokenType::eIf:
    case ast::eTokenType::eElif:
    case ast::eTokenType::eElse: {
        block = handleCondition(type, token, elements);
    } break;
    case ast::eTokenType::eWhile: {
        block = std::get_if<ast::Block>(&elements.emplace_back(std::in_place_type<ast::Block>));
    } break;
    default:
        break;
    }
    // we shouldn't have a current item
    if (_cur_item && (item || block)) {
        error_register("unexpected start token");
        end();
    }
    // add new item/block
    if (item) {
        setItem(*item, type, token);
        _cur_item = item;
    } else if (block) {
        setItem(*block, type, token);
        block->_parent = _cur_block;
        _cur_block = block;
    }
}

bool MscParser::checkValidConditionBlock(ast::eTokenType type, std::list<ast::Element> &elements)
{
    bool r = true;
    if ((type == ast::eTokenType::eElif) || (type == ast::eTokenType::eElse)) {
        if (elements.empty()) {
            return false;
        } else if (auto *block = std::get_if<ast::Block>(&elements.back())) {
            r = ((block->_type == ast::eTokenType::eIf) || (block->_type == ast::eTokenType::eElif));
        }
    }
    return r;
}

ast::Block *MscParser::handleCondition(ast::eTokenType type,
                                       const MscLexer::Token *token,
                                       std::list<ast::Element> &elements)
{
    ast::Block *block = nullptr;
    ast::Condition *cond = nullptr;
    if (type == ast::eTokenType::eIf) {
        cond = std::get_if<ast::Condition>(
            &elements.emplace_back(std::in_place_type<ast::Condition>));
        setItem(*cond, ast::eTokenType::eCondition, token);
        cond->_parent = _cur_block;
    } else if (_cur_block && !_cur_block->_elements.empty()) {
        cond = std::get_if<ast::Condition>(&elements.back());
    }
    if (!cond) {
        error_register("elif/else without preceding if", token->startPos(), token->endPos());
    } else {
        if (!checkValidConditionBlock(type, cond->_elements)) {
            error_register("elif/else without preceding if/elif",
                           token->startPos(),
                           token->endPos());
        }
        _cur_block = cond;
        block = std::get_if<ast::Block>(
            &cond->_elements.emplace_back(std::in_place_type<ast::Block>));
    }
    return block;
}

void MscParser::addConditionProperty(const MscLexer::Token *tokv)
{
    if (!_cur_item && _cur_block) {
        switch (_cur_block->_type) {
        case ast::eTokenType::eIf:
        case ast::eTokenType::eElif:
        case ast::eTokenType::eWhile:
            _cur_block->_condition = tokv->value();
            break;
        default:
            break;
        }
    }
}

void MscParser::addProperty(const MscLexer::Token *tokp, const MscLexer::Token *tokv)
{
    auto type = ast::getTokenType(tokp->_id);
    if (_cur_item) {
        switch (_cur_item->_type) {
        case ast::eTokenType::eInstance: {
            auto *instance = static_cast<ast::Instance *>(_cur_item);
            auto &p = addProperty(type, tokv, instance->_properties);
            if (p._type == ast::eTokenType::eXForm) {
                handleXform(_cur_item, p, tokv);
            }
        } break;
        case ast::eTokenType::eText:
        case ast::eTokenType::eMessage: {
            std::list<ast::Property> *props = nullptr;
            if (auto *message = static_cast<ast::Message *>(_cur_item)) {
                props = &message->_properties;
            } else if (auto *text = static_cast<ast::Text *>(_cur_item)) {
                props = &text->_properties;
            }
            auto &p = addProperty(type, tokv, *props);
            if (p._type == ast::eTokenType::eAnno) {
                handleAnnoFlag(p, *props, tokp, tokv);
            } else if (p._type == ast::eTokenType::eXForm) {
                handleXform(_cur_item, p, tokv);
            }
        } break;
        case ast::eTokenType::eReference: {
            auto *ref = static_cast<ast::Reference *>(_cur_item);
            auto &p = addProperty(type, tokv, ref->_properties);
            if (p._type == ast::eTokenType::eXForm) {
                handleXform(_cur_item, p, tokv);
            }
        } break;
        default:
            break;
        }
    } else if (_cur_block) {
        switch (_cur_block->_type) {
        case ast::eTokenType::eMsc: {
            auto *msc = static_cast<ast::Msc *>(_cur_block);
            auto &p = addProperty(type, tokv, msc->_properties);
            if (p._type == ast::eTokenType::eXForm) {
                handleXform(_cur_block, p, tokv);
            }
        } break;
        default:
            break;
        }
    }
}

ast::Property &MscParser::addProperty(ast::eTokenType type,
                                      const MscLexer::Token *tokv,
                                      std::list<ast::Property> &properties)
{
    auto &p = properties.emplace_back();
    p._type = type;
    p._value = tokv->value();
    return p;
}

void MscParser::handleAnnoFlag(ast::Property &property,
                               std::list<ast::Property> &properties,
                               const MscLexer::Token *tokp,
                               const MscLexer::Token *tokv)
{
    int nAnno = std::count_if(properties.begin(), properties.end(), [](const ast::Property &p) {
        return p._type == ast::eTokenType::eAnno;
    });
    if (nAnno > 2) {
        error_register("too much annotations (only 2 max)", tokp->startPos(), tokp->endPos());
        return;
    }
    int index = 0;
    int flag = ast::eFlag::eNone;
    std::string warning;
    if ((index = startsWithNoCase(property._value, "left=")) != 0) {
        if (_cur_item->_flags & (ast::eFlag::eAnno1Left | ast::eFlag::eAnno2Left))
            warning = "annotations placement cannot be both 'left'";
        flag = (nAnno == 1) ? ast::eFlag::eAnno1Left : ast::eFlag::eAnno2Left;
    } else if ((index = startsWithNoCase(property._value, "right=")) != 0) {
        if (_cur_item->_flags & (ast::eFlag::eAnno1Right | ast::eFlag::eAnno2Right))
            warning = "annotations placement cannot be both 'right'";
        flag = (nAnno == 1) ? ast::eFlag::eAnno1Right : ast::eFlag::eAnno2Right;
    } else if ((index = startsWithNoCase(property._value, "from=")) != 0) {
        if (_cur_item->_flags & (ast::eFlag::eAnno1From | ast::eFlag::eAnno2From))
            warning = "annotations placement cannot be both 'from'";
        flag = (nAnno == 1) ? ast::eFlag::eAnno1From : ast::eFlag::eAnno2From;
    } else if ((index = startsWithNoCase(property._value, "to=")) != 0) {
        if (_cur_item->_flags & (ast::eFlag::eAnno1To | ast::eFlag::eAnno2To))
            warning = "annotations placement cannot be both 'to'";
        flag = (nAnno == 1) ? ast::eFlag::eAnno1To : ast::eFlag::eAnno2To;
    }
    if (index) {
        property._value = property._value.substr(index);
        if (!warning.empty()) {
            error_register(warning, tokv->startPos(), tokv->endPos());
        } else {
            _cur_item->_flags |= flag;
        }
    }
}

void MscParser::handleXform(ast::Item *item, ast::Property &property, const MscLexer::Token *tokv)
{
    std::string warning;
    if (item->_type == ast::eTokenType::eInstance) {
        if (compareNoCase(property._value, "shape=fifo")) {
            item->_flags |= ast::eFlag::eInstanceShapeFIFO;
        } else {
            warning = "unexpected xform value for instance";
        }
    } else if (item->_type == ast::eTokenType::eMessage) {
        if (compareNoCase(property._value, "arrow=dash")) {
            item->_flags |= ast::eFlag::eMessageArrowDash;
        } else {
            warning = "unexpected xform value for message";
        }
    } else if (item->_type == ast::eTokenType::eText) {
        if (compareNoCase(property._value, "shape=round")) {
            item->_flags |= ast::eFlag::eTextShapeRound;
        } else {
            warning = "unexpected xform value for text";
        }
    } else if (item->_type == ast::eTokenType::eReference) {
        if (compareNoCase(property._value, "expanded")) {
            item->_flags |= ast::eFlag::eRefExpanded;
            item->_flags &= ~ast::eFlag::eRefInline;
        } else if (compareNoCase(property._value, "inline")) {
            item->_flags |= ast::eFlag::eRefInline;
            item->_flags &= ~ast::eFlag::eRefExpanded;
        } else {
            warning = "unexpected xform value for reference";
        }
    } else if (item->_type == ast::eTokenType::eMsc) {
        auto *msc = static_cast<ast::Msc *>(item);
        if (int index = startsWithNoCase(property._value, "yitem")) {
            msc->_yitem = setParsedValue(property._value.substr(index), ast::defs::itemYSpace);
        } else if (int index = startsWithNoCase(property._value, "xinst")) {
            msc->_xinst = setParsedValue(property._value.substr(index), ast::defs::instanceXSpace);
        } else if (int index = startsWithNoCase(property._value, "szfont")) {
            msc->_fontSize = setParsedValue(property._value.substr(index),
                                            ast::defs::invalidFontSize);
        } else if (int index = startsWithNoCase(property._value, "font=")) {
            msc->_fontName = property._value.substr(index);
        } else {
            warning = "unexpected xform value for msc";
        }
    }
    if (!warning.empty()) {
        error_register(warning, tokv->startPos(), tokv->endPos());
    }
}

int MscParser::setParsedValue(const std::string_view &svalue, int defaultValue)
{
    int rvalue = defaultValue;
    std::size_t index = 0;
    if (svalue.size() > index) {
        char op = svalue[index];
        char aff = op;
        if ((op == '+') || (op == '-') || (op == '/') || (op == '*')) {
            ++index;
            if (svalue.size() > index)
                aff = svalue[index];
        }
        if (aff == '=') {
            ++index;
            if (svalue.size() > index) {
                long value = std::strtol(svalue.data() + index, nullptr, 10);
                if (value) {
                    // clang-format off
                    switch(op) {
                    case '+': rvalue = defaultValue + value; break;
                    case '-': rvalue = defaultValue - value; break;
                    case '/': rvalue = defaultValue / value; break;
                    case '*': rvalue = defaultValue * value; break;
                    default: rvalue = value; break;
                    }
                    // clang-format on
                }
            }
        }
    }
    return (rvalue > 0) ? rvalue : defaultValue;
}

//
namespace ast {
// clang-format off
#define AST_GET_ELEMENT(E) do { \
        if (auto *message = std::get_if<Message>(E)) { \
            return message; \
        } else if (auto *ref = std::get_if<Reference>(E)) { \
            return ref; \
        } else if (auto *text = std::get_if<Text>(E)) { \
            return text; \
        } else if (auto *cond = std::get_if<Condition>(E)) { \
            return cond; \
        } else if (auto *block = std::get_if<Block>(E)) { \
            return block; \
        } \
        return nullptr; \
    } while (0)

Item *getElement(Element *element) { AST_GET_ELEMENT(element); }
const Item *getElement(const Element *element) { AST_GET_ELEMENT(element); }
// clang-format on

const Item *findItemByPos(const MscList &ast, unsigned int pos)
{
    for (auto &msc : ast._mscs) {
        if (msc._pos == pos)
            return &msc;
        for (auto &i : msc._instances) {
            if (i._pos == pos)
                return &i;
        }
        if (auto *found = findItemByPos(msc, pos))
            return found;
    }
    return nullptr;
}

const Item *findItemByPos(const Block &block, unsigned int pos)
{
    for (auto &e : block._elements) {
        auto i = getElement(&e);
        if (i->_pos == pos)
            return i;
        if (auto *block = dynamic_cast<const Block *>(i)) {
            if (auto *found = findItemByPos(*block, pos))
                return found;
        }
    }
    return nullptr;
}

eTokenType getTokenType(int token)
{
    // clang-format off
    switch(token) {
    case TK_STRING : return eTokenType::eString;
    case TK_LCOMMENT :
    case TK_BCOMMENT : return eTokenType::eComment;
    case TK_MSC : return eTokenType::eMsc;
    case TK_INSTANCE : return eTokenType::eInstance;
    case TK_MESSAGE : return eTokenType::eMessage;
    case TK_REFERENCE : return eTokenType::eReference;
    case TK_TEXT : return eTokenType::eText;
    case TK_WHILE : return eTokenType::eWhile;
    case TK_IF : return eTokenType::eIf;
    case TK_ELIF : return eTokenType::eElif;
    case TK_ELSE : return eTokenType::eElse;
    case TK_S_NAME : return eTokenType::eName;
    case TK_S_FROM : return eTokenType::eFrom;
    case TK_S_TO : return eTokenType::eTo;
    case TK_S_ANNO : return eTokenType::eAnno;
    case TK_S_SUBTEXT : return eTokenType::eSubtext;
    case TK_S_XFORM : return eTokenType::eXForm;
    case TK_OB : return eTokenType::eOB;
    case TK_CB : return eTokenType::eCB;
    case TK_OP : return eTokenType::eOP;
    case TK_CP : return eTokenType::eCP;
    case TK_COLON : return eTokenType::eColon;
    }
    // clang-format on
    return eTokenType::eUnknown;
}

eTokenCategory getTokenCategory(int token)
{
    // clang-format off
    switch(token) {
    case TK_STRING : return eTokenCategory::eString;
    case TK_LCOMMENT:
    case TK_BCOMMENT: return eTokenCategory::eComment;
    case TK_INSTANCE :
    case TK_MESSAGE :
    case TK_REFERENCE :
    case TK_TEXT : return eTokenCategory::eElement;
    case TK_WHILE :
    case TK_MSC :
    case TK_IF :
    case TK_ELIF :
    case TK_ELSE : return eTokenCategory::eBlock;
    case TK_S_NAME :
    case TK_S_FROM :
    case TK_S_TO :
    case TK_S_ANNO :
    case TK_S_SUBTEXT :
    case TK_S_XFORM : return eTokenCategory::eProperty;
    case TK_OB :
    case TK_CB :
    case TK_OP :
    case TK_CP :
    case TK_COLON : return eTokenCategory::eSymbol;
    }
    // clang-format on
    return eTokenCategory::eUnknown;
}

std::string_view getPropertyName(eTokenType type)
{
    // clang-format off
    switch(type) {
    case eTokenType::eName: return "name";
    case eTokenType::eXForm: return "xform";
    case eTokenType::eFrom: return "from";
    case eTokenType::eTo: return "to";
    case eTokenType::eSubtext: return "subtext";
    case eTokenType::eAnno: return "anno";
    default: break;
    }
    // clang-format on
    return "UNKNOWN";
}

std::optional<std::string_view> getPropertyValue(const std::list<Property> &properties,
                                                 eTokenType type,
                                                 int index)
{
    for (auto const &p : properties) {
        if (p._type == type) {
            if (index == 0) {
                return p._value;
            }
            --index;
        }
    }
    return std::nullopt;
}

std::optional<std::string_view> getPropertyValue(const Element &element, eTokenType type, int index)
{
    std::optional<std::string_view> r = std::nullopt;
    if (auto *text = std::get_if<ast::Text>(&element)) {
        r = getPropertyValue(text->_properties, type, index);
    } else if (auto *message = std::get_if<ast::Message>(&element)) {
        r = getPropertyValue(message->_properties, type, index);
    } else if (auto *ref = std::get_if<ast::Reference>(&element)) {
        r = getPropertyValue(ref->_properties, type, index);
    }
    return r;
}

const std::list<Property> &getProperties(const ast::Item *item)
{
    static const std::list<Property> empty;
    // clang-format off
    switch (item->_type) {
    case eTokenType::eMsc: return dynamic_cast<const Msc *>(item)->_properties;
    case eTokenType::eInstance: return dynamic_cast<const Instance *>(item)->_properties;
    case eTokenType::eReference: return dynamic_cast<const Reference *>(item)->_properties;
    case eTokenType::eText: return dynamic_cast<const Text *>(item)->_properties;
    case eTokenType::eMessage: return dynamic_cast<const Message *>(item)->_properties;
    default: break;
    }
    // clang-format on
    return empty;
}

void getPropertiesForItem(const ast::Item &item,
                          const std::function<void(const std::string_view &name)> &cb)
{
    std::vector<eTokenType> allowed;
    if (item._type == eTokenType::eMsc || item._type == eTokenType::eInstance
        || item._type == eTokenType::eReference) {
        allowed = {eTokenType::eName, eTokenType::eXForm};
    } else if (item._type == eTokenType::eMessage || item._type == eTokenType::eText) {
        allowed = {eTokenType::eName,
                   eTokenType::eFrom,
                   eTokenType::eTo,
                   eTokenType::eAnno,
                   eTokenType::eXForm};
        if (item._type == eTokenType::eMessage) {
            allowed.push_back(eTokenType::eSubtext);
        }
    }
    // remove already set properties
    auto const &props = getProperties(&item);
    for (auto const &i : allowed) {
        bool alreadyPresent = false;
        if (i != eTokenType::eXForm) {
            int max = (i == eTokenType::eAnno) ? 2 : 1;
            alreadyPresent = (std::count_if(props.begin(),
                                            props.end(),
                                            [i](const Property &p) { return p._type == i; })
                              >= max);
        }
        if (!alreadyPresent) {
            cb(getPropertyName(i));
        }
    }
}

void getPropertyValuesForItem(const ast::MscList &ast,
                              const ast::Item &item,
                              ast::eTokenType propType,
                              const std::function<void(const std::string_view &name)> &cb)
{
    std::set<std::string_view> wList;
    if (item._type == eTokenType::eMessage || item._type == eTokenType::eText) {
        if (!ast._mscs.empty() && (propType == eTokenType::eTo || propType == eTokenType::eFrom)) {
            for (auto const &i : ast._mscs.front()._instances) {
                if (auto name = getPropertyValueT(i, eTokenType::eName)) {
                    wList.insert(*name);
                }
            }
        }
    } else if (item._type == eTokenType::eReference) {
        if (propType == eTokenType::eName) {
            for (auto const &msc : ast._mscs) {
                if (auto name = getPropertyValueT(msc, eTokenType::eName)) {
                    wList.insert(*name);
                }
            }
        }
    }
    if (propType == eTokenType::eAnno) {
        wList = {"from=", "to=", "left=", "right="};
    } else if (propType == eTokenType::eXForm) {
        if (item._type == eTokenType::eInstance) {
            wList = {"shape=fifo"};
        } else if (item._type == eTokenType::eText) {
            wList = {"shape=round"};
        } else if (item._type == eTokenType::eMessage) {
            wList = {"arrow=dash"};
        } else if (item._type == eTokenType::eReference) {
            wList = {"inline", "expanded"};
        } else if (item._type == eTokenType::eMsc) {
            wList = {"yitem+=0", "xinst+=0", "szfont+=0", "font="};
        }
    }
    for (auto const &w : wList) {
        if (!w.empty()) {
            cb(w);
        }
    }
}

} // namespace ast

} // namespace qmsc
