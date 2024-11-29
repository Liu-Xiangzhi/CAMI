module Foundation.Utils
  ( module Foundation.Utils,
  )
where

import Config (Config (coloredPrint), config)
import Control.Exception (assert)
import Data.Bits

is_2's_power :: (Bits a, Num a) => a -> Bool
is_2's_power x = x .&. (x - 1) == 0

two's_power :: (Bits a2, Num a2, Integral a1) => a1 -> a2
two's_power x = assert (x > 0) $ 1 `shiftL` (fromIntegral x :: Int)

round :: (Bits a, Num a) => a -> a -> a
round base x =
  assert (is_2's_power base) $
    (x + (base - 1)) .&. (complement (base - 1))

concatString :: String -> [String] -> String
concatString delimiter strs = foldr (\i acc -> i ++ if null acc then "" else delimiter ++ acc) "" strs

-- let uint_of_bytes len b =
--   assert (len > 0 && len <= 8 && Bytes.length b >= len);
--   let res = ref 0L in
--   if Sys.big_endian then
--     for i = 0 to len - 1 do
--       let v = Int64.of_int @@ int_of_char @@ Bytes.get b i in
--       res := Int64.add (Int64.shift_left !res 8) v
--     done
--   else
--     for i = len - 1 downto 0 do
--       let v = Int64.of_int @@ int_of_char @@ Bytes.get b i in
--       res := Int64.add (Int64.shift_left !res 8) v
--     done;
--   !res

-- let uint_to_bytes len x =
--   assert (len > 0 && len <= 8);
--   let b = Bytes.create len in
--   for i = 0 to len - 1 do
--     let v = Int64.logand (Int64.shift_right_logical x (i * 8)) 0xffL in
--     Bytes.set b (if Sys.big_endian then len - 1 - i else i) @@ char_of_int @@ Int64.to_int v
--   done;
--   b

-- let z_to_bytes len x =
--   assert (x > Z.of_int 0);
--   let b = Bytes.create len in
--   for i = 0 to len - 1 do
--     let v = Z.logand (Z.shift_right_trunc x (i * 8)) (Z.of_int 0xff) in
--     Bytes.set b (if Sys.big_endian then len - 1 - i else i) @@ char_of_int @@ Z.to_int v
--   done;
--   b

-- let z_of_bytes len b =
--   assert (Bytes.length b >= len);
--   let res = ref @@ Z.of_int 0 in
--   if Sys.big_endian then
--     for i = 0 to len - 1 do
--       let v = Z.of_int @@ int_of_char @@ Bytes.get b i in
--       res := Z.add (Z.shift_left !res 8) v
--     done
--   else
--     for i = len - 1 downto 0 do
--       let v = Z.of_int @@ int_of_char @@ Bytes.get b i in
--       res := Z.add (Z.shift_left !res 8) v
--     done;
--   !res

-- let uint_array_of_bytes len b =
--   assert (len > 0 && len <= 8);
--   let insufficient = Bytes.length b mod len <> 0 in
--   let array_len = (Bytes.length b / len) + if insufficient then 1 else 0 in
--   Array.init array_len (fun i ->
--       let sub_size = if insufficient && i = array_len - 1 then Bytes.length b mod len else len in
--       uint_of_bytes sub_size @@ Bytes.sub b (i * len) sub_size)

-- let reinterpret_unsigned_as_signed len v =
--   assert (len > 0 && len <= 8);
--   let ( << ) = Int64.shift_left in
--   let ( >> ) = Int64.shift_right in
--   let shift_size = 64 - (len * 8) in
--   v << shift_size >> shift_size
data Color = Color
  { red :: String,
    green :: String,
    yellow :: String,
    blue :: String,
    magenta :: String,
    cyan :: String,
    clear :: String
  }

getColor :: Color
getColor =
  let f x = if config.coloredPrint then x else ""
   in Color
        { red = f "\033[31m",
          green = f "\033[32m",
          yellow = f "\033[33m",
          blue = f "\033[34m",
          magenta = f "\033[35m",
          cyan = f "\033[36m",
          clear = f "\033[0m"
        }

-- let flatten x =
--   let (a, b), c = x in
--   (a, b, c)

-- let flatten2 x =
--   let ((a, b), c), d = x in
--   (a, b, c, d)

-- let flatten3 x =
--   let (((a, b), c), d), e = x in
--   (a, b, c, d, e)

-- let flatten4 x =
--   let ((((a, b), c), d), e), f = x in
--   (a, b, c, d, e, f)

-- let uncurry f (a, b) = f a b
