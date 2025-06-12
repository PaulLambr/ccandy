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
#define SPECIAL_TILES 2

int board[BOARD_SIZE][BOARD_SIZE];  // Now using tile indices (0-4)
bool matched[BOARD_SIZE][BOARD_SIZE] = { 0 };
float fall_offset[BOARD_SIZE][BOARD_SIZE] = { 0 };

int score = 0;
Vector2 grid_origin;
Texture2D background;
#define TOTAL_TILE_TYPES 7  // 5 normal + 2 special
Texture2D tile_textures[TOTAL_TILE_TYPES];

Font score_font;
Vector2 selected_tile = { -1, -1 };
float fall_speed = 8.0f;
float match_delay_timer = 0.0f;
const float MATCH_DELAY_DURATION = 0.2f;

float score_scale = 1.0f;
float score_scale_velocity = 0.0f;
bool score_animating = false;
Sound match_sound;
int bomb_x = 2, bomb_y = 3;
int match_x = 4, match_y = 3;
bool bomb_triggered = false;
float bomb_timer = 0.0f;
Vector2 screen_shake = { 0 };




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

bool is_bomb_match_combo(Vector2 a, Vector2 b) {
    int t1 = board[(int)a.y][(int)a.x];
    int t2 = board[(int)b.y][(int)b.x];
    return (t1 == 5 && t2 == 6) || (t1 == 6 && t2 == 5);
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

    // Correct global assignments:
    bomb_x = 2; bomb_y = 3;
    match_x = 4; match_y = 3;
    board[bomb_y][bomb_x] = 5;
    board[match_y][match_x] = 6;

    // Center grid:
    int grid_width = BOARD_SIZE * TILE_SIZE;
    int grid_height = BOARD_SIZE * TILE_SIZE;

    grid_origin = (Vector2){
    (GetScreenWidth() - grid_width) / 2,
    (GetScreenHeight() - grid_height) / 2
};

// Apply screen shake if active
grid_origin.x += screen_shake.x;
grid_origin.y += screen_shake.y;


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
tile_textures[5] = LoadTexture("assets/bombbg.png");
tile_textures[6] = LoadTexture("assets/litmatch.jpg");


    score_font = LoadFontEx("assets/04b03.ttf", SCORE_FONT_SIZE, NULL, 0);

    init_board();
    Vector2 mouse = { 0, 0 };

 while (!WindowShouldClose()) {
    // 1) Update audio
    UpdateMusicStream(bgm);

    // 2) Handle bomb-triggered shake
    if (bomb_triggered) {
        screen_shake.x = GetRandomValue(-5, 5);
        screen_shake.y = GetRandomValue(-5, 5);
        bomb_timer -= GetFrameTime();
        if (bomb_timer <= 0.0f) {
            bomb_triggered = false;
            screen_shake = (Vector2){ 0 };
            init_board();  // reset the level
        }
    } else {
        screen_shake = (Vector2){ 0 };
    }

    // 3) Calculate shaken_origin for rendering
    Vector2 shaken_origin = {
        grid_origin.x + screen_shake.x,
        grid_origin.y + screen_shake.y
    };

    // 4) Mouse handling (selection, swap, special combo)
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
                    if (is_bomb_match_combo(selected_tile, current_tile)) {
                        // instant bomb+match clear
                        for (int yy = 0; yy < BOARD_SIZE; yy++)
                          for (int xx = 0; xx < BOARD_SIZE; xx++)
                            board[yy][xx] = -1, fall_offset[yy][xx] = 0;
                        score += 100;
                        PlaySound(match_sound);
                        bomb_triggered = true;
                        bomb_timer     = 0.6f;
                    } else {
                        // normal swap & match
                        swap_tiles(selected_tile.x, selected_tile.y,
                                   current_tile.x, current_tile.y);
                        if (find_matches()) resolve_matches();
                        else swap_tiles(selected_tile.x, selected_tile.y,
                                        current_tile.x, current_tile.y);
                    }
                }
                selected_tile = (Vector2){ -1, -1 };
            }
        }
    }

    // 5) Auto-detect adjacency combo by scanning the board each frame
    if (tile_state == STATE_IDLE && !bomb_triggered) {
        Vector2 bomb_pos = { -1, -1 }, match_pos = { -1, -1 };
        for (int yy = 0; yy < BOARD_SIZE; yy++) {
            for (int xx = 0; xx < BOARD_SIZE; xx++) {
                if (board[yy][xx] == 5) bomb_pos = (Vector2){ xx, yy };
                else if (board[yy][xx] == 6) match_pos = (Vector2){ xx, yy };
            }
        }
        if (are_tiles_adjacent(bomb_pos, match_pos)) {
            for (int yy = 0; yy < BOARD_SIZE; yy++)
                for (int xx = 0; xx < BOARD_SIZE; xx++)
                    board[yy][xx] = -1, fall_offset[yy][xx] = 0;
            score += 100;
            PlaySound(match_sound);
            bomb_triggered = true;
            bomb_timer     = 0.6f;
        }
    }

    // 6) Falling animation
    if (tile_state == STATE_ANIMATING) {
        bool still_animating = false;
        for (int yy = 0; yy < BOARD_SIZE; yy++) {
            for (int xx = 0; xx < BOARD_SIZE; xx++) {
                if (fall_offset[yy][xx] > 0) {
                    fall_offset[yy][xx] -= fall_speed;
                    if (fall_offset[yy][xx] < 0) fall_offset[yy][xx] = 0;
                    else still_animating = true;
                }
            }
        }
        if (!still_animating) {
            tile_state = STATE_MATCH_DELAY;
            match_delay_timer = MATCH_DELAY_DURATION;
        }
    }

    // 7) Match-delay state
    if (tile_state == STATE_MATCH_DELAY) {
        match_delay_timer -= GetFrameTime();
        if (match_delay_timer <= 0.0f) {
            if (find_matches()) resolve_matches();
            else tile_state = STATE_IDLE;
        }
    }

    // 8) DRAWING
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
        shaken_origin.x,
        shaken_origin.y,
        BOARD_SIZE * TILE_SIZE,
        BOARD_SIZE * TILE_SIZE,
        Fade(DARKGRAY, 0.60f)
    );

    for (int yy = 0; yy < BOARD_SIZE; yy++) {
        for (int xx = 0; xx < BOARD_SIZE; xx++) {
            int tile_index = board[yy][xx];
            if (tile_index >= 0 && tile_index < TOTAL_TILE_TYPES) {
                DrawTexturePro(
                    tile_textures[tile_index],
                    (Rectangle){0, 0,
                        tile_textures[tile_index].width,
                        tile_textures[tile_index].height},
                    (Rectangle){
                        shaken_origin.x + (xx * TILE_SIZE),
                        shaken_origin.y + (yy * TILE_SIZE)
                          - fall_offset[yy][xx],
                        TILE_SIZE, TILE_SIZE
                    },
                    (Vector2){0, 0},
                    0.0f,
                    matched[yy][xx] ? GREEN : WHITE
                );
            }
        }
    }

    if (selected_tile.x >= 0) {
        DrawRectangleLinesEx((Rectangle){
            shaken_origin.x + (selected_tile.x * TILE_SIZE),
            shaken_origin.y + (selected_tile.y * TILE_SIZE),
            TILE_SIZE, TILE_SIZE
        }, 2, YELLOW);
    }

    DrawTextEx(
        score_font,
        TextFormat("SCORE: %d", score),
        (Vector2){20, 20},
        SCORE_FONT_SIZE * score_scale,
        1.0f,
        BLACK
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
