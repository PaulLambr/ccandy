#include "raylib.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#define FRAME_COUNT 3
#define MOVE_SPEED 2.0f

bool CheckCircleClick(Vector2 mouse, Vector2 center, float radius);
void ShowDialog(const char *text);

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "Smooth Animated Cowboy");
    Texture2D background = LoadTexture("assets/sweetwater.png");
    Texture2D cowboy = LoadTexture("assets/spritecowboy.png");

    int frameWidth = cowboy.width / FRAME_COUNT;
    int frameHeight = cowboy.height;

    int currentFrame = 2;
    int frameCounter = 0;
    int frameSpeed = 10;

    Vector2 position = {screenWidth / 2.0f, screenHeight / 2.0f};

    Vector2 doorknobPos = {475, 580};
    float doorknobRadius = 10.0f;

    bool dialogActive = false;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        Vector2 velocity = {0, 0};

        // Movement input
        if (IsKeyDown(KEY_RIGHT)) velocity.x += MOVE_SPEED;
        if (IsKeyDown(KEY_LEFT))  velocity.x -= MOVE_SPEED;
        if (IsKeyDown(KEY_DOWN))  velocity.y += MOVE_SPEED;
        if (IsKeyDown(KEY_UP))    velocity.y -= MOVE_SPEED;

        position.x += velocity.x;
        position.y += velocity.y;

        // Keep inside window bounds
        position.x = fmaxf(0, fminf(position.x, screenWidth - frameWidth));
        position.y = fmaxf(0, fminf(position.y, screenHeight - frameHeight));

        // Animate if moving
        if (velocity.x != 0 || velocity.y != 0) {
            frameCounter++;
            if (frameCounter >= frameSpeed) {
                frameCounter = 0;
                currentFrame = (currentFrame + 1) % 2;
            }
        } else {
            currentFrame = 2;
        }

        // Handle click on doorknob
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            if (CheckCircleClick(mouse, doorknobPos, doorknobRadius)) {
                dialogActive = true;
            }
        }

         // Close dialog on any click if it's active
if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && dialogActive == true) {
    Vector2 mouse = GetMousePosition();
    // Only close if NOT clicking the doorknob again
    if (!CheckCircleClick(mouse, doorknobPos, doorknobRadius)) {
        dialogActive = false;
    }
}


        Rectangle source = {
            currentFrame * frameWidth, 0,
            frameWidth, frameHeight
        };

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw background
        DrawTexturePro(
            background,
            (Rectangle){0, 0, (float)background.width, (float)background.height},
            (Rectangle){0, 0, (float)screenWidth, (float)screenHeight},
            (Vector2){0, 0},
            0.0f,
            WHITE
        );

        // Draw doorknob
        DrawCircleV(doorknobPos, doorknobRadius, YELLOW);

        // Draw cowboy
        DrawTextureRec(cowboy, source, position, WHITE);

        // Show dialog if triggered
        if (dialogActive) {
            ShowDialog("The door is locked.");
        }

        EndDrawing();
    }

    UnloadTexture(background);
    UnloadTexture(cowboy);
    CloseWindow();
    return 0;
}

// Utility function to check if mouse clicked inside a circle
bool CheckCircleClick(Vector2 mouse, Vector2 center, float radius)
{
    return CheckCollisionPointCircle(mouse, center, radius);
}

// Draw a simple dialog box with text
void ShowDialog(const char *text)
{
    DrawRectangle(200, 600, 400, 100, BLACK);
    DrawRectangleLines(200, 600, 400, 100, WHITE);
    DrawText(text, 220, 630, 20, WHITE);
}
