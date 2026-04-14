#pragma once
#include "ast.h"
#include "visitor.h"

namespace qmsc {

class DefaultVisitor : public VisitorInterface
{
public:
    void visit(const ast::MscList &) override;
    void visit(const ast::Msc &) override;
    // Element
    void visit(const ast::Element &) override;
    void visit(const ast::Instance &) override;
    void visit(const ast::Element &, const ast::Text &) override;
    void visit(const ast::Element &, const ast::Message &) override;
    void visit(const ast::Element &, const ast::Reference &) override;
    void visit(const ast::Element &, const ast::Block &) override;

public:
    DefaultVisitor();
    void visit(const std::list<ast::Element> &);

protected:
    std::size_t _current_depth;
};

} // namespace qmsc
