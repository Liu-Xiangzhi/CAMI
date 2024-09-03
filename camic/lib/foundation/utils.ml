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
