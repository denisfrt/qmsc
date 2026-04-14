#include "visitor_default.h"

namespace qmsc {

// DefaultVisitor
void DefaultVisitor::visit(const ast::MscList &msc_list)
{
    for (auto const &msc : msc_list._mscs) {
        visit(msc);
    }
}

void DefaultVisitor::visit(const ast::Msc &msc)
{
    ++_current_depth;
    for (auto const &inst : msc._instances) {
        visit(inst);
    }
    --_current_depth;
    visit(msc._elements);
}

void DefaultVisitor::visit(const ast::Element &element)
{
    ++_current_depth;
    if (auto *text = std::get_if<ast::Text>(&element)) {
        visit(element, *text);
    } else if (auto *message = std::get_if<ast::Message>(&element)) {
        visit(element, *message);
    } else if (auto *ref = std::get_if<ast::Reference>(&element)) {
        visit(element, *ref);
    } else if (auto *cond = std::get_if<ast::Condition>(&element)) {
        --_current_depth;
        visit(cond->_elements);
        ++_current_depth;
    } else if (auto *block = std::get_if<ast::Block>(&element)) {
        visit(element, *block);
    }
    --_current_depth;
}

void DefaultVisitor::visit(const ast::Instance &) {}
void DefaultVisitor::visit(const ast::Element &, const ast::Text &) {}
void DefaultVisitor::visit(const ast::Element &, const ast::Message &) {}
void DefaultVisitor::visit(const ast::Element &, const ast::Reference &) {}

void DefaultVisitor::visit(const ast::Element &, const ast::Block &block)
{
    visit(block._elements);
}

DefaultVisitor::DefaultVisitor()
    : _current_depth(0)
{}

void DefaultVisitor::visit(const std::list<ast::Element> &elements)
{
    for (auto &e : elements) {
        visit(e);
    }
}

} // namespace qmsc
