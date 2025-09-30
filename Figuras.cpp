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
// En esta sección presente, se definen las estructuras de datos y las variables globales
// utilizadas en el juego. a continuación se describen en una lista los slementos clabe que hay dentro:
// - Se define la estructura "bloque", que representa los bloques que deben destruirse.
// - Se configuran constantes de dimensiones del campo de juego.
// - Se almacenan posiciones de palas y pelota, su movimiento, puntajes y estados del juego.
// - Se incluyen mapas para guardar los puntajes por jugador y por modo de juego.
// - Se definen mutex y variables de condición para la correcta sincronización entre hilos.

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
int modoVelocidadGlobal = 0; // 0 = media, 1 = rápida
int ultimoJugador = 0;

string jugador1, jugador2;
map<string, int> puntajesMedia;   // 1 jugador velocidad media
map<string, int> puntajesRapida;  // 1 jugador velocidad rápida
map<string, int> puntajes2Jug;    // 2 jugadores

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
// Esta sección contiene las funciones principales para la mecánica del juego:
// - Inicialización y dibujo correcto de todos los bloques, palas y pelota.
// - Captura del nombre del jugador.
// Estas funciones son invocadas tanto por los hilos de ejecución como por la lógica
// del juego principal para actualizar la pantalla y el estado de los elementos.

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
// Aquí se definen las diferentes pantallas que componen el menú principal:
// - Menú inicial con opciones de juego, instrucciones, puntajes y salir.
// - Pantalla de instrucciones con controles y objetivos.
// - Visualización de puntajes destacados para cada modo de juego.
// - Selección de cantidad de jugadores y velocidad de la partida.
// - Pantalla de "Game Over" con las opciones de reintentar o volver al menú.
// Estas pantallas utilizan la librería ncurses para mostrar interfaces interactivas para generar una mejor experiencia de usuario,
// cada pantalla retorna el estado siguiente del juego según la interacción del usuario.

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
            case '\n': goto MostrarLista;
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

int MostrarSeleccionVelocidad() {
    const char *opciones[] = { "Velocidad media", "Velocidad rapida" };
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
                return seleccion;
        }
    }
}

Estado MostrarGameOver(const string &jug1, int punt1, const string &jug2, int punt2) {
    const char *opciones[] = { "Reintentar", "Volver al menu" };
    int seleccion = 0;
    int numOpciones = 2;

    clear();
    refresh();

    int width = 40, height = 18;
    int startx = (ancho / 2) - (width / 2);
    int starty = 2;
    WINDOW *gameoverwin = newwin(height, width, starty, startx);
    keypad(gameoverwin, TRUE);

    while (true) {
        werase(gameoverwin);
        box(gameoverwin, 0, 0);

        wattron(gameoverwin, COLOR_PAIR(1) | A_BOLD);
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
        wattroff(gameoverwin, COLOR_PAIR(1) | A_BOLD);

        mvwprintw(gameoverwin, 12, 2, "Jugador 1: %s - Puntaje: %d", jug1.c_str(), punt1);
        if (!jug2.empty())
            mvwprintw(gameoverwin, 13, 2, "Jugador 2: %s - Puntaje: %d", jug2.c_str(), punt2);

        for (int i = 0; i < numOpciones; i++) {
            if (i == seleccion) {
                wattron(gameoverwin, A_REVERSE | COLOR_PAIR(9));
                mvwprintw(gameoverwin, 15 + i, 4, "%s", opciones[i]);
                wattroff(gameoverwin, A_REVERSE | COLOR_PAIR(9));
            } else {
                mvwprintw(gameoverwin, 15 + i, 4, "%s", opciones[i]);
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
// En esta sección se encuentran las funciones que representan cada hilo de ejecución:
// - HiloInput: gestiona la entrada del teclado para controlar palas, pausa o salir.
// - HiloPelota: gestiona la física de la pelota, sus rebotes y colisiones.
// - HiloDibujo: refresca constantemente la pantalla con los elementos actualizados.
// - HiloBloques: maneja las colisiones con los bloques y actualiza los puntajes.
// Gracias a los mutex y variables de condición, se asegura la sincronización
// entre hilos evitando conflictos de acceso a recursos compartidos.

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
                pthread_mutex_lock(&mutexPausa);
                bool estaPausado = pausado;
                pthread_mutex_unlock(&mutexPausa);
                
                if (!estaPausado) {
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

        pthread_mutex_lock(&mutexPala);
        int pala1Pos = Pala1X;
        int pala2Pos = Pala2X;
        pthread_mutex_unlock(&mutexPala);

        if (bx >= pala1Pos && bx <= pala1Pos + 6 && by == Pala1Y) {
            ddy = -ddy;
            ultimoJugador = 1;
        }

        if (numJugadoresGlobal == 2) {
            if (bx >= pala2Pos && bx <= pala2Pos + 6 && by == Pala2Y) {
                ddy = -ddy;
                ultimoJugador = 2;
            }
        }

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

        bx += ddx;
        by += ddy;

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

        int velocidad;
        if (numJugadoresGlobal == 2) {
            velocidad = 165500; // Más lenta para 2 jugadores
        } else if (modoVelocidadGlobal == 1) {
            velocidad = 80000;  // Rápida para 1 jugador
        } else {
            velocidad = 130000; // Media para 1 jugador
        }
        usleep(velocidad);
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

void* HiloBloques(void* arg) {
    while (jugando) {
        pthread_mutex_lock(&mutexBola);
        int bx = bolaX;
        int by = bolaY;
        int ddx = dx;
        int ddy = dy;
        pthread_mutex_unlock(&mutexBola);

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

        pthread_mutex_lock(&mutexBola);
        dx = ddx;
        dy = ddy;
        bolaY = by;
        pthread_mutex_unlock(&mutexBola);

        usleep(35000);
    }
    return nullptr;
}

// =======================
// Juego principal
// =======================}
// La función IniciarJuego() prepara y ejecuta una partida:
// - Inicializa posiciones de palas, pelota y bloques.
// - Configura las variables de estado del juego (puntajes, pausa, etc.).
// - Crea y lanza los hilos principales (input, pelota, dibujo y bloques).
// - Espera a que los hilos terminen y actualiza puntajes globales al finalizar.
// Finalmente, invoca la pantalla de "Game Over" para decidir el siguiente estado.

Estado IniciarJuego() {
    Pala1X = ancho / 2;
    Pala1Y = alto - 2;

    if (numJugadoresGlobal == 2) {
        Pala2X = ancho / 4;
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

    pthread_t hiloInput, hiloPelota, hiloDibujo, hiloBloques;
    pthread_create(&hiloInput, nullptr, HiloInput, nullptr);
    pthread_create(&hiloPelota, nullptr, HiloPelota, nullptr);
    pthread_create(&hiloDibujo, nullptr, HiloDibujo, nullptr);
    pthread_create(&hiloBloques, nullptr, HiloBloques, nullptr);

    pthread_join(hiloInput, nullptr);
    pthread_join(hiloPelota, nullptr);
    pthread_join(hiloDibujo, nullptr);
    pthread_join(hiloBloques, nullptr);

    if (numJugadoresGlobal == 1) {
        if (modoVelocidadGlobal == 0) {
            if (puntajesMedia.find(jugador1) == puntajesMedia.end() || puntaje1 > puntajesMedia[jugador1]) {
                puntajesMedia[jugador1] = puntaje1;
            }
        } else {
            if (puntajesRapida.find(jugador1) == puntajesRapida.end() || puntaje1 > puntajesRapida[jugador1]) {
                puntajesRapida[jugador1] = puntaje1;
            }
        }
    } else {
        if (puntajes2Jug.find(jugador1) == puntajes2Jug.end() || puntaje1 > puntajes2Jug[jugador1]) {
            puntajes2Jug[jugador1] = puntaje1;
        }
        if (puntajes2Jug.find(jugador2) == puntajes2Jug.end() || puntaje2 > puntajes2Jug[jugador2]) {
            puntajes2Jug[jugador2] = puntaje2;
        }   
    }

    Estado siguiente = MostrarGameOver(jugador1, puntaje1, jugador2, puntaje2);
    return siguiente;
}

// =======================
// Main
// =======================
// Punto de entrada del programa.
// - Inicializa la librería ncurses y configura la interfaz (colores, teclado, etc.).
// - Controla el bucle principal del juego, cambiando de estado según las pantallas
//   (menú, instrucciones, partida, puntajes).
// - Se mantiene en ejecución hasta que el jugador seleccione la opción de salir.

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

                if (numJugadoresGlobal == 1) {
                    modoVelocidadGlobal = MostrarSeleccionVelocidad();
                }

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