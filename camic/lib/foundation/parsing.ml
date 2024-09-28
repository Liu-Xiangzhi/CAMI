module Make (T : sig
  type t
  type payload

  val run1 : t -> payload option * t
end) =
struct
  module State = Monad.MakeState (T)
  include State

  type 'a t = 'a option State.t

  let ( let$ ) = ( >>= )

  let ( let$* ) m f =
    let$ v = m in
    if Option.is_none v then return None else f (Option.get v)

  (** parse either a or b *)
  let ( |- ) a b =
    let$ st = get () in
    let$ v1 = a in
    if Option.is_some v1 then return v1 else set st >> b

  (** try to parse m, which means state will not be changed if parsing falied *)
  let ( ~? ) m = m |- return None

  let ( ~$ ) m = Option.some <$> m
  let ( <$$> ) f m = Option.map f <$> m
  let ( --> ) m f = f <$> m
  let ( ->> ) m f = f <$$> m

  let ( *>! ) ma mb =
    let$* _ = ma in
    let$ v = mb in
    return @@ Some v

  let ( *!> ) ma mb =
    let$ _ = ma in
    let$* v = mb in
    return @@ Some v

  let ( *> ) ma mb =
    let$* _ = ma in
    mb

  let ( *<! ) ma mb =
    let$* v = ma in
    let$ _ = mb in
    return @@ Some v

  let ( *!< ) ma mb =
    let$ v = ma in
    let$* _ = mb in
    return @@ Some v

  let ( << ) ma mb =
    let$ v = ma in
    let$ _ = mb in
    return v

  let ( *< ) ma mb =
    let$* v = ma in
    let$* _ = mb in
    return @@ Some v

  let ( ++ ) ma mb =
    let$* va = ma in
    let$* vb = mb in
    return @@ Some (va, vb)

  let ( ++! ) ma mb =
    let$* va = ma in
    let$ vb = mb in
    return @@ Some (va, vb)

  let ( +!+ ) ma mb =
    let$ va = ma in
    let$* vb = mb in
    return @@ Some (va, vb)

  let ( +!+! ) ma mb =
    let$ va = ma in
    let$ vb = mb in
    return (va, vb)

  let take1 =
    let$ st = get () in
    let payload, st' = T.run1 st in
    set st' >> return payload

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

  let sequence m ~delimiter =
    let m' = delimiter *> m in
    let$ v = m in
    match v with None -> return [] | Some v' -> List.cons v' <$> many m'

  let sequence' m ~delimiter =
    let$ v = sequence m ~delimiter in
    let$ _ = ~?delimiter in
    return v

  let sequence1 m ~delimiter =
    let m' = delimiter *> m in
    let$* v = m in
    let$ vs = many m' in
    return @@ Some (v :: vs)

  let sequence1' m ~delimiter =
    let$* v = sequence1 m ~delimiter in
    let$ _ = ~?delimiter in
    return @@ Some v

  let rec concat m =
    let$ v = ~?m in
    match v with None -> return @@ Some [] | Some v' -> List.append v' <$$> concat m

  let take_if pred =
    let$* payload = take1 in
    return @@ if pred payload then Some payload else None

  let rec take_while pred =
    let$ payload = ~?(take_if pred) in
    if Option.is_none payload then return [] else List.cons (Option.get payload) <$> take_while pred
end
