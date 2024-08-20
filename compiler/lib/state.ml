module type Monad = sig
  type 'a t

  val return : 'a -> 'a t
  val ( >>= ) : 'a t -> ('a -> 'b t) -> 'b t
end

module type Context = sig
  type t
end

module State (Ctx : Context) : Monad with type 'a t = Ctx.t -> 'a * Ctx.t = struct
  type 'a t = Ctx.t -> 'a * Ctx.t

  let return x ctx = (x, ctx)

  let ( >>= ) m f ctx =
    let x, ctx' = m ctx in
    f x ctx'

  let set ctx _ = ((), ctx)
  let get () ctx = (ctx, ctx)
  let ( let$ ) = ( >>= )
end
(* module ProceContext : Context with type t = Lex.Preprocessor.t = struct
  type t = Lex.Preprocessor.t
end
module LexerContext : Context with type t = Lex.Preprocessor.t = struct
  type t = Lex.Preprocessor.t
end

module ParserContext : Context with type t = Lex.Lexer.t = struct
  type t = Lex.Lexer.t
end

module Lexer = State (LexerContext)
module Parser = State (ParserContext) *)
