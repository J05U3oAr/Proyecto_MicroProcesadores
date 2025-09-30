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

int puntaje1 = 0;
int puntaje2 = 0;
vector<bloque> bloques;

int numJugadoresGlobal = 1;
int ultimoJugador = 0; // 0 = ninguno, 1 = jugador1, 2 = jugador2

string jugador1, jugador2;
map<string, int> puntajesMedia;   // para un jugador, velocidad media
map<string, int> puntajesRapida;  // para un jugador, velocidad rápida
map<string, int> puntajes2Jug;    // para 2 jugadores

// Mutex para proteger la pala y la pantalla
pthread_mutex_t mutexPala = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPantalla = PTHREAD_MUTEX_INITIALIZER;

//variables de estado para el menú
enum Estado { MENU, INSTRUCCIONES, JUEGO, PUNTAJES, SALIR };

// Variables de control
bool juegoPausado = false;
bool salirJuego = false;


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

void DibujarPalas() {
    pthread_mutex_lock(&mutexPala);
    int x1 = Pala1X;
    int x2 = Pala2X;
    pthread_mutex_unlock(&mutexPala);

    // Jugador 1
    attron(COLOR_PAIR(6) | A_BOLD);
    mvprintw(Pala1Y, x1, "======");
    attroff(COLOR_PAIR(6) | A_BOLD);

    // Jugador 2
    if (numJugadoresGlobal == 2) {
        attron(COLOR_PAIR(9) | A_BOLD);
        mvprintw(Pala2Y, x2, "======");
        attroff(COLOR_PAIR(9) | A_BOLD);
    }
}

void DibujarPelota() {
    attron(COLOR_PAIR(7) | A_BOLD);
    mvprintw(bolaY, bolaX, "O");
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
    const char *opcionesModo[] = { "1 Jugador - Media", "1 Jugador - Rapida", "2 Jugadores" };
    int seleccion = 0;
    int numOpciones = 3;

    clear();
    refresh();

    int width = 40, height = 12;
    int startx = (ancho / 2) - (width / 2);
    int starty = 3;
    WINDOW *win = newwin(height, width, starty, startx);
    keypad(win, TRUE);

    // Menú de selección de modo de puntajes
    while (true) {
        werase(win);
        box(win, 0, 0);

        wattron(win, COLOR_PAIR(8) | A_BOLD);
        mvwprintw(win, 1, (width/2) - 8, "=== PUNTAJES ===");
        wattroff(win, COLOR_PAIR(8) | A_BOLD);

        for (int i = 0; i < numOpciones; i++) {
            if (i == seleccion) {
                wattron(win, A_REVERSE | COLOR_PAIR(9));
                mvwprintw(win, 3 + i, 4, "%s", opcionesModo[i]);
                wattroff(win, A_REVERSE | COLOR_PAIR(9));
            } else {
                mvwprintw(win, 3 + i, 4, "%s", opcionesModo[i]);
            }
        }

        wattron(win, A_DIM);
        mvwprintw(win, height - 2, 2, "Usa flechas arriba, abajo y ENTER");
        wattroff(win, A_DIM);

        wrefresh(win);

        int tecla = wgetch(win);
        switch (tecla) {
            case KEY_UP: seleccion = (seleccion - 1 + numOpciones) % numOpciones; break;
            case KEY_DOWN: seleccion = (seleccion + 1) % numOpciones; break;
            case '\n': goto MostrarLista; // salir del menú
        }
    }

MostrarLista:
    werase(win);
    wrefresh(win);
    delwin(win);
    clear();

    vector<pair<string,int>> lista;
    if (seleccion == 0) lista = vector<pair<string,int>>(puntajesMedia.begin(), puntajesMedia.end());
    else if (seleccion == 1) lista = vector<pair<string,int>>(puntajesRapida.begin(), puntajesRapida.end());
    else lista = vector<pair<string,int>>(puntajes2Jug.begin(), puntajes2Jug.end());

    sort(lista.begin(), lista.end(), [](auto &a, auto &b) { return a.second > b.second; });

    int y = 5;
    mvprintw(3, 10, "=== PUNTAJES DESTACADOS ===");
    for (auto &p : lista) {
        mvprintw(y++, 10, "%s - %d", p.first.c_str(), p.second);
    }

    if (lista.empty()) mvprintw(5, 10, "No hay puntajes registrados.");

    mvprintw(y + 2, 10, "Presiona cualquier tecla para volver al menu...");
    refresh();
    flushinp();
    getch();
}

// =======================
// Pantalla de selección de jugadores
// =======================

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
                return seleccion + 1; // devuelve 1 o 2
        }
    }
}
// =======================
// Pantalla de selección de modo un jugador
// =======================
int modoUnJugador() {
    const char *opciones[] = { "Velocidad media", "Velocidad rápida" };
    int seleccion = 0;
    int numOpciones = 2;

    clear();
    refresh();

    int width = 30, height = 8;
    int startx = (ancho / 2) - (width / 2);
    int starty = 5;
    WINDOW *win = newwin(height, width, starty, startx);
    keypad(win, TRUE);

    while (true) {
        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 1, (width/2) - 7, "Seleccione velocidad");

        for (int i = 0; i < numOpciones; i++) {
            if (i == seleccion) {
                wattron(win, A_REVERSE | COLOR_PAIR(9));
                mvwprintw(win, 3 + i, 4, "%s", opciones[i]);
                wattroff(win, A_REVERSE | COLOR_PAIR(9));
            } else {
                mvwprintw(win, 3 + i, 4, "%s", opciones[i]);
            }
        }

        wrefresh(win);

        int tecla = wgetch(win);
        switch (tecla) {
            case KEY_UP:
                seleccion = (seleccion - 1 + numOpciones) % numOpciones;
                break;
            case KEY_DOWN:
                seleccion = (seleccion + 1) % numOpciones;
                break;
            case '\n':
                werase(win);
                wrefresh(win);
                delwin(win);
                return seleccion; // 0 = media, 1 = rápida
        }
    }
}


// =======================
// Pantalla de Game Over
// =======================
Estado MostrarGameOver(const string &jugador1, int puntaje1, const string &jugador2, int puntaje2) {
    const char *opciones[] = { "Reintentar", "Volver al menú" };
    int seleccion = 0;
    int numOpciones = 2;

    clear();
    refresh();

    int width = 40, height = 18;
    int startx = (ancho / 2) - (width / 2);
    int starty = 5;
    WINDOW *gameoverwin = newwin(height, width, starty, startx);
    keypad(gameoverwin, TRUE);

    while (true) {
        werase(gameoverwin);
        box(gameoverwin, 0, 0);

        // Dibujar "GAME OVER" grande dentro de la ventana
        wattron(gameoverwin, COLOR_PAIR(1) | A_BOLD | A_BLINK);
        mvwprintw(gameoverwin, 1, 5, "  ____    _    __  __ _____");
        mvwprintw(gameoverwin, 2, 5, " / ___|  / \\  |  \\/  | ____|");
        mvwprintw(gameoverwin, 3, 5, "| |  _  / _ \\ | |\\/| |  _|");
        mvwprintw(gameoverwin, 4, 5, "| |_| |/ ___ \\| |  | | |___");
        mvwprintw(gameoverwin, 5, 5, " \\____/_/   \\_\\_|  |_|_____|");
        mvwprintw(gameoverwin, 6, 5, "   ___  __     _______ ____");
        mvwprintw(gameoverwin, 7, 5, "  / _ \\ \\ \\   / / ____|  _ \\");
        mvwprintw(gameoverwin, 8, 5, " | | | | \\ \\ / /|  _| | |_) |");
        mvwprintw(gameoverwin, 9, 5, " | |_| |  \\ V / | |___|  _ <");
        mvwprintw(gameoverwin,10, 5, "  \\___/    \\_/  |_____|_| \\_\\");
        wattroff(gameoverwin, COLOR_PAIR(1) | A_BOLD | A_BLINK);

        // Mostrar puntajes
        mvwprintw(gameoverwin, 12, 2, "Jugador 1: %s - Puntaje: %d", jugador1.c_str(), puntaje1);
        if (!jugador2.empty())
            mvwprintw(gameoverwin, 13, 2, "Jugador 2: %s - Puntaje: %d", jugador2.c_str(), puntaje2);

        // Opciones
        for (int i = 0; i < numOpciones; i++) {
            if (i == seleccion) {
                wattron(gameoverwin, A_REVERSE | COLOR_PAIR(9));
                mvwprintw(gameoverwin, 15 + i, 2, "%s", opciones[i]);
                wattroff(gameoverwin, A_REVERSE | COLOR_PAIR(9));
            } else {
                mvwprintw(gameoverwin, 15 + i, 2, "%s", opciones[i]);
            }
        }

        wrefresh(gameoverwin);

        int tecla = wgetch(gameoverwin);
        switch (tecla) {
            case KEY_UP: seleccion = (seleccion - 1 + numOpciones) % numOpciones; break;
            case KEY_DOWN: seleccion = (seleccion + 1) % numOpciones; break;
            case '\n':
                werase(gameoverwin);
                wrefresh(gameoverwin);
                delwin(gameoverwin);
                return (seleccion == 0) ? JUEGO : MENU;
        }
    }

}

// =======================
// Hilos de movimiento de las palas
// =======================
void* HiloPala1(void* arg) {
    while (true) {
        pthread_mutex_lock(&mutexPantalla);
        int tecla = getch();
        pthread_mutex_unlock(&mutexPantalla);

        if (tecla != ERR) {
            pthread_mutex_lock(&mutexPantalla);
            while (getch() != ERR); // limpiar buffer
            pthread_mutex_unlock(&mutexPantalla);
            
            pthread_mutex_lock(&mutexPala);
            if ((tecla == 'a' || tecla == 'A') && Pala1X > 1) Pala1X--;
            if ((tecla == 'd' || tecla == 'D') && Pala1X < ancho - 6) Pala1X++;
            pthread_mutex_unlock(&mutexPala);

            if (tecla == 'p' || tecla == 'P') juegoPausado = !juegoPausado;
            if (tecla == 'q' || tecla == 'Q') salirJuego = true;
        }
        usleep(5000);
    }
    return nullptr;
}
void* HiloPala2(void* arg) {
    while (true) {
        pthread_mutex_lock(&mutexPantalla);
        int tecla = getch();
        pthread_mutex_unlock(&mutexPantalla);

        if (tecla != ERR) {
            pthread_mutex_lock(&mutexPantalla);
            while (getch() != ERR); // limpiar buffer
            pthread_mutex_unlock(&mutexPantalla);

            pthread_mutex_lock(&mutexPala);
            if (numJugadoresGlobal == 2) {
                if ((tecla == KEY_LEFT) && Pala2X > 1) Pala2X--;
                if ((tecla == KEY_RIGHT) && Pala2X < ancho - 6) Pala2X++;
            }
            pthread_mutex_unlock(&mutexPala);

            if (tecla == 'p' || tecla == 'P') juegoPausado = !juegoPausado;
            if (tecla == 'q' || tecla == 'Q') salirJuego = true;
        }
        usleep(5000);
    }
    return nullptr;
}

// =======================
// Juego principal
// =======================
Estado IniciarJuego(int modo) {
    // Jugador 1
    Pala1X = ancho / 2;
    Pala1Y = alto - 2;

    // Jugador 2
    if (numJugadoresGlobal == 2) {
        Pala2X = ancho / 2;
        Pala2Y = alto - 2;
    }

    // Pelota
    bolaX = ancho / 2 + 2;
    bolaY = alto - 3;

    // Reiniciar puntajes
    puntaje1 = 0;
    puntaje2 = 0;

    InicializarBloques();

    int dx = 1;
    int dy = -1; 
    bool jugando = true; 
    int puntaje = 0;

    pthread_t hiloPala1, hiloPala2;
    pthread_create(&hiloPala1, nullptr, HiloPala1, nullptr);
    pthread_create(&hiloPala2, nullptr, HiloPala2, nullptr);

    while (jugando) {
        if (salirJuego) break;

        while (juegoPausado) {
            usleep(100000); // espera mientras está pausado
            pthread_mutex_lock(&mutexPantalla);
            mvprintw(alto / 2, ancho / 2 - 5, "== PAUSA ==");
            refresh();
            pthread_mutex_unlock(&mutexPantalla);
        }

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

        refresh();
        pthread_mutex_unlock(&mutexPantalla);

        // Colisión con pala
        pthread_mutex_lock(&mutexPala);
        int pala1Pos = Pala1X;
        int pala2Pos = Pala2X;
        pthread_mutex_unlock(&mutexPala);

        // Jugador 1
        if (bolaX >= pala1Pos && bolaX <= pala1Pos + 6 && bolaY == Pala1Y) {
            dy = -dy;
            ultimoJugador = 1;
        }

        // Jugador 2
        if (bolaX >= pala2Pos && bolaX <= pala2Pos + 6 && bolaY == Pala2Y + 1) {
            dy = -dy;
            ultimoJugador = 2;
        }

        // Colisión con bloques
        for (auto &b : bloques) {
            if (!b.destruido) {
                if (bolaX >= b.x && bolaX <= b.x + 3 && bolaY == b.y + 1) {
                    b.destruido = true;
                    dy = -dy;

                    if (ultimoJugador == 1) puntaje1 += 10;
                    else if (ultimoJugador == 2) puntaje2 += 10;
                }
            }
        }

        bolaX += dx;
        bolaY += dy;

        if (bolaX <= 1 || bolaX >= ancho - 1) dx = -dx;
        if (bolaY <= 1) dy = -dy;
        //if (bolaY >= alto - 1) dy = -dy;
        if (bolaY >= alto) jugando = false;

        int velocidadPelota = 105000; // predeterminado
        if (numJugadoresGlobal == 1) {
            if (modo == 0) velocidadPelota = 105000; // media
            else velocidadPelota = 50000;           // rápida
        }
        usleep(velocidadPelota);
    }

    pthread_cancel(hiloPala1);
    pthread_cancel(hiloPala2);
    pthread_join(hiloPala1, nullptr);
    pthread_join(hiloPala2, nullptr);

    if (numJugadoresGlobal == 1) {
        if (modo == 0) { // media
            if (puntajesMedia.find(jugador1) == puntajesMedia.end() || puntaje1 > puntajesMedia[jugador1])
                puntajesMedia[jugador1] = puntaje1;
        } else { // rápida
            if (puntajesRapida.find(jugador1) == puntajesRapida.end() || puntaje1 > puntajesRapida[jugador1])
            puntajesRapida[jugador1] = puntaje1;
        }
    } else { // 2 jugadores
        if (puntajes2Jug.find(jugador1) == puntajes2Jug.end() || puntaje1 > puntajes2Jug[jugador1])
            puntajes2Jug[jugador1] = puntaje1;
        if (puntajes2Jug.find(jugador2) == puntajes2Jug.end() || puntaje2 > puntajes2Jug[jugador2])
            puntajes2Jug[jugador2] = puntaje2;
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
            case JUEGO: {
                numJugadoresGlobal = MostrarSeleccionJugadores();

                nodelay(stdscr, FALSE);
                jugador1 = PedirNombreJugador();
                jugador2.clear();
                if (numJugadoresGlobal == 2)
                    jugador2 = PedirNombreJugador();

                nodelay(stdscr, TRUE);
                int modo = 0;
                if (numJugadoresGlobal == 1) {
                    modo = modoUnJugador();
                }

                Estado siguiente = IniciarJuego(modo);

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
    pthread_mutex_destroy(&mutexPantalla);

    return 0;
}