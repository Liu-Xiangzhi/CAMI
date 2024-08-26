type state

val create : Preprocessor.state -> state
val next_token : state -> Token.t option * state
