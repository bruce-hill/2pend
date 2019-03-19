PREFIX=
CC=cc
CFLAGS=-O3 -std=gnu99
LIBS=-ltermbox -lm
NAME=2pend

all: $(NAME)

clean:
	rm $(NAME)

run: $(NAME)
	./$(NAME)

$(NAME): $(NAME).c
	$(CC) $(NAME).c $(LIBS) $(CFLAGS) -o $(NAME)

install: $(NAME)
	@prefix="$(PREFIX)"; \
	if [[ ! $$prefix ]]; then \
		read -p $$'\033[1mWhere do you want to install? (default: /usr/local) \033[0m' prefix; \
	fi; \
	if [[ ! $$prefix ]]; then \
		prefix="/usr/local"; \
	fi; \
	mkdir -pv $$prefix/bin $$prefix/share/man/man1 \
	&& cp -v $(NAME) $$prefix/bin/ \
	&& cp -v doc/$(NAME).1 $$prefix/share/man/man1/

uninstall:
	@prefix="$(PREFIX)"; \
	if [[ ! $$prefix ]]; then \
		read -p $$'\033[1mWhere do you want to uninstall from? (default: /usr/local) \033[0m' prefix; \
	fi; \
	if [[ ! $$prefix ]]; then \
		prefix="/usr/local"; \
	fi; \
	echo "Deleting..."; \
	rm -rvf $$prefix/bin/$(NAME) $$prefix/share/man/man1/$(NAME).1

