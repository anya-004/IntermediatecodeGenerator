%option noyywrap
%option nounput noinput

%{
#include "optimizer.h"
#include <string.h>
%}

%%

[ \t\r]+                  ;
"//"[^\n]*                ;
\#[^\n]*                  ;
"print"                   { return T_PRINT; }
[0-9]+                    { return T_NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]*    { return T_ID; }
"="                       { return T_ASSIGN; }
"+"                       { return T_PLUS; }
"-"                       { return T_MINUS; }
"*"                       { return T_STAR; }
"/"                       { return T_SLASH; }
"%"                       { return T_PERCENT; }
";"                       { return T_EOL; }
\n                        { return T_EOL; }
.                         { return T_UNKNOWN; }

%%
