let source_name = ref ""
let free_standing = ref false
let short_size = ref 2
let int_size = ref 4
let long_size = ref 8
let long_long_size = ref 8
let char_as_schar = ref true
let colored_print = ref true

let check_validity () =
  let res = ref true in
  let report_if_not v err =
    if not v then (
      res := false;
      Printf.eprintf "\x1b[31mInvalid command line arguments: %s\x1b[0m\n" err)
    else ()
  in
  let is_2_power x = x land (x - 1) = 0 in
  report_if_not (!short_size <= !int_size) "size of int type is less than that of short type";
  report_if_not (!int_size <= !long_size) "size of long type is less than that of int type";
  report_if_not (!long_size <= !long_long_size) "size of long long type is less than that of long type";
  report_if_not (is_2_power !short_size) "size of short type is not 2's power";
  report_if_not (is_2_power !int_size) "size of int type is not 2's power";
  report_if_not (is_2_power !long_size) "size of long type is not 2's power";
  report_if_not (is_2_power !long_long_size) "size of long long type is not 2's power";
  (* remove this statement if customization of config is supported  *)
  report_if_not
    (!free_standing = false && !short_size = 2 && !int_size = 4 && !long_size = 8 && !long_long_size = 8)
    "customization of config is not supported by CAMI yet";
  !res
