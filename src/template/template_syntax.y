/* -------------------------------------------------------------------------- */
/* Copyright 2002-2008, Distributed Systems Architecture Group, Universidad   */
/* Complutense de Madrid (dsa-research.org)                                   */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may    */
/* not use this file except in compliance with the License. You may obtain    */
/* a copy of the License at                                                   */
/*                                                                            */
/* http://www.apache.org/licenses/LICENSE-2.0                                 */
/*                                                                            */
/* Unless required by applicable law or agreed to in writing, software        */
/* distributed under the License is distributed on an "AS IS" BASIS,          */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/* See the License for the specific language governing permissions and        */
/* limitations under the License.                                             */
/* -------------------------------------------------------------------------- */

%{
#include <iostream>
#include <string>
#include <map>
#include <algorithm>

#include <ctype.h>
#include <string.h>

#include "template_syntax.h"
#include "Template.h"

#define YYERROR_VERBOSE

extern "C" 
{
void template_error(
	YYLTYPE *		llocp,
	Template *		tmpl,
	char **			error_msg,
	const char *	str);

int template_lex (YYSTYPE *lvalp, YYLTYPE *llocp);

int template_parse(Template * tmpl, char ** errmsg);
}

%}

%parse-param {Template * tmpl}
%parse-param {char ** error_msg}

%union {
    char * val_str;
    void * val_attr;
};

%defines
%locations
%pure_parser
%name-prefix = "template_"
%output      = "template_syntax.cc"

%token EQUAL BLANK COMMA OBRACKET CBRACKET 
%token <void>  		NL
%token <val_str>  	STRING
%type  <val_attr>  	array_val
%type  <void>		attribute
%type  <void>   	template

%%

template:  attribute
	| template attribute
    ;
         
attribute:	STRING EQUAL STRING NL 
			{
				Attribute * pattr;
				string 		name($1);
				string		value($3);
				
				pattr = new SingleAttribute(name,value);

				tmpl->set(pattr);
				
				free($1);
				free($3);
			}
		 | 	STRING EQUAL OBRACKET array_val CBRACKET NL 
		 	{
				Attribute * pattr;
				string 		name($1);
				map<string,string> * amap;
				
				amap    = static_cast<map<string,string> *>($4);
				pattr   = new VectorAttribute(name,*amap);
				
				tmpl->set(pattr);
				
				delete amap;
				free($1);
		 	}
		 | 	STRING EQUAL NL
		 	{
				Attribute * pattr;
				string 		name($1);
				string		value("");
				
				pattr = new SingleAttribute(name,value);
				
				tmpl->set(pattr);
				
				free($1);
		 	}
		 | 	NL {};
		 ;
	    
array_val: 	STRING EQUAL STRING
			{
			  	map<string,string>* vattr;
			  	string				name($1);
			  	string				value($3);

			    transform (name.begin(),name.end(),name.begin(),(int(*)(int))toupper);
				
				vattr = new map<string,string>;
				vattr->insert(make_pair(name,value));

				$$ = static_cast<void *>(vattr);

				free($1);
				free($3);
			}
		| 	array_val COMMA STRING EQUAL STRING 
			{
				string				 name($3);
			  	string				 value($5);
				map<string,string> * attrmap;
						
			    transform (name.begin(),name.end(),name.begin(),(int(*)(int))toupper);                
				attrmap = static_cast<map<string,string> *>($1);
				
				attrmap->insert(make_pair(name,value));
				$$ = $1;
				
				free($3);
				free($5);
			}
		;
%%

extern "C" void template_error(
	YYLTYPE *		llocp,
	Template *		tmpl,
	char **			error_msg,
	const char *	str)
{
	int length;
	
	length = strlen(str)+ 64;
	
	*error_msg = (char *) malloc(sizeof(char)*length);
	
	if (*error_msg != 0)
	{
		snprintf(*error_msg,
			length,
			"%s at line %i, columns %i:%i",
			str,
	    	llocp->first_line,
    		llocp->first_column,
        	llocp->last_column);
	}
} 
