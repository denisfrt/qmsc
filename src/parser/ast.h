#pragma once

#include <functional>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace qmsc::ast {

namespace defs {
inline constexpr int instanceXSpace = 200;
inline constexpr int itemYSpace = 6;
inline constexpr int fontSize = 9;
inline constexpr int invalidFontSize = -1;
inline constexpr std::string_view fontName = "Sans Serif";
} // namespace defs

struct Text;
struct Message;
struct Reference;
struct Condition;
struct Block;

using Element = std::variant<Text, Message, Reference, Condition, Block>;

// clang-format off
enum class eTokenType : int { eUnknown = 0, eMsc, eInstance, eText, eMessage, eReference,
                              eCondition, eIf, eElif, eElse, eWhile, eString, eComment,
                              eName, eXForm, eFrom, eTo, eSubtext, eAnno,
                              eOB, eCB, eOP, eCP, eColon };
enum class eTokenCategory : int { eUnknown = 0, eElement, eBlock, eProperty, eString, eComment, eSymbol };
// clang-format on

enum eFlag : int {
    eNone = 0x0,
    eAnno1Left = 0x01,
    eAnno1Right = 0x02,
    eAnno1From = 0x04,
    eAnno1To = 0x08,
    eAnno1 = eAnno1Left | eAnno1Right | eAnno1From | eAnno1To,

    eAnno2Left = 0x10,
    eAnno2Right = 0x20,
    eAnno2From = 0x40,
    eAnno2To = 0x80,
    eAnno2 = eAnno2Left | eAnno2Right | eAnno2From | eAnno2To,

    eRefExpanded = 0x100,
    eRefInline = 0x200,
    eRefFound = 0x400,
    eTextShapeRound = 0x800,
    eInstanceShapeFIFO = 0x1000,
    eMessageArrowDash = 0x2000

};

struct Property
{
    eTokenType _type = eTokenType::eUnknown;
    std::string_view _value;
};

struct Item
{
    eTokenType _type = eTokenType::eUnknown;
    int _flags = eFlag::eNone;
    unsigned int _pos = 0;
    unsigned int _line = 0;
    unsigned int _col = 0;
    unsigned int _len = 0;
    virtual ~Item() = default;
};

struct Instance : Item
{
    std::list<Property> _properties;
};

struct Text : Item
{
    std::list<Property> _properties;
};

struct Message : Item
{
    std::list<Property> _properties;
};

struct Reference : Item
{
    std::list<Property> _properties;
};

struct Block : Item
{
    std::string _condition;
    std::list<Element> _elements;
    struct Block *_parent = nullptr;
};

struct Condition : Block
{};

struct Msc : Block
{
    int _yitem = defs::itemYSpace;       // y-pixels added/sub between elements
    int _xinst = defs::instanceXSpace;   // x-pixels between instances
    int _fontSize = defs::invalidFontSize; // font size
    std::string_view _fontName;            // font used
    std::list<Property> _properties;
    std::list<Instance> _instances;
};

struct MscList
{
    std::list<Msc> _mscs;
};

Item *getElement(Element *element);
const Item *getElement(const Element *element);
const Item *findItemByPos(const MscList &ast, unsigned int pos);
const Item *findItemByPos(const Block &block, unsigned int pos);
eTokenType getTokenType(int token);
eTokenCategory getTokenCategory(int token);
std::string_view getPropertyName(eTokenType type);
std::optional<std::string_view> getPropertyValue(const std::list<Property> &properties,
                                                 eTokenType type,
                                                 int index = 0);
std::optional<std::string_view> getPropertyValue(const Element &element,
                                                 eTokenType type,
                                                 int index = 0);
template<typename T>
std::optional<std::string_view> getPropertyValueT(const T &element, eTokenType type, int index = 0)
{
    return getPropertyValue(element._properties, type, index);
}
const std::list<Property> &getProperties(const ast::Item *item);
void getPropertiesForItem(const ast::Item &item,
                          const std::function<void(const std::string_view &name)> &cb);
void getPropertyValuesForItem(const ast::MscList &ast,
                              const ast::Item &item,
                              ast::eTokenType propType,
                              const std::function<void(const std::string_view &name)> &cb);
} // namespace qmsc::ast
