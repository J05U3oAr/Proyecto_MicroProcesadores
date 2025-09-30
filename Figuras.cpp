#include <ncurses.h>
#include <unistd.h> 
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <pthread.h>

using namespace std;

// =======================
// Estructuras y variables
// =======================
struct bloque {
    int x;
    int y;
    bool destruido;
};

const int ancho = 57;
const int alto = 20;

bool jugando = true;  
int puntaje = 0;       


int PalaX;
int PalaY;
int bolaX; 
int bolaY;
int dx, dy;
vector<bloque> bloques;

map<string, int> puntajes;
string jugadorActual;

// Mutex para proteger variables compartidas
pthread_mutex_t mutexPala = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBola = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPantalla = PTHREAD_MUTEX_INITIALIZER;

//variables de estado para el menú
enum Estado { MENU, INSTRUCCIONES, JUEGO, PUNTAJES, SALIR };

// =======================
// Funciones del juego
// =======================

void InicializarBloques() {
    bloques.clear(); 
    int X_inicial = 1;
    int Y_inicial = 1;
    int columnas = 14;
    int filas = 5;
    int espaciado = 4; 

    for (int r = 0; r < filas; r++) {
        for (int c = 0; c < columnas; c++) {
            bloque nuevoBloque;
            nuevoBloque.x = X_inicial + c * espaciado;
            nuevoBloque.y = Y_inicial + r;
            nuevoBloque.destruido = false;
            bloques.push_back(nuevoBloque);
        }
    }
}

void DibujarBloques() {
    for (auto &b : bloques) {
        if (!b.destruido) {
            int filaColor = (b.y % 5) + 1; 
            attron(COLOR_PAIR(filaColor) | A_BOLD);
            mvprintw(b.y, b.x, "[##]");
            attroff(COLOR_PAIR(filaColor) | A_BOLD);
        }
    }
}

void DibujarPala() {
    pthread_mutex_lock(&mutexPala);
    int x = PalaX;
    pthread_mutex_unlock(&mutexPala);

    attron(COLOR_PAIR(6) | A_BOLD);
    mvprintw(PalaY, x, "======");
    attroff(COLOR_PAIR(6) | A_BOLD);
}

void DibujarPelota() {
    pthread_mutex_lock(&mutexBola);
    int x = bolaX;
    int y = bolaY;
    pthread_mutex_unlock(&mutexBola);
    
    attron(COLOR_PAIR(7) | A_BOLD);
    mvprintw(y, x, "O");
    attroff(COLOR_PAIR(7) | A_BOLD);
}

string PedirNombreJugador() {
    echo();
    curs_set(TRUE);

    char buffer[50];
    clear();
    mvprintw(5, 10, "Ingresa tu nombre: ");
    getnstr(buffer, 49);

    noecho();
    curs_set(FALSE);

    return string(buffer);
}

// =======================
// Pantallas del menú
// =======================

Estado MostrarMenu() {
    const char *opciones[] = { 
        "Iniciar partida", 
        "Instrucciones", 
        "Puntajes destacados", 
        "Salir" 
    };
    int seleccion = 0;
    int numOpciones = 4;

    clear();
    refresh();

    int width = 40, height = 12;
    int startx = (ancho / 2) - (width / 2);
    int starty = 3;
    WINDOW *menuwin = newwin(height, width, starty, startx);
    keypad(menuwin, TRUE); 

    while (true) {
        werase(menuwin);  
        box(menuwin, 0, 0); 

        wattron(menuwin, COLOR_PAIR(8) | A_BOLD);
        mvwprintw(menuwin, 1, (width/2) - 10, "=== BREAKOUT ASCII ===");
        wattroff(menuwin, COLOR_PAIR(8) | A_BOLD);

        for (int i = 0; i < numOpciones; i++) {
            if (i == seleccion) {
                wattron(menuwin, A_REVERSE | COLOR_PAIR(9));
                mvwprintw(menuwin, 3 + i, 4, "%s", opciones[i]);
                wattroff(menuwin, A_REVERSE | COLOR_PAIR(9));
            } else {
                mvwprintw(menuwin, 3 + i, 4, "%s", opciones[i]);
            }
        }

        wattron(menuwin, A_DIM);
        mvwprintw(menuwin, height - 2, 2, "Usa flechas arriba, abajo y ENTER");
        wattroff(menuwin, A_DIM);

        wrefresh(menuwin);

        int tecla = wgetch(menuwin);
        switch (tecla) {
            case KEY_UP: 
                seleccion = (seleccion - 1 + numOpciones) % numOpciones;
                break;
            case KEY_DOWN:
                seleccion = (seleccion + 1) % numOpciones;
                break;
            case '\n': 
                werase(menuwin);
                wrefresh(menuwin);
                delwin(menuwin);
                clear(); 
                refresh();
                switch (seleccion) {
                    case 0: return JUEGO;
                    case 1: return INSTRUCCIONES;
                    case 2: return PUNTAJES;
                    case 3: return SALIR;
                }
        }
    }
}

void MostrarInstrucciones() {
    clear();
    attron(COLOR_PAIR(8) | A_BOLD);
    mvprintw(3, 5, "=== INSTRUCCIONES ===");
    mvprintw(5, 5, "Objetivo: Romper todos los bloques con la pelota.");
    mvprintw(7, 5, "Controles:");
    mvprintw(8, 7, "A o <- : mover pala a la izquierda");
    mvprintw(9, 7, "D o -> : mover pala a la derecha");
    mvprintw(10, 7, "P      : pausar el juego");
    mvprintw(11, 7, "Q      : salir al menu");
    attroff(COLOR_PAIR(8) | A_BOLD);
    mvprintw(13, 5, "Elementos del juego:");
    mvprintw(14, 7, "O      : pelota");
    mvprintw(15, 7, "====== : pala");
    mvprintw(16, 7, "[##]   : bloque");
    mvprintw(18, 5, "Presiona cualquier tecla para volver al menu...");
    refresh();
    
    flushinp();
    getch();
}

void MostrarPuntajes() {
    clear();
    mvprintw(3, 10, "=== PUNTAJES DESTACADOS ===");

    vector<pair<string,int>> lista(puntajes.begin(), puntajes.end());
    sort(lista.begin(), lista.end(), [](auto &a, auto &b) {
        return a.second > b.second;
    });

    int y = 5;
    for (auto &p : lista) {
        mvprintw(y++, 10, "%s - %d", p.first.c_str(), p.second);
    }

    if (lista.empty()) {
        mvprintw(5, 10, "No hay puntajes registrados.");
    }

    mvprintw(y + 2, 10, "Presiona cualquier tecla para volver al menu...");
    refresh();

    flushinp();
    getch();
}

void MostrarGameOver(int puntajeFinal) {
    clear();
    
    // Dibujar "GAME OVER" grande y en rojo parpadeante
    attron(COLOR_PAIR(1) | A_BOLD | A_BLINK);
    mvprintw(alto / 2 - 2, (ancho / 2) - 14, "  ____    _    __  __ _____");
    mvprintw(alto / 2 - 1, (ancho / 2) - 14, " / ___|  / \\  |  \\/  | ____|");
    mvprintw(alto / 2,     (ancho / 2) - 14, "| |  _  / _ \\ | |\\/| |  _|");
    mvprintw(alto / 2 + 1, (ancho / 2) - 14, "| |_| |/ ___ \\| |  | | |___");
    mvprintw(alto / 2 + 2, (ancho / 2) - 14, " \\____/_/   \\_\\_|  |_|_____|");
    
    mvprintw(alto / 2 + 4, (ancho / 2) - 14, "   ___  __     _______ ____");
    mvprintw(alto / 2 + 5, (ancho / 2) - 14, "  / _ \\ \\ \\   / / ____|  _ \\");
    mvprintw(alto / 2 + 6, (ancho / 2) - 14, " | | | | \\ \\ / /|  _| | |_) |");
    mvprintw(alto / 2 + 7, (ancho / 2) - 14, " | |_| |  \\ V / | |___|  _ <");
    mvprintw(alto / 2 + 8, (ancho / 2) - 14, "  \\___/    \\_/  |_____|_| \\_\\");
    attroff(COLOR_PAIR(1) | A_BOLD | A_BLINK);
    
    // Mostrar puntaje final
    attron(COLOR_PAIR(8) | A_BOLD);
    mvprintw(alto / 2 + 10, (ancho / 2) - 10, "Puntaje final: %d", puntajeFinal);
    attroff(COLOR_PAIR(8) | A_BOLD);
    
    // Instrucción
    attron(A_DIM);
    mvprintw(alto / 2 + 12, (ancho / 2) - 20, "Presiona Q para volver al menu...");
    attroff(A_DIM);
    
    refresh();
    
    // Esperar a que presione la tecla Q
    nodelay(stdscr, FALSE);
    flushinp();
    int tecla;
    while (true) {
        tecla = getch();
        if (tecla == 'q' || tecla == 'Q') {
            break;
        }
    }
    nodelay(stdscr, TRUE);
}

// =======================
// Hilo de movimiento de la pala
// =======================
void* HiloPala(void* arg) {
    while (jugando) {
        pthread_mutex_lock(&mutexPantalla);
        int tecla = getch();
        pthread_mutex_unlock(&mutexPantalla);
        
        if (tecla != ERR) {
            pthread_mutex_lock(&mutexPantalla);
            while (getch() != ERR); // limpiar buffer
            pthread_mutex_unlock(&mutexPantalla);

            pthread_mutex_lock(&mutexPala);
            if ((tecla == 'a' || tecla == 'A') && PalaX > 1) PalaX--;
            if ((tecla == 'd' || tecla == 'D') && PalaX < ancho - 6) PalaX++;
            pthread_mutex_unlock(&mutexPala);

            if (tecla == 'q' || tecla == 'Q') {
                jugando = false;
                break;
            }
        }
        usleep(5000);
    }
    return nullptr;
}

// =======================
// Hilo de lógica de la pelota
// =======================
void* HiloPelota(void* arg) {
    while (jugando) {
        // Leer posición actual de la bola
        pthread_mutex_lock(&mutexBola);
        int bx = bolaX;
        int by = bolaY;
        int ddx = dx;
        int ddy = dy;
        pthread_mutex_unlock(&mutexBola);

        // Colisión con pala
        pthread_mutex_lock(&mutexPala);
        int palaPos = PalaX;
        pthread_mutex_unlock(&mutexPala);
        
        if (bx >= palaPos && bx <= palaPos + 6 && by == PalaY) {
            ddy = -ddy;
        }

        // Colisión con bloques
        for (auto &b : bloques) {
            if (!b.destruido) {
                if (bx >= b.x && bx <= b.x + 3 && by == b.y + 1) {
                    b.destruido = true;
                    ddy = -ddy;
                    puntaje += 10;
                    break;
                }
            }
        }

        // Actualizar posición
        bx += ddx;
        by += ddy;

        // Rebotes en paredes
        if (bx <= 1 || bx >= ancho - 1) ddx = -ddx;
        if (by <= 1) ddy = -ddy;
        if (by >= alto) {
            jugando = false;
        }

        // Guardar nueva posición y dirección
        pthread_mutex_lock(&mutexBola);
        bolaX = bx;
        bolaY = by;
        dx = ddx;
        dy = ddy;
        pthread_mutex_unlock(&mutexBola);

        usleep(120000);
    }
    return nullptr;
}

// =======================
// Hilo de dibujo de la pantalla
// =======================
void* HiloDibujo(void* arg) {
    while (jugando) {
        pthread_mutex_lock(&mutexPantalla);
        clear();

        // para dibujar bordes
        for (int x = 0; x < ancho; x++) {
            mvprintw(0, x, "_");
            mvprintw(alto, x, "_");
        }
        for (int y = 0; y <= alto; y++) {
            mvprintw(y, 0, "|");
            mvprintw(y, ancho, "|");
        }

        // se dibujan los elementos del juego
        DibujarBloques();
        DibujarPala();
        DibujarPelota();

        // se muestra el puntaje y el jugador
        mvprintw(alto + 1, 5, "Jugador: %s  Puntaje: %d", 
                 jugadorActual.c_str(), puntaje);

        refresh();
        pthread_mutex_unlock(&mutexPantalla);

        usleep(50000); // ~20 fps
    }
    return nullptr;
}


// =======================
// Juego principal
// =======================
void IniciarJuego() {
    PalaX = ancho / 2;
    PalaY = alto - 2;
    
    pthread_mutex_lock(&mutexBola);
    bolaX = ancho / 2 + 2;
    bolaY = alto - 3;
    dx = 1;
    dy = -1;
    pthread_mutex_unlock(&mutexBola);

    InicializarBloques();

    jugando = true;
    puntaje = 0;

    pthread_t hiloPala, hiloPelota, hiloDibujo;
    pthread_create(&hiloPala, nullptr, HiloPala, nullptr);
    pthread_create(&hiloPelota, nullptr, HiloPelota, nullptr);
    pthread_create(&hiloDibujo, nullptr, HiloDibujo, nullptr);

    // Esperar a que termine el juego
    pthread_join(hiloPala, nullptr);
    pthread_join(hiloPelota, nullptr);
    pthread_join(hiloDibujo, nullptr);

    // Guardar puntaje
    if (puntajes.find(jugadorActual) == puntajes.end() || puntaje > puntajes[jugadorActual]) {
        puntajes[jugadorActual] = puntaje;
    }

    // Mostrar pantalla de Game Over
    MostrarGameOver(puntaje);
}


// =======================
// Main
// =======================
int main() {
    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);
    nodelay(stdscr, TRUE); // entrada no bloqueante

    Estado estado = MENU;

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_RED,     COLOR_BLACK);
        init_pair(2, COLOR_YELLOW,  COLOR_BLACK);
        init_pair(3, COLOR_GREEN,   COLOR_BLACK);
        init_pair(4, COLOR_CYAN,    COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_WHITE,   COLOR_BLACK);
        init_pair(7, COLOR_BLUE,    COLOR_BLACK);
        init_pair(8, COLOR_CYAN,    COLOR_BLACK);
        init_pair(9, COLOR_YELLOW,  COLOR_BLACK);
    }

    while (estado != SALIR) {
        switch (estado) {
            case MENU:
                nodelay(stdscr, FALSE);
                estado = MostrarMenu();
                break;
            case INSTRUCCIONES:
                nodelay(stdscr, FALSE);
                MostrarInstrucciones();
                estado = MENU;
                break;
            case JUEGO:
                jugadorActual = PedirNombreJugador();
                nodelay(stdscr, TRUE);
                IniciarJuego();
                estado = MENU;
                break;
            case PUNTAJES:
                nodelay(stdscr, FALSE);
                MostrarPuntajes();
                estado = MENU;
                break;
            default:
                estado = SALIR;
        }
    }

    endwin();
    pthread_mutex_destroy(&mutexPala);
    pthread_mutex_destroy(&mutexBola);
    pthread_mutex_destroy(&mutexPantalla);
    return 0;
}