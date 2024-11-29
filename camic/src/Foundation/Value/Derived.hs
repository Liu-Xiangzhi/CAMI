-- module Foundation.Value.Derived
--   ( module Foundation.Value.Derived,
--   )
-- where

-- import Control.Exception (assert)
-- import qualified Foundation.Type as T
-- import Foundation.Utils (getColor)
-- import qualified Foundation.Value.Basic as B (BasicValue, typeof)
-- import Text.Printf (printf)

-- data Value
--   = Basic B.BasicValue
--   | Null
--   | Pointer StaticObject Int
--   | Pointer' T.Type Word
--   | Array [Value]
--   | Struct T.Type [Value]
--   | Union T.Type [Value] Word -- the third member indicate the index of activated member

-- data StaticObject = StaticObject {name :: String, tp :: T.Type, value :: Value}

-- typeof :: Value -> T.Type
-- typeof (Basic b) = B.typeof b
-- typeof Null = T.Null
-- typeof (Pointer obj _) = T.Pointer obj.tp
-- typeof (Pointer' tp _) = tp
-- typeof (Array vals) = assert (length vals > 0) $ T.Array T.Array_ {T.element = (typeof $ head vals), T.len = fromIntegral $ length vals}
-- typeof (Struct tp _) = tp
-- typeof (Union tp _ _) = tp

-- instance Show Value where
--   show x = printf "<%s>%s%s%s" (show $ typeof x) getColor.green (toString x) getColor.clear

-- `toString` does not contains type info & color while `show` does
-- toString :: Value -> String
-- toString x = case x of
--   Basic b -> toString b
--   Null -> "nullptr"
--   Pointer obj off -> printf "(&%s + %d)" obj.name off
--   Pointer' _ v -> printf "0x%X" v
--   Array arr -> printf "[%s]" $ fmtList ',' $ map toString arr
--   Struct tp mem ->
--     let T.Struct tpName fields = tp
--      in printf "{%s}" fmtList ';' $ zip tp mem
--   Union _ mem act -> assert (act < fromIntegral (length mem)) $ ""
--   where
--     fmtList delimiter lst = foldr (\i acc -> i ++ (if null acc then "" else delimiter : ' ' : acc)) "" lst
module Foundation.Value.Derived() where