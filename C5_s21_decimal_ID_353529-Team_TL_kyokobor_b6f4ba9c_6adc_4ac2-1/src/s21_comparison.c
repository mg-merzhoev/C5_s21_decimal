#include "s21_decimal.h"

// ============================================================
// ВСПОМОГАТЕЛЬНАЯ ФУНКЦИЯ ДЛЯ СРАВНЕНИЙ
// ============================================================

/**
 * @brief Приводит два числа к общему масштабу
 * @param a Указатель на первое число
 * @param b Указатель на второе число
 * @return 0 - OK, 1 - ошибка (переполнение при умножении на 10)
 *
 * Вызывается только из s21_is_equal/s21_is_less, которые уже отсеивают
 * случай "оба нуля" и всегда передают адреса локальных копий (не NULL),
 * так что отдельные проверки на этот случай здесь не нужны.
 *
 * Алгоритм нормализации:
 * 1. Если масштабы равны - ничего не делаем
 * 2. Приводим число с меньшим масштабом к большему
 * 3. Умножение на 10 выполняется через сдвиги: (x << 3) + (x << 1)
 * 4. При переполнении возвращаем ошибку
 */
static int s21_normalize(s21_decimal *a, s21_decimal *b) {
  // Получаем масштабы чисел
  int scale_a = s21_get_scale(*a);
  int scale_b = s21_get_scale(*b);

  // Если масштабы уже равны, ничего не делаем
  if (scale_a == scale_b) {
    return 0;
  }

  // Приводим число a к масштабу числа b (увеличиваем масштаб a)
  while (scale_a < scale_b) {
    // Если a равно нулю, просто увеличиваем его масштаб
    // (ноль можно масштабировать без потери точности)
    if (s21_is_zero(*a)) {
      s21_set_scale(a, ++scale_a);
      continue;
    }

    // Умножаем a на 10: (x << 3) + (x << 1) = x*8 + x*2 = x*10
    // Это быстрее и эффективнее, чем умножение в цикле
    s21_decimal temp;
    s21_copy_decimal(&temp, *a);

    // temp = a * 8 (сдвиг влево на 3 бита)
    // Сдвиг влево на 1 = умножение на 2
    // Повторяем 3 раза = умножение на 8
    for (int i = 0; i < 3; i++) {
      if (s21_shift_left_96(&temp)) {
        return 1;  // Переполнение при нормализации
      }
    }

    // a_shifted = a * 2 (сдвиг влево на 1 бит)
    s21_decimal a_shifted;
    s21_copy_decimal(&a_shifted, *a);
    if (s21_shift_left_96(&a_shifted)) {
      return 1;  // Переполнение при нормализации
    }

    // Складываем temp + a_shifted = a*8 + a*2 = a*10
    // Используем сложение с переносом для 96-битных чисел
    unsigned int carry = 0;
    for (int i = 0; i < 3; i++) {
      unsigned long long sum =
          (unsigned long long)temp.bits[i] + a_shifted.bits[i] + carry;
      temp.bits[i] = (unsigned int)(sum & 0xFFFFFFFFu);
      carry = (unsigned int)(sum >> 32);
    }

    if (carry) {
      return 1;  // Переполнение при нормализации
    }

    // Сохраняем знак числа a (он не должен измениться)
    int sign = s21_get_sign(*a);
    s21_copy_decimal(a, temp);
    s21_set_sign(a, sign);
    s21_set_scale(a, ++scale_a);
  }

  // Приводим число b к масштабу числа a (увеличиваем масштаб b)
  while (scale_b < scale_a) {
    // Если b равно нулю, просто увеличиваем его масштаб
    if (s21_is_zero(*b)) {
      s21_set_scale(b, ++scale_b);
      continue;
    }

    // Умножаем b на 10: (x << 3) + (x << 1) = x*8 + x*2 = x*10
    s21_decimal temp;
    s21_copy_decimal(&temp, *b);

    // temp = b * 8 (сдвиг влево на 3)
    for (int i = 0; i < 3; i++) {
      if (s21_shift_left_96(&temp)) {
        return 1;  // Переполнение при нормализации
      }
    }

    // b_shifted = b * 2 (сдвиг влево на 1)
    s21_decimal b_shifted;
    s21_copy_decimal(&b_shifted, *b);
    if (s21_shift_left_96(&b_shifted)) {
      return 1;  // Переполнение при нормализации
    }

    // Складываем temp + b_shifted = b*8 + b*2 = b*10
    unsigned int carry = 0;
    for (int i = 0; i < 3; i++) {
      unsigned long long sum =
          (unsigned long long)temp.bits[i] + b_shifted.bits[i] + carry;
      temp.bits[i] = (unsigned int)(sum & 0xFFFFFFFFu);
      carry = (unsigned int)(sum >> 32);
    }

    if (carry) {
      return 1;  // Переполнение при нормализации
    }

    // Сохраняем знак числа b (он не должен измениться)
    int sign = s21_get_sign(*b);
    s21_copy_decimal(b, temp);
    s21_set_sign(b, sign);
    s21_set_scale(b, ++scale_b);
  }

  return 0;
}

/**
 * @brief Сравнивает два 96-битных числа (без учета знака и масштаба)
 * @param a Первое число
 * @param b Второе число
 * @return 1 - a > b, 0 - a == b, -1 - a < b
 */
static int s21_compare_mantissa(s21_decimal a, s21_decimal b) {
  // Сравниваем от старших битов к младшим
  if (a.bits[2] > b.bits[2]) return 1;
  if (a.bits[2] < b.bits[2]) return -1;

  if (a.bits[1] > b.bits[1]) return 1;
  if (a.bits[1] < b.bits[1]) return -1;

  if (a.bits[0] > b.bits[0]) return 1;
  if (a.bits[0] < b.bits[0]) return -1;

  return 0;  // Равны
}

// ============================================================
// ОПЕРАЦИИ СРАВНЕНИЯ
// ============================================================

/**
 * @brief Проверяет, равно ли число a числу b
 * @param a Первое число
 * @param b Второе число
 * @return 1 - TRUE (равны), 0 - FALSE (не равны)
 *
 * Алгоритм:
 * 1. Если оба нуля → TRUE (даже если знаки разные)
 * 2. Приводим к общему масштабу
 * 3. Сравниваем знаки
 * 4. Сравниваем мантиссы
 */
int s21_is_equal(s21_decimal a, s21_decimal b) {
  // Если оба нуля, они равны (даже если знаки разные)
  if (s21_is_zero(a) && s21_is_zero(b)) {
    return 1;
  }

  // Копируем для нормализации
  s21_decimal a_copy, b_copy;
  s21_copy_decimal(&a_copy, a);
  s21_copy_decimal(&b_copy, b);

  // Приводим к общему масштабу
  if (s21_normalize(&a_copy, &b_copy) != 0) {
    return 0;  // Ошибка нормализации - считаем не равными
  }

  // Сравниваем знаки
  int sign_a = s21_get_sign(a_copy);
  int sign_b = s21_get_sign(b_copy);

  if (sign_a != sign_b) {
    return 0;  // Разные знаки → не равны
  }

  // Сравниваем мантиссы
  return s21_compare_mantissa(a_copy, b_copy) == 0;
}

/**
 * @brief Проверяет, меньше ли число a числа b
 * @param a Первое число
 * @param b Второе число
 * @return 1 - TRUE (a < b), 0 - FALSE (a >= b)
 */
int s21_is_less(s21_decimal a, s21_decimal b) {
  if (s21_is_zero(a) && s21_is_zero(b)) {
    return 0;  // 0 не меньше 0
  }

  s21_decimal a_copy, b_copy;
  s21_copy_decimal(&a_copy, a);
  s21_copy_decimal(&b_copy, b);

  if (s21_normalize(&a_copy, &b_copy) != 0) {
    return 0;
  }

  int sign_a = s21_get_sign(a_copy);
  int sign_b = s21_get_sign(b_copy);

  // Если знаки разные
  if (sign_a != sign_b) {
    return sign_a > sign_b;  // Отрицательное всегда меньше положительного
  }

  // Если оба отрицательные, сравнение инвертируется
  int cmp = s21_compare_mantissa(a_copy, b_copy);

  if (sign_a == 0) {
    // Оба положительные
    return cmp < 0;
  } else {
    // Оба отрицательные
    return cmp > 0;
  }
}

/**
 * @brief Проверяет, меньше ли число a числа b или равно ему
 * @param a Первое число
 * @param b Второе число
 * @return 1 - TRUE (a <= b), 0 - FALSE (a > b)
 */
int s21_is_less_or_equal(s21_decimal a, s21_decimal b) {
  return s21_is_less(a, b) || s21_is_equal(a, b);
}

/**
 * @brief Проверяет, больше ли число a числа b
 * @param a Первое число
 * @param b Второе число
 * @return 1 - TRUE (a > b), 0 - FALSE (a <= b)
 */
int s21_is_greater(s21_decimal a, s21_decimal b) {
  return !s21_is_less_or_equal(a, b);
}

/**
 * @brief Проверяет, больше ли число a числа b или равно ему
 * @param a Первое число
 * @param b Второе число
 * @return 1 - TRUE (a >= b), 0 - FALSE (a < b)
 */
int s21_is_greater_or_equal(s21_decimal a, s21_decimal b) {
  return !s21_is_less(a, b);
}

/**
 * @brief Проверяет, не равно ли число a числу b
 * @param a Первое число
 * @param b Второе число
 * @return 1 - TRUE (не равны), 0 - FALSE (равны)
 */
int s21_is_not_equal(s21_decimal a, s21_decimal b) {
  return !s21_is_equal(a, b);
}