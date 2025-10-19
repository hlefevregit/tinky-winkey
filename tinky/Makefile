# Makefile pour Tinky Service avec Microsoft Visual C++ Compiler (cl)

# Variables
CC = cl
CFLAGS = /W4 /TC /wd5045 /wd4668 /wd4820 /wd4777 /D_WIN32_WINNT=0x0601 /DWINVER=0x0601
LIBS = advapi32.lib wtsapi32.lib user32.lib
TARGET = svc.exe
SOURCES = svc.c cli.c token.c
HEADERS = cli.h token.h
LOGFILE = svc_trace.log

# Règle par défaut
all: $(TARGET)

# Compilation du service principal
$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) /Fe:$(TARGET) /link $(LIBS)

# Nettoyage des fichiers générés
clean:
	-del $(TARGET) 2>nul
	-del *.obj 2>nul
	-del *.pdb 2>nul
	-del *.ilk 2>nul

# Nettoyage complet (inclut les logs)
distclean: clean
	-del $(LOGFILE) 2>nul
	-del C:\Temp\winkey.log 2>nul
	-del C:\Temp\child_output.log 2>nul

# Rebuild complet
rebuild: clean all

# Debug build (avec informations de débogage)
debug: 
	$(CC) $(CFLAGS) /Zi /Od $(SOURCES) /Fe:$(TARGET) /link $(LIBS)

# Release build (optimisé)
release: 
	$(CC) $(CFLAGS) /O2 /DNDEBUG $(SOURCES) /Fe:$(TARGET) /link $(LIBS)

# Vérification de la syntaxe sans compilation
check:
	$(CC) /Zs $(CFLAGS) $(SOURCES)

# Affichage des informations de compilation
info:
	@echo Compilateur: $(CC)
	@echo Flags: $(CFLAGS)
	@echo Librairies: $(LIBS)
	@echo Cible: $(TARGET)

# Aide
help:
	@echo Makefile pour Tinky Service - Options disponibles:
	@echo.
	@echo   all         - Compile le service (défaut)
	@echo   clean       - Supprime les fichiers objets et exécutables
	@echo   distclean   - Supprime tous les fichiers générés (inclut logs)
	@echo   rebuild     - Nettoyage complet puis recompilation
	@echo   debug       - Compile en mode debug
	@echo   release     - Compile en mode release (optimisé)
	@echo   check       - Vérifie la syntaxe sans compiler
	@echo   info        - Affiche les informations de compilation
	@echo   help        - Affiche cette aide

# Déclaration des cibles qui ne correspondent pas à des fichiers
.PHONY: all clean distclean rebuild info debug release check help