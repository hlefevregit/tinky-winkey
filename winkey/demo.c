/* Test simple des fonctionnalités améliorées */

#include "winkey.h"

// Simulation simple pour tester la logique
void simulate_key_sequence(void) {
    printf("=== Démonstration des caractères spéciaux ===\n");
    char buffer[128];
    
    struct test_key {
        int vk;
        const char* name;
    };
    
    struct test_key test_keys[] = {
        {VK_SPACE, "VK_SPACE"},
        {VK_RETURN, "VK_RETURN"}, 
        {VK_TAB, "VK_TAB"},
        {VK_BACK, "VK_BACK"},
        {VK_DELETE, "VK_DELETE"},
        {VK_ESCAPE, "VK_ESCAPE"},
        {VK_F1, "VK_F1"},
        {VK_F12, "VK_F12"},
        {VK_UP, "VK_UP"},
        {VK_LEFT, "VK_LEFT"},
        {VK_NUMPAD5, "VK_NUMPAD5"},
        {VK_LSHIFT, "VK_LSHIFT"},
        {VK_LCONTROL, "VK_LCONTROL"},
        {VK_LMENU, "VK_LMENU"},
        {'A', "'A'"},
        {'1', "'1'"},
        {0x41, "0x41 (A)"},
        {0x31, "0x31 (1)"},
        {0, NULL}
    };
    
    for (int i = 0; test_keys[i].name != NULL; i++) {
        // Simuler getUtf8Char
        switch (test_keys[i].vk) {
            case VK_SPACE:
                strcpy(buffer, "[SPACE]");
                break;
            case VK_RETURN:
                strcpy(buffer, "[ENTER]");
                break;
            case VK_TAB:
                strcpy(buffer, "[TAB]");
                break;
            case VK_BACK:
                strcpy(buffer, "[BACKSPACE]");
                break;
            case VK_DELETE:
                strcpy(buffer, "[DELETE]");
                break;
            case VK_ESCAPE:
                strcpy(buffer, "[ESC]");
                break;
            case VK_LSHIFT:
                strcpy(buffer, "[SHIFT]");
                break;
            case VK_LCONTROL:
                strcpy(buffer, "[CTRL]");
                break;
            case VK_LMENU:
                strcpy(buffer, "[ALT]");
                break;
            case VK_F1:
                strcpy(buffer, "[F1]");
                break;
            case VK_F12:
                strcpy(buffer, "[F12]");
                break;
            case VK_UP:
                strcpy(buffer, "[UP]");
                break;
            case VK_LEFT:
                strcpy(buffer, "[LEFT]");
                break;
            case VK_NUMPAD5:
                strcpy(buffer, "[NUM5]");
                break;
            case 'A':
                strcpy(buffer, "A");
                break;
            case '1':
                strcpy(buffer, "1");
                break;
            default:
                snprintf(buffer, sizeof(buffer), "[VK_%02X]", test_keys[i].vk);
                break;
        }
        
        printf("%-15s -> '%s'\n", test_keys[i].name, buffer);
    }
    
    printf("\n=== Démonstration de la logique Backspace ===\n");
    printf("Simulation: Tapez 'Hello[SPACE]World[ENTER]' puis plusieurs backspaces\n");
    printf("Résultat attendu:\n");
    printf("  'Hello[SPACE]World[ENTER]'\n");
    printf("  'Hello[SPACE]World' (après backspace sur [ENTER])\n");
    printf("  'Hello[SPACE]Worl' (après backspace sur 'd')\n");
    printf("  'Hello[SPACE]Wor' (après backspace sur 'l')\n");
    printf("  'Hello[SPACE]' (après plusieurs backspaces)\n");
    printf("  'Hello' (après backspace sur [SPACE])\n");
}

int main(void) {
    printf("Test des fonctionnalités améliorées de WinKey\n");
    printf("==========================================\n\n");
    
    simulate_key_sequence();
    
    printf("\n=== Avantages des modifications ===\n");
    printf("1. Caractères spéciaux explicites: Plus de [VK_20], maintenant [SPACE]\n");
    printf("2. Backspace intelligent: Supprime au lieu d'afficher [BACKSPACE]\n");
    printf("3. Gestion des tokens: Backspace sur [SPACE] supprime tout le token\n");
    printf("4. Lisibilité améliorée: Les logs sont plus compréhensibles\n");
    
    return 0;
}