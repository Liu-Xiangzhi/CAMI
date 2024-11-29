module Main (main) where

import Config
import Control.Exception (Exception (fromException), SomeException, try)
import Control.Monad.Except (ExceptT)
import Data.IORef (readIORef, writeIORef)
import Foundation.Diag (AbortCompilation (AbortCompilation), systemError)
import Foundation.Value.MPFloat
import System.ByteOrder (ByteOrder (Mixed), byteOrder)
import System.Environment (getArgs)
import System.Exit (ExitCode (ExitFailure), exitSuccess, exitWith)
import System.IO (hPutStrLn, stderr)
import Text.Printf (printf)

main :: IO ()
main = do
  res <- try main' :: IO (Either SomeException ())
  case res of
    Right () -> exitSuccess
    Left ex ->
      case fromException ex of
        Just (AbortCompilation msg ec) ->
          hPutStrLn stderr (printf "\033[31mCompilation aborted due to fatal error\n%s\033[0m\n" msg) >> (exitWith $ ExitFailure ec)
        _ ->
          hPutStrLn stderr "\033[31mInternal compiler error\n"
            >> hPutStrLn stderr (printf "Caught unexpected exception: %s\n" (show ex))
            >> (exitWith $ ExitFailure 1)

main' :: IO ()
main' = do
  checkByteorder
  args <- getArgs
  defaultConfig <- readIORef config'
  writeIORef config' $ parseCli args defaultConfig

  putStrLn $ show $ create 256 "0x123456aaP-2" roundTiesToEven
  print $ config.intSize

checkByteorder :: IO ()
checkByteorder = case byteOrder of
  Mixed _ -> systemError "Mixed byteorder is not supported"
  _ -> return ()

parseCli :: [String] -> Config -> Config
parseCli _ defaultConfig = defaultConfig {intSize = 8}

foo :: ExceptT Int IO ()
foo = undefined