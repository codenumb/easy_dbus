CFLAGS_DBUS = $(shell pkg-config --cflags --libs dbus-1)
CFLAGS_DBUS_GLIB = $(shell pkg-config --cflags --libs dbus-glib-1)
BLD_DIR = build
CFLAGS_EASYDBUS_INCLUDE = -I ./include
LDFLAGS_DBUS = -ldbus-client -levent -L$(BLD_DIR)
LDFLAGS_LOGMODULE = -llogmodule -L$(BLD_DIR)

all: mkdir liblogmodule libdbus-client app1 app2

mkdir:
	-mkdir -p ${BLD_DIR} ${BLD_DIR}/app1 ${BLD_DIR}/app2

liblogmodule: src/logmodule.c
	$(CC) -shared -o ${BLD_DIR}/$@.so $^ ${CFLAGS_EASYDBUS_INCLUDE} -fPIC -s

libdbus-client: src/dbus-client.c
	$(CC) -shared -o ${BLD_DIR}/$@.so $< ${CFLAGS_DBUS} ${CFLAGS_DBUS_GLIB} ${CFLAGS_EASYDBUS_INCLUDE} ${LDFLAGS_LOGMODULE} -levent -lpthread -fPIC -s

app1: examples/app1/app1.c
	$(CC) $^ -o $(BLD_DIR)/$@/$@ ${CFLAGS_DBUS} ${CFLAGS_DBUS_GLIB} ${CFLAGS_EASYDBUS_INCLUDE} ${LDFLAGS_DBUS} ${LDFLAGS_LOGMODULE} -s

app2: examples/app2/app2.c
	$(CC) $^ -o $(BLD_DIR)/$@/$@  ${CFLAGS_DBUS} ${CFLAGS_DBUS_GLIB} ${CFLAGS_EASYDBUS_INCLUDE} ${LDFLAGS_DBUS} ${LDFLAGS_LOGMODULE} -s

clean: 
	rm -rf $(BLD_DIR)

.PHONY: all clean
