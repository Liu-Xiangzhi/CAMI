(*
Here is the template for the TASTFolder

module type PreorderVisitor = sig
  val decl : 'a
end

module type PostorderVisitor = sig
  val decl' : 'a (* postorder version *)
end

module type Visitor = sig
  include PreorderVisitor
  include PostorderVisitor
end

module EmptyPreorderVisitor = struct
  let decl = ()
end

module PostorderVisitorCopyFromPreorder (V:PreorderVisitor) = struct
  let decl' = V.decl
end

module Make(V:Visitor) = struct
  let fold ast = 0(*undefined*)
end *)
