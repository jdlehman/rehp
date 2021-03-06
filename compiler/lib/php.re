/* Js_of_ocaml compiler
 * http://www.ocsigen.org/js_of_ocaml/
 * Copyright (C) 2010 Jérôme Vouillon
 * Laboratoire PPS - CNRS Université Paris Diderot
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, with linking exception;
 * either version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* A.3 Expressions */

open Stdlib;

type var_info = (StringMap.t(int), Code.Var.Map.t(int));
type array_litteral = element_list
and element_list = list(option(expression))
and binop =
  | Eq
  | StarEq
  | SlashEq
  | ModEq
  | PlusEq
  | MinusEq
  | LslEq
  | AsrEq
  | BandEq
  | BxorEq
  | BorEq
  | Or
  | And
  | Bor
  | Bxor
  | Band
  | EqEq
  | NotEq
  | EqEqEq
  | NotEqEq
  | Lt
  | Le
  | Gt
  | Ge
  | InstanceOf
  | In
  | Lsl
  | Asr
  | Plus
  | FloatPlus
  | IntPlus
  | StrPlus
  | Minus
  | Mul
  | Div
  | Mod
and unop =
  | Not
  | Neg
  | Pl
  | Typeof
  | Void
  | Delete
  | Bnot
  | IncrA
  | DecrA
  | IncrB
  | DecrB
and arguments = list(expression)
and property_name_and_value_list = list((Id.property_name, expression))
and expression =
  | ERaw(string)
  | ECond(expression, expression, expression)
  | EBin(binop, expression, expression)
  | EUn(unop, expression)
  | ECall(expression, arguments, Loc.t)
  /*
   * $identifier[expression]
   */
  | EAccess(expression, expression)
  /*
   *    $identifier->identifier
   *
   * TODO: Php actually supports $identifier->{arbitraryExpression}
   */
  | EDot(expression, Id.identifier)
  | ENew(expression, option(arguments))
  | EVar(Id.t)
  | ELam(function_expression)
  | EFun(function_expression)
  | EStr(string, [ | `Bytes | `Utf8])
  | EArr(array_litteral)
  | EBool(bool)
  | ENum(float)
  | EObj(property_name_and_value_list)
  | EQuote(string)
  | ERegexp(string, option(string))
  | ENULL
  /*
   * TODO: Probably delete these:
   */
  | EStructAccess(expression, expression)
  | EStruct(arguments)
  /* Instructions for accessing standard library arrays.  */
  | EArrAccess(expression, expression)
  | EArrLen(expression)
  /* Like Struct but for cases where you have a tag */
  | ETag(expression, arguments)
/****/
/* A.4 Statements */
and statement =
  | Raw_statement(string)
  | Block(block)
  | Variable_statement(list(variable_declaration))
  | Empty_statement
  | Expression_statement(expression)
  | If_statement(
      expression,
      (statement, Loc.t),
      option((statement, Loc.t)),
    )
  | Do_while_statement((statement, Loc.t), expression)
  | While_statement(expression, (statement, Loc.t))
  | For_statement(
      Stdlib.either(option(expression), list(variable_declaration)),
      option(expression),
      option(expression),
      (statement, Loc.t),
    )
  | ForIn_statement(
      Stdlib.either(expression, variable_declaration),
      expression,
      (statement, Loc.t),
    )
  | Continue_statement(option(Javascript.Label.t))
  | Break_statement(option(Javascript.Label.t))
  | Return_statement(option(expression))
  /* | With_statement of expression * statement */
  | Labelled_statement(Javascript.Label.t, (statement, Loc.t))
  | Switch_statement(
      expression,
      list(case_clause),
      option(statement_list),
      list(case_clause),
    )
  | Throw_statement(expression)
  | Try_statement(block, option((Id.t, block)), option(block))
  | Debugger_statement
  | Global_statement(Id.t)
and block = statement_list
and statement_list = list((statement, Loc.t))
and variable_declaration = (Id.t, option(initialiser))
and case_clause = (expression, statement_list)
and initialiser = (expression, Loc.t)
/****/
/* A.5 Functions and programs */
/*
 * TODO: function_declaration should not have the ability to "use".  In its
 * current form, it's just a fork of Rehp.function_declaration, so it retains
 * the lexical "use" tracking. It should be migrated to a pure Php top level
 * function with no ability to use - only globally scoped.
 */
and function_declaration = (
  Id.t,
  formal_parameter_list,
  function_body,
  /* Global use */
  formal_parameter_list,
  /* Lexical use */
  formal_parameter_list,
  Loc.t,
)
and function_expression = (
  option(Id.t),
  formal_parameter_list,
  function_body,
  /* Global use */
  formal_parameter_list,
  /* Lexical use */
  formal_parameter_list,
  Loc.t,
)
and formal_parameter_list = list(Id.t)
and function_body = source_elements
and program = source_elements
and source_elements = list((source_element, Loc.t))
and source_element =
  | Statement(statement)
  | Function_declaration(function_declaration);

let string_of_number = v =>
  if (v == infinity) {
    "Infinity";
  } else if (v == neg_infinity) {
    "-Infinity";
  } else if (v != v) {
    "NaN";
  } else if
    /* [1/-0] = -inf seems to be the only way to detect -0 in JavaScript */
    (v == 0. && 1. /. v == neg_infinity) {
    "-0";
  } else {
    let vint = int_of_float(v);
    /* compiler 1000 into 1e3 */
    if (float_of_int(vint) == v) {
      let rec div = (n, i) =>
        if (n != 0 && n mod 10 == 0) {
          div(n / 10, succ(i));
        } else if (i > 2) {
          Printf.sprintf("%de%d", n, i);
        } else {
          string_of_int(vint);
        };
      div(vint, 0);
    } else {
      let s1 = Printf.sprintf("%.12g", v);
      if (v == float_of_string(s1)) {
        s1;
      } else {
        let s2 = Printf.sprintf("%.15g", v);
        if (v == float_of_string(s2)) {
          s2;
        } else {
          Printf.sprintf("%.18g", v);
        };
      };
    };
  };
