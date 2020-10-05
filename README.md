# CPaurs 

A C implementation of the Parus language

# Postfixed Reprogrammable Stack language

The language is named after Paridae bird family ( Tits, Titmices and Chickadees ).

Members of the Paridae family are small and highly intelligent birds.

# NOTE

THIS IS AN EARLY STAGE PROJECT SO SEGFAULTS AND MEMORY LEAKS COULD ACCURE.

Further more some sections in the code should be re-written


# Technical Details

This interpreter is built around simplicity,
It doesn't use any bytecode compliation, as a result programs won't run quickly as they should if they would have been compiled.

Nevertheless it optimize the last call in a macro ( a re-call ) and it "compiles" usermacro to an intermediate representation instead of evaluating it as string.
This implementation for simplicity doesn't implement quotation as a pointer, but as prefix to a symbol.

Further more, for sake of simplicity this implementation doesn't support strings and arrays.

CParus can also be used as a library, for details refer to repl.c

# The 4 Laws of the Parus language

See the definition of the language in

https://gitlab.com/oren_daniel/parus-definition/-/blob/master/parus-definition

if token is self evaluating:
	push it to the stack

elseif token is ! ( imperative form ):
	apply the item in the top of the stack

elseif token is pre-fixed with ' ( the quote form - a symbol ):
	quotate it and push it the stack

else:
	apply the tokens binding

# Examples

; Inspect the memory using ?stk and ?lex.

3 2 + OUT

; ==> 5

; spaces are required between parenthesis

( DPL * ) '^2 DEF

5 ^2 OUT

; ==> 25

''a 'b DEF

3 'a DEF

b ! OUT 

; ==> 3

