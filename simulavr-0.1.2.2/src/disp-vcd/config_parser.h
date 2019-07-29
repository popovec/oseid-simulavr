/* A Bison parser, made by GNU Bison 1.875d.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     CP_FREQUENCY = 258,
     CP_VCD_FILE = 259,
     CP_TRACE = 260,
     CP_SRAM = 261,
     CP_EEPROM = 262,
     CP_PROGMEM = 263,
     CP_REG = 264,
     CP_FREG = 265,
     CP_IOREG = 266,
     CP_SP = 267,
     CP_PC = 268,
     CP_NAME = 269,
     CP_FILENAME = 270,
     CP_NUMBER = 271
   };
#endif
#define CP_FREQUENCY 258
#define CP_VCD_FILE 259
#define CP_TRACE 260
#define CP_SRAM 261
#define CP_EEPROM 262
#define CP_PROGMEM 263
#define CP_REG 264
#define CP_FREG 265
#define CP_IOREG 266
#define CP_SP 267
#define CP_PC 268
#define CP_NAME 269
#define CP_FILENAME 270
#define CP_NUMBER 271




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 64 "config_parser.y"
typedef union YYSTYPE {
    int  integer;
    char *string;
    struct
    {
        int from;
        int to;
    } t_range;
} YYSTYPE;
/* Line 1285 of yacc.c.  */
#line 79 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE config_lval;



