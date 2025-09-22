#include <ncurses.h>
#include <unistd.h> 
#include <vector>
#include <map>
#include <string>
#include <algorithm>

using namespace std;

// =====================
// Estructura y variables del juego
// =====================
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

// =====================
// Estados del programa
// =====================
enum Estado { MENU, INSTRUCCIONES, JUEGO, PUNTAJES, SALIR };

// =====================
// Funciones del juego
// =====================

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
            mvprintw(b.y, b.x, "[##]");
        }
    }
}

void DibujarPala() {
    mvprintw(PalaY, PalaX, "======");
}

void DibujarPelota() {
    mvprintw(bolaY, bolaX, "O");
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

// =====================
// Pantallas del menú
// =====================

Estado MostrarMenu() {
    clear();
    mvprintw(5, 10, "=== BREAKOUT ASCII ===");
    mvprintw(7, 10, "1. Iniciar partida");
    mvprintw(8, 10, "2. Instrucciones");
    mvprintw(9, 10, "3. Puntajes destacados");
    mvprintw(10, 10, "4. Salir");
    mvprintw(12, 10, "Selecciona una opcion: ");
    refresh();

    int opcion;
    while (true) {
        opcion = getch();
        switch (opcion) {
            case '1': return JUEGO;
            case '2': return INSTRUCCIONES;
            case '3': return PUNTAJES;
            case '4': return SALIR;
            default:
                // Ignorar otras teclas
                break;
        }
    }
}

void MostrarInstrucciones() {
    clear();
    mvprintw(3, 5, "=== INSTRUCCIONES ===");
    mvprintw(5, 5, "Objetivo: Romper todos los bloques con la pelota.");
    mvprintw(7, 5, "Controles:");
    mvprintw(8, 7, "A o <- : mover pala a la izquierda");
    mvprintw(9, 7, "D o -> : mover pala a la derecha");
    mvprintw(10, 7, "P      : pausar el juego");
    mvprintw(11, 7, "Q      : salir al menu");
    mvprintw(13, 5, "Elementos del juego:");
    mvprintw(14, 7, "O      : pelota");
    mvprintw(15, 7, "====== : pala");
    mvprintw(16, 7, "[##]   : bloque");
    mvprintw(18, 5, "Presiona cualquier tecla para volver al menu...");
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

// =====================
// Juego principal
// =====================
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

// =====================
// Main principal
// =====================
int main() {
    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);

    Estado estado = MENU;

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