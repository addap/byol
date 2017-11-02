_
x?
x+
x*

digit : '0' | '1' | '2' | '3' | ...;
number : digit+
real-number = number . number;


protocol : "http" | "https" | "ftp" ...
domain : letter+
url : protocol :// domain [. domain]+

// simple sentences
subject: "the"? noun | name
verb:
object: adverbial? subject
adverbial: preposition | modal something
sentence: subject verb object?

// json
null: _
array: '[' [value, ]* '}'
string: " letter* "
number: 1 | 2 ...
bool: true | false
value: string | array | object | null | number | bool
key: string
attribute: key:value
object: '{' [attribute, ]* '}'
json: object+
