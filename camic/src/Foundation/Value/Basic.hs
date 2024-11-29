{-# LANGUAGE TemplateHaskell #-}
{-# OPTIONS_GHC -Wno-unused-top-binds #-}

module Foundation.Value.Basic
  ( Short,
    UShort,
    Foundation.Value.Basic.Int,
    UInt,
    Long,
    ULong,
    LongLong,
    ULongLong,
    Foundation.Value.Basic.Float,
    Foundation.Value.Basic.Double,
    LongDouble,
    BasicKind (..),
    IntegerKind (..),
    FloatKind (..),
    BasicValue (..),
    typeof,
  )
where

import Config (Config (intSize, longSize, longlongSize, shortSize))
import Control.Exception (assert)
import Data.Proxy
import qualified Foundation.Type as T (Type (Bool, Char, Double, Float, Int, Long, LongDouble, LongLong, SChar, Short, UChar, UInt, ULong, ULongLong, UShort))
import Foundation.Utils (two's_power)
import Foundation.Value.BasicTH
import Foundation.Value.MPFloat (MPFloat)

class (Show a) => BasicKind a where
  size :: Proxy a -> Word
  minValue :: a
  maxValue :: a
  fromString :: String -> Maybe a

class (BasicKind a) => IntegerKind a where
  toInteger :: a -> Integer
  fromInteger :: Integer -> Maybe a

class (BasicKind a) => FloatKind a where
  toMPFloat :: a -> MPFloat
  fromMPFloat :: MPFloat -> Maybe a

newtype Char = Char_ Integer

newtype SChar = SChar_ Integer

newtype UChar = UChar_ Integer

newtype Short = Short_ Integer

newtype UShort = UShort_ Integer

newtype Int = Int_ Integer

newtype UInt = UInt_ Integer

newtype Long = Long_ Integer

newtype ULong = ULong_ Integer

newtype LongLong = LongLong_ Integer

newtype ULongLong = ULongLong_ Integer

newtype Float = Float_ MPFloat

newtype Double = Double_ MPFloat

newtype LongDouble = LongDouble_ MPFloat

signedMin :: Word -> Integer
signedMin sz = -(two's_power (sz - 1) :: Integer)

signedMax :: Word -> Integer
signedMax sz = (two's_power (sz - 1) :: Integer) - 1

unsignedMax :: Word -> Integer
unsignedMax sz = (two's_power sz :: Integer) - 1

stringToInteger :: String -> Integer
stringToInteger str = case str of
  '0' : 'b' : s' -> parseBinary s' 0
  '0' : s' | not $ null s' -> read $ "0o" ++ s'
  _ -> read str
  where
    parseBinary s res = case s of
      [] -> res
      '0' : s' -> parseBinary s' $ res * 2
      c : s' -> assert (c == '1') parseBinary s' $ res * 2 + 1

$(generate)

-- instance BasicKind Foundation.Value.Basic.Float where
--   size _ = 1
--   min = Float_ 0
--   max = Float_ 0xff
--   fromString str = Foundation.Value.Basic.fromInteger (read str :: Integer)

data BasicValue
  = Char Foundation.Value.Basic.Char
  | SChar Foundation.Value.Basic.SChar
  | UChar Foundation.Value.Basic.UChar
  | Short Short
  | UShort UShort
  | Int Foundation.Value.Basic.Int
  | UInt UInt
  | Long Long
  | ULong ULong
  | LongLong LongLong
  | ULongLong ULongLong
  | Float Foundation.Value.Basic.Float
  | Double Foundation.Value.Basic.Double
  | LongDouble LongDouble
  | Bool Bool

typeof :: BasicValue -> T.Type
typeof (Char _) = T.Char
typeof (SChar _) = T.SChar
typeof (UChar _) = T.UChar
typeof (Short _) = T.Short
typeof (UShort _) = T.UShort
typeof (Int _) = T.Int
typeof (UInt _) = T.UInt
typeof (Long _) = T.Long
typeof (ULong _) = T.ULong
typeof (LongLong _) = T.LongLong
typeof (ULongLong _) = T.ULongLong
typeof (Float _) = T.Float
typeof (Double _) = T.Double
typeof (LongDouble _) = T.LongDouble
typeof (Bool _) = T.Bool

-- instance Show BasicValue where
--   show (Char x) = T.Char
--   show (SChar x) = T.SChar
--   show (UChar x) = T.UChar
--   show (Short x) = T.Short
--   show (UShort x) = T.UShort
--   show (Int x) = T.Int
--   show (UInt x) = T.UInt
--   show (Long x) = T.Long
--   show (ULong x) = T.ULong
--   show (LongLong x) = T.LongLong
--   show (ULongLong x) = T.ULongLong
--   show (Float x) = T.Float
--   show (Double x) = T.Double
--   show (LongDouble x) = T.LongDouble
--   show (Bool x) = T.Bool
