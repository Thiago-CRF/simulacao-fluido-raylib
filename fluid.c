// gcc -Wall -Wextra -g3 -std=c2x -lm fluid.c -o output/fluid_sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 && ./output/fluid_sim
/* 
Modificar de uma forma que a simulação ocorra mais frequentemente que a renderização
mudando o frametime na simulação, fazendo que seja o dobro
*/

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>

#define FPS 50
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
#define DEEP_BLUE (Color){5,58,117,255}

typedef struct
{
    int type; // líquido ou sólido
    float fill_level; // vazio: 0, cheio: 1. valor mínimo para ter água: 0.0005
    int x, y; // coordenadas x, y
    int flow_direction; // 0: cima. 1: baixo. 2: direita. 3: esquerda. 4: ambos os lados
} Cell;

Color get_interpolate_color(Color max, Color min, double percentage)
{
    unsigned char R_new = (min.r - max.r) * percentage + max.r;
    unsigned char G_new = (min.g - max.g) * percentage + max.g;
    unsigned char B_new = (min.b - max.b) * percentage + max.b;
    unsigned char A_new = (min.a - max.a) * percentage + max.a;

    return (Color){R_new, G_new, B_new, A_new};
}

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

    if(cell.type == WATER_TYPE && cell.fill_level < 0.0005)
    {
        DrawRectangle(pixel_x, pixel_y, CELL_SIZE, CELL_SIZE, BLACK);
        return;
    }
    if(cell.type == SOLID_TYPE)
    {
        DrawRectangle(pixel_x, pixel_y, CELL_SIZE, CELL_SIZE, WHITE);
        return;
    }
    /* ainda preciso diferenciar quando pode desenhar ela pela metade, e quando não pode
    Quero que desenhe ela pela metade quando não tiver nada em cima e quando não estiver caindo
        
    Desenhar ela cheia, mesmo se tiver metade, caso esteja caindo ou esteja com outra agua em cima*/
    
    Color interpolated_color = get_interpolate_color(WATER_BLUE, DEEP_BLUE, cell.fill_level);

    if(cell.fill_level >= 1)
    {
        DrawRectangle(pixel_x, pixel_y, CELL_SIZE, CELL_SIZE, interpolated_color);
        return;
    }
    else if(cell.flow_direction == 1)
    {
        DrawRectangle(pixel_x, pixel_y, CELL_SIZE, CELL_SIZE, interpolated_color);
    }
    else
    //if(não tem água acima da célula)
    {
        // desenha a água no nível que ela está
        int water_height = cell.fill_level * CELL_SIZE;
        int empty_height = CELL_SIZE - water_height;
        DrawRectangle(pixel_x, (pixel_y + empty_height), CELL_SIZE, water_height, interpolated_color);
            
    }
}

void draw_environment(Cell environment[])
{
    for(int i=0; i<ROWS*COLUMNS; i++)
    {
        // corrige valores de fluido antes de desenhar
        if(environment[i].fill_level > 0 && environment[i].fill_level < 0.0005)
        {
            environment[i].fill_level = 0;
        }

        draw_cell(environment[i]);
        // reseta as direções após desenhar
        environment[i].flow_direction = -1;
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

void sim_rule_1(Cell environment[], Cell updt_environment[])
{
    // array temporario para armazenar as mudanças
    for(int i=0; i<ROWS*COLUMNS; i++)
    {
        updt_environment[i] = environment[i];
    }

    Cell *source_cell, *cell_below;
    Cell *updt_source_cell, *updt_cell_below;
    float flow = 0;
    for(int i=0; i<ROWS; i++)
    {
        for(int j=0; j<COLUMNS; j++)
        {
            source_cell = &environment[j + COLUMNS*i];
            // regra de eliminação
            if(source_cell->fill_level <= 0.0005)
            {
                source_cell->fill_level = 0;
            }

            if(source_cell->type == SOLID_TYPE || i == ROWS-1)
            // pula caso a celula seja solido, ou esteja na ultima linha
                continue;

            cell_below = &environment[j + COLUMNS*(i+1)];
            if(cell_below->type == SOLID_TYPE || cell_below->fill_level >= source_cell->fill_level)
            // pula caso a celula abaixo seja sólido ou a água de baixo não seja menor que a celula origem
                continue;

            updt_source_cell = &updt_environment[j + COLUMNS*i];
            updt_cell_below = &updt_environment[j + COLUMNS*(i+1)];

            // se a soma é menor ou um, passa tudo pra baixo
            float soma = (source_cell->fill_level + cell_below->fill_level);
            if(soma <= 1)
            {
                flow = source_cell->fill_level;
                if(flow < 0.005)
                        continue;

                updt_source_cell->fill_level -= flow;
                updt_cell_below->fill_level += flow;

                updt_cell_below->flow_direction = 1;
                //updt_source_cell->flow_direction = 1;
            }
            /*outra ideia é fazer um calculo diferente caso a soma seja menor que 2 * (Valor_maximo+compressao_máxima)
            sendo o valor: (valor_maximo*valor_maximo + compressao_maxima*soma) / (valor_máximo + comressao_maxima).
            transferindo mais para baixo caso a soma de ambos seja menos que o valor máximo que eles juntos 
            podem receber com compressão, tranferindo água mais rápido para baixo. Como por exemplo:
            # FEITO # */
            else if(soma < (2 * 1+MAX_COMPRESSION))
            {   // valor máximo: 1. Por enquanto quero que o valor máximo seja sempre 1, por isso não criei
                // uma variavel ou uma constante para mudar o valor máximo no código todo
                flow = (1 + MAX_COMPRESSION * soma) / (1 + MAX_COMPRESSION);
                if(flow < 0.005)
                    continue;
                updt_source_cell->fill_level -= flow;
                updt_cell_below->fill_level += flow;

                updt_cell_below->flow_direction = 1;
                //updt_source_cell->flow_direction = 1;
            }
            else
            {
                flow = (soma + MAX_COMPRESSION) / 2;
                if(flow < 0.005)
                    continue;

                updt_source_cell->fill_level -= flow;
                updt_cell_below->fill_level += flow;

                updt_cell_below->flow_direction = 1;
                //updt_source_cell->flow_direction = 1;
            }
        }
    }
    // atualiza o array original com as mudanças
    for(int i=0; i<ROWS*COLUMNS; i++)
    {
        environment[i] = updt_environment[i];
    }
}

void sim_rule_2(Cell environment[], Cell updt_environment[])
{   // TODO: refinar regra 2, parece que estou perdendo volume de água
    // OBS: talvez a regra 3, da comopressão, resolva isso
// array temporario para armazenar as mudanças
    /*for(int i=0; i<ROWS*COLUMNS; i++)
    {
        updt_environment[i] = environment[i];
    } */

    Cell *source_cell, *right_cell, *left_cell;
    Cell *updt_source_cell, *updt_right_cell, *updt_left_cell;
    Cell *cell_below;
    float flow_out = 0, flow_r = 0, flow_l = 0;
    for(int i=0; i<ROWS; i++)
    {
        for(int j=0; j<COLUMNS; j++)
        {
            source_cell = &environment[j + COLUMNS*i];
            if(source_cell->type == SOLID_TYPE || source_cell->fill_level <= 0.0005)
                continue;

            /* Precisa verificar se o lado direito ou esquerdo está bloqueado, 
            sendo solido ou fim da tela, para transferir só para um lado ou para ambos*/

            // se tiver no fim da tela, não verifica, tendo que completar toda regra 1 antes
            if(i != ROWS-1)
            {
                // se ainda puder ir para baixo, não calcula a transferencia lateral
                cell_below = &environment[j + COLUMNS*(i+1)];
                if(cell_below->fill_level < source_cell->fill_level && cell_below->type == WATER_TYPE)
                    continue;
            }

            updt_source_cell = &updt_environment[j + COLUMNS*i];
            // caso esteja na esquerda da tela, flui só para direita
            if(j == 0)
            {   
                updt_right_cell = &updt_environment[(j+1) + COLUMNS*i];
                right_cell = &environment[(j+1) + COLUMNS*i];
                if(right_cell->type == SOLID_TYPE || right_cell->fill_level >= source_cell->fill_level)
                    continue;

                // calcula o fluxo e transfere apenas para direita
                flow_out = source_cell->fill_level / 3;
                if(flow_out < 0.005)
                    continue;

                flow_r = flow_out;
                updt_source_cell->fill_level -= flow_out;
                updt_right_cell->fill_level += flow_r;

                updt_source_cell->flow_direction = 2;
                //updt_right_cell->flow_direction = -1;
            }

            // caso esteja na direita da tela, flui só para esquerda
            else if(j == COLUMNS-1)
            {
                updt_left_cell = &updt_environment[(j-1) + COLUMNS*i];
                left_cell = &environment[(j-1) + COLUMNS*i];
                if(left_cell->type == SOLID_TYPE || left_cell->fill_level >= source_cell->fill_level)
                    continue;

                // calcula o fluxo e transfere apenas para esquerda
                flow_out = source_cell->fill_level / 3;
                if(flow_out < 0.005)
                    continue;

                flow_l = flow_out;
                updt_source_cell->fill_level -= flow_out;
                updt_left_cell->fill_level += flow_l;

                updt_source_cell->flow_direction = 3;
                //updt_left_cell->flow_direction = -1;
            }
            else 
            {
                right_cell = &environment[(j+1) + COLUMNS*i];
                left_cell = &environment[(j-1) + COLUMNS*i];

                updt_right_cell = &updt_environment[(j+1) + COLUMNS*i];
                updt_left_cell = &updt_environment[(j-1) + COLUMNS*i];
                // se ambas as laterais forem sólido, não faz nada
                if(right_cell->type == SOLID_TYPE && left_cell->type == SOLID_TYPE)
                    continue;

                if(right_cell->type == WATER_TYPE && left_cell->type == WATER_TYPE)
                {
                    // faz o calcula para transferir as duas, transfere 1/4 para ambas e pula
                    flow_out = source_cell->fill_level / 2;
                    if(flow_out < 0.005)
                        continue;

                    flow_l = flow_out / 2;
                    flow_r = flow_out / 2;

                    updt_source_cell->fill_level -= flow_out;
                    updt_right_cell->fill_level += flow_r;
                    updt_left_cell->fill_level += flow_l;

                    updt_source_cell->flow_direction = 4;
                    //updt_right_cell->flow_direction = -1;
                    //updt_left_cell->flow_direction = -1;
                    continue;
                }
                if(right_cell->type == WATER_TYPE)
                {
                    // calcula para o lado direito apenas
                    flow_out = source_cell->fill_level / 3;
                    if(flow_out < 0.005)
                        continue;

                    flow_r = flow_out;
                    updt_source_cell->fill_level -= flow_out;
                    updt_right_cell->fill_level += flow_r;

                    updt_source_cell->flow_direction = 2;
                    //updt_right_cell->flow_direction = -1;
                }
                else if(left_cell->type == WATER_TYPE)
                {
                    // calcula para o lado esquerdo apenas
                    flow_out = source_cell->fill_level / 3;
                    if(flow_out < 0.005)
                        continue;

                    flow_l = flow_out;
                    updt_source_cell->fill_level -= flow_out;
                    updt_left_cell->fill_level += flow_l;

                    updt_source_cell->flow_direction = 3;
                    //updt_left_cell->flow_direction = -1;
                }
            }
        }
    }
    // atualiza o array original com as mudanças
    for(int i=0; i<ROWS*COLUMNS; i++)
    {
        environment[i] = updt_environment[i];
    }
}

void sim_rule_3(Cell environment[], Cell updt_environment[])
{   /*
    Regra #3: 
        - Se não pode mais executar nem a regra 1, nem a regra 2 e
        a célula está pressurizada (fill_value > 1). 
        Então passa todo volume extra para célula acima, ficando apenas com
        volume de 1: flow = fill_value - 1
    */
    Cell *source_cell, *cell_above;
    Cell *updt_source_cell, *updt_cell_above;
    float flow = 0;
    for(int i=0; i<ROWS; i++)
    {
        for(int j=0; j<COLUMNS; j++)
        {
            source_cell = &environment[j + COLUMNS*i];
            if(source_cell->type == SOLID_TYPE || i == 0)
                continue;
            
            cell_above = &environment[j + COLUMNS*(i-1)];
            if(cell_above->type == SOLID_TYPE || source_cell->fill_level < 0.0005)
                continue;

            updt_source_cell = &updt_environment[j + COLUMNS*i];
            updt_cell_above = &updt_environment[j + COLUMNS*(i-1)];
            if(source_cell->flow_direction == -1)
            {
                flow = source_cell->fill_level - 1;
                if(flow < 0.005)
                    continue;
                updt_source_cell->fill_level -= flow;
                updt_cell_above->fill_level += flow;
                updt_source_cell->flow_direction = 0;
            }
        }
    }
    // atualiza o array original com as mudanças
    for(int i=0; i<ROWS*COLUMNS; i++)
    {
        environment[i] = updt_environment[i];
    }
}

void simulation_step(Cell environment[], Cell buffer_environment[])
{
    /*
    Regras do automata em ordem:
    Regra #1: Liquido desce para celula de baixo apenas quando, além de ser líquido:
        - a celula de baixo tem menos liquido que a de cima
        - e a celula de baixo é uma celula de água, e não é o fim da tela
        Calculo da transferencia de liquido:
        se a soma dos dois é menor ou igual que o valor máximo(1), transfere tudo
        se é maior, transfere a (soma + valor máximo de compressão) / 2

    Regra #2: Liquido flui para esquerda e direita, além de ser líquido:
        - só transferir se a célula do lado tenha menos que a origem
        - deve fluir para direita e esquerda na mesma iteração
        - só pode ir para os lados quando não pode mais ir para baixo
        - verificando para onde pode fluir e calculando a quantidade para fluir
        - fluindo mais para a celula vizinha caso só possa fluir para um lado,
        - quantia para fluir é menor se for só para um lado, e maior se for para os dois
        - se puder transferir para ambos os lados: transfere 1/4 para cada lado
        - se puder transferir para os dois lados: transfere 1/3 para o lado

    Regra #3: 
        - Se a célula está pressurizada (fill_value > 1), e 
        não pode mais executar nem a regra 1, nem a regra 2. 
        Então passa todo volume extra para célula acima, ficando apenas com
        volume de 1: flow = fill_value - 1

    */
    sim_rule_1(environment, buffer_environment);
    // TODO: refinar regra 2, parece que estou perdendo volume de água
    sim_rule_2(environment, buffer_environment);
    sim_rule_3(environment, buffer_environment);
}

int main()
{
    InitWindow(WIDTH, HEIGHT, "Simulação de fluido com automata celular");
    SetTargetFPS(FPS);

    float delta_t; // delta tempo para cálculo da simulação
    Vector2 mouse_position;
    Cell cell;

    Cell *environment = (Cell*)malloc(ROWS * COLUMNS * sizeof(Cell));
    // array buffer para armazenar mudanças com as regras
    Cell *buffer_environment = (Cell*)malloc(ROWS * COLUMNS * sizeof(Cell));
    // Cell environment[ROWS * COLUMNS];
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
        simulation_step(environment, buffer_environment);
        draw_environment(environment);

        draw_grid();
        DrawFPS(10, 10);
        
        //printf("%d", del_mode);
        EndDrawing();
    }

    CloseWindow();
    free(environment);
    free(buffer_environment);

    return 0;
}