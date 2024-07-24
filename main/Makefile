CC = gcc
CFLAGS = -I./include -Wall -Wextra
LDFLAGS = -lssl -lcrypto
SRC = main.c src/email_client.c src/imap.c src/utils.c src/utils_ssl.c
OBJ = $(SRC:.c=.o)
TARGET = fetchmail

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)
