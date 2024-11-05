module Config
  ( Config (..),
    config',
    config,
  )
where

import Data.IORef (IORef, newIORef, readIORef)
import GHC.IO (unsafePerformIO)
import System.ByteOrder ( byteOrder, ByteOrder(LittleEndian) )

data Config = Config
  { sourceName :: String,
    freeStanding :: Bool,
    shortSize :: Word,
    intSize :: Word,
    longSize :: Word,
    longlongSize :: Word,
    wcharSize :: Word,
    charIsSigned :: Bool,
    floatSize :: Word,
    doubleSize :: Word,
    longdoubleSize :: Word,
    floatPrecision :: Word,
    doublePrecision :: Word,
    longdoublePrecision :: Word,
    coloredPrint :: Bool,
    executionCharset :: String,
    executionWideCharset :: String
  }

{-# NOINLINE config' #-}
config' :: IORef Config
config' =
  unsafePerformIO $ do
    newIORef $
      Config
        { sourceName = "",
          freeStanding = False,
          shortSize = 2,
          intSize = 4,
          longSize = 8,
          longlongSize = 8,
          wcharSize = 4,
          charIsSigned = True,
          floatSize= 4,
          doubleSize = 8,
          longdoubleSize = 8,
          floatPrecision = 24,
          doublePrecision = 53,
          longdoublePrecision = 53,
          coloredPrint = True,
          executionCharset = "UTF-8",
          executionWideCharset = case byteOrder of
            LittleEndian -> "UTF-32LE"
            _ -> "UTF-32GE"
        }

config :: Config
config = unsafePerformIO $ readIORef config'

-- checkConfig :: Config -> Either String ()
-- checkConfig = undefined