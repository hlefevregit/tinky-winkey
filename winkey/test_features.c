/* Test program pour vérifier la gestion du backspace */

#include "winkey.h"

// Variable globale pour les tests
t_winkey g_winkey;

void test_backspace_functionality(void) {
    printf("=== Test de la fonctionnalité Backspace ===\n");
    
    // Initialiser le buffer
    memset(&g_winkey, 0, sizeof(t_winkey));
    
    // Simuler quelques frappes
    printf("1. Ajout de 'Hello': ");
    sendKeyToLogFile('H');
    sendKeyToLogFile('e');
    sendKeyToLogFile('l');
    sendKeyToLogFile('l');
    sendKeyToLogFile('o');
    printf("Buffer: '%s'\n", g_winkey.key_buffer);
    
    // Simuler un backspace
    printf("2. Après un backspace: ");
    sendKeyToLogFile(VK_BACK);
    printf("Buffer: '%s'\n", g_winkey.key_buffer);
    
    // Ajouter un espace
    printf("3. Ajout d'un espace: ");
    sendKeyToLogFile(VK_SPACE);
    printf("Buffer: '%s'\n", g_winkey.key_buffer);
    
    // Simuler un backspace sur le token espace
    printf("4. Backspace sur [SPACE]: ");
    sendKeyToLogFile(VK_BACK);
    printf("Buffer: '%s'\n", g_winkey.key_buffer);
    
    // Ajouter du texte
    printf("5. Ajout de 'World[ENTER]': ");
    sendKeyToLogFile('W');
    sendKeyToLogFile('o');
    sendKeyToLogFile('r');
    sendKeyToLogFile('l');
    sendKeyToLogFile('d');
    sendKeyToLogFile(VK_RETURN);
    printf("Buffer: '%s'\n", g_winkey.key_buffer);
    
    // Backspace sur [ENTER]
    printf("6. Backspace sur [ENTER]: ");
    sendKeyToLogFile(VK_BACK);
    printf("Buffer: '%s'\n", g_winkey.key_buffer);
}

int main(void) {
    printf("Test des fonctionnalités améliorées de WinKey\n");
    printf("==========================================\n\n");
    
    test_backspace_functionality();
    
    printf("\n=== Test des caractères spéciaux ===\n");
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
        {'A', "'A'"},
        {'1', "'1'"},
        {0, NULL}
    };
    
    for (int i = 0; test_keys[i].name != NULL; i++) {
        getUtf8Char(test_keys[i].vk, buffer, sizeof(buffer));
        printf("%-12s -> '%s'\n", test_keys[i].name, buffer);
    }
    
    return 0;
}