{-# LANGUAGE TemplateHaskell #-}

module Foundation.Value.BasicTH
  ( generate,
  )
where

import Config (Config (..), config)
import qualified Foundation.Type as T
import Language.Haskell.TH

mkVar :: (Quote m) => String -> m Exp -> m Dec
mkVar name e = mkFunc name [] e

mkFunc :: (Quote m) => String -> [m Pat] -> m Exp -> m Dec
mkFunc name pattern e = funD (mkName name) [clause pattern (normalB e) []]

genShowInstance :: (Quote m) => String -> m [Dec]
genShowInstance tp =
  let fullName = "Foundation.Value.Basic." ++ tp
      conName = fullName ++ "_"
   in [d|
        instance Show $(conT (mkName fullName)) where
          show $(conP (mkName conName) [varP (mkName "x")]) = show x
        |]

genBasicKind :: Quote m => [Char] -> m Exp -> m Exp -> m Exp -> m [Dec]
genBasicKind typeName size minV maxV =
  let cons = conE $ mkName $ "Foundation.Value.Basic." ++ typeName ++ "_"
   in (\x -> [x])
        <$> instanceD
          (return [])
          (appT (conT $ mkName "BasicKind") (conT $ mkName $ "Foundation.Value.Basic." ++ typeName))
          [ mkFunc "size" [varP $ mkName "_"] size,
            mkVar "minValue" $ [|$cons $minV|],
            mkVar "maxValue" $ [|$cons $maxV|],
            mkFunc
              "fromString"
              [varP (mkName "str")]
              [|let x = $(varE $ mkName "stringToInteger") str in if x >= $minV && x <= $maxV then Just $ $cons $ x else Nothing|]
          ]

genIntegerKind :: Quote m => [Char] -> m Exp -> m Exp -> m [Dec]
genIntegerKind typeName minV maxV =
  let consName = "Foundation.Value.Basic." ++ typeName ++ "_"
      cons = conE $ mkName consName
   in (\x -> [x])
        <$> instanceD
          (return [])
          (appT (conT $ mkName "IntegerKind") (conT $ mkName $ "Foundation.Value.Basic." ++ typeName))
          [ mkFunc
              "fromInteger"
              [varP (mkName "x")]
              [|if x >= $minV && x <= $maxV then Just $ $cons $ x else Nothing|],
            mkFunc "toInteger" [conP (mkName consName) [varP (mkName "x")]] [|x|]
          ]

-- genFloatKind :: Quote m => [Char] -> m Exp -> m Exp -> m [Dec]
-- genFloatKind typeName minV maxV =
--   let consName = "Foundation.Value.Basic." ++ typeName ++ "_"
--       cons = conE $ mkName consName
--    in (\x -> [x])
--         <$> instanceD
--           (return [])
--           (appT (conT $ mkName "FloatKind") (conT $ mkName $ "Foundation.Value.Basic." ++ typeName))
--           [ mkFunc
--               "fromMPFloat"
--               [varP (mkName "x")]
--               [|if x >= $minV && x <= $maxV then Just $ $cons $ x else Nothing|],
--             mkFunc "toMPFloat" [conP (mkName consName) [varP (mkName "x")]] [|x|]
--           ]

genInteger :: T.Type -> Q [Dec]
genInteger tp =
  let (typeName, size, isUnsigned) = case tp of
        T.Char -> ("Char", [|1|], not config.charIsSigned)
        T.UChar -> ("UChar", [|1|], True)
        T.SChar -> ("SChar", [|1|], False)
        T.Short -> ("Short", [|config.shortSize|], False)
        T.UShort -> ("UShort", [|config.shortSize|], True)
        T.Int -> ("Int", [|config.intSize|], False)
        T.UInt -> ("UInt", [|config.intSize|], True)
        T.Long -> ("Long", [|config.longSize|], False)
        T.ULong -> ("ULong", [|config.longSize|], True)
        T.LongLong -> ("LongLong", [|config.longlongSize|], False)
        T.ULongLong -> ("ULongLong", [|config.longlongSize|], True)
        _ -> error "Except integer type, code generation aborted."
   in genInteger' typeName size isUnsigned
  where
    genInteger' typeName size isUnsigned =
      let minV = if isUnsigned then [|0|] else [|signedMin $size|]
          maxV = if isUnsigned then [|unsignedMax $size|] else [|signedMax $size|]
       in do
            s <- genShowInstance typeName
            b <- genBasicKind typeName size minV maxV
            i <- genIntegerKind typeName minV maxV
            return $ s ++ b ++ i

-- genFloat :: a
-- genFloat = undefined

generate :: Q [Dec]
generate =
  foldr1 (\x acc -> (++) <$> x <*> acc) $
    map
      genInteger
      [ T.Char,
        T.SChar,
        T.UChar,
        T.Short,
        T.UShort,
        T.Int,
        T.UInt,
        T.Long,
        T.ULong,
        T.LongLong,
        T.ULongLong
      ]
      -- ++ map
      --   genFloat
      --   [ T.Float,
      --     T.Double,
      --     T.LongDouble
      --   ]

-- genBasicKindIntegerInstance :: String -> Bool -> Q [Dec]
-- genBasicKindIntegerInstance typeName isUnsigned =
--   let tpSize = [|$(varE $ mkName $ (toLower <$> typeName) ++ "Size") config|] :: Q Exp
--       typeName' = if isUnsigned then ('U' : typeName) else typeName
--       conName = "Foundation.Value.Basic." ++ typeName' ++ "_"
--       cons = conE $ mkName conName
--       minVal = if isUnsigned then [|0|] else [|signedMin $ $tpSize|]
--       maxVal = if isUnsigned then [|unsignedMax $ $tpSize|] else [|signedMax $ $tpSize|]
--    in (\x -> [x])
--         <$> instanceD
--           (return [])
--           (appT (conT $ mkName "BasicKind") (conT $ mkName $ "Foundation.Value.Basic." ++ typeName'))
--           [ mkFunc "size" [varP $ mkName "_"] tpSize,
--             mkVar "minValue" $ [|$cons $ $minVal|],
--             mkVar "maxValue" $ [|$cons $ $maxVal|],
--             mkFunc
--               "fromString"
--               [varP (mkName "str")]
--               [|let x = read str :: Integer in if x >= $minVal && x <= $maxVal then Just $ $cons $ x else Nothing|]
--           ]

-- genBasicKindFloatInstance :: String -> Int -> Int -> Q [Dec]
-- genBasicKindFloatInstance typeName size precision =
--   let conName = "Foundation.Value.Basic." ++ typeName ++ "_"
--       expWidth = size * 8 - 1 {- sign bit -} - precision
--       cons = conE $ mkName conName
--       -- maxVal =
--       minVal = [|- $maxVal|]
--    in (\x -> [x])
--         <$> instanceD
--           (return [])
--           (appT (conT $ mkName "BasicKind") (conT $ mkName $ "Foundation.Value.Basic." ++ typeName))
--           [ mkFunc "size" [varP $ mkName "_"] $ litE $ IntegerL $ Prelude.toInteger size,
--             mkVar "min" $ [|$cons $ $minVal|],
--             mkVar "max" $ [|$cons $ $maxVal|],
--             mkFunc
--               "fromString"
--               [varP (mkName "str")]
--               [|let x = read str :: Integer in if x >= $minVal && x <= $maxVal then Just $ $cons $ x else Nothing|]
--           ]

-- genIntegerKindInstance :: String -> Bool -> Q [Dec]
-- genIntegerKindInstance typeName isUnsigned =
--   let tpSize = [|$(varE $ mkName $ (toLower <$> typeName) ++ "Size") config|] :: Q Exp
--       typeName' = if isUnsigned then ('U' : typeName) else typeName
--       conName = "Foundation.Value.Basic." ++ typeName' ++ "_"
--       cons = conE $ mkName conName
--       minVal = if isUnsigned then [|0|] else [|signedMin $ $tpSize|]
--       maxVal = if isUnsigned then [|unsignedMax $ $tpSize|] else [|signedMax $ $tpSize|]
--    in (\x -> [x])
--         <$> instanceD
--           (return [])
--           (appT (conT $ mkName "IntegerKind") (conT $ mkName $ "Foundation.Value.Basic." ++ typeName'))
--           [ mkFunc
--               "fromInteger"
--               [varP (mkName "x")]
--               [|if x >= $minVal && x <= $maxVal then Just $ $cons $ x else Nothing|],
--             mkFunc "toInteger" [conP (mkName conName) [varP (mkName "x")]] [|x|]
--           ]