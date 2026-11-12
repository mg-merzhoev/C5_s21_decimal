#include "s21_decimal.h"

// ============================================================
// КОНВЕРТЕР ИЗ INT В DECIMAL
// ============================================================

/**
 * @brief Конвертирует целое число в десятичное
 * @param src Исходное целое число (int)
 * @param dst Указатель на структуру decimal для результата
 * @return 0 - OK, 1 - ошибка конвертации
 *
 * Алгоритм:
 * 1. Очищаем структуру-результат
 * 2. Проверяем знак числа
 * 3. Записываем абсолютное значение в bits[0]
 * 4. Устанавливаем знак и масштаб (масштаб = 0)
 */
int s21_from_int_to_decimal(int src, s21_decimal *dst) {
  // Проверка на NULL указатель
  if (dst == NULL) {
    return 1;  // Ошибка конвертации
  }

  // 1. Очищаем результат
  s21_zero_decimal(dst);

  // 2. Проверяем знак
  if (src < 0) {
    // Отрицательное число
    s21_set_sign(dst, 1);

    // Проверяем на минимальное значение int (-2147483648)
    // Для него нельзя просто сделать src = -src (переполнение, min_int =
    // -2147483648, max_int = +2147483647)
    if (src == -2147483648) {
      dst->bits[0] = 2147483648u;  // dst->bits[0] имеет тип unsigned int и туда
                                   // спокойно помещается число <= 4294967295
    } else {
      dst->bits[0] = (unsigned int)(-src);
    }
  } else {
    // Положительное или ноль
    dst->bits[0] =
        (unsigned int)src;  // здесь мы не проверяем src на переполнение т.к.
                            // dst->bits[0] имеет тип unsigned int
  }

  // 3. Масштаб уже равен 0 (после s21_zero_decimal)
  // 4. Знак уже установлен

  return 0;  // OK
}

// ============================================================
// КОНВЕРТЕР ИЗ DECIMAL В INT
// ============================================================

/**
 * @brief Конвертирует десятичное число в целое
 * @param src Исходное десятичное число
 * @param dst Указатель на целое число для результата
 * @return 0 - OK, 1 - ошибка конвертации
 *
 * Алгоритм:
 * 1. Проверяем на NULL
 * 2. Проверяем, что число помещается в int
 * 3. Отбрасываем дробную часть (truncate)
 * 4. Применяем знак
 * 5. Проверяем диапазон
 */
int s21_from_decimal_to_int(s21_decimal src, int *dst) {
  // Проверка на NULL указатель
  if (dst == NULL) {
    return 1;  // Ошибка конвертации
  }

  // Если число равно нулю
  if (s21_is_zero(src)) {
    *dst = 0;
    return 0;
  }

  // Создаем копию для работы
  s21_decimal temp;
  s21_copy_decimal(&temp, src);

  // Получаем масштаб
  int scale = s21_get_scale(temp);

  // Отбрасываем дробную часть из мантиссы (делим на 10^scale), т.к конвертируем
  // в целое число
  /*
      Деление в столбик (слева направо)
      Начинаем со старших разрядов
      Переносим остаток к младшим

      Алгоритм деления в столбик требует обхода СЛЕВА НАПРАВО (от старших к
     младшим) Остаток от деления старших разрядов передается к младшим Именно
     поэтому мы начинаем с bits[2] (самого старшего) и идем к bits[0]
  */
  for (int i = 0; i < scale; i++) {
    // Делим 96-битное число на 10
    unsigned int remainder = 0;
    for (int j = 2; j >= 0; j--) {
      unsigned long long current =
          ((unsigned long long)remainder << 32) | temp.bits[j];
      temp.bits[j] = (unsigned int)(current / 10u);
      remainder = (unsigned int)(current % 10u);
    }
  }

  // Устанавливаем масштаб в 0
  s21_set_scale(&temp, 0);

  // Проверяем, что число помещается в int (биты 31-95 должны быть нулевыми)
  if (temp.bits[1] != 0u || temp.bits[2] != 0u) {
    return 1;  // Слишком большое число
  }

  // Получаем знак
  int sign = s21_get_sign(src);

  // Проверяем диапазон
  if (sign) {
    // Отрицательное число
    if (temp.bits[0] > 2147483648u) {
      return 1;  // Слишком маленькое число (< -2147483648)
    }
    *dst = -(int)temp.bits[0];
  } else {
    // Положительное число
    if (temp.bits[0] > 2147483647u) {
      return 1;  // Слишком большое число (> 2147483647), так как конвертировать
                 // нужно в int, а не в unsigned int
    }
    *dst = (int)temp.bits[0];
  }

  return 0;  // OK
}

// ============================================================
// КОНВЕРТЕР ИЗ FLOAT В DECIMAL
// ============================================================

/**
 * @brief Конвертирует число с плавающей точкой в десятичное
 * @param src Исходное число (float)
 * @param dst Указатель на структуру decimal для результата
 * @return 0 - OK, 1 - ошибка конвертации
 *
 * Алгоритм:
 * 1. Проверка на особые случаи (NaN, Infinity, ноль)
 * 2. Проверка на диапазон (1e-28 .. 79228162514264337593543950335)
 * 3. Находим порядок числа (степень 10) и целевой scale = 6 - порядок,
 *    то есть масштаб, при котором мантисса получает ровно 7 значащих цифр.
 * 4. Банковское округление мантиссы до этого масштаба (с одной запасной
 *    цифрой точности, чтобы определить остаток и его "хвост" правильно).
 * 5. Если целевой scale вышел за пределы [0, 28] - переносим лишние
 *    порядки в конечные нули мантиссы (scale < 0) либо пересчитываем
 *    мантиссу заново с scale = 28 (значений не хватает на 7 цифр -
 *    это ожидаемо для чисел, близких к границе 1e-28).
 */

// Возвращает округлённую по банковскому правилу мантиссу числа
// abs_src * 10^scale, используя одну дополнительную цифру точности.
static unsigned long long s21_float_mantissa_at_scale(long double abs_src,
                                                      int scale) {
  long double scaled_extra = abs_src * powl(10.0L, (long double)(scale + 1));
  unsigned long long extra_int = (unsigned long long)scaled_extra;
  int last_digit = (int)(extra_int % 10u);
  unsigned long long mantissa = extra_int / 10u;
  return s21_bank_round_ull(mantissa, last_digit);
}

// Умножает 96-битную мантиссу decimal на 10^count (для zero-padding при
// scale < 0). Возвращает 1 при переполнении.
static int s21_decimal_pad_zeros(s21_decimal *dst, int count) {
  for (int i = 0; i < count; i++) {
    uint64_t carry = 0;
    for (int j = 0; j < 3; j++) {
      uint64_t tmp = (uint64_t)dst->bits[j] * 10u + carry;
      dst->bits[j] = (uint32_t)(tmp & 0xFFFFFFFFu);
      carry = tmp >> 32;
    }
    if (carry) return 1;
  }
  return 0;
}

static void s21_mantissa_to_bits(unsigned long long mantissa,
                                 s21_decimal *dst) {
  dst->bits[0] = (unsigned int)(mantissa & 0xFFFFFFFFu);
  mantissa >>= 32;
  dst->bits[1] = (unsigned int)(mantissa & 0xFFFFFFFFu);
  mantissa >>= 32;
  dst->bits[2] = (unsigned int)(mantissa & 0xFFFFFFFFu);
}

int s21_from_float_to_decimal(float src, s21_decimal *dst) {
  if (dst == NULL) {
    return 1;
  }
  s21_zero_decimal(dst);

  if (isnan(src) || isinf(src)) {
    return 1;
  }
  if (src == 0.0f) {
    if (signbit(src)) {
      s21_set_sign(dst, 1);
    }
    return 0;
  }

  int sign = signbit(src) ? 1 : 0;
  long double abs_src = fabsl((long double)src);
  if (abs_src < 1e-28L) {
    return 1;
  }
  if (abs_src > 79228162514264337593543950335.0L) {
    return 1;
  }

  // Порядок числа: 10^e <= abs_src < 10^(e+1)
  int e = (int)floorl(log10l(abs_src));
  if (powl(10.0L, (long double)e) > abs_src) e--;
  if (powl(10.0L, (long double)(e + 1)) <= abs_src) e++;

  int raw_scale = 6 - e;  // масштаб, дающий ровно 7 значащих цифр
  int final_scale;
  unsigned long long mantissa;

  if (raw_scale >= 0 && raw_scale <= 28) {
    final_scale = raw_scale;
    mantissa = s21_float_mantissa_at_scale(abs_src, raw_scale);
  } else if (raw_scale < 0) {
    // Число >= 10^7 - берём 7 значащих цифр и дополняем нулями
    mantissa = s21_float_mantissa_at_scale(abs_src, raw_scale);
    s21_mantissa_to_bits(mantissa, dst);
    if (s21_decimal_pad_zeros(dst, -raw_scale)) {
      s21_zero_decimal(dst);
      return 1;
    }
    s21_set_scale(dst, 0);
    if (sign) s21_set_sign(dst, 1);
    return 0;
  } else {
    // raw_scale > 28: число очень близко к границе 1e-28,
    // 7 значащих цифр не помещаются - пересчитываем на scale = 28
    final_scale = 28;
    mantissa = s21_float_mantissa_at_scale(abs_src, 28);
  }

  s21_mantissa_to_bits(mantissa, dst);
  s21_set_scale(dst, final_scale);
  if (sign) s21_set_sign(dst, 1);
  return 0;
}
int s21_from_decimal_to_float(s21_decimal src, float *dst) {
  // Проверка на NULL указатель
  if (dst == NULL) {
    return 1;
  }

  // Если число равно нулю
  if (s21_is_zero(src)) {
    *dst = 0.0f;
    return 0;
  }

  // 1. Собираем 96-битное целое число
  // Используем long double для большей точности
  long double value = 0.0L;

  // bits[2] - старшие 32 бита
  // bits[1] - средние 32 бита
  // bits[0] - младшие 32 бита
  value = (long double)src.bits[2] * 18446744073709551616.0L;  // 2^64
  value += (long double)src.bits[1] * 4294967296.0L;           // 2^32
  value += (long double)src.bits[0];

  // 2. Делим на 10^scale, чтобы из мантиссы получить исходное число типа float
  int scale = s21_get_scale(src);
  for (int i = 0; i < scale; i++) {
    value /= 10.0L;
  }

  // 3. Применяем знак
  if (s21_get_sign(src)) {
    value = -value;
  }

  // 4. Сохраняем в float
  *dst = (float)value;

  return 0;
}