CC = gcc
CFLAGS = -Wall -Werror -Wextra -std=c11 -I.
LDFLAGS = -lm

UNAME = $(shell uname -s)
ifeq ($(UNAME), Linux)
	CHECK_LIBS = -lcheck -pthread -lrt -lm -lsubunit
	OPEN_CMD = xdg-open
endif
ifeq ($(UNAME), Darwin)
	CHECK_LIBS = -lcheck -pthread -lm
	OPEN_CMD = open
endif

LIB = s21_decimal.a
SRCS = s21_helpers.c s21_convertors.c s21_comparison.c s21_other.c s21_arithmetic.c
OBJECTS = $(SRCS:.c=.o)

TEST_DIR = tests
TEST_SRCS = $(TEST_DIR)/s21_tests.c
TEST_OBJECTS = $(TEST_SRCS:.c=.o)

.PHONY: all clean test s21_decimal.a gcov_report style format rebuild

all: $(LIB)

$(LIB): $(OBJECTS)
	ar rcs $@ $^
	ranlib $@

%.o: %.c s21_decimal.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_DIR)/%.o: $(TEST_DIR)/%.c s21_decimal.h
	$(CC) $(CFLAGS) -c $< -o $@

test: $(LIB) $(TEST_OBJECTS)
	$(CC) $(CFLAGS) $(TEST_OBJECTS) -L. -l:$(LIB) $(CHECK_LIBS) -o s21_test
	./s21_test

gcov_report:
	mkdir -p gcov_report
	$(CC) $(CFLAGS) --coverage $(SRCS) $(TEST_SRCS) -o s21_test $(CHECK_LIBS)
	./s21_test
	gcovr --html --html-details -o gcov_report/coverage_report.html --exclude "tests/.*"
	rm -f *.gcda *.gcno s21_test

style:
	clang-format -n --style=file *.c *.h tests/*.c tests/*.h

format:
	clang-format -i --style=file *.c *.h tests/*.c tests/*.h

clean:
	rm -f $(OBJECTS) $(LIB)
	rm -f $(TEST_OBJECTS)
	rm -f s21_test
	rm -rf gcov_report

rebuild: clean all