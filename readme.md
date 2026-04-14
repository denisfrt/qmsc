# QMSC

# Philosophy
Qmsc is a simple Qt application which permit to create, edit and visualize a [Message Sequence Chart](https://en.wikipedia.org/wiki/Message_sequence_chart).

In spirit, it is similar to [mermaid](https://mermaid.ai/web/) [Sequence Diagrams](https://mermaid.ai/open-source/syntax/sequenceDiagram.html) but all integrated in the same application with helpers for edition, syntax highlighting and visualization as you type.

I developed/tested this application on Linux/Windows with [Qt 6.10.2](https://www.qt.io/development/qt-framework).

Building steps are detailed in [build.md](docs/build.md).

This project is licensed under [LGPL 2.1](LICENSE).

# GUI Quick overview

An Msc document is just a textual file (.msc extension) written with the language defined in [language details](#language-details).

Multiple Msc documents can be opened at the same time as Qmsc is an MDI application.

Each document is displayed in a split window showing the graphical and textual representation of the chart (see [hereafter](#main-view))

Examples are available in the menu File&rarr;Recent Files:
 - default.msc : complete language possibilities (also used when creating a new document using File&rarr;New).
 - anno.msc : examples focusing on annotations
 - condition.msc : examples focusing on if/elif/else/while blocks
 - reference.msc : examples focusing on msc references

## Main view
![Main View](docs/main.png)

The main document view is composed of two parts:
- Upper side &rarr; graph view : graphical MSC representation
- Lower side &rarr; editor view : textual source definition of the MSC

Both views are syncrhonized in real-time and modifications in the source text will instantaneously be displayed in the graphical view.

Selecting an element in the editor view will select it graphical counterpart in the graph view and vice-versa.

### Editor view details

The editor view have the following features:
 - language syntax highlighting (colors can be changed in [settings](#settings))
 - language grammar error reporting:
   - status displayed in lower-right part (OK/green or X errors as red)
   - if a syntax error is detected, a red squiggly line is displayed under the faulty text (hover over it to display more details as tooltip).
 - contextual auto-completion triggered with ctrl+space:
   - in elements, it will show available properties to add,
   - in properties, it will show possible values:
     - from/to property: lists already defined instances.
     - reference name: lists other defined msc.
     - xform property: lists supported values for elements.

### Graph view details

The graph view have the following features:
 - automatic graphical components placement.
 - instances can be locked in the view to always display them while scrolling vertically (Menu Window&rarr;Lock Instances).
 - whole graph can be centered in the view (Menu Window&rarr;Center Msc)
 - you can zoom in/out using ctrl + mouse wheel (if not 100%, zoom factor is shown in the lower right window corner).

## Settings
![Settings Dialog](docs/settings.png)

This dialog lets you change colors and font used in the editor view and the default font used in the graph view. Note that the graph view default font can be overriden by its source definion (see [msc properties](#msc-blocks)).

# Language details

## EBNF Grammar
```
mscs = msc*
msc = "msc" , "{" , {property} , {element} , "}" ;
element = instance | message | text | reference | block ;
instance = "instance" , "{" , {property} , "}" ;
message = "message" , "{" , {property} , "}" ;
reference = "reference" , "{" , {property} , "}" ;
block = condition | while ;
condition = "if" , "(", string , ")" , "{" , {element} , "}" ,
           {"elif" , "(" , string , ")" , "{" {element} , "}"} ,
           ["else" , "{" , {element} , "}"]
while = "while" , "(" , string , ")" , "{" , {element} , "}"
property = ("name" | "from" | "to" | "anno" | "subtext" | "xform") ,":" , string
string = '"' , any - '"' , '"' ;
any = ? any character ? ;
```

## msc blocks

This block element represents the main message sequence diagram definition. Multiple msc blocks can be set in the source but only the first one will be displayed in the view. Others can still be used as [reference](#reference-elements).

The following properties are supported for this element:
 - **name**: logical name of the msc
 - **xform**: graphical oriented properties
   - **yitem**:
     - xform:"**yitem=** X": sets vertical margin between elements to X pixels.
     - xform:"**yitem+=** Y": adds Y pixels to the default vertical margin.
     - xform:"**yitem-=** Z": removes Z pixels to the default vertical margin.
   - **xinst**:
     - xform:"**xinst=** X": sets horizontal space between instances to X pixels.
     - xform:"**xinst+=** Y": adds Y pixels to the default space between instances.
     - xform:"**xinst-=** Z": removes Z pixels to the default space between instances.
   - **szfont**: (overrides the font size set in [settings](#settings))
     - xform:"**szfont=** X": sets graph font size to X points.
     - xform:"**szfont+=** Y": adds Y points to the font size.
     - xform:"**szfont-=** Y": removes Z points to the font size.
   - xform:"**font=** family": sets the font family used to display graph (overrides the font set in [settings](#settings))

## instance elements

This element represents an instance/task in an MSC.

![Instance Example](docs/instance.png)

The following properties are supported for this element:
 - **name**: displayed instance name
 - **xform**: graphical oriented properties
   - xform:"**shape=fifo**": display the instance as a FIFO instead of a rectangle.

## message/text elements

These elements represent a message/text displayed between instances (from/to).

![Message Example](docs/message.png)

A "message" element have an arrow drawn between "from" and "to" instance, a main text ("name") shown above the arrow and a "subtext" shown below the arrow.

A "text" element is displayed in a rectangle between "from" and "to" instance.

The following properties are supported for this element:
 - **name**: displayed text above arrow for message or in rectangle for text element.
 - **from** / **to**: instance name source and destination of the element.
 - **anno**: textual annotations displayed at its left and/or right side (maximum two). By default, the first annotation will be displayed left and the second to the right side. This can be changed with the following keywords:
   - anno:"**from=** text": forces the annotation to be displayed on the 'from' element side.
   - anno:"**to=** text": forces the annotation to be displayed on the 'to' element side.
   - anno:"**left=** text": forces the annotation to be displayed on the 'left' element side.
   - anno:"**right=** text": forces the annotation to be displayed on the 'right' element side.
 - **subtext**: text displayed under the arrow (only supported for messages element)
 - **xform**: graphical oriented properties
   - xform:"**arrow=dash**": display arrow with a dash line (only message element). 
   - xform:"**shape=round**": display text in a rounded rather than a square rectangle (only text element).

## reference elements

A "reference" is an alias for another defined msc in the document. By default, a reference is displayed as a rounded rectangle with its name in the middle and a '+' sign in the upper left corner. References have the following features:
 - in the graph view, it can be temporarily expanded/collapsed by clicking the '+'/'-' part or double-clicking in the rectangle.
 - in the editor view, you can choose to expand it by default (expanded) or just as is (inline) with the xform property.

![Reference Example](docs/reference.png)

The following properties are supported for this element:
 - **name**: referenced msc name displayed in a rounded rectangle by default.
 - **xform**: graphical oriented properties
   - xform:**"expanded"**: referenced msc is displated expanded by default.
   - xform:**"inline"**: referenced msc is displayed as if defined in-place (no visual clue that it is a reference).

## logical blocks

To define optional elements of an MSC or multiple alternatives, you can use IF/ELIF/ELSE keywords. Each optional parts will be displayed in a dash rectangle titled with its respective condition.

![Block Example](docs/block.png)

**Condition Example**:
```
if ("condition") {
    message { ... }
    text { ... }
} elif ("alternative 1") {
    ...
} elif ("alternative 2") {
    ...
} else {
    ...
}
```

To define a repeated message sequence, you can used the WHILE keyword. Repeated part elements will be displayed in a solid line rectangle title by the WHILE condition.

**While Example**:
```
while ("condition") {
    message { ... }
    text { ... }
}
```

Blocks can be nested as much as you want.

## special string formatting keywords
For most properties, you can use the following special escape sequences:
 - **\n**: multiline text
 - **\b**: bold text line
 - **\i**: italic text line
 - **\u**: underlined text line
 - **\l**: left aligned text line
 - **\r**: right aligned text line
 - color definition either with hexadecimal AARRGGBB or supported [color keywords](https://www.w3.org/TR/SVG11/types.html#ColorKeywords):
   - "**\cb\<color\>**": background color, applied for whole element:
     - blue background: "**\cbff**" or "**\cb blue**"
   - "**\ct\<color\>**": text color, applied for the current line:
     - red text: "**\ctff0000**" or "**\cb red**""
