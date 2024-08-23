let is_2s_power x = x land (x - 1) = 0

let two's_power x = 1 lsl x

let round base x =
  assert (is_2s_power base);
  (x + (base - 1)) land lnot (base - 1)
