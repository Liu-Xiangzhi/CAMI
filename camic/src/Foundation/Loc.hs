module Foundation.Loc (Loc (Loc), file, line, column) where

import Text.Printf (printf)

data Loc = Loc {file :: String, line :: Int, column :: Int}

instance Show Loc where
  show Loc {file, line, column} = printf "%s:%d:%d" file line column