// gcc -Wall -Wextra -g3 -std=c2x -lm fluid.c -o output/fluid_sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 && ./output/fluid_sim
/* 
Modificar de uma forma que a simulação ocorra mais frequentemente que a renderização
mudando o frametime na simulação, fazendo que seja o dobro
*/

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
#define MAX_COMPRESSION 0.25

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

void draw_environment(Cell environment[])
{
    for(int i=0; i<ROWS*COLUMNS; i++)
    {
        draw_cell(environment[i]);
    }
}

void initialize_environment(Cell environment[])
{
    for(int i=0; i<ROWS; i++)
    {
        for(int j=0; j<COLUMNS; j++)
        {
            environment[j + COLUMNS*i] = (Cell){WATER_TYPE, 0, j, i, -1};
        }
    }
}

void update_cell(Cell *cell, Cell environment[], Vector2 mouse_position, int cell_type, int fill_level)
{
    mouse_position = GetMousePosition();
    int cell_x = mouse_position.x / CELL_SIZE;
    int cell_y = mouse_position.y / CELL_SIZE;

    if(cell_x>=0 && cell_x<COLUMNS && cell_y>=0 && cell_y<ROWS)
    {
        *cell = (Cell){cell_type, fill_level, cell_x, cell_y, 0};
        environment[cell_x + cell_y*COLUMNS] = *cell;
    }
}

void sim_rule_1(Cell environment[])
{   // renderização não funciona direito com a água caindo, regra 1 não refinada
    Cell *source_cell, *cell_below;
    float flow = 0;
    for(int i=0; i<ROWS; i++)
    {
        for(int j=0; j<COLUMNS; j++)
        {
            source_cell = &environment[j + COLUMNS*(i)];
            if(source_cell->type == SOLID_TYPE || i >= ROWS)
                continue;

            cell_below = &environment[j + COLUMNS*(i+1)];
            if(cell_below->type == SOLID_TYPE || cell_below->fill_level > source_cell->fill_level)
                continue;

            if((source_cell->fill_level + cell_below->fill_level) <= 1)
            {
                flow = source_cell->fill_level;
                source_cell->fill_level -= flow;
                cell_below->fill_level += flow;
            }
            else
            {
                flow = (source_cell->fill_level + cell_below->fill_level + MAX_COMPRESSION) / 2;
                source_cell->fill_level -= flow;
                cell_below->fill_level += flow;
            }
        }
    }
}

void simulation_step(Cell environment[])
{
    /*
    Regras do automata em ordem:
    Regra #1: Liquido desce para celula de baixo apenas quando, além de ser líquido:
        - a celula de baixo tem menos liquido que a de cima
        - e a celula de baixo é uma celula de água, e não é o fim da tela
        Calculo da transferencia de liquido:
        se a soma dos dois é menor ou igual que o valor máximo(1), transfere tudo
        se é maior, transfere a (soma + valor máximo de compressão) / 2

    Regra #2: Liquido flui para esquerda e direita:


    */
   sim_rule_1(environment);
}

int main()
{
    InitWindow(WIDTH, HEIGHT, "Simulação de fluido automata celular");
    SetTargetFPS(FPS);

    float delta_t; // delta tempo para cálculo da simulação
    Vector2 mouse_position;
    Cell cell;

    Cell environment[ROWS * COLUMNS];
    initialize_environment(environment);

    int del_mode = 0;
    while (!WindowShouldClose())
    {
        BeginDrawing();
            
        ClearBackground(BLACK);
        // delta de tempo para calcular com base no FPS da simulação
        delta_t = GetFrameTime();
        
        // modo de delete, apaga a célula clicada
        if(del_mode)
        {
            if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                update_cell(&cell, environment, mouse_position, WATER_TYPE, 0);
            }
        }

        // botão direito coloca água
        else if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            update_cell(&cell, environment, mouse_position, WATER_TYPE, 1);
        }
        // botão esquerdo desenha o sólido
        else if(IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            update_cell(&cell, environment, mouse_position, SOLID_TYPE, 1);
        }
        // muda o modo para apagar caso clique "d" no teclado
        if(IsKeyPressed(KEY_D))
            del_mode = !del_mode;

        //fazer os passos da simulação
        simulation_step(environment);

        draw_environment(environment);
        draw_grid();
        DrawFPS(10, 10);
        
        //printf("%d", del_mode);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}