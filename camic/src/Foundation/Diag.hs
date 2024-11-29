module Foundation.Diag
  ( module Foundation.Diag,
  )
where

import Config (Config (sourceName), config)
import Control.Exception (Exception, throw)
import Data.Data (Typeable)
import Foundation.Loc (Loc)
import Text.Printf (printf)

--  source, lexical, and syntax error will cause the abort of compilation
data AbortCompilation = AbortCompilation String Int deriving (Show, Typeable)

instance Exception AbortCompilation

systemError :: String -> a
systemError msg = throw $ AbortCompilation msg (-1)

sourceError :: String -> Maybe Loc -> a
sourceError msg Nothing = throw $ AbortCompilation msg (-2)
sourceError msg (Just loc) = throw $ AbortCompilation (printf "%s: [Encoding Error]%s" (show loc) msg) (-2)

lexppsError :: Int -> Maybe String -> Maybe String -> String -> a
lexppsError line column file msg =
  let sourceName' = maybe (if config.sourceName == "" then "<stdin>" else config.sourceName) id file
      columInfo = maybe "" (printf ":%d" :: String -> String) column
   in throw $ AbortCompilation (printf "%s:%d%s: [LexPreprocess Error]%s" sourceName' line columInfo msg) (-3)

lexicalError :: Loc -> String -> a
lexicalError loc msg = throw $ AbortCompilation (printf "%s: [LexicalError]%s" (show loc) msg) (-3)

parsingError :: Loc -> String -> a
parsingError loc msg = throw $ AbortCompilation (printf "%s: [ParsingError]%s" (show loc) msg) (-4)

lexicalWarning :: Loc -> String -> String
lexicalWarning loc msg = (printf "%s: [LexicalWarning]%s" (show loc) msg)
