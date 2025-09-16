#include <ncurses.h>
#include <unistd.h> 
#include <vector>
#include <string>

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

// Variable para ver los estados del programa

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

//Menú principal

Estado MostrarMenu() {
    clear();
    mvprintw(5, 10, "=== BREAKOUT ASCII ===");
    mvprintw(7, 10, "1. Iniciar partida");
    mvprintw(8, 10, "2. Instrucciones");
    mvprintw(9, 10, "3. Puntajes destacados");
    mvprintw(10, 10, "4. Salir");
    refresh();

    int opcion;
    while (true) {
        opcion = getch();
        switch (opcion) {
            case '1': return JUEGO;
            case '2': return INSTRUCCIONES;
            case '3': return PUNTAJES;
            case '4': return SALIR;
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
    getch();
}

void MostrarPuntajes() {
    clear();
    mvprintw(5, 10, "=== PUNTAJES DESTACADOS ===");
    mvprintw(7, 10, "Aqui se mostrara la tabla de puntajes...");
    mvprintw(9, 10, "Presiona cualquier tecla para volver al menu");
    refresh();
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

        // Teclado
        int tecla = getch();
        if (tecla != ERR) {
            while (getch() != ERR); // limpiar buffer
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
            jugando = false; // pelota cayó → fin de partida
        }

        refresh();
        usleep(80000); 
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
    nodelay(stdscr, TRUE);

    Estado estado = MENU;

    while (estado != SALIR) {
        switch (estado) {
            case MENU:
                estado = MostrarMenu();
                break;
            case INSTRUCCIONES:
                MostrarInstrucciones();
                estado = MENU;
                break;
            case JUEGO:
                IniciarJuego();
                estado = MENU;
                break;
            case PUNTAJES:
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
