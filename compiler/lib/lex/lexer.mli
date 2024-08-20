type state
type 'a t

val create : Preprocessor.state -> state
val next_token : state -> Token.t option * state
