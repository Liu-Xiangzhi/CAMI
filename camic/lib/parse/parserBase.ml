type state = {
  lexer : Lexer.state;
  typedefs : Unicode.Set.t list;
}

module Parser = Parsing.MakeLazy (struct
  type t = state
  type payload = Token.t

  let run1 st =
    let tk, lexer = Lexer.next_token st.lexer in
    (tk, { st with lexer })
end)