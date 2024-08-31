type t =
  | Basic of BasicValue.t
  | Null
  | Pointer of {
      obj : static_object;
      offset : int;
    }
  | Pointer' of {
      tp : Type.t;
      value : int;
    }
  | Array of t array
  | Struct of {
      tp : Type.t;
      value : t array;
    }
  | Union of {
      tp : Type.t;
      value : t array;
      activated_member_id : int;
    }

and static_object = {
  name : Unicode.string;
  tp : Type.t;
  value : t;
}

let rec typeof = function
  | Basic v -> BasicValue.typeof v
  | Null -> Type.Null
  | Pointer { obj = { tp; _ }; _ } -> Type.Pointer tp
  | Pointer' { tp; _ } -> tp
  | Array arr ->
      let len = Array.length arr and element = typeof @@ Array.get arr 0 in
      Type.Array { len; element }
  | Struct { tp; _ } -> tp
  | Union { tp; _ } -> tp

let rec show = function
  | Basic v -> BasicValue.show v
  | Null -> "nullptr"
  | Pointer { obj = { name; _ }; offset } -> Printf.sprintf "pointer to '%s' with offset %d" (Unicode.to_u8_string name) offset
  | Pointer' { value; _ } -> Printf.sprintf "dissociative pointer with value %d" value
  | Array arr ->
      let lst = Array.fold_right (fun x acc -> show x :: acc) arr [] in
      Printf.sprintf "[%s]" (String.concat "," lst)
  | Struct { tp; value } ->
      let (Type.Struct { name; members }) = tp [@@warning "-8"] in
      let lst =
        Array.fold_right
          (fun ((member_field, member_v) : Type.field * t) acc ->
            Printf.sprintf "%s = %s" (Unicode.to_u8_string member_field.name) (show member_v) :: acc)
          (Array.combine members value) []
      in
      Printf.sprintf "struct %s{ %s }" (Unicode.to_u8_string name) (String.concat "; " lst)
  | Union { tp; value; activated_member_id } ->
      let (Type.Union { name; members }) = tp [@@warning "-8"] in
      Printf.sprintf "union %s{ activated field:%s = %s }" (Unicode.to_u8_string name)
        (Unicode.to_u8_string members.(activated_member_id).name)
        (show value.(activated_member_id))
