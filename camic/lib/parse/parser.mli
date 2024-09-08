type state

val create : Lexer.state -> state
val parse : state -> AST.t
