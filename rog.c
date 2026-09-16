#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// ==========================================
// 1. КОНФИГУРАЦИЯ (Макросы)
// ==========================================
#define START_PLAYER_X 3
#define START_PLAYER_Y 3
#define WIDE_WALL_X 1
#define WIDE_WALL_Y 3
#define NUM_STONES 20
#define NUM_DOORS 3
#define MAX_SKELETONS 1
#define MAX_GOBLINS 10 + MAX_SKELETONS
#define HP_GOBLIN_MIN 1
#define HP_GOBLIN_MAX 3
#define WALLET_GOBLIN_MIN 1
#define WALLET_GOBLIN_MAX 10
#define START_PLAYER_HP 100

#define NUM_DRINKS 5

// ==========================================
// 2. СТРУКТУРЫ (Наш новый тип данных "Враг")
// ==========================================
typedef struct {
    int x;
    int y;
    int hp;
    int wallet;
    int is_alive;
    char type;
} Goblin;

// Структура для описания одного напитка
typedef struct {
    const char* name; // Название
    int cost;         // Цена
    int heal;         // Лечение
} Drink;

Drink tavern_menu[] = {
    {"LAGER",  5,  5},
    {"STOUT",  5,  5},
    {"ALES",  10, 10},
    {"MEAD",  15, 15},
    {"CIDER", 20, 20}
};
// ==========================================
// 3. ЛОГИКА ДАННЫХ
// ==========================================

void generate_map(int rows, int cols, char map[][cols]) {
    // заполнение стенами
    for (int yy = 0; yy < rows; yy++) {
        for (int xx = 0; xx < cols; xx++) {
            map[yy][xx] = '#';
        }            
    }
    
    // пустуая комната внутри - заполенение ' '
    for (int yy = WIDE_WALL_Y; yy < rows - 1; yy++) {
        for (int xx = WIDE_WALL_X; xx < cols - 1; xx++) {
            map[yy][xx] = ' ';
        }            
    }

    // генерация камней
    for (int i = 0; i < NUM_STONES; i++) {
        // Выбираем случайную точку внутри комнаты
        int rx = WIDE_WALL_X + rand() % (cols - WIDE_WALL_X - 1);
        int ry = WIDE_WALL_Y + rand() % (rows - WIDE_WALL_Y - 1);
        
        // Ставим камень ТОЛЬКО если:
        // 1. Клетка сейчас пустая (' ')
        // 2. Это не стартовая позиция игрока (чтобы он не застрял сразу)
        if (map[ry][rx] == ' ' && !(rx == START_PLAYER_X && ry == START_PLAYER_Y)) {
            map[ry][rx] = '.'; 
        }
    }
        // генерация дверей ('D')
    for (int i = 0; i < NUM_DOORS; i++) {
        int rx, ry;
        do {
                // Случайная точка внутри комнаты
            rx = WIDE_WALL_X + rand() % (cols - WIDE_WALL_X - 1);
            ry = WIDE_WALL_Y + rand() % (rows - WIDE_WALL_Y - 1);
        } while (map[ry][rx] != ' ' || (rx == START_PLAYER_X && ry == START_PLAYER_Y));
        map[ry][rx] = 'D';
    }

        // генерация таверны
    int tx, ty;
    do {
        tx = WIDE_WALL_X + rand() % (cols - WIDE_WALL_X - 1);
        ty = WIDE_WALL_Y + rand() % (rows - WIDE_WALL_Y - 1);
    } while (map[ty][tx] != ' ' || (tx == START_PLAYER_X && ty == START_PLAYER_Y)); 
    map[ty][tx] = 'T';

    // генерация ключа ('K')
    int kx, ky;
    do {
        kx = WIDE_WALL_X + rand() % (cols - WIDE_WALL_X - 1);
        ky = WIDE_WALL_Y + rand() % (rows - WIDE_WALL_Y - 1);
    } while (map[ky][kx] != ' ' || (kx == START_PLAYER_X && ky == START_PLAYER_Y));
    map[ky][kx] = 'K';
}


void move_player_and_goblins(int c, int *px, int *py, int *player_hp, Goblin goblins[], int *score, int *has_key, int rows, int cols, char map[][cols]){
    
    // движение игрока (можно ходить по ' ', 'D', 'T')
    if (c == KEY_UP && *py - 1 >= 0 && (map[*py - 1][*px] == ' ' || map[*py - 1][*px] == 'D' || map[*py - 1][*px] == 'T' || map[*py - 1][*px] == 'K')) (*py)--;
    else if (c == KEY_DOWN && *py + 1 < rows && (map[*py + 1][*px] == ' ' || map[*py + 1][*px] == 'D' || map[*py + 1][*px] == 'T' || map[*py + 1][*px] == 'K')) (*py)++;
    else if (c == KEY_LEFT && *px - 1 >= 0 && (map[*py][*px - 1] == ' ' || map[*py][*px - 1] == 'D' || map[*py][*px - 1] == 'T'  || map[*py][*px - 1] == 'K')) (*px)--;
    else if (c == KEY_RIGHT && *px + 1 < cols && (map[*py][*px + 1] == ' ' || map[*py][*px + 1] == 'D' || map[*py][*px + 1] == 'T' || map[*py][*px + 1] == 'K')) (*px)++;

    // поднять ключ
    if (map[*py][*px] == 'K') {
        *has_key = 1; // Передаем по указателю! (не забудь добавить int *has_key в аргументы функции)
        map[*py][*px] = ' '; // Убираем ключ с карты
    }

    // движение и логика всех врагов
    for (int i = 0; i < MAX_GOBLINS; i++) {
        
        if (!goblins[i].is_alive) continue;

        // А) Игрок наступил на гоблина?
        if (*px == goblins[i].x && *py == goblins[i].y) {
            (*score) += goblins[i].wallet; // ИСПРАВЛЕНО: разыменовываем указатель
            (*player_hp) -= goblins[i].hp;
            // спавн новых гоблинов
            // do {
            //     goblins[i].x = WIDE_WALL_X + rand() % (cols - WIDE_WALL_X - 1);
            //     goblins[i].y = WIDE_WALL_Y + rand() % (rows - WIDE_WALL_Y - 1);
            // } while (map[goblins[i].y][goblins[i].x] != ' ' || (goblins[i].x == *px && goblins[i].y == *py));
            goblins[i].is_alive = 0;
            continue; 
        }

        // Б) Движение гоблина (зависит от типа)
        int next_x = goblins[i].x;
        int next_y = goblins[i].y;

        if (goblins[i].type == 'S') {
            // === СКЕЛЕТ ПРЕСЛЕДУЕТ ИГРОКА ===
            
            // Вычисляем расстояние по осям
            int dx = *px - goblins[i].x; // Положительное = игрок правее, отрицательное = левее
            int dy = *py - goblins[i].y; // Положительное = игрок ниже, отрицательное = выше
            
            // Определяем, по какой оси двигаться (если расстояние по X больше, идем по X)
            if (abs(dx) > abs(dy)) {
                // Двигаемся по оси X
                if (dx > 0) next_x++;      // Игрок правее
                else if (dx < 0) next_x--; // Игрок левее
            } else {
                // Двигаемся по оси Y
                if (dy > 0) next_y++;      // Игрок ниже
                else if (dy < 0) next_y--; // Игрок выше
            }
            
            // Если не можем пойти по выбранной оси (там стена), пробуем другую
            if (next_x == goblins[i].x && next_y == goblins[i].y) {
                // Значит, первая попытка не удалась, пробуем альтернативу
                if (abs(dx) > abs(dy)) {
                    // Пробуем Y вместо X
                    if (dy > 0) next_y++;
                    else if (dy < 0) next_y--;
                } else {
                    // Пробуем X вместо Y
                    if (dx > 0) next_x++;
                    else if (dx < 0) next_x--;
                }
            }
            
        } else {
            // === ГОБЛИН ХОДИТ СЛУЧАЙНО (старая логика) ===
            int dir = rand() % 4;
            if (dir == 0) next_y--;
            else if (dir == 1) next_y++;
            else if (dir == 2) next_x--;
            else if (dir == 3) next_x++;
        }

        // Проверка: можно ли пойти? (Не стена и не камень)
        if (next_y >= 0 && next_y < rows && next_x >= 0 && next_x < cols) {
            if (map[next_y][next_x] == ' ') {
                goblins[i].x = next_x;
                goblins[i].y = next_y;
            }
        }
        // В) ГОБЛИН НАСТУПИЛ НА ИГРОКА? (Добавить эту проверку!)
        if (*px == goblins[i].x && *py == goblins[i].y) {
            (*score) += goblins[i].wallet;
            (*player_hp) -= goblins[i].hp;
            goblins[i].is_alive = 0; // Убиваем гоблина
        }
    }
}

// ==========================================
// 4. ОТРИСОВКА
// ==========================================

void draw_map(int rows, int cols, char map[][cols]) {
    for (int yy = 0; yy < rows; yy++) {
        for (int xx = 0; xx < cols; xx++) {
            mvaddch(yy, xx, map[yy][xx]);
        }            
    }
}

void draw_ui(int x, int y, int score, int level, int player_hp, int has_key) {
    mvprintw(1, 1, " HP: %d\tSCORE: %d\tKEY: %s \tLEVEL: %d\t X: %d : Y: %d\t", player_hp, score, has_key == 1 ? "Yes" : "NO", level, x, y);
}

void draw_entities(int player_x, int player_y, Goblin goblins[]) {
    for (int i = 0; i < MAX_GOBLINS; i++) {
        if (goblins[i].is_alive) {
            // МАГИЯ: рисуем тот символ, который записан в поле type!
            mvaddch(goblins[i].y, goblins[i].x, goblins[i].type);
        }
    }
    mvaddch(player_y, player_x, '@'); 
}

void init_ncurses() {
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
}

void next_level(int *px, int *py, Goblin goblins[], int *level, int rows, int cols, char map[][cols]) {
    (*level)++;
    
    // 1. Генерируем новую карту
    generate_map(rows, cols, map);
    
    // 2. Возвращаем игрока на старт
    *px = START_PLAYER_X;
    *py = START_PLAYER_Y;
    
    // 3. Респавним всех гоблинов
    for (int i = 0; i < MAX_GOBLINS; i++) {
        goblins[i].is_alive = 1;

        // РАЗДЕЛЯЕМ ЛОГИКУ ПО ТИПАМ ВРАГОВ
        if (i < MAX_SKELETONS) {
            // Первый враг - Скелет ('S')
            goblins[i].type = 'S';
            goblins[i].hp = 5;          // У скелета больше здоровья
            goblins[i].wallet = 15;     // И он дает больше монет
        } else {
            // Остальные - обычные Гоблины ('G')
            goblins[i].type = 'G';
            goblins[i].hp = HP_GOBLIN_MIN + rand() % (HP_GOBLIN_MAX - HP_GOBLIN_MIN + 1);
            goblins[i].wallet = WALLET_GOBLIN_MIN + rand() % (WALLET_GOBLIN_MAX - WALLET_GOBLIN_MIN + 1);
        }

        do {
            goblins[i].x = WIDE_WALL_X + rand() % (cols - WIDE_WALL_X - 1);
            goblins[i].y = WIDE_WALL_Y + rand() % (rows - WIDE_WALL_Y - 1);
        } while (map[goblins[i].y][goblins[i].x] != ' ' || (goblins[i].x == *px && goblins[i].y == *py));
    }
}



void merchant_menu(int *player_hp, int *score, int rows, int cols) {
    int choice = 0;
    
    while (1) {
        clear();
        
        int start_y = rows / 3;
        int start_x = (cols / 2) - 15; // Чуть сдвинул для красоты длинных строк

        mvprintw(start_y, start_x, "=== TAVERN ===");
        mvprintw(start_y + 1, start_x, "=== Buy a drink ===");

        // 1. ОТРИСОВКА МЕНЮ ЦИКЛОМ (вместо 5 строк mvprintw)
        for (int i = 0; i < NUM_DRINKS; i++) {
            mvprintw(start_y + 3 + i, start_x, "%d. Buy %-6s (+%d HP) for %d coins", 
                     i + 1, tavern_menu[i].name, tavern_menu[i].heal, tavern_menu[i].cost);
        }

        mvprintw(start_y + 3 + NUM_DRINKS, start_x, "0. Leave the tavern");
        mvprintw(start_y + 5 + NUM_DRINKS, start_x, "Your HP: %d | Coins: %d", *player_hp, *score);
        mvprintw(start_y + 7 + NUM_DRINKS, start_x, "Choose action (0-%d): ", NUM_DRINKS);
        
        refresh();
        choice = getch();

        // 2. УНИВЕРСАЛЬНАЯ ЛОГИКА ПОКУПКИ
        // Превращаем символ '1'-'5' в индекс массива 0-4
        int drink_index = choice - '1';

        // Проверяем, что нажата цифра от 1 до NUM_DRINKS
        if (drink_index >= 0 && drink_index < NUM_DRINKS) {
            
            // Проверяем, хватает ли денег
            if (*score >= tavern_menu[drink_index].cost) {
                // Покупаем!
                *score -= tavern_menu[drink_index].cost;
                *player_hp += tavern_menu[drink_index].heal;
                
                mvprintw(start_y + 9 + NUM_DRINKS, start_x, "You're recovered! Press any key...");
            } else {
                // Не хватает денег
                mvprintw(start_y + 9 + NUM_DRINKS, start_x, "Not enough coins! Press any key...");
            }
            refresh();
            getch(); // Ждем нажатия, чтобы игрок прочитал сообщение
        } 
        // 3. ВЫХОД
        else if (choice == '0' || choice == 27) {
            break; // Выходим из меню
        }
    }
}

// ==========================================
// 5. ГЛАВНЫЙ ЦИКЛ
// ==========================================
int main() {
    int c = 0;
    int px = START_PLAYER_X, py = START_PLAYER_Y;
    int rows, cols;
    int score = 100;
    int has_key = 0;
    int level = 1;
    int player_hp = START_PLAYER_HP;
    
    init_ncurses();
    getmaxyx(stdscr, rows, cols);
    
    char map[rows][cols];
    generate_map(rows, cols, map);
    srand(time(NULL)); 

    Goblin goblins[MAX_GOBLINS];

    // ЗАПОЛНЯЕМ МАССИВ врагов ПРИ СТАРТЕ
    for (int i = 0; i < MAX_GOBLINS; i++) {
        goblins[i].is_alive = 1;

        // РАЗДЕЛЯЕМ ЛОГИКУ ПО ТИПАМ ВРАГОВ
        if (i < MAX_SKELETONS) {
            // Первый враг - Скелет ('S')
            goblins[i].type = 'S';
            goblins[i].hp = 5;          // У скелета больше здоровья
            goblins[i].wallet = 15;     // И он дает больше монет
        } else {
            // Остальные - обычные Гоблины ('G')
            goblins[i].type = 'G';
            goblins[i].hp = HP_GOBLIN_MIN + rand() % (HP_GOBLIN_MAX - HP_GOBLIN_MIN + 1);
            goblins[i].wallet = WALLET_GOBLIN_MIN + rand() % (WALLET_GOBLIN_MAX - WALLET_GOBLIN_MIN + 1);
        }

        do {
            goblins[i].x = WIDE_WALL_X + rand() % (cols - WIDE_WALL_X - 1);
            goblins[i].y = WIDE_WALL_Y + rand() % (rows - WIDE_WALL_Y - 1);
        } while (map[goblins[i].y][goblins[i].x] != ' ' || (goblins[i].x == px && goblins[i].y == py));
    }
    
    // === ВОТ ЭТОГО ЦИКЛА НЕ ХВАТАЛО ===
    do {
        clear();
        
        move_player_and_goblins(c, &px, &py, &player_hp, goblins, &score, &has_key, rows, cols, map);
        
        if (player_hp <= 0) {
            clear();
            mvprintw(rows / 2, (cols / 2) - 10, "GAME OVER! Score: %d", score);
            mvprintw((rows / 2) + 1, (cols / 2) - 15, "Press any key to exit...");
            refresh();
            getch(); // Ждем нажатия любой клавиши
            break;   // Выходим из цикла do-while
        }
        // === ПРОВЕРКА 1: Все гоблины убиты? ===
        int alive_count = 0;
        for (int i = 0; i < MAX_GOBLINS; i++) {
            if (goblins[i].is_alive) alive_count++;
        }
        
        if (alive_count == 0) {
            next_level(&px, &py, goblins, &level, rows, cols, map);
        }
        
        // === ПРОВЕРКА 2: Игрок наступил на дверь? ===
        if (map[py][px] == 'D') {
            if (has_key) {
                next_level(&px, &py, goblins, &level, rows, cols, map);
                has_key = 0; // На новом уровне ключа снова нет
            } else {
                // Можно добавить временное сообщение, но пока просто не пускаем
                // Чтобы игрок не "застревал" в проверке, можно оттолкнуть его назад, 
                // но пока оставим так: он просто не пройдет проверку next_level.
            }
        }
                // === ПРОВЕРКА 3: Игрок наступил на торговца? ===
        if (map[py][px] == 'T') {
            merchant_menu(&player_hp, &score, rows, cols); // Запускаем меню!
            
            // После выхода из меню, чтобы игрок не "застрял" на клетке торговца 
            // и не вызвал меню снова при следующем движении, 
            // можно его немного отодвинуть или просто позволить ему уйти.
            // Пока оставим как есть, при движении в сторону меню закроется.
        }
        
        draw_map(rows, cols, map);
        draw_entities(px, py, goblins);
        draw_ui(px, py, score, level, player_hp, has_key);
        
        refresh();
        
    } while ((c = getch()) != 27);
    
    endwin();
    return 0;
}