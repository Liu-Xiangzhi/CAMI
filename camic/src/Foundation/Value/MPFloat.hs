{-# LANGUAGE CApiFFI #-}

module Foundation.Value.MPFloat
  ( MPFloat,
    Precision,
    RoundMode,
    ArithResult (..),
    roundTiesToEven,
    roundTowardNegative,
    roundTowardPositive,
    roundTowardZero,
    roundAwayZero,
    roundFaithful,
    create,
    add,
    -- ...
  )
where

import Foreign (FinalizerPtr, ForeignPtr, Ptr, alloca, newForeignPtr, withForeignPtr)
import Foreign.C (CString, peekCString, withCString)
import Foreign.C.Types (CInt (..), CLong (..))
import Foreign.Storable
import GHC.IO (unsafePerformIO)

newtype MPFloat = MPFloat {mpfr :: ForeignPtr ()}

newtype {-# CTYPE "long" #-} RoundMode = RoundMode {roundMode :: CInt}

type Precision = CLong

data Accurary = Exact | Greater | Less

data ArithResult = ArithResult {result :: MPFloat, accurary :: Accurary}

foreign import capi "mpfr.h value MPFR_RNDN" roundTiesToEven :: RoundMode

foreign import capi "mpfr.h value MPFR_RNDD" roundTowardNegative :: RoundMode

foreign import capi "mpfr.h value MPFR_RNDU" roundTowardPositive :: RoundMode

foreign import capi "mpfr.h value MPFR_RNDZ" roundTowardZero :: RoundMode

foreign import capi "mpfr.h value MPFR_RNDA" roundAwayZero :: RoundMode

foreign import capi "mpfr.h value MPFR_RNDF" roundFaithful :: RoundMode

foreign import capi "mpfloat.h init" initMPFloat :: Precision -> IO (Ptr ())

foreign import capi "mpfloat.h init2" initMPFloat2 :: CString -> CInt -> Ptr () -> IO (CInt)

foreign import capi "mpfloat.h &destory" finalizer :: FinalizerPtr ()

foreign import capi "mpfloat.h fmt" fmt :: Ptr () -> IO CString

foreign import capi "mpfloat.h add" addMPFloat :: Ptr () -> Ptr () -> RoundMode -> Ptr CInt -> IO (Ptr ())

create :: Precision -> String -> RoundMode -> Maybe MPFloat
create prec str rnd =
  unsafePerformIO $ withCString str $ \cstr -> do
    ptr <- initMPFloat prec
    ec <- initMPFloat2 cstr rnd.roundMode ptr
    if ec == 0 then Just . MPFloat <$> newForeignPtr finalizer ptr else return Nothing

binaryOperation ::
  (Ptr () -> Ptr () -> RoundMode -> Ptr CInt -> IO (Ptr ())) ->
  MPFloat ->
  MPFloat ->
  RoundMode ->
  ArithResult
binaryOperation op x y rnd =
  unsafePerformIO $ withForeignPtr x.mpfr $ \x' ->
    withForeignPtr y.mpfr $ \y' -> alloca $ \p -> do
      ptr <- op x' y' rnd p
      mpfloat <- newForeignPtr finalizer ptr
      accurary <- peek p
      return $ ArithResult (MPFloat mpfloat) (if accurary == 0 then Exact else if accurary > 0 then Greater else Less)

add :: MPFloat -> MPFloat -> RoundMode -> ArithResult
add = binaryOperation addMPFloat

instance Show MPFloat where
  show x = unsafePerformIO $ withForeignPtr x.mpfr $ \x' -> fmt x' >>= peekCString
