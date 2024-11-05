module Foundation.Type.Qualifier
  ( Qualifier,
    none,
    Foundation.Type.Qualifier.const,
    volatile,
    restrict,
    atomic,
    (+=),
    (?),
  )
where

import Data.Bits
import Foundation.Utils (concatString)

newtype Qualifier = Qualifier Int

none :: Qualifier
none = Qualifier 0

const :: Qualifier
const = Qualifier 1

volatile :: Qualifier
volatile = Qualifier 2

restrict :: Qualifier
restrict = Qualifier 4

atomic :: Qualifier
atomic = Qualifier 8

infixl 9 +=

(+=) :: Qualifier -> Qualifier -> Qualifier
(Qualifier x) += (Qualifier y) = Qualifier $ x .|. y

infix 9 ?

(?) :: Qualifier -> Qualifier -> Bool
(Qualifier x) ? (Qualifier y) = (x .&. y) /= 0

instance Show Qualifier where
  show x =
    concatString " " $
      filter (not . null) $
        map
          (\(q, str) -> if x ? q then str else "")
          [(Foundation.Type.Qualifier.const, "const"), (volatile, "volatile"), (restrict, "restrict"), (atomic, "atomic")]
