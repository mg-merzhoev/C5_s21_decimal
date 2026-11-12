#include "s21_decimal.h"

// ============================================================
// ВСПОМОГАТЕЛЬНАЯ ФУНКЦИЯ: БАНКОВСКОЕ ОКРУГЛЕНИЕ
// ============================================================

/**
 * @brief Выполняет банковское округление числа
 * @param value Число для округления (целая часть)
 * @param digit Цифра, которая определяет округление (0-9)
 * @return Округленное число
 *
 * Банковское округление (округление к ближайшему четному):
 * - Если digit < 5: округление вниз
 * - Если digit > 5: округление вверх
 * - Если digit == 5: округление к четному
 *
 * Примеры:
 * - 2.5 → 2 (четное)
 * - 3.5 → 4 (нечетное)
 * - 2.4 → 2
 * - 2.6 → 3
 */
unsigned long long s21_bank_round_ull(unsigned long long value, int digit) {
  if (digit < 5) {
    // Округление вниз - ничего не делаем
    return value;
  } else if (digit > 5) {
    // Округление вверх
    return value + 1;
  } else {
    // digit == 5 - округление к четному
    if (value % 2 == 0) {
      return value;  // Четное - оставляем
    } else {
      return value + 1;  // Нечетное - увеличиваем
    }
  }
}

/**
 * @brief Выполняет банковское округление для decimal
 * @param value Указатель на decimal для округления
 * @param scale Текущий масштаб
 * @param digit Цифра для округления
 * @return 0 - OK, 1 - ошибка
 */
int s21_bank_round_decimal(s21_decimal *value, int scale, int digit) {
  if (value == NULL || scale < 0 || scale > 28) {
    return 1;
  }

  // Для чисел < 1 нужно обработать особый случай
  if (s21_is_zero(*value)) {
    return 0;
  }

  // Применяем банковское округление к младшей части
  // Округляем bits[0] (младшие 32 бита)
  unsigned long long rounded = s21_bank_round_ull(value->bits[0], digit);
  value->bits[0] = (unsigned int)rounded;

  // Если было переполнение при округлении (было 999... и стало 1000...)
  if (rounded > 0xFFFFFFFFu) {
    // Перенос в следующий разряд
    value->bits[0] = 0;
    unsigned long long carry = 1;
    for (int i = 1; i < 3; i++) {
      unsigned long long sum = (unsigned long long)value->bits[i] + carry;
      value->bits[i] = (unsigned int)sum;
      carry = sum >> 32;
      if (carry == 0) break;
    }
    if (carry > 0) {
      return 1;  // Переполнение при округлении
    }
  }

  return 0;
}

// ============================================================
// ФУНКЦИЯ: s21_floor
// ============================================================

/**
 * @brief Округляет Decimal до ближайшего целого в сторону -∞
 * @param value Исходное десятичное число
 * @param result Указатель на структуру для результата
 * @return 0 - OK, 1 - ошибка вычисления
 *
 * Алгоритм:
 * 1. Копируем исходное число
 * 2. Отбрасываем дробную часть (truncate)
 * 3. Если число отрицательное и была дробная часть, уменьшаем на 1
 */
int s21_floor(s21_decimal value, s21_decimal *result) {
  if (result == NULL) {
    return 1;
  }

  int sign = s21_get_sign(value);
  // int scale = s21_get_scale(value);

  // Проверяем, есть ли дробная часть
  s21_decimal truncated;
  s21_truncate(value, &truncated);
  int has_fraction = !s21_is_equal(value, truncated);

  // Отбрасываем дробную часть
  s21_truncate(value, result);

  // Если число отрицательное и есть дробная часть
  if (sign && has_fraction) {
    // Увеличиваем модуль на 1 (двигаемся в сторону -∞)
    // Например: -123.45 → -124 (123 → 124)
    if (result->bits[0] < 0xFFFFFFFFu) {
      result->bits[0]++;
    } else if (result->bits[1] < 0xFFFFFFFFu) {
      result->bits[1]++;
      result->bits[0] = 0;
    } else if (result->bits[2] < 0xFFFFFFFFu) {
      result->bits[2]++;
      result->bits[1] = 0;
      result->bits[0] = 0;
    } else {
      return 1;  // Переполнение
    }
  }

  return 0;
}
// ============================================================
// ФУНКЦИЯ: s21_round
// ============================================================

/**
 * @brief Округляет Decimal до ближайшего целого числа
 * @param value Исходное десятичное число
 * @param result Указатель на структуру для результата
 * @return 0 - OK, 1 - ошибка вычисления
 *
 * Алгоритм:
 * 1. Копируем исходное число
 * 2. Если масштаб > 0, смотрим на первую дробную цифру
 * 3. Применяем банковское округление
 */
int s21_round(s21_decimal value, s21_decimal *result) {
  if (result == NULL) {
    return 1;
  }

  int scale = s21_get_scale(value);
  int sign = s21_get_sign(value);

  if (scale == 0) {
    s21_copy_decimal(result, value);
    return 0;
  }

  // Получаем первую дробную цифру
  s21_decimal temp;
  s21_copy_decimal(&temp, value);

  // Отбрасываем все кроме первой дробной цифры
  for (int i = 0; i < scale - 1; i++) {
    unsigned int remainder = 0;
    for (int j = 2; j >= 0; j--) {
      unsigned long long current =
          ((unsigned long long)remainder << 32) | temp.bits[j];
      temp.bits[j] = (unsigned int)(current / 10u);
      remainder = (unsigned int)(current % 10u);
    }
  }

  // Получаем последнюю цифру (первая дробная)
  unsigned int remainder = 0;
  for (int j = 2; j >= 0; j--) {
    unsigned long long current =
        ((unsigned long long)remainder << 32) | temp.bits[j];
    temp.bits[j] = (unsigned int)(current / 10u);
    remainder = (unsigned int)(current % 10u);
  }

  int digit = (int)remainder;

  // Проверяем, есть ли дробная часть
  s21_decimal truncated;
  s21_truncate(value, &truncated);
  int has_fraction = !s21_is_equal(value, truncated);

  // Отбрасываем дробную часть
  s21_truncate(value, result);

  // Применяем банковское округление
  if (sign) {
    // Для отрицательных чисел
    if (has_fraction &&
        (digit > 5 || (digit == 5 && result->bits[0] % 2 == 1))) {
      // Увеличиваем модуль (двигаемся в сторону -∞)
      // Например: -123.56 → -124 (123 → 124)
      if (result->bits[0] < 0xFFFFFFFFu) {
        result->bits[0]++;
      } else if (result->bits[1] < 0xFFFFFFFFu) {
        result->bits[1]++;
        result->bits[0] = 0;
      } else if (result->bits[2] < 0xFFFFFFFFu) {
        result->bits[2]++;
        result->bits[1] = 0;
        result->bits[0] = 0;
      } else {
        return 1;  // Переполнение
      }
    }
  } else {
    // Для положительных чисел
    if (digit > 5 || (digit == 5 && result->bits[0] % 2 == 1)) {
      if (result->bits[0] < 0xFFFFFFFFu) {
        result->bits[0]++;
      } else if (result->bits[1] < 0xFFFFFFFFu) {
        result->bits[1]++;
        result->bits[0] = 0;
      } else if (result->bits[2] < 0xFFFFFFFFu) {
        result->bits[2]++;
        result->bits[1] = 0;
        result->bits[0] = 0;
      } else {
        return 1;  // Переполнение
      }
    }
  }

  return 0;
}

// ============================================================
// ФУНКЦИЯ: s21_truncate
// ============================================================

/**
 * @brief Отбрасывает дробную часть Decimal числа
 * @param value Исходное десятичное число
 * @param result Указатель на структуру для результата
 * @return 0 - OK, 1 - ошибка вычисления
 *
 * Алгоритм:
 * 1. Копируем исходное число
 * 2. Делим мантиссу на 10^scale (целочисленное деление)
 * 3. Устанавливаем масштаб = 0
 * 4. Сохраняем знак
 */
int s21_truncate(s21_decimal value, s21_decimal *result) {
  if (result == NULL) {
    return 1;
  }

  // Копируем исходное число
  s21_copy_decimal(result, value);

  // Получаем масштаб
  int scale = s21_get_scale(value);

  // Если масштаб равен 0, ничего не делаем
  if (scale == 0) {
    return 0;
  }

  // Делим мантиссу на 10^scale (целочисленное деление)
  for (int i = 0; i < scale; i++) {
    unsigned int remainder = 0;
    for (int j = 2; j >= 0; j--) {
      unsigned long long current =
          ((unsigned long long)remainder << 32) | result->bits[j];
      result->bits[j] = (unsigned int)(current / 10u);
      remainder = (unsigned int)(current % 10u);
    }
  }

  // Устанавливаем масштаб в 0
  s21_set_scale(result, 0);

  // Знак сохраняется
  return 0;
}

// ============================================================
// ФУНКЦИЯ: s21_negate
// ============================================================

/**
 * @brief Меняет знак числа на противоположный (умножает на -1)
 * @param value Исходное десятичное число
 * @param result Указатель на структуру для результата
 * @return 0 - OK, 1 - ошибка вычисления
 */
int s21_negate(s21_decimal value, s21_decimal *result) {
  if (result == NULL) {
    return 1;
  }

  s21_copy_decimal(result, value);

  // Инвертируем знак (XOR с битом 31)
  result->bits[3] ^= (1u << 31);

  return 0;
}