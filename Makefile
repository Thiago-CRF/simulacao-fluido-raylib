CC = gcc

CFLAGS = -Wall -Wextra -g3 -std=c2x
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

SRC = fluid.c

OUT_DIR = output

TARGET = $(OUT_DIR)/fluid_sim

ifdef OS
   # Windows detectado
   EXT = .exe
   RM = del /Q
   RMDIR = rmdir /S /Q
   # Função para trocar / por \ (necessário no CMD do Windows)
   FixPath = $(subst /,\,$1)
   # Comando mkdir seguro para Windows (ignora erro se já existe)
   MKDIR = if not exist $(call FixPath, $1) mkdir $(call FixPath, $1)
   # Comando para rodar (Windows não precisa de ./)
   RUN_CMD = $(call FixPath, $(TARGET)$(EXT))
   NULL_DEV = NUL
else
   # Linux / Unix detectado
   EXT =
   RM = rm -f
   RMDIR = rm -rf
   # No Linux não precisa trocar barras
   FixPath = $1
   # mkdir -p cria pastas aninhadas e não reclama se já existir
   MKDIR = mkdir -p $1
   # Linux precisa de ./ para rodar executável local
   RUN_CMD = ./$(TARGET)
   NULL_DEV = /dev/null
endif

BIN = $(TARGET)$(EXT)

all: $(BIN)

$(BIN): $(SRC)
	@$(call MKDIR, $(OUT_DIR))
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LIBS)

run: all
	$(RUN_CMD)

clean:
	-@$(RMDIR) $(call FixPath, $(OUT_DIR)) 2>$(NULL_DEV) || true 