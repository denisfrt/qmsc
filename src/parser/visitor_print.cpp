#include "visitor_print.h"
#include <iostream>

namespace qmsc {

PrintVisitor::PrintVisitor(std::ostream &os, const std::string &newline)
    : _os(os)
    , _newline(newline)
    , _current_indent(0)
    , _indent_size(2)
{}

std::string_view PrintVisitor::getIndent()
{
    auto indent = (_current_indent + _current_depth) * _indent_size;
    if (indent > _indent_buf.size()) {
        _indent_buf.assign(indent, ' ');
    }
    return std::string_view(_indent_buf.data(), indent);
}

void PrintVisitor::visit(const ast::MscList &msc_list)
{
    for (auto const &msc : msc_list._mscs) {
        visit(msc);
        _os << _newline;
    }
}

void PrintVisitor::visit(const ast::Msc &msc)
{
    _os << "msc {" << _newline;
    ++_current_indent;
    if (!msc._properties.empty()) {
        _os << getIndent();
        visit(msc._properties);
        _os << _newline;
    }
    --_current_indent;
    DefaultVisitor::visit(msc);
    _os << '}' << _newline;
}

void PrintVisitor::visit(const ast::Instance &inst)
{
    _os << getIndent() << "instance { ";
    visit(inst._properties);
    _os << '}' << _newline;
}

void PrintVisitor::visit(const ast::Element &, const ast::Text &text)
{
    _os << getIndent() << "text { ";
    visit(text._properties);
    _os << '}' << _newline;
}

void PrintVisitor::visit(const ast::Element &, const ast::Message &msg)
{
    _os << getIndent() << "message { ";
    visit(msg._properties);
    _os << '}' << _newline;
}

void PrintVisitor::visit(const ast::Element &, const ast::Reference &ref)
{
    _os << getIndent() << "reference { ";
    visit(ref._properties);
    _os << '}' << _newline;
}

void PrintVisitor::visit(const ast::Element &, const ast::Block &block)
{
    switch (block._type) {
    case ast::eTokenType::eIf:
        _os << getIndent() << "if (\"" << block._condition << "\") {" << _newline;
        break;
    case ast::eTokenType::eElif:
        _os << getIndent() << "elif (\"" << block._condition << "\") {" << _newline;
        break;
    case ast::eTokenType::eWhile:
        _os << getIndent() << "while (\"" << block._condition << "\") {" << _newline;
        break;
    case ast::eTokenType::eElse:
        _os << getIndent() << "else {" << _newline;
        break;
    default:
        break;
    }
    DefaultVisitor::visit(block._elements);
    _os << getIndent() << '}' << _newline;
}

void PrintVisitor::visit(const std::list<ast::Property> &properties)
{
    for (auto const &p : properties) {
        _os << ast::getPropertyName(p._type) << ':' << '\"' << p._value << "\" ";
    }
}

} // namespace qmsc
