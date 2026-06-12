TARGET_EXEC = smoggy

CFLAGS := -Os -std=c99 -Wall -Wextra -Werror -Wpedantic
LIBS := -lcurl -lcjson

all:
	$(CC) $(CFLAGS) smoggy.c $(LIBS) -o $(TARGET_EXEC)

clean:
	rm -f $(TARGET_EXEC)
