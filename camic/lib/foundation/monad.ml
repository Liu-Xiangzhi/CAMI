module type Functor = sig
  type 'a t

  val ( <$> ) : ('a -> 'b) -> 'a t -> 'b t
end

module type Applicative = sig
  include Functor

  val pure : 'a -> 'a t
  val ( <*> ) : ('a -> 'b) t -> 'a t -> 'b t
end

module type Monad = sig
  include Applicative

  val return : 'a -> 'a t
  val ( >>= ) : 'a t -> ('a -> 'b t) -> 'b t
  val ( >> ) : 'a t -> 'b t -> 'b t
end

module type State = sig
  include Monad

  type state_t

  val set : state_t -> unit t
  val get : unit -> state_t t
  val pass : unit t
  val gets : (state_t -> 'a) -> 'a t
  val modify : (state_t -> state_t) -> unit t
  val run : 'a t -> state_t -> 'a * state_t
end

module MakeState (St : sig
  type t
end) : State with type 'a t = St.t -> 'a * St.t and type state_t = St.t = struct
  type 'a t = St.t -> 'a * St.t

  let ( <$> ) f m st =
    let x, st' = m st in
    (f x, st')

  let pure x st = (x, st)

  let ( <*> ) a b st =
    let f, st' = a st in
    let x, st'' = b st' in
    (f x, st'')

  let return = pure

  let ( >>= ) m f st =
    let x, st' = m st in
    f x st'

  let ( >> ) ma mb = ma >>= fun _ -> mb

  type state_t = St.t

  let set st _ = ((), st)
  let get () st = (st, st)
  let pass st = ((), st)
  let gets f st = (f st, st)
  let modify f st = ((), f st)
  let run f x = f x
end
