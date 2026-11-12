#ifndef S21_DECIMAL_H
#define S21_DECIMAL_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Определение структуры Decimal
// bits[0] - младшие 32 бита 96-битного целого числа
// bits[1] - средние 32 бита 96-битного целого числа
// bits[2] - старшие 32 бита 96-битного целого числа
// bits[3] - знак (бит 31) и масштаб (биты 16-23)
typedef struct {
  unsigned int bits[4];
} s21_decimal;

// ============ ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ============

// Инициализация и копирование
void s21_zero_decimal(s21_decimal *dec);
void s21_copy_decimal(s21_decimal *dest, s21_decimal src);

// Работа с битами
int s21_get_bit(s21_decimal dec, int position);
void s21_set_bit(s21_decimal *dec, int position);
void s21_clear_bit(s21_decimal *dec, int position);

// Работа со знаком (бит 31 в bits[3])
int s21_get_sign(s21_decimal dec);
void s21_set_sign(s21_decimal *dec, int sign);
void s21_negate_sign(s21_decimal *dec);

// Работа с масштабом (биты 16-23 в bits[3])
int s21_get_scale(s21_decimal dec);
void s21_set_scale(s21_decimal *dec, int scale);

// Проверки
int s21_is_zero(s21_decimal dec);
int s21_is_negative(s21_decimal dec);
int s21_is_positive(s21_decimal dec);

// Сдвиги 96-битного числа
int s21_shift_left_96(s21_decimal *dec);
void s21_shift_right_96(s21_decimal *dec);

// Преобразование в строку (для отладки)
void s21_print_decimal(s21_decimal dec);

// ============ БАНКОВСКОЕ ОКРУГЛЕНИЕ ============
unsigned long long s21_bank_round_ull(unsigned long long value, int digit);
int s21_bank_round_decimal(s21_decimal *value, int scale, int digit);

// ============ КОНВЕРТЕРЫ ============
int s21_from_int_to_decimal(int src, s21_decimal *dst);
int s21_from_decimal_to_int(s21_decimal src, int *dst);
int s21_from_float_to_decimal(float src, s21_decimal *dst);
int s21_from_decimal_to_float(s21_decimal src, float *dst);

// ============ ОПЕРАЦИИ СРАВНЕНИЯ ============
int s21_is_less(s21_decimal a, s21_decimal b);
int s21_is_less_or_equal(s21_decimal a, s21_decimal b);
int s21_is_greater(s21_decimal a, s21_decimal b);
int s21_is_greater_or_equal(s21_decimal a, s21_decimal b);
int s21_is_equal(s21_decimal a, s21_decimal b);
int s21_is_not_equal(s21_decimal a, s21_decimal b);

// ============ ДРУГИЕ ФУНКЦИИ ============
int s21_floor(s21_decimal value, s21_decimal *result);
int s21_round(s21_decimal value, s21_decimal *result);
int s21_truncate(s21_decimal value, s21_decimal *result);
int s21_negate(s21_decimal value, s21_decimal *result);

// ============ АРИФМЕТИКА ============
int s21_add(s21_decimal value_1, s21_decimal value_2, s21_decimal *result);
int s21_sub(s21_decimal value_1, s21_decimal value_2, s21_decimal *result);
int s21_mul(s21_decimal value_1, s21_decimal value_2, s21_decimal *result);
int s21_div(s21_decimal value_1, s21_decimal value_2, s21_decimal *result);

#endif