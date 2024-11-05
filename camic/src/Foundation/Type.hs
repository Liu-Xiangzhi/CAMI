{-# LANGUAGE DuplicateRecordFields #-}
{-# LANGUAGE ScopedTypeVariables #-}

module Foundation.Type
  ( module Foundation.Type,
  )
where

import Config (Config (intSize, longSize, longlongSize, shortSize), config)
import Control.Exception (ErrorCall (ErrorCall), throw)
import Foundation.Type.Qualifier (Qualifier)
import Foundation.Utils (Color (..), concatString, getColor)
import qualified Foundation.Utils as U (round)
import Text.Printf (printf)

data Type
  = Char
  | SChar
  | UChar
  | Short
  | UShort
  | Int
  | UInt
  | Long
  | ULong
  | LongLong
  | ULongLong
  | Float
  | Double
  | LongDouble
  | Bool
  | Void
  | Null
  | Pointer Type
  | Qualify Qualify
  | Array Array
  | Struct StructOrUnion
  | Union StructOrUnion
  | Function Function
  | IncompleteArray Type
  | IncompleteStruct String
  | IncompleteUnion String

data Field = Field {name :: String, tp :: Type}

data Qualify = Qualify_ {qualified :: Type, qualifier :: Qualifier}

data Array = Array_ {element :: Type, len :: Word}

data StructOrUnion = StructOrUnion {name :: String, members :: [Field]}

data Function = Function_ {ret :: Type, params :: [Field]}

alignof :: Type -> Word
alignof x = case x of
  Char -> 1
  SChar -> 1
  UChar -> 1
  Short -> config.shortSize
  UShort -> config.shortSize
  Int -> config.intSize
  UInt -> config.intSize
  Long -> config.longSize
  ULong -> config.longSize
  LongLong -> config.longlongSize
  ULongLong -> config.longlongSize
  Float -> 4
  Double -> 8
  LongDouble -> 8
  Bool -> 1
  Pointer _ -> 8
  Array arr -> alignof arr.element
  Qualify q -> alignof q.qualified
  Struct s -> maximum $ (\(Field _ tp) -> alignof tp) <$> s.members
  Union u -> maximum $ (\(Field _ tp) -> alignof tp) <$> u.members
  _ -> throw $ ErrorCall "invalid argument for `alignof`"

sizeof :: Type -> Word
sizeof x = case x of
  Char -> 1
  SChar -> 1
  UChar -> 1
  Short -> config.shortSize
  UShort -> config.shortSize
  Int -> config.intSize
  UInt -> config.intSize
  Long -> config.longSize
  ULong -> config.longSize
  LongLong -> config.longlongSize
  ULongLong -> config.longlongSize
  Float -> 4
  Double -> 8
  LongDouble -> 8
  Bool -> 1
  Pointer _ -> 16
  Array arr -> sizeof arr.element * arr.len
  Qualify q -> sizeof q.qualified
  struct@(Struct s) -> max 1 $ U.round (alignof struct) $ foldl (\acc (Field _ tp) -> U.round (alignof tp) acc + sizeof tp) 0 s.members
  Union u -> max 1 $ maximum $ (\(Field _ tp) -> sizeof tp) <$> u.members
  _ -> throw $ ErrorCall "invalid argument for `sizeof`"

simpleShowType :: Type -> String
simpleShowType x = case x of
  Char -> "char"
  SChar -> "signed char"
  UChar -> "unsigned char"
  Short -> "short"
  UShort -> "unsigned short"
  Int -> "int"
  UInt -> "unsigned int"
  Long -> "long"
  ULong -> "unsigned long"
  LongLong -> "long long"
  ULongLong -> "unsigned long long"
  Float -> "float"
  Double -> "double"
  LongDouble -> "long double"
  Bool -> "bool"
  Void -> "void"
  Null -> "nullptr_t"
  Pointer t -> simpleShowType t ++ "*"
  Qualify q -> let q' = show q.qualifier in simpleShowType q.qualified ++ if null q' then "" else ' ' : q'
  Array arr -> printf "%s[%s]" (simpleShowType arr.element) arr.len
  Struct s -> "struct " ++ s.name
  Union u -> "union " ++ u.name
  Function f -> printf "%s -> (%s)" (simpleShowType f.ret) $ concatString ", " $ map show f.params
  IncompleteArray t -> simpleShowType t ++ "[]"
  IncompleteStruct name -> "struct " ++ name ++ "?"
  IncompleteUnion name -> "union " ++ name ++ "?"

fullShowType :: Type -> String
fullShowType x = case x of
  Struct s -> fullShowStructOrUnion "struct" s.name s.members
  Union u -> fullShowStructOrUnion "union" u.name u.members
  _ -> simpleShowType x
  where
    fullShowStructOrUnion (stuctOrUnion :: String) name fields =
      printf "%s %s{%s}" stuctOrUnion name $ concatString "; " $ map show fields

instance Show Field where
  show x = x.name ++ ": " ++ simpleShowType x.tp

instance Show Type where
  show x = getColor.blue ++ (fullShowType x) ++ getColor.clear