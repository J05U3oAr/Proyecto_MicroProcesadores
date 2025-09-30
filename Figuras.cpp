#include <ncurses.h>
#include <unistd.h> 
#include <vector>
#include <map>
#include <string>
#include <algorithm>

using namespace std;

// Estructura y variables para movimiento y desarrollo del juego
struct bloque {
    int x;
    int y;
    bool destruido;
};

const int ancho = 57;
const int alto = 20;

int PalaX;
int PalaY;
int bolaX; 
int bolaY;
vector<bloque> bloques;

map<string, int> puntajes;
string jugadorActual;

//variables de estado para el menú
enum Estado { MENU, INSTRUCCIONES, JUEGO, PUNTAJES, SALIR };

// Funciones del juego

// Inicializa bloques en filas
void InicializarBloques() {
    bloques.clear(); // reiniciar bloques
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
            int filaColor = (b.y % 5) + 1; // alterna entre 1..5
            attron(COLOR_PAIR(filaColor) | A_BOLD);
            mvprintw(b.y, b.x, "[##]");
            attroff(COLOR_PAIR(filaColor) | A_BOLD);
        }
    }
}

void DibujarPala() {
    attron(COLOR_PAIR(6) | A_BOLD);
    mvprintw(PalaY, PalaX, "======");
    attroff(COLOR_PAIR(6) | A_BOLD);
}

void DibujarPelota() {
    attron(COLOR_PAIR(7) | A_BOLD);
    mvprintw(bolaY, bolaX, "O");
    attroff(COLOR_PAIR(7) | A_BOLD);
}

string PedirNombreJugador() {
    echo(); // habilitar entrada visible
    curs_set(TRUE);

    char buffer[50];
    clear();
    mvprintw(5, 10, "Ingresa tu nombre: ");
    getnstr(buffer, 49);

    noecho();
    curs_set(FALSE);

    return string(buffer);
}

// Pantallas del menú

Estado MostrarMenu() {
    const char *opciones[] = { 
        "Iniciar partida", 
        "Instrucciones", 
        "Puntajes destacados", 
        "Salir" 
    };
    int seleccion = 0;
    int numOpciones = 4;

    // SOLUCIÓN: Limpiar pantalla principal al entrar al menú
    clear();
    refresh();

    // Crear ventana del menú
    int width = 40, height = 12;
    int startx = (ancho / 2) - (width / 2);
    int starty = 3;
    WINDOW *menuwin = newwin(height, width, starty, startx);
    keypad(menuwin, TRUE); // habilitar flechas

    while (true) {
        werase(menuwin);   // limpiar ventana
        box(menuwin, 0, 0); // redibujar borde

        // Título
        wattron(menuwin, COLOR_PAIR(8) | A_BOLD);
        mvwprintw(menuwin, 1, (width/2) - 10, "=== BREAKOUT ASCII ===");
        wattroff(menuwin, COLOR_PAIR(8) | A_BOLD);

        // Opciones
        for (int i = 0; i < numOpciones; i++) {
            if (i == seleccion) {
                wattron(menuwin, A_REVERSE | COLOR_PAIR(9));
                mvwprintw(menuwin, 3 + i, 4, "%s", opciones[i]);
                wattroff(menuwin, A_REVERSE | COLOR_PAIR(9));
            } else {
                mvwprintw(menuwin, 3 + i, 4, "%s", opciones[i]);
            }
        }

        // Instrucción
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
            case '\n': // enter
                // limpiar menú antes de salir
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
    attroff(COLOR_PAIR(9) | A_BOLD);
    mvprintw(13, 5, "Elementos del juego:");
    mvprintw(14, 7, "O      : pelota");
    mvprintw(15, 7, "====== : pala");
    mvprintw(16, 7, "[##]   : bloque");
    mvprintw(18, 5, "Presiona cualquier tecla para volver al menu...");
    attroff(COLOR_PAIR(9) | A_BOLD);
    refresh();
    
    // Limpiar cualquier entrada pendiente
    flushinp();
    getch();
}

void MostrarPuntajes() {
    clear();
    mvprintw(3, 10, "=== PUNTAJES DESTACADOS ===");

    // Pasar mapa a vector para ordenar
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

// Juego principal
void IniciarJuego() {
    // Inicializar objetos
    PalaX = ancho / 2;
    PalaY = alto - 2;
    bolaX = ancho / 2 + 2;
    bolaY = alto - 3;

    InicializarBloques();

    int dx = 1;
    int dy = -1; 
    bool jugando = true; 
    int puntaje = 0;

    while (jugando) {
        clear();

        // Dibujar bordes
        for (int x = 0; x < ancho; x++) {
            mvprintw(0, x, "_");
            mvprintw(alto, x, "_");
        }
        for (int y = 0; y <= alto; y++) {
            mvprintw(y, 0, "|");
            mvprintw(y, ancho, "|");
        }

        // Dibujar objetos
        DibujarBloques();
        DibujarPala();
        DibujarPelota();

        // Mostrar puntaje actual
        mvprintw(alto + 1, 5, "Jugador: %s  Puntaje: %d", jugadorActual.c_str(), puntaje);

        // Teclado
        int tecla = getch();
        if (tecla != ERR) {
            // Limpiar buffer de entrada adicional
            while (getch() != ERR); 
        }

        if ((tecla == 'a' || tecla == 'A') && PalaX > 1) {
            PalaX--;
        }
        if ((tecla == 'd' || tecla == 'D') && PalaX < ancho - 6) {
            PalaX++;
        }
        if (tecla == 'q' || tecla == 'Q') { // salir al menu
            jugando = false;
        }

        // Colisión con pala
        if (bolaX >= PalaX && bolaX <= PalaX + 6 && bolaY == PalaY) {
            dy = -dy;
        }

        // Colisión con bloques
        for (auto &b : bloques) {
            if (!b.destruido) {
                if (bolaX >= b.x && bolaX <= b.x + 3 && bolaY == b.y + 1) {
                    b.destruido = true;
                    dy = -dy;
                }
            }
        }

        // Actualizar posición de la pelota
        bolaX += dx;
        bolaY += dy;

        // Rebotes
        if (bolaX <= 1 || bolaX >= ancho - 1) dx = -dx;
        if (bolaY <= 1) dy = -dy;
        if (bolaY >= alto - 1) {
            //jugando = false; // pelota cayó → fin de partida
            dy = -dy;
        }

        refresh();
        usleep(80000); 
    }
    // Guardar puntaje si es mejor
    if (puntajes.find(jugadorActual) == puntajes.end() || puntaje > puntajes[jugadorActual]) {
        puntajes[jugadorActual] = puntaje;
    }
}

// Main principal
int main() {
    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);

    Estado estado = MENU;

    if (has_colors()) {
    start_color();

    // Bloques multicolor (filas distintas)
    init_pair(1, COLOR_RED,     COLOR_BLACK);
    init_pair(2, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(3, COLOR_GREEN,   COLOR_BLACK);
    init_pair(4, COLOR_CYAN,    COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);

    // Pala y pelota
    init_pair(6, COLOR_WHITE,   COLOR_BLACK);   // pala
    init_pair(7, COLOR_BLUE,    COLOR_BLACK);   // pelota

    // Textos del menú
    init_pair(8, COLOR_CYAN,    COLOR_BLACK); //parte 1
    init_pair(9, COLOR_YELLOW,  COLOR_BLACK); //parte 2
}


    while (estado != SALIR) {
        switch (estado) {
            case MENU:
                nodelay(stdscr, FALSE); // esperar entrada en el menú
                estado = MostrarMenu();
                break;
            case INSTRUCCIONES:
                nodelay(stdscr, FALSE); // esperar entrada en instrucciones
                MostrarInstrucciones();
                estado = MENU;
                break;
            case JUEGO:
                nodelay(stdscr, FALSE); // no esperar en juego
                jugadorActual = PedirNombreJugador();  // pedir nombre ANTES de iniciar juego
                nodelay(stdscr, TRUE);  // no bloquear entrada en juego
                IniciarJuego();
                estado = MENU;
                break;
            case PUNTAJES:
                nodelay(stdscr, FALSE); // esperar entrada en puntajes
                MostrarPuntajes();
                estado = MENU;
                break;
            default:
                estado = SALIR;
        }
    }

    endwin();
    return 0;
}