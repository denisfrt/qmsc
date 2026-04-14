#include <string>
#include <iostream>
#include "lexer.h"
#include "generated_parser.h"

namespace qmsc {

/*!include:re2c "unicode_categories.re" */

bool MscLexer::next(Token &res)
{
  MscLexer::resetToken(res);
  _cursor = _buf;
  while (1) {
    _token = _cursor;
    /*!re2c
    re2c:encoding:utf8   = 1;
    re2c:define:YYCTYPE  = "unsigned char";
    re2c:define:YYCURSOR = _cursor;
    re2c:define:YYMARKER = _marker;
    re2c:indent:top      = 2;
    re2c:yyfill:enable   = 0;
    re2c:yych:conversion = 1;

    WS     = [ \t\r]+ ;
    STRING = "\""[^"\r\n\000]*"\"" ;
    ANY    = [^] ;

    WS              { continue; }
    "/" "/"         { startToken(res, TK_LCOMMENT);  goto cpp_comment; }
    "/" "*"         { startToken(res, TK_BCOMMENT);  goto blk_comment; }
    'MSC'           { startToken(res, TK_MSC);       goto tok_found; }
    'INSTANCE'      { startToken(res, TK_INSTANCE);  goto tok_found; }
    'MESSAGE'       { startToken(res, TK_MESSAGE);   goto tok_found; }
    'REFERENCE'     { startToken(res, TK_REFERENCE); goto tok_found; }
    'TEXT'          { startToken(res, TK_TEXT);      goto tok_found; }
    'WHILE'         { startToken(res, TK_WHILE);     goto tok_found; }
    'IF'            { startToken(res, TK_IF);        goto tok_found; }
    'ELIF'          { startToken(res, TK_ELIF);      goto tok_found; }
    'ELSE'          { startToken(res, TK_ELSE);      goto tok_found; }

    'from'          { startToken(res, TK_S_FROM);    goto tok_found; }
    'to'            { startToken(res, TK_S_TO);      goto tok_found; }
    'name'          { startToken(res, TK_S_NAME);    goto tok_found; }
    'anno'          { startToken(res, TK_S_ANNO);    goto tok_found; }
    'subtext'       { startToken(res, TK_S_SUBTEXT); goto tok_found; }
    'xform'         { startToken(res, TK_S_XFORM);   goto tok_found; }

    ":"             { startToken(res, TK_COLON);     goto tok_found; }
    "("             { startToken(res, TK_OP);        goto tok_found; }
    ")"             { startToken(res, TK_CP);        goto tok_found; }
    "{"             { startToken(res, TK_OB);        goto tok_found; }
    "}"             { startToken(res, TK_CB);        goto tok_found; }

    STRING          { startToken(res, TK_STRING);    goto tok_found; }
    "\n"            { _line++;
                      _col = 0;
                      _buf = _cursor;                continue; }
    "\000"          { break; }
    ANY             { startToken(res, TK_UNKNOWN);   continue; }
    */

    blk_comment:
    /*!re2c
      "*" "/"       {                                goto comment_end; }
      "\n"          { _line++;
                      _col = 0;
                      _buf = _cursor;                goto blk_comment; }
      "\000"        {                                goto comment_end; }
      ANY           { goto blk_comment; }
    */

    cpp_comment:
    /*!re2c
      "\n"          { _line++; _col = 0;             goto comment_end; }
      "\000"        { _line++; _col = 0;             goto comment_end; }
      ANY           { goto cpp_comment; }
    */

    comment_end:
      _buf = _cursor;
      endToken(res);
      //MscLexer::showToken(res);
      return true;

    tok_found :
      _col += (_cursor - _buf);
      _buf = _cursor;
      endToken(res);
      //MscLexer::showToken(res);
      return true;
  }
  return false;
}

} // namespace qmsc