/*******************************************************************************
 * Copyright (c) 2024. Liu Xiangzhi
 * This file is part of CAMI.
 *
 * CAMI is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 2 of the License, or any later version.
 *
 * CAMI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with CAMI.
 * If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef CAMI_AM_REPORTER_H
#define CAMI_AM_REPORTER_H

#include <iostream>
#include <lib/format.h>

namespace cami::am {

enum class UB
{
    data_race [[maybe_unused]] = 5,
    refer_del_obj = 9,
    use_ptr_value_which_ref_del_obj = 10,
    read_ir_obj = 11, // ir means indeterminate representation
    read_nvr = 12, // nvr means non-value representation
    store_nvr = 13,
    cast_to_or_from_integer = 16, // UB if result value is out of range that can be represented
    real_float_demotion = 17,
    eva_ivd_lvalue = 18,
    read_before_init = 20,
    unaligned_ptr_cast = 24,
    incompatible_func_call = 25, // e.g. void f(int){} int main(){ auto x = (void(*)(double))f; x(1.2); }
    modify_string_literal = 32,
    unsequenced_access = 34,
    exceptional_condition = 35,
    incompatible_read = 36, // e.g. float x = 1.2; auto y = *(uint32_t*)&x;
    access_member_of_atomic [[maybe_unused]] = 38,
    deref_ivd_ptr = 39,
    div_or_mod_zero = 41,
    div_not_representable = 42,
    ptr_addition_oob = 43, // oob means Out Of Boundary
    deref_ending_ptr = 44, // e.g. int x[2] = {0}; *(x + 2);
    ivd_ptr_subtraction = 45,
    idx_oob = 46,
    ivd_rhs_of_shift = 48,
    ivd_result_of_left_shit = 49,
    ivd_ptr_compare = 50,
    overlap_obj_assign = 51,
    modify_const_obj = 61,
    ivd_read_volatile_obj = 62,
    ivd_modify_restrict_obj [[maybe_unused]] = 65,
    ivd_restrict_ptr_assign [[maybe_unused]] = 66,
    nonpositive_len_of_vla [[maybe_unused]] = 72,
    return_undefined = 85,
};
extern const char* const ub_descriptions[218];
} // namespace cami::am

CAMI_DECLARE_FORMATTER(cami::am::UB);

#endif //CAMI_AM_REPORTER_H
