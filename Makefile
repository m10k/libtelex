OBJECTS = src/token.o src/error.o src/parser.o src/telex.o
TARGET = libtelex.so
INCLUDES = -Iinclude
CFLAGS = -Wall -g -c -fPIC -O2 $(INCLUDES)
ASFLAGS = $(CFLAGS)
LDFLAGS = -shared -Wl,-soname,$(TARGET)

PHONY = clean install

ifeq ($(PREFIX), )
	PREFIX = /usr
endif

all: $(TARGET)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	install -o root -g root -m755 $(TARGET) $(DESTDIR)$(PREFIX)/lib/.
	mkdir -p $(DESTDIR)$(PREFIX)/include/telex
	cp -r include/telex/*.h $(DESTDIR)$(PREFIX)/include/telex/.
	chmod 755 $(DESTDIR)$(PREFIX)/include/telex
	find $(DESTDIR)$(PREFIX)/include/telex -type f -iname "*.h" -exec chmod 644 {} \;
	chown -R root:root $(DESTDIR)$(PREFIX)/include/telex

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/include/telex
	rm -rf $(DESTDIR)$(PREFIX)/lib/$(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -fPIC $(LDFLAGS) -o $@ $^

clean:
	rm -rf $(OBJECTS) $(TARGET)

.PHONY: $(PHONY)
