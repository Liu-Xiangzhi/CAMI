module Make (T : sig
  type t
  type payload

  val run1 : t -> payload option * t
end) =
struct
  module State = Monad.MakeState (T)

  type 'a t = 'a option State.t
  type state_t = T.t

  open State

  let ( let$ ) = ( >>= )

  let ( let$* ) m f =
    let$ v = m in
    if Option.is_none v then return None else f (Option.get v)

  let ( |- ) a b =
    let$ st = get () in
    let$ v1 = a in
    if Option.is_some v1 then return v1 else set st >> b

  let ( ~? ) a = a |- return None

  let take1 =
    let$ st = get () in
    let pchar, st' = T.run1 st in
    set st' >> return pchar

  let take n =
    let rec repeat i s =
      if i >= n then return @@ Some (Array.of_list @@ List.rev s)
      else
        let$* v = take1 in
        repeat (i + 1) (v :: s)
    in
    repeat 0 []

  let rec many m =
    let$ v = ~?m in
    match v with None -> return [] | Some v' -> List.cons v' <$> many m

  let many1 m =
    let$* v = m in
    let$ vs = many m in
    return @@ Some (v :: vs)

  let take_if pred =
    let$* pchar = take1 in
    return @@ if pred pchar then Some pchar else None

  let rec take_while pred =
    let$ pchar = ~?(take_if pred) in
    if Option.is_none pchar then return [] else List.cons (Option.get pchar) <$> take_while pred
end
