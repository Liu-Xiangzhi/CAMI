let is_2s_power x = x land (x - 1) = 0
let two's_power x = 1 lsl x

let round base x =
  assert (is_2s_power base);
  (x + (base - 1)) land lnot (base - 1)

let flatten x =
  let (a, b), c = x in
  (a, b, c)

let flatten2 x =
  let ((a, b), c), d = x in
  (a, b, c, d)

let flatten3 x =
  let (((a, b), c), d), e = x in
  (a, b, c, d, e)

let flatten4 x =
  let ((((a, b), c), d), e), f = x in
  (a, b, c, d, e, f)

let uncurry f (a, b) = f a b

let uint_of_bytes len b =
  assert (len > 0 && len <= 8 && Bytes.length b >= len);
  let res = ref 0L in
  if Sys.big_endian then
    for i = 0 to len - 1 do
      let v = Int64.of_int @@ int_of_char @@ Bytes.get b i in
      res := Int64.add (Int64.shift_left !res 8) v
    done
  else
    for i = len - 1 downto 0 do
      let v = Int64.of_int @@ int_of_char @@ Bytes.get b i in
      res := Int64.add (Int64.shift_left !res 8) v
    done;
  !res

let uint_to_bytes len x =
  assert (len > 0 && len <= 8);
  let b = Bytes.create len in
  for i = 0 to len - 1 do
    let v = Int64.logand (Int64.shift_right_logical x (i * 8)) 0xffL in
    Bytes.set b (if Sys.big_endian then len - 1 - i else i) @@ char_of_int @@ Int64.to_int v
  done;
  b

let z_to_bytes len x =
  assert (x > Z.of_int 0);
  let b = Bytes.create len in
  for i = 0 to len - 1 do
    let v = Z.logand (Z.shift_right_trunc x (i * 8)) (Z.of_int 0xff) in
    Bytes.set b (if Sys.big_endian then len - 1 - i else i) @@ char_of_int @@ Z.to_int v
  done;
  b

let z_of_bytes len b =
  assert (Bytes.length b >= len);
  let res = ref @@ Z.of_int 0 in
  if Sys.big_endian then
    for i = 0 to len - 1 do
      let v = Z.of_int @@ int_of_char @@ Bytes.get b i in
      res := Z.add (Z.shift_left !res 8) v
    done
  else
    for i = len - 1 downto 0 do
      let v = Z.of_int @@ int_of_char @@ Bytes.get b i in
      res := Z.add (Z.shift_left !res 8) v
    done;
  !res

let uint_array_of_bytes len b =
  assert (len > 0 && len <= 8);
  let insufficient = Bytes.length b mod len <> 0 in
  let array_len = (Bytes.length b / len) + if insufficient then 1 else 0 in
  Array.init array_len (fun i ->
      let sub_size = if insufficient && i = array_len - 1 then Bytes.length b mod len else len in
      uint_of_bytes sub_size @@ Bytes.sub b (i * len) sub_size)

let reinterpret_unsigned_as_signed len v =
  assert (len > 0 && len <= 8);
  let ( << ) = Int64.shift_left in
  let ( >> ) = Int64.shift_right in
  let shift_size = 64 - (len * 8) in
  v << shift_size >> shift_size

module Color = struct
  let red' = "\x1b[31m"
  let green' = "\x1b[32m"
  let yellow' = "\x1b[33m"
  let blue' = "\x1b[34m"
  let magenta' = "\x1b[35m"
  let cyan' = "\x1b[36m"
  let clear' = "\x1b[0m"

  type t = {
    red : string;
    green : string;
    yellow : string;
    blue : string;
    magenta : string;
    cyan : string;
    clear : string;
  }

  let get () =
    let map x = if !Config.colored_print then x else "" in
    {
      red = map red';
      green = map green';
      yellow = map yellow';
      blue = map blue';
      magenta = map magenta';
      cyan = map cyan';
      clear = map clear';
    }
end
