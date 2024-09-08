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
  let ((a, b), c), d = x in
  (a, b, c, d)

let flatten4 x =
  let (((a, b), c), d), e = x in
  (a, b, c, d, e)

let flatten5 x =
  let ((((a, b), c), d), e), f = x in
  (a, b, c, d, e, f)

let uncurry f (a, b) = f a b

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
