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

int Pala1X, Pala1Y;
int Pala2X, Pala2Y;
int bolaX; 
int bolaY;
int dx, dy;

bool jugando = true;
bool pausado = false;
int puntaje1 = 0;
int puntaje2 = 0;
vector<bloque> bloques;

int numJugadoresGlobal = 1;
int ultimoJugador = 0;

string jugador1, jugador2;
map<string, int> puntajes;

// Mutex para proteger variables compartidas
pthread_mutex_t mutexPala = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBola = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPantalla = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBloques = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPausa = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t condPausa = PTHREAD_COND_INITIALIZER;

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
    pthread_mutex_lock(&mutexBloques);
    for (auto &b : bloques) {
        if (!b.destruido) {
            int filaColor = (b.y % 5) + 1; 
            attron(COLOR_PAIR(filaColor) | A_BOLD);
            mvprintw(b.y, b.x, "[##]");
            attroff(COLOR_PAIR(filaColor) | A_BOLD);
        }
    }
    pthread_mutex_unlock(&mutexBloques);
}

void DibujarPalas() {
    pthread_mutex_lock(&mutexPala);
    int x1 = Pala1X;
    int x2 = Pala2X;
    pthread_mutex_unlock(&mutexPala);

    attron(COLOR_PAIR(6) | A_BOLD);
    mvprintw(Pala1Y, x1, "======");
    attroff(COLOR_PAIR(6) | A_BOLD);

    if (numJugadoresGlobal == 2) {
        attron(COLOR_PAIR(9) | A_BOLD);
        mvprintw(Pala2Y, x2, "======");
        attroff(COLOR_PAIR(9) | A_BOLD);
    }
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
    mvprintw(7, 5, "Controles Jugador 1:");
    mvprintw(8, 7, "A : mover izquierda");
    mvprintw(9, 7, "D : mover derecha");
    mvprintw(11, 5, "Controles Jugador 2:");
    mvprintw(12, 7, "Flecha_Izq : mover izquierda");
    mvprintw(13, 7, "Flecha_Der : mover derecha");
    mvprintw(15, 5, "Otros:");
    mvprintw(16, 7, "P : pausar");
    mvprintw(17, 7, "Q : salir al menu");
    attroff(COLOR_PAIR(8) | A_BOLD);
    mvprintw(19, 5, "Presiona cualquier tecla para volver al menu...");
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

int MostrarSeleccionJugadores() {
    const char *opciones[] = { "1 Jugador", "2 Jugadores" };
    int seleccion = 0;
    int numOpciones = 2;

    clear();
    refresh();

    int width = 30, height = 8;
    int startx = (ancho / 2) - (width / 2);
    int starty = 5;
    WINDOW *selwin = newwin(height, width, starty, startx);
    keypad(selwin, TRUE);

    while (true) {
        werase(selwin);
        box(selwin, 0, 0);

        mvwprintw(selwin, 1, (width/2) - 9, "Seleccione jugadores");

        for (int i = 0; i < numOpciones; i++) {
            if (i == seleccion) {
                wattron(selwin, A_REVERSE | COLOR_PAIR(9));
                mvwprintw(selwin, 3 + i, 4, "%s", opciones[i]);
                wattroff(selwin, A_REVERSE | COLOR_PAIR(9));
            } else {
                mvwprintw(selwin, 3 + i, 4, "%s", opciones[i]);
            }
        }

        wrefresh(selwin);

        int tecla = wgetch(selwin);
        switch (tecla) {
            case KEY_UP: 
                seleccion = (seleccion - 1 + numOpciones) % numOpciones;
                break;
            case KEY_DOWN:
                seleccion = (seleccion + 1) % numOpciones;
                break;
            case '\n':
                werase(selwin);
                wrefresh(selwin);
                delwin(selwin);
                return seleccion + 1;
        }
    }
}

Estado MostrarGameOver(const string &jug1, int punt1, const string &jug2, int punt2) {
    const char *opciones[] = { "Reintentar", "Volver al menu" };
    int seleccion = 0;
    int numOpciones = 2;

    clear();
    refresh();

    int width = 40, height = 10;
    int startx = (ancho / 2) - (width / 2);
    int starty = 5;
    WINDOW *gameoverwin = newwin(height, width, starty, startx);
    keypad(gameoverwin, TRUE);

    while (true) {
        werase(gameoverwin);
        box(gameoverwin, 0, 0);

        wattron(gameoverwin, COLOR_PAIR(8) | A_BOLD);
        mvwprintw(gameoverwin, 1, (width/2) - 5, "GAME OVER");
        wattroff(gameoverwin, COLOR_PAIR(8) | A_BOLD);

        mvwprintw(gameoverwin, 3, 2, "Jugador 1: %s - Puntaje: %d", jug1.c_str(), punt1);
        if (!jug2.empty()) {
            mvwprintw(gameoverwin, 4, 2, "Jugador 2: %s - Puntaje: %d", jug2.c_str(), punt2);
        }

        for (int i = 0; i < numOpciones; i++) {
            if (i == seleccion) {
                wattron(gameoverwin, A_REVERSE | COLOR_PAIR(9));
                mvwprintw(gameoverwin, 6 + i, 4, "%s", opciones[i]);
                wattroff(gameoverwin, A_REVERSE | COLOR_PAIR(9));
            } else {
                mvwprintw(gameoverwin, 6 + i, 4, "%s", opciones[i]);
            }
        }

        wrefresh(gameoverwin);

        int tecla = wgetch(gameoverwin);
        switch (tecla) {
            case KEY_UP:
                seleccion = (seleccion - 1 + numOpciones) % numOpciones;
                break;
            case KEY_DOWN:
                seleccion = (seleccion + 1) % numOpciones;
                break;
            case '\n':
                werase(gameoverwin);
                wrefresh(gameoverwin);
                delwin(gameoverwin);
                if (seleccion == 0) return JUEGO;
                else return MENU;
        }
    }
}

// =======================
// Hilos del juego
// =======================

void* HiloInput(void* arg) {
    while (jugando) {
        pthread_mutex_lock(&mutexPantalla);
        int tecla = getch();
        pthread_mutex_unlock(&mutexPantalla);

        if (tecla != ERR) {
            if (tecla == 'p' || tecla == 'P') {
                pthread_mutex_lock(&mutexPausa);
                pausado = !pausado;
                if (!pausado) {
                    pthread_cond_broadcast(&condPausa);
                }
                pthread_mutex_unlock(&mutexPausa);
            } else if (tecla == 'q' || tecla == 'Q') {
                jugando = false;
                pthread_cond_broadcast(&condPausa);
                break;
            } else {
                pthread_mutex_lock(&mutexPala);
                if ((tecla == 'a' || tecla == 'A') && Pala1X > 1) Pala1X--;
                if ((tecla == 'd' || tecla == 'D') && Pala1X < ancho - 6) Pala1X++;
                if (numJugadoresGlobal == 2) {
                    if ((tecla == KEY_LEFT) && Pala2X > 1) Pala2X--;
                    if ((tecla == KEY_RIGHT) && Pala2X < ancho - 6) Pala2X++;
                }
                pthread_mutex_unlock(&mutexPala);
            }
        }
        usleep(1000);
    }
    return nullptr;
}

void* HiloPelota(void* arg) {
    while (jugando) {
        pthread_mutex_lock(&mutexPausa);
        while (pausado && jugando) {
            pthread_cond_wait(&condPausa, &mutexPausa);
        }
        pthread_mutex_unlock(&mutexPausa);

        if (!jugando) break;

        pthread_mutex_lock(&mutexBola);
        int bx = bolaX;
        int by = bolaY;
        int ddx = dx;
        int ddy = dy;
        pthread_mutex_unlock(&mutexBola);

        // Colisión con pala jugador 1
        pthread_mutex_lock(&mutexPala);
        int pala1Pos = Pala1X;
        int pala2Pos = Pala2X;
        pthread_mutex_unlock(&mutexPala);

        if (bx >= pala1Pos && bx <= pala1Pos + 6 && by == Pala1Y) {
            ddy = -ddy;
            ultimoJugador = 1;
        }

        // Colisión con pala jugador 2
        if (numJugadoresGlobal == 2) {
            if (bx >= pala2Pos && bx <= pala2Pos + 6 && by == Pala2Y + 1) {
                ddy = -ddy;
                ultimoJugador = 2;
            }
        }

        // Colisión con bloques
        pthread_mutex_lock(&mutexBloques);
        for (auto &b : bloques) {
            if (!b.destruido) {
                if (bx >= b.x && bx <= b.x + 3 && by == b.y + 1) {
                    b.destruido = true;
                    ddy = -ddy;

                    if (ultimoJugador == 1) puntaje1 += 10;
                    else if (ultimoJugador == 2) puntaje2 += 10;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&mutexBloques);

        // Actualizar posición
        bx += ddx;
        by += ddy;

        // Rebotes en paredes
        if (bx <= 1 || bx >= ancho - 1) ddx = -ddx;
        if (by <= 1) ddy = -ddy;
        if (by >= alto) {
            jugando = false;
            pthread_cond_broadcast(&condPausa);
        }

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

void* HiloDibujo(void* arg) {
    while (jugando) {
        pthread_mutex_lock(&mutexPantalla);
        clear();

        for (int x = 0; x < ancho; x++) {
            mvprintw(0, x, "_");
            mvprintw(alto, x, "_");
        }
        for (int y = 0; y <= alto; y++) {
            mvprintw(y, 0, "|");
            mvprintw(y, ancho, "|");
        }

        DibujarBloques();
        DibujarPalas();
        DibujarPelota();

        mvprintw(alto + 1, 5, "J1: %d   J2: %d", puntaje1, puntaje2);

        pthread_mutex_lock(&mutexPausa);
        if (pausado) {
            attron(COLOR_PAIR(2) | A_BOLD);
            mvprintw(alto / 2, (ancho / 2) - 5, "|| PAUSA ||");
            attroff(COLOR_PAIR(2) | A_BOLD);
        }
        pthread_mutex_unlock(&mutexPausa);

        refresh();
        pthread_mutex_unlock(&mutexPantalla);

        usleep(50000);
    }
    return nullptr;
}

// =======================
// Juego principal
// =======================
Estado IniciarJuego() {
    Pala1X = ancho / 2;
    Pala1Y = alto - 2;

    if (numJugadoresGlobal == 2) {
        Pala2X = ancho / 2;
        Pala2Y = alto - 2;
    }

    pthread_mutex_lock(&mutexBola);
    bolaX = ancho / 2 + 2;
    bolaY = alto - 3;
    dx = 1;
    dy = -1;
    pthread_mutex_unlock(&mutexBola);

    puntaje1 = 0;
    puntaje2 = 0;

    InicializarBloques();

    jugando = true;
    pausado = false;

    pthread_t hiloInput, hiloPelota, hiloDibujo;
    pthread_create(&hiloInput, nullptr, HiloInput, nullptr);
    pthread_create(&hiloPelota, nullptr, HiloPelota, nullptr);
    pthread_create(&hiloDibujo, nullptr, HiloDibujo, nullptr);

    pthread_join(hiloInput, nullptr);
    pthread_join(hiloPelota, nullptr);
    pthread_join(hiloDibujo, nullptr);

    if (numJugadoresGlobal == 1) {
        if (puntajes.find(jugador1) == puntajes.end() || puntaje1 > puntajes[jugador1]) {
            puntajes[jugador1] = puntaje1;
        }
    } else {
        if (puntajes.find(jugador1) == puntajes.end() || puntaje1 > puntajes[jugador1]) {
            puntajes[jugador1] = puntaje1;
        }
        if (puntajes.find(jugador2) == puntajes.end() || puntaje2 > puntajes[jugador2]) {
            puntajes[jugador2] = puntaje2;
        }   
    }

    Estado siguiente = MostrarGameOver(jugador1, puntaje1, jugador2, puntaje2);
    return siguiente;
}

// =======================
// Main
// =======================
int main() {
    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

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
            case JUEGO: {
                numJugadoresGlobal = MostrarSeleccionJugadores();

                nodelay(stdscr, FALSE);
                jugador1 = PedirNombreJugador();
                jugador2.clear();
                if (numJugadoresGlobal == 2)
                    jugador2 = PedirNombreJugador();

                nodelay(stdscr, TRUE);

                Estado siguiente = IniciarJuego();

                if (siguiente == JUEGO) {
                    jugador1.clear();
                    jugador2.clear();
                    estado = JUEGO;
                } else if (siguiente == MENU) {
                    estado = MENU;
                } else {
                    estado = siguiente;
                }
                break;
            }
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
    pthread_mutex_destroy(&mutexBloques);
    pthread_mutex_destroy(&mutexPausa);
    pthread_cond_destroy(&condPausa);

    return 0;
}