#include <check.h>
#include <math.h>

#include "../s21_decimal.h"

// ============================================================
// ТЕСТЫ ДЛЯ ВСПОМОГАТЕЛЬНЫХ ФУНКЦИЙ (s21_helpers.c)
// ============================================================

START_TEST(test_get_bit) {
  s21_decimal dec = {0};

  dec.bits[0] = 1u;
  ck_assert_int_eq(s21_get_bit(dec, 0), 1);
  ck_assert_int_eq(s21_get_bit(dec, 1), 0);

  dec.bits[0] = 0x80000000u;
  ck_assert_int_eq(s21_get_bit(dec, 31), 1);
  ck_assert_int_eq(s21_get_bit(dec, 30), 0);

  dec.bits[1] = 1u;
  ck_assert_int_eq(s21_get_bit(dec, 32), 1);
}
END_TEST

START_TEST(test_set_bit) {
  s21_decimal dec = {0};

  s21_set_bit(&dec, 0);
  ck_assert_int_eq(dec.bits[0], 1u);

  s21_set_bit(&dec, 5);
  ck_assert_int_eq(dec.bits[0], 33u);

  s21_set_bit(&dec, 32);
  ck_assert_int_eq(dec.bits[1], 1u);
}
END_TEST

START_TEST(test_clear_bit) {
  s21_decimal dec = {0};
  dec.bits[0] = 0xFFFFFFFFu;
  dec.bits[1] = 0xFFFFFFFFu;

  s21_clear_bit(&dec, 0);
  ck_assert_int_eq(dec.bits[0], 0xFFFFFFFEu);

  s21_clear_bit(&dec, 31);
  ck_assert_int_eq(dec.bits[0], 0x7FFFFFFEu);
}
END_TEST

START_TEST(test_get_sign) {
  s21_decimal dec = {0};

  dec.bits[3] = 0u;
  ck_assert_int_eq(s21_get_sign(dec), 0);

  dec.bits[3] = 0x80000000u;
  ck_assert_int_eq(s21_get_sign(dec), 1);
}
END_TEST

START_TEST(test_set_sign) {
  s21_decimal dec = {0};

  s21_set_sign(&dec, 1);
  ck_assert_int_eq(dec.bits[3] & 0x80000000u, 0x80000000u);

  s21_set_sign(&dec, 0);
  ck_assert_int_eq(dec.bits[3] & 0x80000000u, 0u);
}
END_TEST

START_TEST(test_negate_sign) {
  s21_decimal dec = {0};

  s21_negate_sign(&dec);
  ck_assert_int_eq(dec.bits[3] & 0x80000000u, 0x80000000u);

  s21_negate_sign(&dec);
  ck_assert_int_eq(dec.bits[3] & 0x80000000u, 0u);
}
END_TEST

START_TEST(test_get_scale) {
  s21_decimal dec = {0};

  dec.bits[3] = 0x00050000u;
  ck_assert_int_eq(s21_get_scale(dec), 5);

  dec.bits[3] = 0x001C0000u;
  ck_assert_int_eq(s21_get_scale(dec), 28);
}
END_TEST

START_TEST(test_set_scale) {
  s21_decimal dec = {0};

  s21_set_scale(&dec, 5);
  ck_assert_int_eq(s21_get_scale(dec), 5);
  ck_assert_int_eq(dec.bits[3], 0x00050000u);

  s21_set_scale(&dec, 30);
  ck_assert_int_eq(s21_get_scale(dec), 28);

  s21_set_scale(&dec, -5);
  ck_assert_int_eq(s21_get_scale(dec), 0);
}
END_TEST

START_TEST(test_is_zero) {
  s21_decimal dec = {0};

  s21_zero_decimal(&dec);
  ck_assert_int_eq(s21_is_zero(dec), 1);

  dec.bits[3] = 5u << 16;
  ck_assert_int_eq(s21_is_zero(dec), 1);

  dec.bits[0] = 1u;
  ck_assert_int_eq(s21_is_zero(dec), 0);
}
END_TEST

START_TEST(test_shift_left_96) {
  s21_decimal dec = {0};

  dec.bits[0] = 1u;
  s21_shift_left_96(&dec);
  ck_assert_int_eq(dec.bits[0], 2u);

  dec.bits[0] = 0x80000000u;
  s21_shift_left_96(&dec);
  ck_assert_int_eq(dec.bits[0], 0u);
  ck_assert_int_eq(dec.bits[1], 1u);
}
END_TEST

START_TEST(test_shift_right_96) {
  s21_decimal dec = {0};

  dec.bits[0] = 2u;
  s21_shift_right_96(&dec);
  ck_assert_int_eq(dec.bits[0], 1u);

  dec.bits[0] = 1u;
  dec.bits[1] = 1u;
  s21_shift_right_96(&dec);
  ck_assert_int_eq(dec.bits[0], 0x80000000u);
  ck_assert_int_eq(dec.bits[1], 0u);
}
END_TEST

// ============================================================
// ТЕСТЫ ДЛЯ КОНВЕРТЕРОВ (s21_convertors.c)
// ============================================================

START_TEST(test_from_int_positive) {
  s21_decimal result = {0};
  int status = s21_from_int_to_decimal(123, &result);

  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq(s21_get_sign(result), 0);
  ck_assert_int_eq(s21_get_scale(result), 0);
}
END_TEST

START_TEST(test_from_int_negative) {
  s21_decimal result = {0};
  int status = s21_from_int_to_decimal(-456, &result);

  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 456);
  ck_assert_int_eq(s21_get_sign(result), 1);
}
END_TEST

START_TEST(test_from_int_min) {
  s21_decimal result = {0};
  int status = s21_from_int_to_decimal(-2147483648, &result);

  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 2147483648u);
  ck_assert_int_eq(s21_get_sign(result), 1);
}
END_TEST

START_TEST(test_from_int_null) {
  int status = s21_from_int_to_decimal(123, NULL);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_to_int_positive) {
  s21_decimal src = {0};
  src.bits[0] = 123;
  int result = 0;

  int status = s21_from_decimal_to_int(src, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result, 123);
}
END_TEST

START_TEST(test_to_int_with_scale) {
  s21_decimal src = {0};
  src.bits[0] = 12345;
  s21_set_scale(&src, 2);

  int result = 0;
  int status = s21_from_decimal_to_int(src, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result, 123);
}
END_TEST

START_TEST(test_to_int_overflow) {
  s21_decimal src = {0};
  src.bits[0] = 3000000000u;

  int result = 0;
  int status = s21_from_decimal_to_int(src, &result);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_to_int_null) {
  s21_decimal src = {0};
  int status = s21_from_decimal_to_int(src, NULL);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_from_float_positive) {
  s21_decimal result = {0};
  int status = s21_from_float_to_decimal(123.45f, &result);

  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_get_sign(result), 0);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 123.45f, 1e-6f);
}
END_TEST

START_TEST(test_from_float_negative) {
  s21_decimal result = {0};
  int status = s21_from_float_to_decimal(-67.89f, &result);

  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_get_sign(result), 1);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, -67.89f, 1e-6f);
}
END_TEST

START_TEST(test_from_float_zero) {
  s21_decimal result = {0};
  int status = s21_from_float_to_decimal(0.0f, &result);

  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_is_zero(result), 1);
  ck_assert_int_eq(s21_get_sign(result), 0);
}
END_TEST

START_TEST(test_from_float_negative_zero) {
  s21_decimal result = {0};
  int status = s21_from_float_to_decimal(-0.0f, &result);

  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_is_zero(result), 1);
  ck_assert_int_eq(s21_get_sign(result), 1);
}
END_TEST

START_TEST(test_from_float_small) {
  s21_decimal result = {0};
  int status = s21_from_float_to_decimal(1e-30f, &result);

  ck_assert_int_eq(status, 1);
  ck_assert_int_eq(s21_is_zero(result), 1);
}
END_TEST

START_TEST(test_from_float_large) {
  s21_decimal result = {0};
  // Используем INFINITY для проверки слишком большого числа
  int status = s21_from_float_to_decimal(INFINITY, &result);

  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_from_float_inf) {
  s21_decimal result = {0};
  int status = s21_from_float_to_decimal(INFINITY, &result);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_from_float_nan) {
  s21_decimal result = {0};
  int status = s21_from_float_to_decimal(NAN, &result);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_from_float_null) {
  int status = s21_from_float_to_decimal(123.45f, NULL);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_to_float_positive) {
  s21_decimal src = {0};
  src.bits[0] = 12345;
  s21_set_scale(&src, 2);

  float result = 0;
  int status = s21_from_decimal_to_float(src, &result);

  ck_assert_int_eq(status, 0);
  ck_assert_float_eq_tol(result, 123.45f, 1e-6f);
}
END_TEST

START_TEST(test_to_float_negative) {
  s21_decimal src = {0};
  src.bits[0] = 6789;
  s21_set_scale(&src, 2);
  s21_set_sign(&src, 1);

  float result = 0;
  int status = s21_from_decimal_to_float(src, &result);

  ck_assert_int_eq(status, 0);
  ck_assert_float_eq_tol(result, -67.89f, 1e-6f);
}
END_TEST

START_TEST(test_to_float_zero) {
  s21_decimal src = {0};
  float result = 0;

  int status = s21_from_decimal_to_float(src, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_float_eq(result, 0.0f);
}
END_TEST

START_TEST(test_to_float_null) {
  s21_decimal src = {0};
  int status = s21_from_decimal_to_float(src, NULL);
  ck_assert_int_eq(status, 1);
}
END_TEST

// ============================================================
// ТЕСТЫ ДЛЯ ОПЕРАЦИЙ СРАВНЕНИЯ (s21_comparison.c)
// ============================================================

START_TEST(test_is_equal_positive) {
  s21_decimal a = {0}, b = {0};
  s21_from_int_to_decimal(123, &a);
  s21_from_int_to_decimal(123, &b);
  ck_assert_int_eq(s21_is_equal(a, b), 1);
}
END_TEST

START_TEST(test_is_equal_negative) {
  s21_decimal a = {0}, b = {0};
  s21_from_int_to_decimal(123, &a);
  s21_from_int_to_decimal(456, &b);
  ck_assert_int_eq(s21_is_equal(a, b), 0);
}
END_TEST

START_TEST(test_is_equal_zero) {
  s21_decimal a = {0}, b = {0};
  ck_assert_int_eq(s21_is_equal(a, b), 1);
}
END_TEST

START_TEST(test_is_equal_negative_zero) {
  s21_decimal a = {0}, b = {0};
  s21_from_float_to_decimal(0.0f, &a);
  s21_from_float_to_decimal(-0.0f, &b);
  ck_assert_int_eq(s21_is_equal(a, b), 1);
}
END_TEST

START_TEST(test_is_equal_different_scales) {
  s21_decimal a = {0}, b = {0};
  a.bits[0] = 12345;
  s21_set_scale(&a, 2);

  b.bits[0] = 12345;
  s21_set_scale(&b, 3);

  ck_assert_int_eq(s21_is_equal(a, b), 0);
}
END_TEST

START_TEST(test_is_less_positive) {
  s21_decimal a = {0}, b = {0};
  s21_from_int_to_decimal(123, &a);
  s21_from_int_to_decimal(456, &b);
  ck_assert_int_eq(s21_is_less(a, b), 1);
  ck_assert_int_eq(s21_is_less(b, a), 0);
}
END_TEST

START_TEST(test_is_less_negative) {
  s21_decimal a = {0}, b = {0};
  s21_from_int_to_decimal(-123, &a);
  s21_from_int_to_decimal(-456, &b);
  ck_assert_int_eq(s21_is_less(a, b), 0);
  ck_assert_int_eq(s21_is_less(b, a), 1);
}
END_TEST

START_TEST(test_is_less_different_signs) {
  s21_decimal a = {0}, b = {0};
  s21_from_int_to_decimal(-123, &a);
  s21_from_int_to_decimal(456, &b);
  ck_assert_int_eq(s21_is_less(a, b), 1);
}
END_TEST

START_TEST(test_is_less_zero) {
  s21_decimal a = {0}, b = {0};
  ck_assert_int_eq(s21_is_less(a, b), 0);
}
END_TEST

START_TEST(test_is_greater) {
  s21_decimal a = {0}, b = {0};
  s21_from_int_to_decimal(456, &a);
  s21_from_int_to_decimal(123, &b);
  ck_assert_int_eq(s21_is_greater(a, b), 1);
  ck_assert_int_eq(s21_is_greater(b, a), 0);
}
END_TEST

START_TEST(test_is_not_equal) {
  s21_decimal a = {0}, b = {0};
  s21_from_int_to_decimal(123, &a);
  s21_from_int_to_decimal(456, &b);
  ck_assert_int_eq(s21_is_not_equal(a, b), 1);

  s21_from_int_to_decimal(123, &b);
  ck_assert_int_eq(s21_is_not_equal(a, b), 0);
}
END_TEST

// ============================================================
// ТЕСТЫ ДЛЯ ДРУГИХ ФУНКЦИЙ (s21_other.c)
// ============================================================

START_TEST(test_bank_round_ull) {
  ck_assert_int_eq(s21_bank_round_ull(2, 4), 2);  // 2.4 → 2
  ck_assert_int_eq(s21_bank_round_ull(2, 5), 2);  // 2.5 → 2 (четное)
  ck_assert_int_eq(s21_bank_round_ull(3, 5), 4);  // 3.5 → 4 (нечетное)
  ck_assert_int_eq(s21_bank_round_ull(2, 6), 3);  // 2.6 → 3
  ck_assert_int_eq(s21_bank_round_ull(4, 5), 4);  // 4.5 → 4 (четное)
  ck_assert_int_eq(s21_bank_round_ull(5, 5), 6);  // 5.5 → 6 (нечетное)
}
END_TEST

START_TEST(test_truncate) {
  s21_decimal src = {0}, result = {0};
  src.bits[0] = 12345;
  s21_set_scale(&src, 2);

  s21_truncate(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq(s21_get_scale(result), 0);
}
END_TEST

START_TEST(test_truncate_negative) {
  s21_decimal src = {0}, result = {0};
  src.bits[0] = 12345;
  s21_set_scale(&src, 2);
  s21_set_sign(&src, 1);

  s21_truncate(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq(s21_get_sign(result), 1);
  ck_assert_int_eq(s21_get_scale(result), 0);
}
END_TEST

START_TEST(test_truncate_zero_scale) {
  s21_decimal src = {0}, result = {0};
  src.bits[0] = 123;

  s21_truncate(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq(s21_get_scale(result), 0);
}
END_TEST

START_TEST(test_truncate_null) {
  s21_decimal src = {0};
  int status = s21_truncate(src, NULL);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_negate) {
  s21_decimal src = {0}, result = {0};
  s21_from_int_to_decimal(123, &src);

  s21_negate(src, &result);
  ck_assert_int_eq(s21_get_sign(result), 1);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, -123);
}
END_TEST

START_TEST(test_negate_negative) {
  s21_decimal src = {0}, result = {0};
  s21_from_int_to_decimal(-456, &src);

  s21_negate(src, &result);
  ck_assert_int_eq(s21_get_sign(result), 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, 456);
}
END_TEST

START_TEST(test_negate_zero) {
  s21_decimal src = {0}, result = {0};
  s21_from_float_to_decimal(0.0f, &src);

  s21_negate(src, &result);
  ck_assert_int_eq(s21_is_zero(result), 1);
  ck_assert_int_eq(s21_get_sign(result), 1);
}
END_TEST

START_TEST(test_negate_negative_zero) {
  s21_decimal src = {0}, result = {0};
  s21_from_float_to_decimal(-0.0f, &src);

  s21_negate(src, &result);
  ck_assert_int_eq(s21_is_zero(result), 1);
  ck_assert_int_eq(s21_get_sign(result), 0);
}
END_TEST

START_TEST(test_negate_null) {
  s21_decimal src = {0};
  int status = s21_negate(src, NULL);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_floor_positive) {
  s21_decimal src = {0}, result = {0};
  src.bits[0] = 12345;
  s21_set_scale(&src, 2);

  s21_floor(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq(s21_get_scale(result), 0);
}
END_TEST

START_TEST(test_floor_negative) {
  s21_decimal src = {0}, result = {0};
  src.bits[0] = 12345;
  s21_set_scale(&src, 2);
  s21_set_sign(&src, 1);

  s21_floor(src, &result);
  ck_assert_int_eq(result.bits[0], 124);
  ck_assert_int_eq(s21_get_sign(result), 1);
}
END_TEST

START_TEST(test_floor_integer) {
  s21_decimal src = {0}, result = {0};
  s21_from_int_to_decimal(123, &src);

  s21_floor(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
}
END_TEST

START_TEST(test_floor_null) {
  s21_decimal src = {0};
  int status = s21_floor(src, NULL);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_round_positive) {
  s21_decimal src = {0}, result = {0};
  src.bits[0] = 12356;
  s21_set_scale(&src, 2);

  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 124);
}
END_TEST

START_TEST(test_round_positive_bank) {
  s21_decimal src = {0}, result = {0};
  src.bits[0] = 12345;
  s21_set_scale(&src, 2);

  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
}
END_TEST

START_TEST(test_round_negative) {
  s21_decimal src = {0}, result = {0};
  src.bits[0] = 12356;
  s21_set_scale(&src, 2);
  s21_set_sign(&src, 1);

  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 124);
  ck_assert_int_eq(s21_get_sign(result), 1);
}
END_TEST

START_TEST(test_round_null) {
  s21_decimal src = {0};
  int status = s21_round(src, NULL);
  ck_assert_int_eq(status, 1);
}
END_TEST

// ============================================================
// ГРАНИЧНЫЕ ТЕСТЫ
// ============================================================

START_TEST(test_boundary_max_int) {
  s21_decimal result = {0};
  int status = s21_from_int_to_decimal(2147483647, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 2147483647u);
}
END_TEST

START_TEST(test_boundary_min_int) {
  s21_decimal result = {0};
  int status = s21_from_int_to_decimal(-2147483648, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 2147483648u);
}
END_TEST

START_TEST(test_boundary_to_int_max) {
  s21_decimal src = {0};
  src.bits[0] = 2147483647u;
  int result = 0;

  int status = s21_from_decimal_to_int(src, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result, 2147483647);
}
END_TEST

START_TEST(test_boundary_to_int_min) {
  s21_decimal src = {0};
  src.bits[0] = 2147483648u;
  s21_set_sign(&src, 1);
  int result = 0;

  int status = s21_from_decimal_to_int(src, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result, -2147483648);
}
END_TEST

START_TEST(test_boundary_float_max) {
  s21_decimal result = {0};
  int status =
      s21_from_float_to_decimal(79228162514264337593543950335.0f, &result);
  ck_assert_int_eq(status, 0);
}
END_TEST

START_TEST(test_boundary_float_min) {
  s21_decimal result = {0};
  int status =
      s21_from_float_to_decimal(-79228162514264337593543950335.0f, &result);
  ck_assert_int_eq(status, 0);
}
END_TEST

START_TEST(test_boundary_float_very_small) {
  s21_decimal result = {0};
  int status = s21_from_float_to_decimal(1e-29f, &result);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_boundary_float_very_large) {
  s21_decimal result = {0};
  // Используем 1.0e30f - помещается в float, но превышает Decimal
  int status = s21_from_float_to_decimal(1.0e30f, &result);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_boundary_bank_round_edge_cases) {
  // 2.5 → 2
  ck_assert_int_eq(s21_bank_round_ull(2, 5), 2);
  // 3.5 → 4
  ck_assert_int_eq(s21_bank_round_ull(3, 5), 4);
  // 4.5 → 4
  ck_assert_int_eq(s21_bank_round_ull(4, 5), 4);
  // 5.5 → 6
  ck_assert_int_eq(s21_bank_round_ull(5, 5), 6);
}
END_TEST

START_TEST(test_boundary_compare_equal_scales) {
  s21_decimal a = {0}, b = {0};
  a.bits[0] = 123;
  b.bits[0] = 123;
  s21_set_scale(&a, 5);
  s21_set_scale(&b, 5);

  ck_assert_int_eq(s21_is_equal(a, b), 1);
}
END_TEST

START_TEST(test_boundary_compare_different_scales) {
  s21_decimal a = {0}, b = {0};
  a.bits[0] = 123;
  b.bits[0] = 1230;
  s21_set_scale(&a, 0);
  s21_set_scale(&b, 1);

  ck_assert_int_eq(s21_is_equal(a, b), 1);
}
END_TEST

START_TEST(test_boundary_compare_zero_different_scales) {
  s21_decimal a = {0}, b = {0};
  s21_set_scale(&a, 5);
  s21_set_scale(&b, 10);

  ck_assert_int_eq(s21_is_equal(a, b), 1);
}
END_TEST

START_TEST(test_boundary_truncate_max_scale) {
  s21_decimal src = {0}, result = {0};
  // Используем число, которое помещается в unsigned int
  src.bits[0] = 1234567890u;
  src.bits[1] = 0u;
  src.bits[2] = 0u;
  s21_set_scale(&src, 28);

  s21_truncate(src, &result);
  ck_assert_int_eq(s21_get_scale(result), 0);
}
END_TEST

START_TEST(test_boundary_negate_zero) {
  s21_decimal src = {0}, result = {0};
  s21_from_float_to_decimal(-0.0f, &src);

  s21_negate(src, &result);
  ck_assert_int_eq(s21_is_zero(result), 1);
  ck_assert_int_eq(s21_get_sign(result), 0);
}
END_TEST

// ============================================================
// ДОПОЛНИТЕЛЬНЫЕ ТЕСТЫ (ТОЛЬКО ЭТИ, ОСТАЛЬНЫЕ УЖЕ ЕСТЬ!)
// ============================================================

START_TEST(test_is_negative) {
  s21_decimal dec = {0};

  // Положительное число
  s21_from_int_to_decimal(123, &dec);
  ck_assert_int_eq(s21_is_negative(dec), 0);

  // Отрицательное число
  s21_from_int_to_decimal(-456, &dec);
  ck_assert_int_eq(s21_is_negative(dec), 1);

  // Ноль
  s21_zero_decimal(&dec);
  ck_assert_int_eq(s21_is_negative(dec), 0);
}
END_TEST

START_TEST(test_is_positive) {
  s21_decimal dec = {0};

  // Положительное число
  s21_from_int_to_decimal(123, &dec);
  ck_assert_int_eq(s21_is_positive(dec), 1);

  // Отрицательное число
  s21_from_int_to_decimal(-456, &dec);
  ck_assert_int_eq(s21_is_positive(dec), 0);

  // Ноль
  s21_zero_decimal(&dec);
  ck_assert_int_eq(s21_is_positive(dec), 0);
}
END_TEST

START_TEST(test_get_bit_boundary) {
  s21_decimal dec = {0};
  dec.bits[0] = 1u;

  // Позиция 0
  ck_assert_int_eq(s21_get_bit(dec, 0), 1);
  // Позиция 127 (несуществующий бит)
  ck_assert_int_eq(s21_get_bit(dec, 127), 0);
  // Позиция -1 (некорректная)
  ck_assert_int_eq(s21_get_bit(dec, -1), 0);
  // Позиция 128 (некорректная)
  ck_assert_int_eq(s21_get_bit(dec, 128), 0);
}
END_TEST

START_TEST(test_set_bit_boundary) {
  s21_decimal dec = {0};

  // Установка бита в позицию 127 (максимальная)
  s21_set_bit(&dec, 127);
  ck_assert_int_eq(s21_get_bit(dec, 127), 1);

  // Попытка установить бит в некорректную позицию (-1 и 128)
  s21_set_bit(&dec, -1);
  s21_set_bit(&dec, 128);
  // Должно быть без изменений
  ck_assert_int_eq(s21_get_bit(dec, 127), 1);
}
END_TEST

START_TEST(test_shift_left_overflow) {
  s21_decimal dec = {0};
  dec.bits[2] = 0x80000000u;  // Старший бит установлен

  int overflow = s21_shift_left_96(&dec);
  ck_assert_int_eq(overflow, 1);  // Должно быть переполнение
}
END_TEST

START_TEST(test_is_zero_negative_scale) {
  s21_decimal dec = {0};
  // Отрицательный ноль с масштабом
  s21_set_sign(&dec, 1);
  s21_set_scale(&dec, 5);
  ck_assert_int_eq(s21_is_zero(dec), 1);  // Все равно ноль
}
END_TEST

START_TEST(test_floor_negative_various) {
  s21_decimal src = {0}, result = {0};

  // -123.45 → -124
  src.bits[0] = 12345;
  s21_set_scale(&src, 2);
  s21_set_sign(&src, 1);
  s21_floor(src, &result);
  ck_assert_int_eq(result.bits[0], 124);
  ck_assert_int_eq(s21_get_sign(result), 1);

  // -123.00 → -123 (без дробной части)
  s21_zero_decimal(&src);
  src.bits[0] = 12300;
  s21_set_scale(&src, 2);
  s21_set_sign(&src, 1);
  s21_floor(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq(s21_get_sign(result), 1);
}
END_TEST

START_TEST(test_floor_max_scale) {
  s21_decimal src = {0}, result = {0};
  src.bits[0] = 1234567890u;
  s21_set_scale(&src, 28);
  s21_set_sign(&src, 1);

  s21_floor(src, &result);
  // Проверяем, что функция не падает при максимальном масштабе
  ck_assert_int_eq(s21_get_scale(result), 0);
}
END_TEST

START_TEST(test_floor_negative_integer) {
  s21_decimal src = {0}, result = {0};
  s21_from_int_to_decimal(-123, &src);

  s21_floor(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq(s21_get_sign(result), 1);
}
END_TEST

START_TEST(test_round_negative_bank) {
  s21_decimal src = {0}, result = {0};

  // -123.5 → -124 (банковское округление)
  src.bits[0] = 1235;
  s21_set_scale(&src, 1);
  s21_set_sign(&src, 1);
  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 124);
  ck_assert_int_eq(s21_get_sign(result), 1);

  // -122.5 → -122 (банковское округление к четному)
  s21_zero_decimal(&src);
  src.bits[0] = 1225;
  s21_set_scale(&src, 1);
  s21_set_sign(&src, 1);
  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 122);
  ck_assert_int_eq(s21_get_sign(result), 1);
}
END_TEST

START_TEST(test_round_zero_scale) {
  s21_decimal src = {0}, result = {0};
  s21_from_int_to_decimal(123, &src);

  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq(s21_get_scale(result), 0);
}
END_TEST

START_TEST(test_truncate_negative_zero_scale) {
  s21_decimal src = {0}, result = {0};
  src.bits[0] = 123;
  s21_set_sign(&src, 1);

  s21_truncate(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq(s21_get_sign(result), 1);
  ck_assert_int_eq(s21_get_scale(result), 0);
}
END_TEST

START_TEST(test_negate_positive) {
  s21_decimal src = {0}, result = {0};
  s21_from_int_to_decimal(123, &src);

  s21_negate(src, &result);
  ck_assert_int_eq(s21_get_sign(result), 1);
  ck_assert_int_eq(result.bits[0], 123);
}
END_TEST

START_TEST(test_bank_round_decimal) {
  s21_decimal value = {0};

  // 2.5 → 2 (четное)
  value.bits[0] = 2;
  int result = s21_bank_round_decimal(&value, 0, 5);
  ck_assert_int_eq(result, 0);
  ck_assert_int_eq(value.bits[0], 2);

  // 3.5 → 4 (нечетное)
  value.bits[0] = 3;
  result = s21_bank_round_decimal(&value, 0, 5);
  ck_assert_int_eq(result, 0);
  ck_assert_int_eq(value.bits[0], 4);
}
END_TEST

START_TEST(test_bank_round_decimal_null) {
  int result = s21_bank_round_decimal(NULL, 0, 5);
  ck_assert_int_eq(result, 1);
}
END_TEST

START_TEST(test_bank_round_decimal_invalid_scale) {
  s21_decimal value = {0};
  int result = s21_bank_round_decimal(&value, 30, 5);
  ck_assert_int_eq(result, 1);

  result = s21_bank_round_decimal(&value, -1, 5);
  ck_assert_int_eq(result, 1);
}
END_TEST

START_TEST(test_bank_round_decimal_zero) {
  s21_decimal value = {0};
  int result = s21_bank_round_decimal(&value, 5, 5);
  ck_assert_int_eq(result, 0);
}
END_TEST

// ============================================================
// ТЕСТЫ ДЛЯ АРИФМЕТИКИ (s21_arithmetic.c)
// ============================================================

START_TEST(test_add_positive) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(123, &a);
  s21_from_int_to_decimal(456, &b);

  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, 579);
}
END_TEST

START_TEST(test_add_negative) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(-123, &a);
  s21_from_int_to_decimal(-456, &b);

  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, -579);
}
END_TEST

START_TEST(test_add_different_signs) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(123, &a);
  s21_from_int_to_decimal(-456, &b);

  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, -333);
}
END_TEST

START_TEST(test_add_with_scale) {
  s21_decimal a = {0}, b = {0}, result = {0};
  a.bits[0] = 12345;
  s21_set_scale(&a, 2);  // 123.45

  b.bits[0] = 456;
  s21_set_scale(&b, 1);  // 45.6

  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 0);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 169.05f, 1e-6f);
}
END_TEST

START_TEST(test_add_zero) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(123, &a);
  s21_from_int_to_decimal(0, &b);

  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, 123);
}
END_TEST

START_TEST(test_add_overflow) {
  s21_decimal a = {0}, b = {0}, result = {0};
  a.bits[0] = 0xFFFFFFFFu;
  a.bits[1] = 0xFFFFFFFFu;
  a.bits[2] = 0xFFFFFFFFu;

  b.bits[0] = 1;

  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 1);  // Переполнение
}
END_TEST

START_TEST(test_add_null) {
  s21_decimal a = {0}, b = {0};
  int status = s21_add(a, b, NULL);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_sub_positive) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(456, &a);
  s21_from_int_to_decimal(123, &b);

  int status = s21_sub(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, 333);
}
END_TEST

START_TEST(test_sub_negative) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(-456, &a);
  s21_from_int_to_decimal(-123, &b);

  int status = s21_sub(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, -333);
}
END_TEST

START_TEST(test_sub_different_signs) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(456, &a);
  s21_from_int_to_decimal(-123, &b);

  int status = s21_sub(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, 579);
}
END_TEST

START_TEST(test_sub_with_scale) {
  s21_decimal a = {0}, b = {0}, result = {0};
  a.bits[0] = 12345;
  s21_set_scale(&a, 2);  // 123.45

  b.bits[0] = 456;
  s21_set_scale(&b, 1);  // 45.6

  int status = s21_sub(a, b, &result);
  ck_assert_int_eq(status, 0);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 77.85f, 1e-6f);
}
END_TEST

START_TEST(test_sub_zero) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(123, &a);
  s21_from_int_to_decimal(0, &b);

  int status = s21_sub(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, 123);
}
END_TEST

START_TEST(test_sub_null) {
  s21_decimal a = {0}, b = {0};
  int status = s21_sub(a, b, NULL);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_mul_positive) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(12, &a);
  s21_from_int_to_decimal(34, &b);

  int status = s21_mul(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, 408);
}
END_TEST

START_TEST(test_mul_negative) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(-12, &a);
  s21_from_int_to_decimal(34, &b);

  int status = s21_mul(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, -408);
}
END_TEST

START_TEST(test_mul_with_scale) {
  s21_decimal a = {0}, b = {0}, result = {0};
  a.bits[0] = 12345;
  s21_set_scale(&a, 2);  // 123.45

  b.bits[0] = 456;
  s21_set_scale(&b, 1);  // 45.6

  int status = s21_mul(a, b, &result);
  ck_assert_int_eq(status, 0);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 5629.32f, 1e-6f);
}
END_TEST

START_TEST(test_mul_zero) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(123, &a);
  s21_from_int_to_decimal(0, &b);

  int status = s21_mul(a, b, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_is_zero(result), 1);
}
END_TEST

START_TEST(test_mul_overflow) {
  s21_decimal a = {0}, b = {0}, result = {0};
  a.bits[0] = 0xFFFFFFFFu;
  a.bits[1] = 0xFFFFFFFFu;
  a.bits[2] = 0xFFFFFFFFu;
  b.bits[0] = 2;

  int status = s21_mul(a, b, &result);
  ck_assert_int_eq(status, 1);  // Переполнение
}
END_TEST

START_TEST(test_mul_null) {
  s21_decimal a = {0}, b = {0};
  int status = s21_mul(a, b, NULL);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_div_positive) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(408, &a);
  s21_from_int_to_decimal(34, &b);

  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, 12);
}
END_TEST

START_TEST(test_div_negative) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(-408, &a);
  s21_from_int_to_decimal(34, &b);

  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, -12);
}
END_TEST

START_TEST(test_div_with_scale) {
  s21_decimal a = {0}, b = {0}, result = {0};
  a.bits[0] = 12345;
  s21_set_scale(&a, 2);  // 123.45

  b.bits[0] = 456;
  s21_set_scale(&b, 1);  // 45.6

  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 2.707236842f, 1e-6f);
}
END_TEST

START_TEST(test_div_by_zero) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(123, &a);
  s21_from_int_to_decimal(0, &b);

  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 3);  // Деление на ноль
}
END_TEST

START_TEST(test_div_zero) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(0, &a);
  s21_from_int_to_decimal(123, &b);

  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_is_zero(result), 1);
}
END_TEST

START_TEST(test_div_null) {
  s21_decimal a = {0}, b = {0};
  int status = s21_div(a, b, NULL);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_div_bank_rounding) {
  s21_decimal a = {0}, b = {0}, result = {0};
  // 10 / 3 = 3.333... -> банковское округление к 3
  s21_from_int_to_decimal(10, &a);
  s21_from_int_to_decimal(3, &b);

  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 3.33333333f, 1e-6f);
}
END_TEST

START_TEST(test_add_bank_rounding) {
  s21_decimal a = {0}, b = {0}, result = {0};
  // 1.005 + 2.005 = 3.01 (банковское округление)
  a.bits[0] = 1005;
  s21_set_scale(&a, 3);
  b.bits[0] = 2005;
  s21_set_scale(&b, 3);

  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 0);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 3.01f, 1e-6f);
}
END_TEST

START_TEST(test_sub_bank_rounding) {
  s21_decimal a = {0}, b = {0}, result = {0};
  // 3.005 - 1.005 = 2.00 (банковское округление)
  a.bits[0] = 3005;
  s21_set_scale(&a, 3);
  b.bits[0] = 1005;
  s21_set_scale(&b, 3);

  int status = s21_sub(a, b, &result);
  ck_assert_int_eq(status, 0);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 2.0f, 1e-6f);
}
END_TEST

START_TEST(test_mul_bank_rounding) {
  s21_decimal a = {0}, b = {0}, result = {0};
  // 1.5 * 1.5 = 2.25 (банковское округление к 2)
  a.bits[0] = 15;
  s21_set_scale(&a, 1);
  b.bits[0] = 15;
  s21_set_scale(&b, 1);

  int status = s21_mul(a, b, &result);
  ck_assert_int_eq(status, 0);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 2.25f, 1e-6f);
}
END_TEST

START_TEST(test_add_large_scale) {
  s21_decimal a = {0}, b = {0}, result = {0};
  a.bits[0] = 1;
  s21_set_scale(&a, 28);  // 1e-28

  b.bits[0] = 1;
  s21_set_scale(&b, 28);  // 1e-28

  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 0);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 2e-28f, 1e-28f);
}
END_TEST

START_TEST(test_sub_large_scale) {
  s21_decimal a = {0}, b = {0}, result = {0};
  a.bits[0] = 1;
  s21_set_scale(&a, 28);  // 1e-28

  b.bits[0] = 1;
  s21_set_scale(&b, 28);  // 1e-28

  int status = s21_sub(a, b, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_is_zero(result), 1);
}
END_TEST

START_TEST(test_div_result_scale) {
  s21_decimal a = {0}, b = {0}, result = {0};
  // 1 / 3 = 0.3333333333333333333333333333
  s21_from_int_to_decimal(1, &a);
  s21_from_int_to_decimal(3, &b);

  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 0.333333333f, 1e-6f);
}
END_TEST

// Добавь в tests/s21_tests.c
START_TEST(test_shift_left_debug) {
  s21_decimal dec = {0};
  dec.bits[0] = 5;

  s21_shift_left_96(&dec);
  // 5 << 1 = 10
  ck_assert_int_eq(dec.bits[0], 10);

  // Проверка переноса между разрядами
  dec.bits[0] = 0x80000000u;
  dec.bits[1] = 0;
  s21_shift_left_96(&dec);
  ck_assert_int_eq(dec.bits[0], 0);
  ck_assert_int_eq(dec.bits[1], 1);
}
END_TEST

START_TEST(test_div_debug) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(408, &a);
  s21_from_int_to_decimal(34, &b);

  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);

  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, 12);
}
END_TEST

START_TEST(test_div_bit_debug) {
  s21_decimal a = {0};
  a.bits[0] = 408;

  // 408 = 0b110011000 (9 бит)
  ck_assert_int_eq(s21_get_bit(a, 0), 0);
  ck_assert_int_eq(s21_get_bit(a, 1), 0);
  ck_assert_int_eq(s21_get_bit(a, 2), 0);
  ck_assert_int_eq(s21_get_bit(a, 3), 1);
  ck_assert_int_eq(s21_get_bit(a, 4), 1);
  ck_assert_int_eq(s21_get_bit(a, 5), 0);
  ck_assert_int_eq(s21_get_bit(a, 6), 0);
  ck_assert_int_eq(s21_get_bit(a, 7), 1);
  ck_assert_int_eq(s21_get_bit(a, 8), 1);
}
END_TEST
// ============================================================
// ДОПОЛНИТЕЛЬНЫЕ ТЕСТЫ ДЛЯ ПОКРЫТИЯ (branches/lines >= 80%)
// ============================================================

// --- helpers.c: NULL-проверки и печать ---
START_TEST(test_set_bit_null) {
  s21_set_bit(NULL, 0);  // не должно упасть
}
END_TEST

START_TEST(test_clear_bit_null) { s21_clear_bit(NULL, 0); }
END_TEST

START_TEST(test_set_sign_null) { s21_set_sign(NULL, 1); }
END_TEST

START_TEST(test_negate_sign_null) { s21_negate_sign(NULL); }
END_TEST

START_TEST(test_shift_right_96_null) { s21_shift_right_96(NULL); }
END_TEST

// --- convertors.c: недостижимые в других тестах ветки ---
START_TEST(test_to_int_overflow_high_words) {
  s21_decimal src = {0};
  src.bits[1] = 1;  // делает число > 32 бит
  int result = 0;
  int status = s21_from_decimal_to_int(src, &result);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_to_int_negative_overflow) {
  s21_decimal src = {0};
  src.bits[0] = 3000000000u;
  s21_set_sign(&src, 1);
  int result = 0;
  int status = s21_from_decimal_to_int(src, &result);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_from_float_big_needs_padding) {
  s21_decimal result = {0};
  int status = s21_from_float_to_decimal(123456789.0f, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_get_scale(result), 0);

  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 123456789.0f, 200.0f);
}
END_TEST

START_TEST(test_from_float_near_min_boundary) {
  s21_decimal result = {0};
  int status = s21_from_float_to_decimal(2e-28f, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_get_scale(result), 28);
}
END_TEST

// --- comparison.c: is_greater_or_equal / is_less_or_equal / прочие ветки ---
START_TEST(test_is_greater_or_equal_true) {
  s21_decimal a = {0}, b = {0};
  s21_from_int_to_decimal(5, &a);
  s21_from_int_to_decimal(5, &b);
  ck_assert_int_eq(s21_is_greater_or_equal(a, b), 1);

  s21_from_int_to_decimal(6, &a);
  ck_assert_int_eq(s21_is_greater_or_equal(a, b), 1);

  s21_from_int_to_decimal(4, &a);
  ck_assert_int_eq(s21_is_greater_or_equal(a, b), 0);
}
END_TEST

START_TEST(test_is_less_or_equal_true) {
  s21_decimal a = {0}, b = {0};
  s21_from_int_to_decimal(5, &a);
  s21_from_int_to_decimal(5, &b);
  ck_assert_int_eq(s21_is_less_or_equal(a, b), 1);

  s21_from_int_to_decimal(4, &a);
  ck_assert_int_eq(s21_is_less_or_equal(a, b), 1);

  s21_from_int_to_decimal(6, &a);
  ck_assert_int_eq(s21_is_less_or_equal(a, b), 0);
}
END_TEST

START_TEST(test_is_equal_zero_different_scales) {
  s21_decimal a = {0}, b = {0};
  s21_set_scale(&a, 5);
  s21_set_scale(&b, 2);
  ck_assert_int_eq(s21_is_equal(a, b), 1);
  ck_assert_int_eq(s21_is_less(a, b), 0);
}
END_TEST

START_TEST(test_is_less_zero_vs_nonzero_scaled) {
  s21_decimal a = {0}, b = {0};
  // a = 0 при малом scale, b ненулевое с большим scale -> цикл
  // s21_normalize должен поднять scale нулевого a без арифметики
  s21_set_scale(&a, 0);
  b.bits[0] = 5;
  s21_set_scale(&b, 3);  // 0.005
  ck_assert_int_eq(s21_is_less(a, b), 1);
}
END_TEST

START_TEST(test_is_equal_same_magnitude_diff_sign) {
  s21_decimal a = {0}, b = {0};
  s21_from_int_to_decimal(5, &a);
  s21_from_int_to_decimal(-5, &b);
  ck_assert_int_eq(s21_is_equal(a, b), 0);
}
END_TEST

START_TEST(test_compare_overflow_scale_mismatch) {
  // Максимум на scale 0 сравнивается с маленьким числом на большом scale -
  // нормализация не может поднять максимум ещё выше -> ветка переполнения
  s21_decimal max_dec = {{0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0}};
  s21_decimal small = {0};
  small.bits[0] = 1;
  s21_set_scale(&small, 10);

  // Функции сравнения должны просто не падать и вернуть какой-то
  // детерминированный (не обязательно "истинный") результат
  int r1 = s21_is_equal(max_dec, small);
  int r2 = s21_is_less(max_dec, small);
  ck_assert_int_eq(r1, 0);
  ck_assert_int_eq(r2, 0);
}
END_TEST

// --- other.c: переносы (carry) при округлении вверх ---
START_TEST(test_bank_round_decimal_carry) {
  s21_decimal value = {0};
  value.bits[0] = 0xFFFFFFFFu;
  int result =
      s21_bank_round_decimal(&value, 0, 6);  // digit>5 -> округление вверх
  ck_assert_int_eq(result, 0);
  ck_assert_int_eq(value.bits[0], 0u);
  ck_assert_int_eq(value.bits[1], 1u);
}
END_TEST

START_TEST(test_bank_round_decimal_full_overflow) {
  s21_decimal value = {{0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0}};
  int result = s21_bank_round_decimal(&value, 0, 6);
  ck_assert_int_eq(result, 1);  // настоящее переполнение
}
END_TEST

START_TEST(test_floor_negative_carry_bits1) {
  // -X.5, где X = 0xFFFFFFFF -> перенос из bits[0] в bits[1]
  s21_decimal src = {0}, result = {0};
  src.bits[0] = 0xFFFFFFFFu;
  src.bits[1] = 0;
  s21_set_scale(&src, 0);
  // делаем дробную часть искусственно: труним не подходит, поэтому
  // используем truncate+сравнение через floor напрямую с дробным числом
  s21_decimal frac = {0};
  frac.bits[0] = 4999999999u % 4294967295u;  // просто ненулевые младшие биты
  (void)frac;
  // Прямой сценарий: (2^32 - 1).5 при scale=1: mantissa = (2^32-1)*10+5
  s21_decimal v = {0};
  unsigned long long mant = (unsigned long long)0xFFFFFFFFu * 10ull + 5ull;
  v.bits[0] = (unsigned int)(mant & 0xFFFFFFFFu);
  v.bits[1] = (unsigned int)(mant >> 32);
  s21_set_scale(&v, 1);
  s21_set_sign(&v, 1);

  int status = s21_floor(v, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 0u);
  ck_assert_int_eq(result.bits[1], 1u);
}
END_TEST

START_TEST(test_floor_negative_carry_bits2) {
  // -X.5, где bits[0]=bits[1]=0xFFFFFFFF -> перенос доходит до bits[2]
  unsigned long long mant_lo = (unsigned long long)0xFFFFFFFFu * 10ull + 5ull;
  s21_decimal v = {0};
  v.bits[0] = (unsigned int)(mant_lo & 0xFFFFFFFFu);
  unsigned long long carry_to_1 = mant_lo >> 32;
  unsigned long long mant_hi =
      (unsigned long long)0xFFFFFFFFu * 10ull + carry_to_1;
  v.bits[1] = (unsigned int)(mant_hi & 0xFFFFFFFFu);
  v.bits[2] = (unsigned int)(mant_hi >> 32);
  s21_set_scale(&v, 1);
  s21_set_sign(&v, 1);

  s21_decimal result;
  int status = s21_floor(v, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 0u);
  ck_assert_int_eq(result.bits[1], 0u);
  ck_assert_int_eq(result.bits[2], 1u);
}
END_TEST

START_TEST(test_round_positive_carry_bits1) {
  unsigned long long mant =
      (unsigned long long)0xFFFFFFFFu * 10ull + 6ull;  // .6 -> округление вверх
  s21_decimal v = {0};
  v.bits[0] = (unsigned int)(mant & 0xFFFFFFFFu);
  v.bits[1] = (unsigned int)(mant >> 32);
  s21_set_scale(&v, 1);

  s21_decimal result;
  int status = s21_round(v, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 0u);
  ck_assert_int_eq(result.bits[1], 1u);
}
END_TEST

START_TEST(test_round_negative_carry_bits1) {
  unsigned long long mant = (unsigned long long)0xFFFFFFFFu * 10ull + 6ull;
  s21_decimal v = {0};
  v.bits[0] = (unsigned int)(mant & 0xFFFFFFFFu);
  v.bits[1] = (unsigned int)(mant >> 32);
  s21_set_scale(&v, 1);
  s21_set_sign(&v, 1);

  s21_decimal result;
  int status = s21_round(v, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 0u);
  ck_assert_int_eq(result.bits[1], 1u);
}
END_TEST

// --- arithmetic.c: недостающие ветки ---
START_TEST(test_add_zero_zero_same_sign) {
  s21_decimal a = {0}, b = {0}, result = {0};
  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_is_zero(result), 1);
}
END_TEST

START_TEST(test_add_left_zero) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(0, &a);
  s21_from_int_to_decimal(789, &b);
  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 0);
  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, 789);
}
END_TEST

START_TEST(test_sub_borrow_across_words) {
  s21_decimal a = {0}, b = {0}, result = {0};
  a.bits[1] = 1;  // a = 2^32
  b.bits[0] = 1;  // b = 1
  int status = s21_sub(a, b, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 0xFFFFFFFFu);
  ck_assert_int_eq(result.bits[1], 0u);
}
END_TEST

START_TEST(test_add_normalize_overflow) {
  // max на scale 0 нельзя поднять до scale 5 -> ошибка переполнения
  s21_decimal a = {{0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0}};
  s21_decimal b = {0};
  b.bits[0] = 1;
  s21_set_scale(&b, 5);
  s21_decimal result;
  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 1);
}
END_TEST

START_TEST(test_add_carry_with_scale_rounds) {
  // max (scale 1) + 2 (scale 1): переполнение суммы через 96 бит,
  // после деления остатка на 10 цифра округления > 5 -> apply_rounding
  s21_decimal a = {{0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0}};
  s21_set_scale(&a, 1);
  s21_decimal b = {0};
  b.bits[0] = 2;
  s21_set_scale(&b, 1);
  s21_decimal result;
  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_get_scale(result), 0);
}
END_TEST

START_TEST(test_mul_large_carry_propagation) {
  s21_decimal a = {{0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0}};
  s21_decimal b = {{0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0}};
  s21_decimal result;
  int status = s21_mul(a, b, &result);
  ck_assert_int_eq(status, 1);  // max*max точно переполняет
}
END_TEST

START_TEST(test_mul_scale_over_28_reduces_with_tie_rounding) {
  // 15 (scale 28) * 1 (scale 1): итоговый scale = 29 > 28,
  // при уменьшении убираем цифру "5" без хвоста -> округление
  // к чётному разряду (get_last_digit + apply_rounding)
  s21_decimal a = {0}, b = {0}, result = {0};
  a.bits[0] = 15;
  s21_set_scale(&a, 28);
  b.bits[0] = 1;
  s21_set_scale(&b, 1);
  int status = s21_mul(a, b, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_get_scale(result), 28);
  ck_assert_int_eq(result.bits[0], 2u);  // 1(нечёт) -> округлили вверх до 2
}
END_TEST

START_TEST(test_div_negative_initial_scale) {
  s21_decimal a = {0}, b = {0}, result = {0};
  a.bits[0] = 100;
  b.bits[0] = 5;
  s21_set_scale(&b, 2);  // 0.05
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, 2000);  // 100 / 0.05 = 2000
}
END_TEST

START_TEST(test_div_large_quotient_shift) {
  s21_decimal a = {0}, b = {0}, result = {0};
  a.bits[1] = 1;  // a = 2^32
  b.bits[0] = 2;
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 2147483648.0f, 1000.0f);
}
END_TEST

START_TEST(test_div_shift_to_scale_0_overflow) {
  // max уже на пределе - умножение на 10 при попытке добавить целых
  // разрядов переполняет мантиссу на первом же шаге
  s21_decimal a = {{0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0}};
  s21_decimal b = {0};
  b.bits[0] = 1;
  s21_set_scale(&b, 20);
  s21_decimal result;
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status == 1 || status == 2, 1);
}
END_TEST

START_TEST(test_div_exact_no_rounding) {
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(100, &a);
  s21_from_int_to_decimal(4, &b);
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
  int check = 0;
  s21_from_decimal_to_int(result, &check);
  ck_assert_int_eq(check, 25);
}
END_TEST

START_TEST(test_div_final_round_up) {
  // 2/3 = 0.666...; на scale=28 следующая цифра для округления - 6 (>5)
  // -> срабатывает s21_div_final_round -> s21_apply_rounding
  s21_decimal a = {0}, b = {0}, result = {0};
  s21_from_int_to_decimal(2, &a);
  s21_from_int_to_decimal(3, &b);
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(s21_get_scale(result), 28);
  float check = 0;
  s21_from_decimal_to_float(result, &check);
  ck_assert_float_eq_tol(check, 0.6666667f, 1e-5f);
}
END_TEST

// ============================================================
// НАБОРЫ ТЕСТОВ
// ============================================================

Suite *s21_all_suite(void) {
  Suite *suite = suite_create("s21_all");
  TCase *tc_helpers = tcase_create("Helpers");
  TCase *tc_convertors = tcase_create("Convertors");
  TCase *tc_comparison = tcase_create("Comparison");
  TCase *tc_other = tcase_create("Other");
  TCase *tc_boundary = tcase_create("Boundary");

  // Тесты для helpers
  tcase_add_test(tc_helpers, test_get_bit);
  tcase_add_test(tc_helpers, test_set_bit);
  tcase_add_test(tc_helpers, test_clear_bit);
  tcase_add_test(tc_helpers, test_get_sign);
  tcase_add_test(tc_helpers, test_set_sign);
  tcase_add_test(tc_helpers, test_negate_sign);
  tcase_add_test(tc_helpers, test_get_scale);
  tcase_add_test(tc_helpers, test_set_scale);
  tcase_add_test(tc_helpers, test_is_zero);
  tcase_add_test(tc_helpers, test_shift_left_96);
  tcase_add_test(tc_helpers, test_shift_right_96);
  tcase_add_test(tc_helpers, test_shift_left_debug);

  // Тесты для convertors
  tcase_add_test(tc_convertors, test_from_int_positive);
  tcase_add_test(tc_convertors, test_from_int_negative);
  tcase_add_test(tc_convertors, test_from_int_min);
  tcase_add_test(tc_convertors, test_from_int_null);
  tcase_add_test(tc_convertors, test_to_int_positive);
  tcase_add_test(tc_convertors, test_to_int_with_scale);
  tcase_add_test(tc_convertors, test_to_int_overflow);
  tcase_add_test(tc_convertors, test_to_int_null);
  tcase_add_test(tc_convertors, test_from_float_positive);
  tcase_add_test(tc_convertors, test_from_float_negative);
  tcase_add_test(tc_convertors, test_from_float_zero);
  tcase_add_test(tc_convertors, test_from_float_negative_zero);
  tcase_add_test(tc_convertors, test_from_float_small);
  tcase_add_test(tc_convertors, test_from_float_large);
  tcase_add_test(tc_convertors, test_from_float_inf);
  tcase_add_test(tc_convertors, test_from_float_nan);
  tcase_add_test(tc_convertors, test_from_float_null);
  tcase_add_test(tc_convertors, test_to_float_positive);
  tcase_add_test(tc_convertors, test_to_float_negative);
  tcase_add_test(tc_convertors, test_to_float_zero);
  tcase_add_test(tc_convertors, test_to_float_null);

  // Тесты для comparison
  tcase_add_test(tc_comparison, test_is_equal_positive);
  tcase_add_test(tc_comparison, test_is_equal_negative);
  tcase_add_test(tc_comparison, test_is_equal_zero);
  tcase_add_test(tc_comparison, test_is_equal_negative_zero);
  tcase_add_test(tc_comparison, test_is_equal_different_scales);
  tcase_add_test(tc_comparison, test_is_less_positive);
  tcase_add_test(tc_comparison, test_is_less_negative);
  tcase_add_test(tc_comparison, test_is_less_different_signs);
  tcase_add_test(tc_comparison, test_is_less_zero);
  tcase_add_test(tc_comparison, test_is_greater);
  tcase_add_test(tc_comparison, test_is_not_equal);

  // Тесты для other
  tcase_add_test(tc_other, test_bank_round_ull);
  tcase_add_test(tc_other, test_truncate);
  tcase_add_test(tc_other, test_truncate_negative);
  tcase_add_test(tc_other, test_truncate_zero_scale);
  tcase_add_test(tc_other, test_truncate_null);
  tcase_add_test(tc_other, test_negate);
  tcase_add_test(tc_other, test_negate_negative);
  tcase_add_test(tc_other, test_negate_zero);
  tcase_add_test(tc_other, test_negate_negative_zero);
  tcase_add_test(tc_other, test_negate_null);
  tcase_add_test(tc_other, test_floor_positive);
  tcase_add_test(tc_other, test_floor_negative);
  tcase_add_test(tc_other, test_floor_integer);
  tcase_add_test(tc_other, test_floor_null);
  tcase_add_test(tc_other, test_round_positive);
  tcase_add_test(tc_other, test_round_positive_bank);
  tcase_add_test(tc_other, test_round_negative);
  tcase_add_test(tc_other, test_round_null);

  // Граничные тесты
  tcase_add_test(tc_boundary, test_boundary_max_int);
  tcase_add_test(tc_boundary, test_boundary_min_int);
  tcase_add_test(tc_boundary, test_boundary_to_int_max);
  tcase_add_test(tc_boundary, test_boundary_to_int_min);
  tcase_add_test(tc_boundary, test_boundary_float_max);
  tcase_add_test(tc_boundary, test_boundary_float_min);
  tcase_add_test(tc_boundary, test_boundary_float_very_small);
  tcase_add_test(tc_boundary, test_boundary_float_very_large);
  tcase_add_test(tc_boundary, test_boundary_bank_round_edge_cases);
  tcase_add_test(tc_boundary, test_boundary_compare_equal_scales);
  tcase_add_test(tc_boundary, test_boundary_compare_different_scales);
  tcase_add_test(tc_boundary, test_boundary_compare_zero_different_scales);
  tcase_add_test(tc_boundary, test_boundary_truncate_max_scale);
  tcase_add_test(tc_boundary, test_boundary_negate_zero);

  // Дополнительные тесты для увеличения покрытия
  TCase *tc_extra = tcase_create("Extra");

  tcase_add_test(tc_extra, test_is_negative);
  tcase_add_test(tc_extra, test_is_positive);
  tcase_add_test(tc_extra, test_get_bit_boundary);
  tcase_add_test(tc_extra, test_set_bit_boundary);
  tcase_add_test(tc_extra, test_shift_left_overflow);
  tcase_add_test(tc_extra, test_is_zero_negative_scale);
  tcase_add_test(tc_extra, test_floor_negative_various);
  tcase_add_test(tc_extra, test_floor_max_scale);
  tcase_add_test(tc_extra, test_floor_negative_integer);
  tcase_add_test(tc_extra, test_round_negative_bank);
  tcase_add_test(tc_extra, test_round_zero_scale);
  tcase_add_test(tc_extra, test_truncate_negative_zero_scale);
  tcase_add_test(tc_extra, test_negate_positive);
  tcase_add_test(tc_extra, test_bank_round_decimal);
  tcase_add_test(tc_extra, test_bank_round_decimal_null);
  tcase_add_test(tc_extra, test_bank_round_decimal_invalid_scale);
  tcase_add_test(tc_extra, test_bank_round_decimal_zero);

  // Тесты для арифметики
  TCase *tc_arithmetic = tcase_create("Arithmetic");

  tcase_add_test(tc_arithmetic, test_add_positive);
  tcase_add_test(tc_arithmetic, test_add_negative);
  tcase_add_test(tc_arithmetic, test_add_different_signs);
  tcase_add_test(tc_arithmetic, test_add_with_scale);
  tcase_add_test(tc_arithmetic, test_add_zero);
  tcase_add_test(tc_arithmetic, test_add_overflow);
  tcase_add_test(tc_arithmetic, test_add_null);
  tcase_add_test(tc_arithmetic, test_sub_positive);
  tcase_add_test(tc_arithmetic, test_sub_negative);
  tcase_add_test(tc_arithmetic, test_sub_different_signs);
  tcase_add_test(tc_arithmetic, test_sub_with_scale);
  tcase_add_test(tc_arithmetic, test_sub_zero);
  tcase_add_test(tc_arithmetic, test_sub_null);
  tcase_add_test(tc_arithmetic, test_mul_positive);
  tcase_add_test(tc_arithmetic, test_mul_negative);
  tcase_add_test(tc_arithmetic, test_mul_with_scale);
  tcase_add_test(tc_arithmetic, test_mul_zero);
  tcase_add_test(tc_arithmetic, test_mul_overflow);
  tcase_add_test(tc_arithmetic, test_mul_null);
  tcase_add_test(tc_arithmetic, test_div_positive);
  tcase_add_test(tc_arithmetic, test_div_negative);
  tcase_add_test(tc_arithmetic, test_div_with_scale);
  tcase_add_test(tc_arithmetic, test_div_by_zero);
  tcase_add_test(tc_arithmetic, test_div_zero);
  tcase_add_test(tc_arithmetic, test_div_null);
  tcase_add_test(tc_arithmetic, test_div_bank_rounding);
  tcase_add_test(tc_arithmetic, test_add_bank_rounding);
  tcase_add_test(tc_arithmetic, test_sub_bank_rounding);
  tcase_add_test(tc_arithmetic, test_mul_bank_rounding);
  tcase_add_test(tc_arithmetic, test_add_large_scale);
  tcase_add_test(tc_arithmetic, test_sub_large_scale);
  tcase_add_test(tc_arithmetic, test_div_result_scale);
  tcase_add_test(tc_arithmetic, test_div_debug);

  tcase_add_test(tc_arithmetic, test_div_bit_debug);

  suite_add_tcase(suite, tc_arithmetic);
  suite_add_tcase(suite, tc_extra);
  suite_add_tcase(suite, tc_helpers);
  suite_add_tcase(suite, tc_convertors);
  suite_add_tcase(suite, tc_comparison);
  suite_add_tcase(suite, tc_other);
  suite_add_tcase(suite, tc_boundary);

  TCase *tc_coverage = tcase_create("CoverageBoost");
  tcase_add_test(tc_coverage, test_set_bit_null);
  tcase_add_test(tc_coverage, test_clear_bit_null);
  tcase_add_test(tc_coverage, test_set_sign_null);
  tcase_add_test(tc_coverage, test_negate_sign_null);
  // tcase_add_test(tc_coverage, test_print_decimal_smoke);
  tcase_add_test(tc_coverage, test_shift_right_96_null);
  tcase_add_test(tc_coverage, test_to_int_overflow_high_words);
  tcase_add_test(tc_coverage, test_to_int_negative_overflow);
  tcase_add_test(tc_coverage, test_from_float_big_needs_padding);
  tcase_add_test(tc_coverage, test_from_float_near_min_boundary);
  tcase_add_test(tc_coverage, test_is_greater_or_equal_true);
  tcase_add_test(tc_coverage, test_is_less_or_equal_true);
  tcase_add_test(tc_coverage, test_is_equal_zero_different_scales);
  tcase_add_test(tc_coverage, test_is_less_zero_vs_nonzero_scaled);
  tcase_add_test(tc_coverage, test_is_equal_same_magnitude_diff_sign);
  tcase_add_test(tc_coverage, test_compare_overflow_scale_mismatch);
  tcase_add_test(tc_coverage, test_bank_round_decimal_carry);
  tcase_add_test(tc_coverage, test_bank_round_decimal_full_overflow);
  tcase_add_test(tc_coverage, test_floor_negative_carry_bits1);
  tcase_add_test(tc_coverage, test_floor_negative_carry_bits2);
  tcase_add_test(tc_coverage, test_round_positive_carry_bits1);
  tcase_add_test(tc_coverage, test_round_negative_carry_bits1);
  tcase_add_test(tc_coverage, test_add_zero_zero_same_sign);
  tcase_add_test(tc_coverage, test_add_left_zero);
  tcase_add_test(tc_coverage, test_sub_borrow_across_words);
  tcase_add_test(tc_coverage, test_add_normalize_overflow);
  tcase_add_test(tc_coverage, test_add_carry_with_scale_rounds);
  tcase_add_test(tc_coverage, test_mul_large_carry_propagation);
  tcase_add_test(tc_coverage, test_mul_scale_over_28_reduces_with_tie_rounding);
  tcase_add_test(tc_coverage, test_div_negative_initial_scale);
  tcase_add_test(tc_coverage, test_div_large_quotient_shift);
  tcase_add_test(tc_coverage, test_div_shift_to_scale_0_overflow);
  tcase_add_test(tc_coverage, test_div_exact_no_rounding);
  tcase_add_test(tc_coverage, test_div_final_round_up);
  suite_add_tcase(suite, tc_coverage);

  return suite;
}

int main(void) {
  int number_failed = 0;
  Suite *suite = s21_all_suite();
  SRunner *runner = srunner_create(suite);

  srunner_run_all(runner, CK_NORMAL);
  number_failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return (number_failed == 0) ? 0 : 1;
}