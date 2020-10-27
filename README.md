# CPaurs 

A C implementation of the Parus language

# Postfixed Reprogrammable Stack language

The language is named after Paridae bird family (Tits, Titmices and Chickadees).

Members of the Paridae family are small and highly intelligent birds.

# NOTE

THIS IS AN EARLY STAGE PROJECT SO SEGFAULTS AND MEMORY LEAKS COULD ACCURE.

Further more some sections in the code should be re-written


# Technical Details

This interpreter is built around simplicity,
It doesn't use any bytecode compliation, as a result programs won't run as quickly as they could run if they were compiled.

Nevertheless it optimize the last call in a macro (a re-call) and it "compiles" usermacro to an intermediate representation instead of evaluating it as string.

Further more, for sake of simplicity this implementation doesn't support strings and arrays.

CParus can also be used as a library, for details refer to repl.c

# The 3 Laws of the Parus language

if token is self evaluating:
	push it to the stack

elseif token is pre-fixed with a ' (quoted form) :
	quotate it and push it the stack

elseif symbol:
	apply the token's binding

Please refer to the parus manual, for more comprehensive explanation:

https://gitlab.com/oren_daniel/parus-manual


# Examples

; Inspect the memory using ?stk and ?lex.

3 2 + OUTLN

; ==> 5

; spaces are required between parenthesis

( DPL * ) '^2 DEF

5 ^2 OUTLN

; ==> 25

''a 'b DEF

3 'a DEF

b ! OUTLN

; ==> 3

; CParus also supports non-english characters

''𐤔𐤋𐤅𐤌
'שלום
DEF

3.14159 'π DEF
