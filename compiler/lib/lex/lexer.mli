type t

val create : Preprocessor.t -> t
val next_token : t -> (Token.t * t) option
