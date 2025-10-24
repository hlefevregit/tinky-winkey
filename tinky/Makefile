# Makefile pour Tinky Service avec Microsoft Visual C++ Compiler (cl)

CC = cl
CFLAGS = /Wall /WX /wd4668 /wd4820
LIBS = advapi32.lib wtsapi32.lib user32.lib
TARGET = svc.exe
SOURCES = svc.c cli.c token.c
HEADERS = cli.h token.h
LOGFILE = svc_trace.log

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) /Fe:$(TARGET) /link $(LIBS)

clean:
	-del $(TARGET) 2>nul
	-del *.obj 2>nul
	-del *.pdb 2>nul
	-del *.ilk 2>nul

fclean: clean
	-del $(LOGFILE) 2>nul
	-del C:\Temp\winkey.log 2>nul
	-del C:\Temp\child_output.log 2>nul

rebuild: clean all

debug: 
	$(CC) $(CFLAGS) /Zi /Od $(SOURCES) /Fe:$(TARGET) /link $(LIBS)

release: 
	$(CC) $(CFLAGS) /O2 /DNDEBUG $(SOURCES) /Fe:$(TARGET) /link $(LIBS)

check:
	$(CC) /Zs $(CFLAGS) $(SOURCES)

info:
	@echo Compilateur: $(CC)
	@echo Flags: $(CFLAGS)
	@echo Librairies: $(LIBS)
	@echo Cible: $(TARGET)

help:
	@echo Makefile pour Tinky Service - Options disponibles:
	@echo.
	@echo   all         - Compile le service (défaut)
	@echo   clean       - Supprime les fichiers objets et exécutables
	@echo   fclean      - Supprime tous les fichiers générés (inclut logs)
	@echo   rebuild     - Nettoyage complet puis recompilation
	@echo   debug       - Compile en mode debug
	@echo   release     - Compile en mode release (optimisé)
	@echo   check       - Vérifie la syntaxe sans compiler
	@echo   info        - Affiche les informations de compilation
	@echo   help        - Affiche cette aide

.PHONY: all clean fclean rebuild info debug release check help