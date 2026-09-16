#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

// ==========================================
// 1. КОНФИГУРАЦИЯ (Макросы)
// ==========================================
#define START_PLAYER_X 3
#define START_PLAYER_Y 3
#define START_GOBLIN_X 10
#define START_GOBLIN_Y 3
#define WIDE_WALL_X 1
#define WIDE_WALL_Y 3
#define WALLET_GOBLIN_MIN 1
#define WALLET_GOBLIN_MAX 10


// ==========================================
// 2. ЛОГИКА ДАННЫХ (Работает только с памятью)
// ==========================================



// Создает структуру карты в памяти, ничего не рисуя
void generate_map(int rows, int cols, char map[][cols]) {
    for (int yy = 0; yy < rows; yy++) {
        for (int xx = 0; xx < cols; xx++) {
            map[yy][xx] = '#';
        }            
    }
    for (int yy = WIDE_WALL_Y; yy < rows - 1; yy++) {
        for (int xx = WIDE_WALL_X; xx < cols - 1; xx++) {
            map[yy][xx] = ' ';
        }            
    }
}

// Обрабатывает движение игрока с проверкой границ
void move_player(int c, int *px, int *py, int *gx, int *gy, int *score, int rows, int cols, char map[][cols]) {
    if (c == KEY_UP && *py - 1 >= 0 && map[*py - 1][*px] == ' ') (*py)--;
    else if (c == KEY_DOWN && *py + 1 < rows && map[*py + 1][*px] == ' ') (*py)++;
    else if (c == KEY_LEFT && *px - 1 >= 0 && map[*py][*px - 1] == ' ') (*px)--;
    else if (c == KEY_RIGHT && *px + 1 < cols && map[*py][*px + 1] == ' ') (*px)++;

    // 2. ПРОВЕРКА СТОЛКНОВЕНИЯ (Коллизия)
    // Если координаты игрока совпали с координатами гоблина
    if (*px == *gx && *py == *gy) {
       (*score) += WALLET_GOBLIN_MIN + rand() % (WALLET_GOBLIN_MAX - WALLET_GOBLIN_MIN + 1);
        
        // 3. РЕСПАВН ГОБЛИНА
        // Ищем случайную пустую клетку, чтобы гоблин не появился в стене или на игроке
        do {
            *gx = WIDE_WALL_X + rand() % (cols - WIDE_WALL_X - 1);
            *gy = WIDE_WALL_Y + rand() % (rows - WIDE_WALL_Y - 1);
        } while (map[*gy][*gx] != ' ' || (*gx == *px && *gy == *py));
        return;
    }
    
    // движение гоблина
    // 0 вверх, 1 вниз, 2 влево, 3 вправо
    int dir_goblin = rand() % 4;

    int dir_goblin_x = *gx;
    int dir_goblin_y = *gy;
    
    if (dir_goblin == 0) dir_goblin_y++;
    else if (dir_goblin == 1) dir_goblin_y--;
    else if (dir_goblin == 2) dir_goblin_x++;
    else if (dir_goblin == 3) dir_goblin_x--;

    if (dir_goblin_y >= 0 && dir_goblin_y < rows && dir_goblin_x >= 0 && dir_goblin_x < cols) {
        if (map[dir_goblin_y][dir_goblin_x] == ' ') {
            // Только если клетка пустая, гоблин реально делает шаг!
            *gx = dir_goblin_x;
            *gy = dir_goblin_y;
        }
    }   
    if (*px == *gx && *py == *gy) {
        (*score) += WALLET_GOBLIN_MIN + rand() % (WALLET_GOBLIN_MAX - WALLET_GOBLIN_MIN + 1);
        do {
            *gx = WIDE_WALL_X + rand() % (cols - WIDE_WALL_X - 1);
            *gy = WIDE_WALL_Y + rand() % (rows - WIDE_WALL_Y - 1);
        } while (map[*gy][*gx] != ' ' || (*gx == *px && *gy == *py));
        // return здесь не обязателен, так как функция и так заканчивается
    }
}

// ==========================================
// 3. ОТРИСОВКА (Работает только с экраном)
// ==========================================

// Рисует карту из массива данных
void draw_map(int rows, int cols, char map[][cols]) {
    for (int yy = 0; yy < rows; yy++) {
        for (int xx = 0; xx < cols; xx++) {
            mvaddch(yy, xx, map[yy][xx]);
        }            
    }
}

// Рисует интерфейс (HUD)
void draw_ui(int x, int y, int score) {
    // \t добавляет отступы для красоты
    mvprintw(1, 1, "\tX = %d : Y = %d\t SCORE = %d\t", x, y, score);
}

// Рисует всех сущностей на карте
void draw_entities(int player_x, int player_y, int goblin_x, int goblin_y) {
    mvaddch(goblin_y, goblin_x, 'G'); // Гоблин
    // mvaddch(goblin_y, goblin_x, 'R'); // Гоблин
    mvaddch(player_y, player_x, '@'); // Игрок (рисуем последним, чтобы быть поверх всего)
}

// ==========================================
// 4. ИНИЦИАЛИЗАЦИЯ
// ==========================================
void init_ncurses() {
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
}

// ==========================================
// 5. ГЛАВНЫЙ ЦИКЛ
// ==========================================
int main() {
    int c = 0;
    int px = START_PLAYER_X, py = START_PLAYER_Y;
    int gx = START_GOBLIN_X, gy = START_GOBLIN_Y;
    int rows, cols;
    int score = 0;

    typedef struct{
    int x;
    int y;
    int hp;
    int wallet;
    int is_alive;
} goblin;

goblin goblins[3];

goblins[0].x = 1;
goblins[0].y = 1;
goblins[0].hp = 10;
goblins[0].wallet = 10;
goblins[0].is_alive = 1;

goblin goblins[3] = {
    {1, 1, 10, 10, 1}, // Гоблин 0: x, y, hp, wallet, is_alive
    {5, 5, 10, 10, 1}, // Гоблин 1
    {8, 8, 10, 10, 1}  // Гоблин 2
};
    
    // 1. Настройка
    init_ncurses();
    getmaxyx(stdscr, rows, cols);
    
    // 2. Подготовка данных (ДЕЛАЕМ ЭТО ОДИН РАЗ!)
    char map[rows][cols];
    generate_map(rows, cols, map);

    srand(time(NULL)); 

    // 3. Игровой цикл
    do {
        clear(); // Очищаем экран перед новым кадром

        draw_map(rows, cols, map);          // Рисуем стены и пол
        move_player(c, &px, &py, &gx, &gy, &score, rows, cols, map); // Считаем новое положение игрока
        draw_entities(px, py, gx, gy); // Рисуем персонажей
        draw_ui(px, py, score);                      // Рисуем интерфейс

        refresh(); // !!! КРИТИЧЕСКИ ВАЖНО: показываем результат на экране !!!

    } while ((c = getch()) != 27); // Ждем нажатия Esc
    
    // 4. Завершение
    endwin();
    return 0;
}

