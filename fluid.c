// gcc -Wall -Wextra -g3 -std=c2x -lm fluid.c -o output/fluid_sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 && ./output/fluid_sim
#include "raylib.h"
#include <stdio.h>

#define FPS 100
#define WIDTH 900
#define HEIGHT 600
#define GRID_THICK 2
#define CELL_SIZE 20
#define COLUMNS WIDTH/CELL_SIZE
#define ROWS HEIGHT/CELL_SIZE

#define SOLID_TYPE 1
#define WATER_TYPE 0

#define WATER_BLUE (Color){69,189,245,255}

typedef struct
{
    int type; // líquido ou sólido
    float fill_level; // vazio: 0, cheio: 1. valor mínimo para ter água: 0.005
    int x, y; // coordenadas x, y
    int flow_direction; // 0: cima. 1: baixo. 2: direita. 3: esquerda
} Cell;

// Desenha o grid de divisão das células.
void draw_grid()
{
    int i;
    for(i=0; i < COLUMNS; i++)
    {
        DrawRectangle(i*CELL_SIZE, 0, GRID_THICK, HEIGHT, DARKGRAY);
    }
    for(i=0; i < ROWS; i++)
    {
        DrawRectangle(0, i*CELL_SIZE, WIDTH, GRID_THICK, DARKGRAY);
    }
}

void draw_cell(Cell cell)
{
    int pixel_x = cell.x * CELL_SIZE;
    int pixel_y = cell.y * CELL_SIZE;
    if(cell.type == WATER_TYPE && cell.fill_level > 0.005)
    {
        DrawRectangle(pixel_x, pixel_y, CELL_SIZE, CELL_SIZE, WATER_BLUE);
    }
    else if(cell.type == SOLID_TYPE)
    {
        DrawRectangle(pixel_x, pixel_y, CELL_SIZE, CELL_SIZE, WHITE);
    }
}

void draw_environment(Cell *environment)
{
    for(int i=0; i<ROWS*COLUMNS; i++)
    {
        draw_cell(environment[i]);
    }
}

void initialize_environment(Cell *environment)
{
    for(int i=0; i<ROWS; i++)
    {
        for(int j=0; j<COLUMNS; j++)
        {
            environment[j + COLUMNS*i] = (Cell){WATER_TYPE, 0, j, i, -1};
        }
    }
}

int main()
{
    InitWindow(WIDTH, HEIGHT, "Simulação de fluido automata celular");
    SetTargetFPS(FPS);

    float delta_t; // delta tempo para cálculo da simulação
    Vector2 mouse_position;

    Cell environment[ROWS * COLUMNS];
    initialize_environment(environment);
    while (!WindowShouldClose())
    {
        BeginDrawing();
            
        ClearBackground(BLACK);
        // delta de tempo para calcular com base no FPS da simulação
        delta_t = GetFrameTime();
        Cell cell;
        // botão esquerdo coloca água
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            mouse_position = GetMousePosition();
            int cell_x = mouse_position.x / CELL_SIZE;
            int cell_y = mouse_position.y / CELL_SIZE;
            
            cell = (Cell){WATER_TYPE, 1, cell_x, cell_y, 0};
            environment[cell_x + cell_y*COLUMNS] = cell;
        }
        // botão direito desenha o sólido
        else if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            mouse_position = GetMousePosition();
            int cell_x = mouse_position.x / CELL_SIZE;
            int cell_y = mouse_position.y / CELL_SIZE;
            
            cell = (Cell){SOLID_TYPE, 1, cell_x, cell_y, 0};
            environment[cell_x + cell_y*COLUMNS] = cell;
        }

        draw_environment(environment);
        draw_grid();
        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}