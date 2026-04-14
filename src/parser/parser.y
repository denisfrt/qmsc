%token_prefix TK_

%name mscParser
%stack_size 200
// terminal type
%token_type {qmsc::MscLexer::Token}
// non-terminal default type
//%default_type {qmsc::MscLexer::Token}
%extra_argument {qmsc::MscParser *ctx}

%include {
#include <string>
#include <any>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
}

%syntax_error {
    ctx->error_register("syntax mismatch");
}

%stack_overflow {
    ctx->error_register("stack overflow");
}

file ::= msc_list.

msc_list ::= msc.
msc_list ::= msc_list msc.

msc ::= msc_start OB prop_msc_list_opt
                     element_list_opt
                  CB(A).                      { ctx->end(&A); }
msc ::= error.                                { ctx->end(); }

msc_start ::= MSC(A).                         { ctx->start(&A); }

property1 ::= S_NAME(A)    COLON STRING(B).   { ctx->addProperty(&A, &B); }
property2 ::= S_FROM(A)    COLON STRING(B).   { ctx->addProperty(&A, &B); }
property2 ::= S_TO(A)      COLON STRING(B).   { ctx->addProperty(&A, &B); }
property2 ::= S_ANNO(A)    COLON STRING(B).   { ctx->addProperty(&A, &B); }
property3 ::= S_XFORM(A)   COLON STRING(B).   { ctx->addProperty(&A, &B); }
property4 ::= S_SUBTEXT(A) COLON STRING(B).   { ctx->addProperty(&A, &B); }

prop_msc ::= property1.
prop_msc ::= property3.
prop_msc_list ::= prop_msc.
prop_msc_list ::= prop_msc_list prop_msc.
prop_msc_list ::= error.
prop_msc_list_opt ::= prop_msc_list.
prop_msc_list_opt ::= .

prop_inst ::= property1.
prop_inst ::= property3.
prop_inst_list ::= prop_inst.
prop_inst_list ::= prop_inst_list prop_inst.
prop_inst_list ::= error.
prop_inst_list_opt ::= prop_inst_list.
prop_inst_list_opt ::= .

prop_ref ::= property1.
prop_ref ::= property3.
prop_ref_list ::= prop_ref.
prop_ref_list ::= prop_ref_list prop_ref.
prop_ref_list ::= error.
prop_ref_list_opt ::= prop_ref_list.
prop_ref_list_opt ::= .

prop_text ::= property1.
prop_text ::= property2.
prop_text ::= property3.
prop_text_list ::= prop_text.
prop_text_list ::= prop_text_list prop_text.
prop_text_list ::= error.
prop_text_list_opt ::= prop_text_list.
prop_text_list_opt ::= .

prop_mesg ::= property1.
prop_mesg ::= property2.
prop_mesg ::= property3.
prop_mesg ::= property4.
prop_mesg_list ::= prop_mesg.
prop_mesg_list ::= prop_mesg_list prop_mesg.
prop_mesg_list ::= error.
prop_mesg_list_opt ::= prop_mesg_list.
prop_mesg_list_opt ::= .

element_list_opt ::= element_list.
element_list_opt ::= .
element_list ::= element.
element_list ::= element_list element.
element_list ::= error.                               { ctx->end(); }

element ::= inst_start   OB prop_inst_list_opt CB(A). { ctx->end(&A); }
element ::= msg_start    OB prop_mesg_list_opt CB(A). { ctx->end(&A); }
element ::= text_start   OB prop_text_list_opt CB(A). { ctx->end(&A); }
element ::= ref_start    OB prop_ref_list_opt CB(A).  { ctx->end(&A); }

element ::= while_start  OB element_list_opt CB(A).   { ctx->end(&A); }
element ::= if_start     OB element_list_opt CB(A).   { ctx->end(&A); }
element ::= elif_start   OB element_list_opt CB(A).   { ctx->end(&A); }
element ::= else_start   OB element_list_opt CB(A).   { ctx->end(&A); }

inst_start  ::= INSTANCE(A).              { ctx->start(&A); }
msg_start   ::= MESSAGE(A).               { ctx->start(&A); }
ref_start   ::= REFERENCE(A).             { ctx->start(&A); }
text_start  ::= TEXT(A).                  { ctx->start(&A); }
while_start ::= WHILE(A) OP STRING(B) CP. { ctx->start(&A); ctx->addConditionProperty(&B); }
if_start    ::= IF(A) OP STRING(B) CP.    { ctx->start(&A); ctx->addConditionProperty(&B); }
elif_start  ::= ELIF(A) OP STRING(B) CP.  { ctx->start(&A); ctx->addConditionProperty(&B); }
else_start  ::= ELSE(A).                  { ctx->start(&A); }
