#include <raylib.h>
#include <raymath.h>

#include "include/File.hpp" // Include header for function File::getPathFile();

// #define RAY_STEP (5)
#define RAY_STEP (1)
// #define RAY_COUNT (60)
#define RAY_COUNT (240)
#define RAY_LENGTH (1000)
#define FOV (60 * DEG2RAD)

#define GET_CENTER(POS) CLITERAL(POS / 2.0f)
#define GET_CENTER_X_TEXT(TEXT, SIZE) CLITERAL(GET_CENTER((GetScreenWidth() - MeasureText(TEXT, SIZE))))
#define GET_CENTER_Y_TEXT CLITERAL(GET_CENTER(GetScreenHeight()))

typedef struct Player
{
    Vector2 position;
    float angle;
    float speed;
    Rectangle rect;
} Player;

typedef struct Wall
{
    Rectangle component;
    Color color;
} Wall;

typedef struct Render
{
    Vector2 rayPos;
    Vector2 rayDir;
    float distance;
    float rayAngle;
    bool hit;

    float correctedDist;
    float wallHeight;
    Vector2 vec;
} Render;

typedef struct RenderTextureMapping
{
    float dxLeft;
    float dxRight;
    float dyTop;
    float dyBottom;
    float minX;
    float minY;
    float hitX;
    Rectangle src;
    Rectangle dst;
    int texX;
    bool hitVertical;
} RenderTextureMapping;

namespace Game
{
    Player control(Player player);
    Player collision(Player player, Wall wall, Vector2 oldPosPlayer);
}

int main(void)
{
    const int WIDTH_SCREEN = 800;
    const int HEIGHT_SCREEN = 600;

    InitWindow(WIDTH_SCREEN, HEIGHT_SCREEN, "Ray Casting Texture Mappping - By Zach Noland");

    Player player = (Player)
    {
        .position = (Vector2){GET_CENTER(GetScreenWidth()), GET_CENTER(GetScreenHeight())},
        .angle = 0.0f,
        .speed = 3.0f,
        .rect = {0.0f, 0.0f, 0.0f, 0.0f}
    };

    Wall wall = (Wall)
    {
        .component = (Rectangle)
        { 
            .x = 500, 
            .y = 150, 
            .width = 64, 
            .height = 64
        },
        .color = WHITE
    };
    // Sparate brickGrayTex texture for save many memory in GPU
    Texture brickDarkGrayTex = LoadTexture(File::getPathFile("assets/textures/brick/brick_darkgray.png", false));
    // If you want texture bilinear vibes
    // SetTextureFilter(brickGrayTex, TEXTURE_FILTER_BILINEAR);

    Render render;
    RenderTextureMapping texMap;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // Save old position
        Vector2 oldPosPlayer = player.position;

        // Player control
        player = Game::control(player);

        // Player collision
        player = Game::collision(player, wall, oldPosPlayer);

        BeginDrawing();
        // Add gradient color for floor and roof
        DrawRectangleGradientV(
            0, 0, 
            GetScreenWidth(), 
            GetScreenHeight(), 
            (Color){130, 130, 130, 255}, 
            (Color){35, 35, 35, 255}
        );

        // DRAW 3D VIEW
        for (int i = 0; i < RAY_COUNT; ++i)
        {
            render.rayAngle = player.angle - (FOV / 2.0f) + (static_cast<float>(i) / static_cast<float>(RAY_COUNT)) * FOV;

            render.rayDir = (Vector2)
            {
                cosf(render.rayAngle),
                sinf(render.rayAngle)
            };

            render.rayPos = static_cast<Vector2>(player.position);
            render.distance = 0;
            render.hit = false;

            while (render.distance < RAY_LENGTH && !render.hit)
            {
                render.rayPos.x += render.rayDir.x * RAY_STEP;
                render.rayPos.y += render.rayDir.y * RAY_STEP;
                render.distance += RAY_STEP;

                if (CheckCollisionPointRec(render.rayPos, wall.component)) render.hit = true;
            }

            if (render.hit)
            {
                // Fish-eye correction
                render.correctedDist = render.distance * cosf(render.rayAngle - player.angle);

                render.wallHeight = static_cast<float>(GetScreenHeight() * 50) / render.correctedDist;

                render.vec.x = i * (static_cast<float>(GetScreenWidth()) / static_cast<float>(RAY_COUNT));
                render.vec.y = (static_cast<float>(GetScreenHeight()) / 2) - (render.wallHeight / 2);

                // ==== Texture Mapping =====

                texMap.dxLeft   = fabsf(render.rayPos.x - wall.component.x);
                texMap.dxRight  = fabsf(render.rayPos.x - (wall.component.x + wall.component.width));
                texMap.dyTop    = fabsf(render.rayPos.y - wall.component.y);
                texMap.dyBottom = fabsf(render.rayPos.y - (wall.component.y + wall.component.height));

                texMap.minX = fminf(texMap.dxLeft, texMap.dxRight);
                texMap.minY = fminf(texMap.dyTop, texMap.dyBottom);

                texMap.hitVertical = (texMap.minX < texMap.minY);

                // Wall left / right using Y
                if (texMap.hitVertical) texMap.hitX = (render.rayPos.y - wall.component.y) / wall.component.height;
                // Wall up / bottom using X
                else texMap.hitX = (render.rayPos.x - wall.component.x) / wall.component.width;
                
                texMap.hitX = Clamp(texMap.hitX, 0.0f, 1.0f);
                texMap.texX = static_cast<int>(texMap.hitX * brickDarkGrayTex.width);

                // Flip texture
                if (!texMap.hitVertical && render.rayDir.y < 0) texMap.texX = brickDarkGrayTex.width - texMap.texX - 1;
                if (texMap.hitVertical && render.rayDir.x > 0) texMap.texX = brickDarkGrayTex.width - texMap.texX - 1;

                texMap.src = (Rectangle)
                {
                    .x = static_cast<float>(texMap.texX), 
                    .y = 0,
                    .width = 1,
                    .height = static_cast<float>(brickDarkGrayTex.height)
                };

                texMap.dst = (Rectangle)
                {
                    .x = render.vec.x,
                    .y = render.vec.y,
                    .width = (static_cast<float>(GetScreenWidth()) / static_cast<float>(RAY_COUNT)) + 1,
                    .height = render.wallHeight
                };

                DrawTexturePro(
                    brickDarkGrayTex,
                    texMap.src,
                    texMap.dst,
                    (Vector2) {0.0f, 0.0f},
                    0.0f,
                    wall.color
                );
            }
        }

        DrawText(
            "RENDER 3D MAP WITH TEXTURE VIEW",
            GET_CENTER_X_TEXT("RENDER 3D MAP WITH TEXTURE VIEW", 15),
            15,
            15,
            WHITE
        );

        EndDrawing();
    }

    // Unload wall texture
    UnloadTexture(brickDarkGrayTex);

    CloseWindow();
    return 0;
}

Player Game::control(Player player)
{
    // Rotate player
    if (IsKeyDown(KEY_A)) player.angle -= 0.05f;
    if (IsKeyDown(KEY_D)) player.angle += 0.05f;

    // Move player
    if (IsKeyDown(KEY_W))
    {
        player.position.x += cosf(player.angle) * player.speed;
        player.position.y += sinf(player.angle) * player.speed;
    }
    if (IsKeyDown(KEY_S))
    {
        player.position.x -= cosf(player.angle) * player.speed;
        player.position.y -= sinf(player.angle) * player.speed;
    }
    return player;
}

Player Game::collision(Player player, Wall wall, Vector2 oldPosPlayer)
{
    // Update rect player
    player.rect = (Rectangle)
    {
        .x = player.position.x - 6,
        .y = player.position.y - 6,
        .width = 12,
        .height = 12
    };

    // Check collision
    if (CheckCollisionRecs(player.rect, wall.component)) player.position = oldPosPlayer;
    return player;
}