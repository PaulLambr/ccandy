#include "raylib.h"
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#define BOARD_SIZE 8
#define TILE_SIZE 42
#define TILE_TYPES 5
#define SCORE_FONT_SIZE 32
#define MAX_SCORE_POPUPS 32

int board[BOARD_SIZE][BOARD_SIZE];  // Now using tile indices (0-4)
bool matched[BOARD_SIZE][BOARD_SIZE] = { 0 };
float fall_offset[BOARD_SIZE][BOARD_SIZE] = { 0 };

int score = 0;
Vector2 grid_origin;
Texture2D background;
Texture2D tile_textures[TILE_TYPES];  // NEW
Font score_font;
Vector2 selected_tile = { -1, -1 };
float fall_speed = 8.0f;
float match_delay_timer = 0.0f;
const float MATCH_DELAY_DURATION = 0.2f;

float score_scale = 1.0f;
float score_scale_velocity = 0.0f;
bool score_animating = false;
Sound match_sound;


typedef enum {
    STATE_IDLE,
    STATE_ANIMATING,
    STATE_MATCH_DELAY
} TileState;

TileState tile_state;

int random_tile() {
    return rand() % TILE_TYPES;
}

void swap_tiles(int x1, int y1, int x2, int y2) {
    int temp = board[y1][x1];
    board[y1][x1] = board[y2][x2];
    board[y2][x2] = temp;
}

bool are_tiles_adjacent(Vector2 a, Vector2 b) {
    return (abs((int)a.x - (int)b.x) + abs((int)a.y - (int)b.y)) == 1;
}

bool find_matches() {
    bool found = false;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            matched[y][x] = false;
        }
    }

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE - 2; x++) {
            int t = board[y][x];
            if (t == board[y][x + 1] && t == board[y][x + 2]) {
                matched[y][x] = matched[y][x + 1] = matched[y][x + 2] = true;
                score += 10;
                found = true;
            }
        }
    }

    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE - 2; y++) {
            int t = board[y][x];
            if (t == board[y + 1][x] && t == board[y + 2][x]) {
                matched[y][x] = matched[y + 1][x] = matched[y + 2][x] = true;
                score += 10;
                found = true;
            }
        }
    }

    if (found) PlaySound(match_sound);
return found;

}

void resolve_matches() {
    for (int x = 0; x < BOARD_SIZE; x++) {
        int write_y = BOARD_SIZE - 1;

        for (int y = BOARD_SIZE - 1; y >= 0; y--) {
            if (!matched[y][x]) {
                if (y != write_y) {
                    board[write_y][x] = board[y][x];
                    fall_offset[write_y][x] = (write_y - y) * TILE_SIZE;
                    board[y][x] = -1;  // empty
                }
                write_y--;
            }
        }

        while (write_y >= 0) {
            board[write_y][x] = random_tile();
            fall_offset[write_y][x] = (write_y + 1) * TILE_SIZE;
            write_y--;
        }
    }

    tile_state = STATE_ANIMATING;
}

void init_board() {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            board[y][x] = random_tile();
        }
    }

    int grid_width = BOARD_SIZE * TILE_SIZE;
    int grid_height = BOARD_SIZE * TILE_SIZE;

    grid_origin = (Vector2){
        (GetScreenWidth() - grid_width) / 2,
        (GetScreenHeight() - grid_height) / 2
    };

    if (find_matches()) {
        resolve_matches();
    } else {
        tile_state = STATE_IDLE;
    }
}

int main(void) {
    const int screen_width = 800;
    const int screen_height = 450;

    InitWindow(screen_width, screen_height, "Raylib 2D Tile Match");
    SetTargetFPS(60);
    srand(time(NULL));

    InitAudioDevice();
Music bgm = LoadMusicStream("assets/bgm.mp3");
match_sound = LoadSound("assets/match.mp3");

PlayMusicStream(bgm);


    background = LoadTexture("assets/slots.png");
    tile_textures[0] = LoadTexture("assets/tile_1.png");
tile_textures[1] = LoadTexture("assets/tile_2.png");
tile_textures[2] = LoadTexture("assets/tile_3.png");
tile_textures[3] = LoadTexture("assets/tile_4.png");
tile_textures[4] = LoadTexture("assets/tile_5.png");


    score_font = LoadFontEx("assets/04b03.ttf", SCORE_FONT_SIZE, NULL, 0);

    init_board();
    Vector2 mouse = { 0, 0 };

    while (!WindowShouldClose()) {
        UpdateMusicStream(bgm);

        mouse = GetMousePosition();

        if (tile_state == STATE_IDLE && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int x = (mouse.x - grid_origin.x) / TILE_SIZE;
            int y = (mouse.y - grid_origin.y) / TILE_SIZE;
            if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
                Vector2 current_tile = (Vector2){ x, y };
                if (selected_tile.x < 0) {
                    selected_tile = current_tile;
                } else {
                    if (are_tiles_adjacent(selected_tile, current_tile)) {
                        swap_tiles(selected_tile.x, selected_tile.y, current_tile.x, current_tile.y);
                        if (find_matches()) {
                            resolve_matches();
                        } else {
                            swap_tiles(selected_tile.x, selected_tile.y, current_tile.x, current_tile.y);
                        }
                    }
                    selected_tile = (Vector2){ -1, -1 };
                }
            }
        }

        if (tile_state == STATE_ANIMATING) {
            bool still_animating = false;

            for (int y = 0; y < BOARD_SIZE; y++) {
                for (int x = 0; x < BOARD_SIZE; x++) {
                    if (fall_offset[y][x] > 0) {
                        fall_offset[y][x] -= fall_speed;
                        if (fall_offset[y][x] < 0) fall_offset[y][x] = 0;
                        else still_animating = true;
                    }
                }
            }

            if (!still_animating) {
                tile_state = STATE_MATCH_DELAY;
                match_delay_timer = MATCH_DELAY_DURATION;
            }
        }

        if (tile_state == STATE_MATCH_DELAY) {
            match_delay_timer -= GetFrameTime();
            if (match_delay_timer <= 0.0f) {
                if (find_matches()) {
                    resolve_matches();
                } else {
                    tile_state = STATE_IDLE;
                }
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(
            background,
            (Rectangle){ 0, 0, background.width, background.height },
            (Rectangle){ 0, 0, GetScreenWidth(), GetScreenHeight() },
            (Vector2){ 0, 0 },
            0.0f,
            WHITE
        );

        DrawRectangle(
            grid_origin.x,
            grid_origin.y,
            BOARD_SIZE * TILE_SIZE,
            BOARD_SIZE * TILE_SIZE,
            Fade(DARKGRAY, 0.60f)
        );

        for (int y = 0; y < BOARD_SIZE; y++) {
            for (int x = 0; x < BOARD_SIZE; x++) {
                int tile_index = board[y][x];
                if (tile_index >= 0 && tile_index < TILE_TYPES) {
                    DrawTexturePro(
    tile_textures[tile_index],
    (Rectangle){ 0, 0, tile_textures[tile_index].width, tile_textures[tile_index].height },
    (Rectangle){
        grid_origin.x + (x * TILE_SIZE),
        grid_origin.y + (y * TILE_SIZE) - fall_offset[y][x],
        TILE_SIZE,
        TILE_SIZE
    },
    (Vector2){ 0, 0 },
    0.0f,
    matched[y][x] ? GREEN : WHITE
);

                }
            }
        }

        if (selected_tile.x >= 0) {
            DrawRectangleLinesEx((Rectangle){
                grid_origin.x + (selected_tile.x * TILE_SIZE),
                grid_origin.y + (selected_tile.y * TILE_SIZE),
                TILE_SIZE, TILE_SIZE
            }, 2, YELLOW);
        }

        DrawTextEx(
            score_font,
            TextFormat("SCORE: %d", score),
            (Vector2){ 20, 20 },
            SCORE_FONT_SIZE * score_scale,
            1.0f,
            YELLOW
        );

        EndDrawing();
    }

    for (int i = 0; i < TILE_TYPES; i++) {
        UnloadTexture(tile_textures[i]);
    }
    UnloadTexture(background);
    UnloadFont(score_font);
    UnloadMusicStream(bgm);
    UnloadSound(match_sound);

CloseAudioDevice();

    CloseWindow();

    return 0;
}
