TARGET_EXEC = smoggy

CFLAGS := -Os -std=c99 -Wall -Wextra -Werror -Wpedantic

# Debugging with sanitize
# CFLAGS := -O0 -std=c99 -Wall -Wextra -Werror -Wpedantic -fsanitize=address,undefined -ggdb

# Debugging with Valgrind
# https://github.com/google/sanitizers/issues/810#issuecomment-395863211
# CFLAGS := -O0 -std=c99 -Wall -Wextra -Werror -Wpedantic -ggdb

LIBS := -lcurl -lcjson

all:
	$(CC) $(CFLAGS) smoggy.c $(LIBS) -o $(TARGET_EXEC)

clean:
	rm -f $(TARGET_EXEC)
