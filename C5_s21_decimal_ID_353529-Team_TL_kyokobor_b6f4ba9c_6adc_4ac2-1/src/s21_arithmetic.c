#include "s21_decimal.h"

/*
 * ВАЖНОЕ ЗАМЕЧАНИЕ О s21_copy_decimal:
 *   void s21_copy_decimal(s21_decimal *dest, s21_decimal src);
 * Первый аргумент - КУДА копируем (указатель), второй - ОТКУДА (значение).
 * Правило вызова: s21_copy_decimal(&приемник, источник);
 */

// ============================================================
// СРАВНЕНИЕ И БАЗОВАЯ АРИФМЕТИКА МАНТИСС (96 бит)
// ============================================================

static int s21_mantissa_cmp(s21_decimal a, s21_decimal b) {
  if (a.bits[2] != b.bits[2]) return (a.bits[2] > b.bits[2]) ? 1 : -1;
  if (a.bits[1] != b.bits[1]) return (a.bits[1] > b.bits[1]) ? 1 : -1;
  if (a.bits[0] != b.bits[0]) return (a.bits[0] > b.bits[0]) ? 1 : -1;
  return 0;
}

static void s21_mantissa_add(s21_decimal a, s21_decimal b, s21_decimal *res,
                             int *carry) {
  uint64_t tmp = (uint64_t)a.bits[0] + b.bits[0];
  res->bits[0] = (uint32_t)(tmp & 0xFFFFFFFFu);
  uint64_t carry64 = tmp >> 32;

  tmp = (uint64_t)a.bits[1] + b.bits[1] + carry64;
  res->bits[1] = (uint32_t)(tmp & 0xFFFFFFFFu);
  carry64 = tmp >> 32;

  tmp = (uint64_t)a.bits[2] + b.bits[2] + carry64;
  res->bits[2] = (uint32_t)(tmp & 0xFFFFFFFFu);
  *carry = (int)(tmp >> 32);
}

static void s21_mantissa_sub(s21_decimal a, s21_decimal b, s21_decimal *res) {
  uint64_t borrow = 0;
  for (int i = 0; i < 3; i++) {
    int64_t tmp = (int64_t)a.bits[i] - (int64_t)b.bits[i] - (int64_t)borrow;
    if (tmp < 0) {
      tmp += 0x100000000LL;
      borrow = 1;
    } else {
      borrow = 0;
    }
    res->bits[i] = (uint32_t)tmp;
  }
}

// Умножает 96-битную мантиссу на 10. Возврат: 1 - переполнение, 0 - OK.
static int s21_mantissa_mul_10(s21_decimal *d) {
  uint64_t carry = 0;
  for (int i = 0; i < 3; i++) {
    uint64_t tmp = (uint64_t)d->bits[i] * 10u + carry;
    d->bits[i] = (uint32_t)(tmp & 0xFFFFFFFFu);
    carry = tmp >> 32;
  }
  return (carry != 0) ? 1 : 0;
}

// Делит {carry_in, d} (97-битное число, где carry_in - бит номер 96) на 10.
// Возвращает остаток (0-9). Обычное деление на 10 - вызов с carry_in = 0.
static int s21_mantissa_div_10_ext(s21_decimal *d, int carry_in) {
  uint64_t rem = (uint64_t)(carry_in & 1);
  for (int i = 2; i >= 0; i--) {
    uint64_t cur = (rem << 32) | d->bits[i];
    d->bits[i] = (uint32_t)(cur / 10u);
    rem = cur % 10u;
  }
  return (int)rem;
}

static int s21_mantissa_div_10(s21_decimal *d) {
  return s21_mantissa_div_10_ext(d, 0);
}

// Последняя десятичная цифра ВСЕЙ 96-битной мантиссы (не только bits[0]!).
static int s21_get_last_digit(s21_decimal d) {
  return s21_mantissa_div_10_ext(&d, 0);
}

// ============================================================
// БАНКОВСКОЕ ОКРУГЛЕНИЕ
// ============================================================

/**
 * @brief Решает, нужно ли округлить мантиссу вверх (round half to even)
 * @param mant Мантисса ПОСЛЕ отбрасывания лишних цифр (для определения
 * чётности)
 * @param digit Первая отброшенная цифра (0-9)
 * @param has_nonzero_tail Есть ли ненулевые цифры дальше отброшенной digit
 * @return 1 - нужно прибавить 1, 0 - не нужно
 */
static int s21_bank_rounding(s21_decimal mant, int digit,
                             int has_nonzero_tail) {
  if (digit < 5) return 0;
  if (digit > 5 || has_nonzero_tail) return 1;
  return (s21_get_last_digit(mant) % 2) != 0;  // digit == 5 ровно: к чётному
}

/**
 * @brief Прибавляет 1 к мантиссе (последний шаг банковского округления)
 * @return 1 - переполнение (мантисса уже была максимальной), 0 - OK
 */
static int s21_apply_rounding(s21_decimal *mant) {
  s21_decimal one = {{1, 0, 0, 0}};
  s21_decimal res = {0};
  int carry = 0;
  s21_mantissa_add(*mant, one, &res, &carry);
  s21_copy_decimal(mant, res);
  return carry;
}

// Убирает конечные нули мантиссы, уменьшая scale (не выполняется для truncate).
static void s21_remove_trailing_zeros(s21_decimal *d, int *scale) {
  while (*scale > 0) {
    s21_decimal tmp;
    s21_copy_decimal(&tmp, *d);
    if (s21_mantissa_div_10(&tmp) != 0) break;
    s21_copy_decimal(d, tmp);
    (*scale)--;
  }
}

// ============================================================
// ПРИВЕДЕНИЕ МАСШТАБА (НОРМАЛИЗАЦИЯ) ДЛЯ СЛОЖЕНИЯ/СРАВНЕНИЯ
// ============================================================

// Поднимает scale числа a до target_scale, умножая мантиссу на 10.
// Возвращает 1, если дальше поднимать нельзя (переполнение).
static int s21_scale_up_to(s21_decimal *a, int *scale, int target_scale) {
  while (*scale < target_scale) {
    s21_decimal tmp;
    s21_copy_decimal(&tmp, *a);
    if (s21_mantissa_mul_10(&tmp) != 0) return 1;
    s21_copy_decimal(a, tmp);
    (*scale)++;
  }
  return 0;
}

// Приводит a и b к общему масштабу (максимальному из двух). Сложение и
// вычитание в decimal всегда работают на масштабе max(scale1, scale2) -
// точность никогда не теряется на этом этапе, поэтому опускать масштаб
// вниз здесь не требуется (в отличие от деления/умножения).
// Возвращает итоговый общий scale, либо -1 при переполнении.
static int s21_normalize_scale(s21_decimal *a, s21_decimal *b, int scale1,
                               int scale2) {
  int target = (scale1 > scale2) ? scale1 : scale2;

  if (s21_scale_up_to(a, &scale1, target)) return -1;
  if (s21_scale_up_to(b, &scale2, target)) return -1;

  return scale1;
}

// ============================================================
// s21_add / s21_sub
// ============================================================

static int s21_add_same_sign(s21_decimal a, s21_decimal b, int result_sign,
                             s21_decimal *res_mant, int *scale) {
  int carry = 0;
  s21_mantissa_add(a, b, res_mant, &carry);

  if (carry) {
    if (*scale == 0) return (result_sign == 1) ? 2 : 1;
    int remainder = s21_mantissa_div_10_ext(res_mant, carry);
    (*scale)--;
    if (s21_bank_rounding(*res_mant, remainder, 0)) {
      if (s21_apply_rounding(res_mant)) return (result_sign == 1) ? 2 : 1;
    }
  }
  return 0;
}

int s21_add(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  if (result == NULL) return 1;
  s21_zero_decimal(result);

  if (s21_is_zero(value_1) && s21_is_zero(value_2)) {
    s21_set_sign(result, s21_get_sign(value_1) && s21_get_sign(value_2));
    return 0;
  }
  if (s21_is_zero(value_1)) {
    s21_copy_decimal(result, value_2);
    return 0;
  }
  if (s21_is_zero(value_2)) {
    s21_copy_decimal(result, value_1);
    return 0;
  }

  int sign1 = s21_get_sign(value_1);
  int sign2 = s21_get_sign(value_2);
  int scale1 = s21_get_scale(value_1);
  int scale2 = s21_get_scale(value_2);

  s21_decimal a, b;
  s21_copy_decimal(&a, value_1);
  s21_copy_decimal(&b, value_2);
  int scale = s21_normalize_scale(&a, &b, scale1, scale2);
  if (scale < 0) return (sign1 && sign2) ? 2 : 1;

  s21_decimal res_mant;
  s21_zero_decimal(&res_mant);
  int result_sign = 0;

  if (sign1 == sign2) {
    result_sign = sign1;
    int status = s21_add_same_sign(a, b, result_sign, &res_mant, &scale);
    if (status) return status;
  } else {
    int cmp = s21_mantissa_cmp(a, b);
    if (cmp == 0) return 0;  // результат 0, result уже обнулён
    if (cmp > 0) {
      s21_mantissa_sub(a, b, &res_mant);
      result_sign = sign1;
    } else {
      s21_mantissa_sub(b, a, &res_mant);
      result_sign = sign2;
    }
  }

  s21_remove_trailing_zeros(&res_mant, &scale);
  s21_copy_decimal(result, res_mant);
  s21_set_scale(result, scale);
  if (result_sign) s21_set_sign(result, 1);

  return 0;
}

int s21_sub(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  if (result == NULL) return 1;
  s21_decimal inverted_v2;
  s21_copy_decimal(&inverted_v2, value_2);
  s21_negate_sign(&inverted_v2);
  return s21_add(value_1, inverted_v2, result);
}

// ============================================================
// s21_mul - УМНОЖЕНИЕ (192-битный промежуточный результат)
// ============================================================

static void s21_mantissa_mul_96x96(s21_decimal a, s21_decimal b,
                                   uint32_t res[6]) {
  for (int i = 0; i < 6; i++) res[i] = 0;
  for (int i = 0; i < 3; i++) {
    uint64_t carry = 0;
    for (int j = 0; j < 3; j++) {
      uint64_t tmp = (uint64_t)a.bits[i] * b.bits[j] + res[i + j] + carry;
      res[i + j] = (uint32_t)(tmp & 0xFFFFFFFFu);
      carry = tmp >> 32;
    }
    int idx = i + 3;
    while (carry && idx < 6) {
      uint64_t tmp = (uint64_t)res[idx] + carry;
      res[idx] = (uint32_t)(tmp & 0xFFFFFFFFu);
      carry = tmp >> 32;
      idx++;
    }
  }
}

static int s21_mantissa_192_div_10(uint32_t d[6]) {
  uint64_t rem = 0;
  for (int i = 5; i >= 0; i--) {
    uint64_t cur = (rem << 32) | d[i];
    d[i] = (uint32_t)(cur / 10u);
    rem = cur % 10u;
  }
  return (int)rem;
}

static int s21_fits_96(const uint32_t product[6]) {
  return (product[3] == 0 && product[4] == 0 && product[5] == 0);
}

static void s21_192_to_decimal(const uint32_t product[6], s21_decimal *d) {
  d->bits[0] = product[0];
  d->bits[1] = product[1];
  d->bits[2] = product[2];
}

int s21_mul(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  if (result == NULL) return 1;
  s21_zero_decimal(result);

  if (s21_is_zero(value_1) || s21_is_zero(value_2)) return 0;

  int sign1 = s21_get_sign(value_1);
  int sign2 = s21_get_sign(value_2);
  int result_sign = sign1 ^ sign2;
  int scale = s21_get_scale(value_1) + s21_get_scale(value_2);

  uint32_t product[6];
  s21_mantissa_mul_96x96(value_1, value_2, product);

  int remainder = 0;
  int has_tail = 0;
  while (!s21_fits_96(product) || scale > 28) {
    if (scale == 0) return (result_sign == 1) ? 2 : 1;
    if (remainder > 0) has_tail = 1;
    remainder = s21_mantissa_192_div_10(product);
    scale--;
  }

  s21_decimal final_mant = {0};
  s21_192_to_decimal(product, &final_mant);

  if (remainder > 0 || has_tail) {
    if (s21_bank_rounding(final_mant, remainder, has_tail)) {
      if (s21_apply_rounding(&final_mant)) {
        return (result_sign == 1) ? 2 : 1;
      }
    }
  }

  s21_decimal max_dec = {{0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0}};
  if (s21_mantissa_cmp(final_mant, max_dec) > 0) {
    return (result_sign == 1) ? 2 : 1;
  }

  s21_remove_trailing_zeros(&final_mant, &scale);
  s21_copy_decimal(result, final_mant);
  s21_set_scale(result, scale);
  if (result_sign) s21_set_sign(result, 1);

  return 0;
}

// ============================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ ДЕЛЕНИЯ
// ============================================================

static int s21_mantissa_shift_left(s21_decimal *d) {
  uint32_t carry = 0;
  for (int i = 0; i < 3; i++) {
    uint32_t next_carry = (d->bits[i] >> 31) & 1u;
    d->bits[i] = (d->bits[i] << 1) | carry;
    carry = next_carry;
  }
  return (int)carry;
}

static int s21_find_max_shift(s21_decimal a, s21_decimal b) {
  int shift = 0;
  s21_decimal temp_b;
  s21_copy_decimal(&temp_b, b);
  while (s21_mantissa_cmp(temp_b, a) <= 0) {
    if (s21_mantissa_shift_left(&temp_b)) {
      shift++;
      break;
    }
    shift++;
  }
  return shift;
}

static void s21_set_quotient_bit(s21_decimal *quotient, int shift) {
  if (shift < 32) {
    quotient->bits[0] |= (1u << shift);
  } else if (shift < 64) {
    quotient->bits[1] |= (1u << (shift - 32));
  } else if (shift < 96) {
    quotient->bits[2] |= (1u << (shift - 64));
  }
}

// Беззнаковое деление двух 96-битных мантисс "в столбик".
static void s21_divide_96x96(s21_decimal a, s21_decimal b,
                             s21_decimal *quotient, s21_decimal *remainder) {
  s21_zero_decimal(quotient);
  s21_zero_decimal(remainder);

  if (s21_mantissa_cmp(a, b) < 0) {
    s21_copy_decimal(remainder, a);
    return;
  }

  int shift = s21_find_max_shift(a, b);
  if (shift > 0) shift--;

  s21_decimal divisor;
  s21_copy_decimal(&divisor, b);
  for (int i = 0; i < shift; i++) {
    s21_mantissa_shift_left(&divisor);
  }

  s21_decimal current_remainder;
  s21_copy_decimal(&current_remainder, a);

  for (int i = shift; i >= 0; i--) {
    if (s21_mantissa_cmp(current_remainder, divisor) >= 0) {
      s21_mantissa_sub(current_remainder, divisor, &current_remainder);
      s21_set_quotient_bit(quotient, i);
    }
    uint32_t borrow = 0;
    for (int j = 2; j >= 0; j--) {
      uint32_t next_borrow = (divisor.bits[j] & 1u) << 31;
      divisor.bits[j] = (divisor.bits[j] >> 1) | borrow;
      borrow = next_borrow;
    }
  }
  s21_copy_decimal(remainder, current_remainder);
}

// Один шаг "приписывания" следующей десятичной цифры к частному:
// quotient = quotient*10 + digit(remainder*10 / b); remainder = остаток.
// Возврат: 1 - переполнение мантиссы (нельзя добавить цифру), 0 - OK.
static int s21_div_next_digit(s21_decimal *quotient, s21_decimal *remainder,
                              s21_decimal b, int *digit_out) {
  s21_decimal new_q, new_r;
  s21_copy_decimal(&new_q, *quotient);
  s21_copy_decimal(&new_r, *remainder);

  int q_overflow = s21_mantissa_mul_10(&new_q);
  int r_overflow = s21_mantissa_mul_10(&new_r);
  if (q_overflow || r_overflow) return 1;

  s21_decimal step_q, step_r;
  s21_divide_96x96(new_r, b, &step_q, &step_r);

  int carry = 0;
  s21_decimal sum = {0};
  s21_mantissa_add(new_q, step_q, &sum, &carry);
  if (carry) return 1;

  s21_copy_decimal(quotient, sum);
  s21_copy_decimal(remainder, step_r);
  if (digit_out) *digit_out = (int)step_q.bits[0];
  return 0;
}

// Смотрит на следующую цифру частного, не приписывая её (для округления).
static void s21_div_peek_digit(s21_decimal remainder, s21_decimal b,
                               int *digit_out, int *has_tail_out) {
  s21_decimal peek_r;
  s21_copy_decimal(&peek_r, remainder);
  s21_mantissa_mul_10(&peek_r);  // remainder < b, переполнение здесь невозможно

  s21_decimal step_q, step_r;
  s21_divide_96x96(peek_r, b, &step_q, &step_r);
  *digit_out = (int)step_q.bits[0];
  *has_tail_out = !s21_is_zero(step_r);
}

// ============================================================
// s21_div - ДЕЛЕНИЕ
// ============================================================

static int s21_div_shift_to_scale_0(s21_decimal *quotient,
                                    s21_decimal *remainder, s21_decimal b,
                                    int *scale) {
  while (*scale < 0) {
    if (s21_div_next_digit(quotient, remainder, b, NULL)) return 1;
    (*scale)++;
  }
  return 0;
}

// Наращивает дробную часть частного, пока есть остаток и scale < 28.
// Если 96-битная мантисса заполнилась раньше, чем scale достиг 28, это НЕ
// ошибка переполнения - просто дальше некуда добавлять точные цифры, и
// округление довершит результат по остатку (s21_div_final_round).
static void s21_div_extend_fraction(s21_decimal *quotient,
                                    s21_decimal *remainder, s21_decimal b,
                                    int *scale) {
  while (!s21_is_zero(*remainder) && *scale < 28) {
    if (s21_div_next_digit(quotient, remainder, b, NULL)) break;
    (*scale)++;
  }
}

static int s21_div_final_round(s21_decimal *quotient, s21_decimal remainder,
                               s21_decimal b) {
  if (s21_is_zero(remainder)) return 0;
  int digit = 0, has_tail = 0;
  s21_div_peek_digit(remainder, b, &digit, &has_tail);
  if (s21_bank_rounding(*quotient, digit, has_tail)) {
    return s21_apply_rounding(quotient);
  }
  return 0;
}

int s21_div(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  if (result == NULL) return 1;
  s21_zero_decimal(result);

  if (s21_is_zero(value_2)) return 3;
  if (s21_is_zero(value_1)) return 0;

  int result_sign = s21_get_sign(value_1) ^ s21_get_sign(value_2);
  int scale = s21_get_scale(value_1) - s21_get_scale(value_2);

  s21_decimal a, b;
  s21_copy_decimal(&a, value_1);
  s21_copy_decimal(&b, value_2);
  s21_set_sign(&a, 0);
  s21_set_sign(&b, 0);

  s21_decimal quotient, remainder;
  s21_divide_96x96(a, b, &quotient, &remainder);

  if (s21_div_shift_to_scale_0(&quotient, &remainder, b, &scale)) {
    return (result_sign == 1) ? 2 : 1;
  }
  s21_div_extend_fraction(&quotient, &remainder, b, &scale);
  if (s21_div_final_round(&quotient, remainder, b)) {
    return (result_sign == 1) ? 2 : 1;
  }

  s21_remove_trailing_zeros(&quotient, &scale);
  s21_copy_decimal(result, quotient);
  s21_set_scale(result, scale);
  if (result_sign) s21_set_sign(result, 1);

  return 0;
}