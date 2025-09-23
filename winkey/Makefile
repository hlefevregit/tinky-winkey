# Makefile pour WinKey avec Microsoft Visual C++ Compiler (cl)

# Variables
CC = cl
CFLAGS = /Wall /WX /TC /wd5045
LIBS = user32.lib
TARGET = main.exe
SOURCES = main.c
HEADERS = winkey.h
LOGFILE = winkey.log

# Règle par défaut
all: $(TARGET)

# Compilation du programme principal
$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) /Fe:$(TARGET) /link $(LIBS)

# Compilation de la démo
demo: demo.exe

demo.exe: demo.c $(HEADERS)
	$(CC) $(CFLAGS) /wd4820 demo.c /Fe:demo.exe

# Exécution de la démo
run-demo: demo.exe
	.\demo.exe

# Exécution du programme principal
run: $(TARGET)
	.\$(TARGET)

# Nettoyage des fichiers générés
clean:
	-del $(TARGET) 2>nul
	-del demo.exe 2>nul
	-del *.obj 2>nul
	-del *.pdb 2>nul
	-del *.ilk 2>nul

# Nettoyage complet (inclut les logs)
distclean: clean
	-del $(LOGFILE) 2>nul

# Rebuild complet
rebuild: clean all

# Affichage des informations de compilation
info:
	@echo Compilateur: $(CC)
	@echo Flags: $(CFLAGS)
	@echo Librairies: $(LIBS)
	@echo Cible: $(TARGET)

# Debug build (avec informations de débogage)
debug: 
	$(CC) $(CFLAGS) /Zi /Od $(SOURCES) /Fe:$(TARGET) /link $(LIBS)

# Release build (optimisé)
release: 
	$(CC) $(CFLAGS) /O2 /DNDEBUG $(SOURCES) /Fe:$(TARGET) /link $(LIBS)

# Vérification de la syntaxe sans compilation
check:
	$(CC) /Zs $(CFLAGS) $(SOURCES)

# Aide
help:
	@echo Makefile pour WinKey - Options disponibles:
	@echo.
	@echo   all         - Compile le programme principal (défaut)
	@echo   run         - Compile et exécute le programme principal
	@echo   demo        - Compile la démo des nouvelles fonctionnalités
	@echo   run-demo    - Compile et exécute la démo
	@echo   clean       - Supprime les fichiers objets et exécutables
	@echo   distclean   - Supprime tous les fichiers générés (inclut logs)
	@echo   rebuild     - Nettoyage complet puis recompilation
	@echo   debug       - Compile en mode debug
	@echo   release     - Compile en mode release (optimisé)
	@echo   check       - Vérifie la syntaxe sans compiler
	@echo   info        - Affiche les informations de compilation
	@echo   help        - Affiche cette aide

# Déclaration des cibles qui ne correspondent pas à des fichiers
.PHONY: all run demo run-demo clean distclean rebuild info debug release check help