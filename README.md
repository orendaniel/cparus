# CPaurs 

A C implementation of the Parus language

# Postfixed Reprogrammable Stack language

The language is named after Paridae bird family ( Tits, Titmices and Chickadees ).

Members of the Paridae family are small and highly intelligent birds.

# NOTE

THIS IS AN EARLY STAGE PROJECT SO SEGFAULTS AND MEMORY LEAKS COULD ACCURE.

Further more some sections in the code should be re-written


# Technical Details

This interpeter is built around simplicity,
It doesn't use any bytecode compliation, as a result programs won't run quickly.

However it optimize the last call in a macro ( tail call ).


# The 4 Laws of the Parus language

if token is self evaluating:
	push it to the stack

if token is ! ( imperative form ):
	apply the item in the top of the stack

if token is pre-fixed with ' ( the quote form - a symbol ):
	quotate it and push it the stack

else:
	apply the tokens binding

# Examples

3 2 + OUT

==> 5

spaces are required between parenthesis

( DPL * ) '^2 DEF
5 ^2 out

==> 25

''a 'b DEF
3 'a DEF
b ! OUT 

==> 3
