#pragma once
#include "ast.h"

namespace qmsc {

class VisitorInterface
{
public:
    // Msc
    virtual void visit(const ast::MscList &) = 0;
    virtual void visit(const ast::Msc &) = 0;
    // Element
    virtual void visit(const ast::Element &) = 0;
    virtual void visit(const ast::Instance &) = 0;
    virtual void visit(const ast::Element &, const ast::Text &) = 0;
    virtual void visit(const ast::Element &, const ast::Message &) = 0;
    virtual void visit(const ast::Element &, const ast::Reference &) = 0;
    virtual void visit(const ast::Element &, const ast::Block &) = 0;
};

} // namespace qmsc
