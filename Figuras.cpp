#include <ncurses.h>
#include <unistd.h> 
#include <vector>
#include <string>

using namespace std;
// Estructura para los bloques
struct bloque {
    int x;
    int y;
    bool destruido;
};

// Tamaño del tablero o ventana
const int ancho = 50;
const int alto = 20;

// Objetos del juego
int PalaX;
int PalaY;
int bolaX; 
int bolaY;
vector<bloque> bloques;

// Inicializa bloques en filas
void InicializarBloques() {
    int X_inicial = 5;
    int Y_inicial = 2;
    int columnas = 10;
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

// Dibuja los bloques
void DibujarBloques() {
    for (auto &bloques : bloques) {
        if (!bloques.destruido) {
            mvprintw(bloques.y, bloques.x, "[##]");
        }
    }
}

// Dibuja la pala
void DibujarPala() {
    mvprintw(PalaY, PalaX, "=====");
}

// Dibuja la pelota
void DibujarPelota() {
    mvprintw(bolaY, bolaX, "O");
}

// control del juego
int main() {
    // Inicializar ncurses
    initscr();
    noecho();
    curs_set(FALSE);

    // Inicializar objetos del juego
    PalaX = ancho / 2;
    PalaY = alto - 2;
    bolaX = ancho / 2 + 2;
    bolaY = alto - 3;

    InicializarBloques();

    // dirección inicial de la pelota
    int dx = 1;
    int dy = -1; 


    int jugar = 1;
    while (jugar == 1) {
        clear();

        // se dibujan los bordes
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

        // refesh de la pantalla
        refresh();

        // controlar la velocidad de refresh de la pantalla ahorita va aprox a 12 fps
        usleep(80000); 
    }

    endwin();
    return 0;
}