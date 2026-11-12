#include "s21_decimal.h"

// ============================================================
// ИНИЦИАЛИЗАЦИЯ И КОПИРОВАНИЕ
// ============================================================

/**
 * @brief Обнуляет все биты структуры decimal
 * @param dec Указатель на структуру для обнуления
 */
void s21_zero_decimal(s21_decimal *dec) {
  if (dec != NULL) {
    dec->bits[0] = 0u;
    dec->bits[1] = 0u;
    dec->bits[2] = 0u;
    dec->bits[3] = 0u;
  }
}

/**
 * @brief Копирует одну структуру decimal в другую
 * @param dest Указатель на структуру-приемник
 * @param src Исходная структура для копирования
 */
void s21_copy_decimal(s21_decimal *dest, s21_decimal src) {
  if (dest != NULL) {
    dest->bits[0] = src.bits[0];
    dest->bits[1] = src.bits[1];
    dest->bits[2] = src.bits[2];
    dest->bits[3] = src.bits[3];
  }
}

// ============================================================
// РАБОТА С БИТАМИ
// ============================================================

/**
 * @brief Получает значение бита по позиции (0-127)
 * @param dec Структура decimal
 * @param position Позиция бита (0 - младший, 127 - старший)
 * @return Значение бита (0 или 1)
 */
int s21_get_bit(s21_decimal dec, int position) {
  // Проверка на валидность позиции
  if (position < 0 || position > 127) {
    return 0;
  }

  // Определяем индекс массива (0-3) и смещение внутри int
  int index = position / 32;  // В каком элементе bits[] находится бит
  int offset = position % 32;  // Позиция внутри элемента

  // Сдвигаем нужный бит в младшую позицию и используем двочиную маску, чтобы
  // извлечь его
  return (dec.bits[index] >> offset) & 1u;
}

/**
 * @brief Устанавливает бит в 1 по указанной позиции
 * @param dec Указатель на структуру decimal
 * @param position Позиция бита (0-127)
 */
void s21_set_bit(s21_decimal *dec, int position) {
  if (dec == NULL || position < 0 || position > 127) {
    return;
  }

  int index = position / 32;
  int offset = position % 32;

  // OR с единицей в нужной позиции, чтобы установить бит 1
  dec->bits[index] |= (1u << offset);
}

/**
 * @brief Устанавливает бит в 0 по указанной позиции
 * @param dec Указатель на структуру decimal
 * @param position Позиция бита (0-127)
 */
void s21_clear_bit(s21_decimal *dec, int position) {
  if (dec == NULL || position < 0 || position > 127) {
    return;
  }

  int index = position / 32;
  int offset = position % 32;

  // AND с инвертированной маской, чтобы занулить бит в позиции offset
  dec->bits[index] &= ~(1u << offset);
}

// ============================================================
// РАБОТА СО ЗНАКОМ (БИТ 31 В bits[3])
// ============================================================

/**
 * @brief Получает знак числа
 * @param dec Структура decimal
 * @return 0 - положительное, 1 - отрицательное
 */
int s21_get_sign(s21_decimal dec) {
  // Сдвигаем бит 31 в младшую позицию и применяем маску, чтобы извлечь бит
  return (dec.bits[3] >> 31) & 1u;
}

/**
 * @brief Устанавливает знак числа
 * @param dec Указатель на структуру decimal
 * @param sign 0 - положительное, 1 - отрицательное
 */
void s21_set_sign(s21_decimal *dec, int sign) {
  if (dec == NULL) {
    return;
  }

  if (sign) {
    // Устанавливаем бит 31 в 1
    dec->bits[3] |= (1u << 31);
  } else {
    // Устанавливаем бит 31 в 0
    dec->bits[3] &= ~(1u << 31);
  }
}

/**
 * @brief Инвертирует знак числа (меняет на противоположный)
 * @param dec Указатель на структуру decimal
 */
void s21_negate_sign(s21_decimal *dec) {
  if (dec == NULL) {
    return;
  }

  // XOR с единицей в бите 31 инвертирует его, [1 XOR 1 == 0], [0 XOR 1 == 1]
  dec->bits[3] ^= (1u << 31);
}

// ============================================================
// РАБОТА С МАСШТАБОМ (БИТЫ 16-23 В bits[3])
// ============================================================

/**
 * @brief Получает масштаб числа (степень 10)
 * @param dec Структура decimal
 * @return Масштаб (0-28)
 */
int s21_get_scale(s21_decimal dec) {
  // Сдвигаем биты 16-23 в младшую позицию и применяем маску, чтобы извлечь их
  return (dec.bits[3] >> 16) & 0xFFu;
}

/**
 * @brief Устанавливает масштаб числа
 * @param dec Указатель на структуру decimal
 * @param scale Масштаб (0-28)
 */
void s21_set_scale(s21_decimal *dec, int scale) {
  if (dec == NULL) {
    return;
  }

  // Ограничиваем масштаб допустимыми значениями
  if (scale < 0) {
    scale = 0;
  } else if (scale > 28) {
    scale = 28;
  }

  // Очищаем биты 16-23 (устанавливаем в 0)
  // Сдвигаем 11111111(0xFFu) на 16 позиций влево, младшие 16 бит образовавшиеся
  // после сдвига влево заполняются нулями затем применяется инверсия(~) и
  // получаем противоположное число: 11111111 00000000 11111111 11111111
  dec->bits[3] &= ~(0xFFu << 16);

  // Устанавливаем новое значение масштаба
  // Масштаб передается как десятичное число и занимает 8 бит, значит нужно
  // сдвинуть его влево на 16 бит, чтобы записать в биты 16-23
  dec->bits[3] |= ((unsigned int)scale << 16);
}

// ============================================================
// ПРОВЕРКИ
// ============================================================

/**
 * @brief Проверяет, равно ли число нулю
 * @param dec Структура decimal
 * @return 1 - число равно нулю, 0 - не равно нулю
 */
int s21_is_zero(s21_decimal dec) {
  // Число равно нулю, если все 96 бит целой части равны нулю
  // Масштаб и знак не влияют на математическое значение нуля
  return (dec.bits[0] == 0u && dec.bits[1] == 0u && dec.bits[2] == 0u);
}

/**
 * @brief Проверяет, является ли число отрицательным
 * @param dec Структура decimal
 * @return 1 - отрицательное, 0 - положительное или ноль
 */
int s21_is_negative(s21_decimal dec) {
  // Если число не ноль и битовый знак установлен
  return (!s21_is_zero(dec) && s21_get_sign(dec));
}

/**
 * @brief Проверяет, является ли число положительным
 * @param dec Структура decimal
 * @return 1 - положительное, 0 - отрицательное или ноль
 */
int s21_is_positive(s21_decimal dec) {
  // Если число не ноль и битовый знак не установлен
  return (!s21_is_zero(dec) && !s21_get_sign(dec));
}

// ============================================================
// ОТЛАДОЧНЫЕ ФУНКЦИИ
// ============================================================

/**
 * @brief Выводит структуру decimal в читаемом виде (для отладки)
 * @param dec Структура decimal для вывода
 */
/* void s21_print_decimal(s21_decimal dec) {
    printf("Decimal {\n");
    printf("  bits[0] = 0x%08X (%u)\n", dec.bits[0], dec.bits[0]);
    printf("  bits[1] = 0x%08X (%u)\n", dec.bits[1], dec.bits[1]);
    printf("  bits[2] = 0x%08X (%u)\n", dec.bits[2], dec.bits[2]);
    printf("  bits[3] = 0x%08X (%u)\n", dec.bits[3], dec.bits[3]);
    printf("  sign    = %d\n", s21_get_sign(dec));
    printf("  scale   = %d\n", s21_get_scale(dec));
    printf("  is_zero = %d\n", s21_is_zero(dec));
    printf("}\n");
} */

// ============================================================
// ДОПОЛНИТЕЛЬНЫЕ ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (для будущих операций)
// ============================================================

/**
 * @brief Сдвигает 96-битное число влево на 1 бит
 * @param dec Указатель на структуру decimal
 * @return 1 - если произошло переполнение, 0 - если нет
 */
int s21_shift_left_96(s21_decimal *dec) {
  if (dec == NULL) {
    return 0;
  }

  // Проверяем, будет ли переполнение (старший бит bits[2])
  int overflow = (dec->bits[2] & 0x80000000u) != 0;

  // Сдвигаем каждый элемент, перенося старший бит в следующий
  unsigned int carry = 0;
  for (int i = 0; i < 3; i++) {
    // в new_carry сохраняем 31й бит из bits[0] чтобы перенести его на позицию 0
    // следующего элемента bits[1]
    unsigned int new_carry = (dec->bits[i] & 0x80000000u) != 0;
    dec->bits[i] <<= 1;
    dec->bits[i] |= carry;
    carry = new_carry;
  }

  return overflow;  // как в вызывающей функции будет обрабатываться значение
                    // overflow?
}

/**
 * @brief Сдвигает 96-битное число вправо на 1 бит
 * @param dec Указатель на структуру decimal
 */
void s21_shift_right_96(s21_decimal *dec) {
  if (dec == NULL) {
    return;
  }

  unsigned int carry = 0;
  for (int i = 2; i >= 0; i--) {
    // сохраняем нулевой бит элемента bits[2], чтобы перенести его в 31й бит
    // элемента bits[1]
    unsigned int new_carry = (dec->bits[i] & 1u) << 31;
    dec->bits[i] >>= 1;
    dec->bits[i] |= carry;
    carry = new_carry;
  }
}