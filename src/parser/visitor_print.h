#pragma once
#include "ast.h"
#include "visitor_default.h"

namespace qmsc {

class PrintVisitor : public DefaultVisitor
{
    using DefaultVisitor::visit;
    // Interface
public:
    void visit(const ast::MscList &) override;
    void visit(const ast::Msc &) override;
    void visit(const ast::Instance &) override;
    void visit(const ast::Element &, const ast::Text &) override;
    void visit(const ast::Element &, const ast::Message &) override;
    void visit(const ast::Element &, const ast::Reference &) override;
    void visit(const ast::Element &, const ast::Block &) override;

public:
    PrintVisitor(std::ostream &os, const std::string &newline);
    std::string_view getIndent();
    std::string getNewline() const;
    void visit(const std::list<ast::Property> &);

private:
    std::ostream &_os;
    std::string _newline;
    std::size_t _current_indent;
    std::size_t _indent_size;
    std::string _indent_buf;
};

} // namespace qmsc
