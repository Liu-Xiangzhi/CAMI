type state

val create : Lexpps.state -> state
val next_token : state -> Token.t option * state
