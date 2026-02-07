#include <raylib.h>
#include <raymath.h>

#include <array>

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

#define TILE_SIZE (64)
#define TILE_WIDTH (10)
#define TILE_HEIGHT (10)

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
    // float dxLeft;
    // float dxRight;
    // float dyTop;
    // float dyBottom;
    // float minX;
    // float minY;
    float dx;
    float dy;
    float hitX;
    Rectangle src;
    Rectangle dst;
    int texX;
    bool hitVertical;
} RenderTextureMapping;

typedef struct Tilemap
{
    int mapX;
    int mapY;
    int hitTile;
} Tilemap;

namespace Game
{
    Player control(Player player);
    Player collision(Player player, Vector2 oldPosPlayer);
}

// Multidimensional array world map
std::array<std::array<int, TILE_WIDTH>, TILE_HEIGHT> worldMap = {{
    {2, 2, 2, 2, 2, 3, 3, 3, 3, 3},
    {2, 0, 0, 0, 2, 3, 0, 0, 0, 3},
    {2, 0, 0, 0, 2, 3, 0, 0, 0, 3},
    {2, 0, 0, 0, 2, 3, 0, 0, 0, 3},
    {2, 2, 0, 2, 2, 3, 3, 0, 3, 3},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
}};

int main(void)
{
    const int WIDTH_SCREEN = 800;
    const int HEIGHT_SCREEN = 600;

    InitWindow(WIDTH_SCREEN, HEIGHT_SCREEN, "Ray Casting Texture Mappping - By Zach Noland");

    // Set player spawn position based tile
    int spawnX = 2;
    int spawnY = 2;

    Player player = (Player)
    {
        .position = (Vector2)
        {
            .x = spawnX * TILE_SIZE + TILE_SIZE / 2.0f,
            .y = spawnY * TILE_SIZE + TILE_SIZE / 2.0f
        },
        .angle = 1.0f,
        .speed = 3.0f,
        .rect = {0.0f, 0.0f, 0.0f, 0.0f}
    };

    // Wall wall = (Wall)
    // {
    //     .component = (Rectangle)
    //     { 
    //         .x = 500, 
    //         .y = 150, 
    //         .width = 64, 
    //         .height = 64
    //     },
    //     .color = WHITE
    // };

    // Sparate brickGrayTex texture for save many memory in GPU
    std::array<Texture, 3> wallTex = {
        LoadTexture(File::getPathFile("assets/textures/brick/brick_gray.png", false)),
        LoadTexture(File::getPathFile("assets/textures/brick/brick_darkgray.png", false)),
        LoadTexture(File::getPathFile("assets/textures/brick/brick_darkblue.png", false))
    };
    // If you want texture bilinear vibes
    // SetTextureFilter(brickGrayTex, TEXTURE_FILTER_BILINEAR);
    // Texture brickDarkGrayTex = LoadTexture(File::getPathFile("assets/textures/brick/brick_darkgray.png", false));

    Tilemap map;
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
        player = Game::collision(player, oldPosPlayer);

        BeginDrawing();
        // Add gradient color for floor and roof
        DrawRectangle(
            0, 
            0,
            GetScreenWidth(),
            static_cast<int>(GET_CENTER(GetScreenHeight())),
            DARKGRAY
        );
        DrawRectangle(
            0, 
            static_cast<int>(GET_CENTER(GetScreenHeight())),
            GetScreenWidth(),
            static_cast<int>(GET_CENTER(GetScreenHeight())),
            GRAY
        );

        // DrawRectangleGradientV(
        //     0, 0, 
        //     GetScreenWidth(), 
        //     GetScreenHeight(), 
        //     (Color){130, 130, 130, 255}, 
        //     (Color){35, 35, 35, 255}
        // );

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
            texMap.hitVertical = false;

            map.hitTile = 0;

            while (render.distance < RAY_LENGTH && !render.hit)
            {
                render.rayPos.x += render.rayDir.x * RAY_STEP;
                render.rayPos.y += render.rayDir.y * RAY_STEP;
                render.distance += RAY_STEP;

                map.mapX = render.rayPos.x / TILE_SIZE;
                map.mapY = render.rayPos.y / TILE_SIZE;

                if (map.mapX < 0 || map.mapY < 0 || map.mapX >= TILE_WIDTH || map.mapY >= TILE_HEIGHT) break;

                if (worldMap[map.mapY][map.mapX] > 0)
                {
                    render.hit = true;
                    map.hitTile = worldMap[map.mapY][map.mapX];

                    texMap.dx = fminf(
                        fabsf(render.rayPos.x - map.mapX * TILE_SIZE),
                        fabsf(render.rayPos.x - (map.mapX + 1) * TILE_SIZE)
                    );
                    texMap.dy = fminf(
                        fabsf(render.rayPos.y - map.mapY * TILE_SIZE),
                        fabsf(render.rayPos.y - (map.mapY + 1) * TILE_SIZE)
                    );
                    texMap.hitVertical = texMap.dx < texMap.dy;
                }
                // if (CheckCollisionPointRec(render.rayPos, wall.component)) render.hit = true;
            }

            if (render.hit)
            {
                // Fish-eye correction
                render.correctedDist = render.distance * cosf(render.rayAngle - player.angle);

                render.wallHeight = static_cast<float>(GetScreenHeight() * 50) / render.correctedDist;

                render.vec.x = i * (static_cast<float>(GetScreenWidth()) / static_cast<float>(RAY_COUNT));
                render.vec.y = (static_cast<float>(GetScreenHeight()) / 2) - (render.wallHeight / 2);

                // ==== Texture Mapping =====

                Texture tex = wallTex[map.hitTile - 1];

                texMap.hitX = texMap.hitVertical ? fmodf(render.rayPos.y, TILE_SIZE) / TILE_SIZE : fmodf(render.rayPos.x, TILE_SIZE) / TILE_SIZE;

                // texMap.dxLeft   = fabsf(render.rayPos.x - wall.component.x);
                // texMap.dxRight  = fabsf(render.rayPos.x - (wall.component.x + wall.component.width));
                // texMap.dyTop    = fabsf(render.rayPos.y - wall.component.y);
                // texMap.dyBottom = fabsf(render.rayPos.y - (wall.component.y + wall.component.height));

                // texMap.minX = fminf(texMap.dxLeft, texMap.dxRight);
                // texMap.minY = fminf(texMap.dyTop, texMap.dyBottom);

                // texMap.hitVertical = (texMap.minX < texMap.minY);

                // Wall left / right using Y
                // if (texMap.hitVertical) texMap.hitX = (render.rayPos.y - wall.component.y) / wall.component.height;
                // Wall up / bottom using X
                // else texMap.hitX = (render.rayPos.x - wall.component.x) / wall.component.width;
                
                texMap.hitX = Clamp(texMap.hitX, 0.0f, 1.0f);
                texMap.texX = static_cast<int>(texMap.hitX * tex.width);

                // Flip texture
                if (!texMap.hitVertical && render.rayDir.y < 0) texMap.texX = tex.width - texMap.texX - 1;
                if (texMap.hitVertical && render.rayDir.x > 0) texMap.texX = tex.width - texMap.texX - 1;

                texMap.src = (Rectangle)
                {
                    .x = static_cast<float>(texMap.texX), 
                    .y = 0,
                    .width = 1,
                    .height = static_cast<float>(tex.height)
                };

                texMap.dst = (Rectangle)
                {
                    .x = render.vec.x,
                    .y = render.vec.y,
                    .width = (static_cast<float>(GetScreenWidth()) / static_cast<float>(RAY_COUNT)) + 1,
                    .height = render.wallHeight
                };

                DrawTexturePro(
                    tex,
                    texMap.src,
                    texMap.dst,
                    (Vector2) {0.0f, 0.0f},
                    0.0f,
                    WHITE
                );
            }
        }

        DrawText(
            "RENDER 3D TILEDMAP",
            GET_CENTER_X_TEXT("RENDER 3D TILEDMAP", 25),
            15,
            25,
            WHITE
        );

        EndDrawing();
    }

    // Unload wall texture
    for (const auto& tex : wallTex) UnloadTexture(tex);
    // UnloadTexture(brickDarkGrayTex);

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

Player Game::collision(Player player, Vector2 oldPosPlayer)
{
    int mapX = player.position.x / TILE_SIZE;
    int mapY = player.position.y / TILE_SIZE;

    if (mapX < 0 || mapY < 0 || mapX >= TILE_WIDTH || mapY >= TILE_HEIGHT)
    {
        player.position = oldPosPlayer;
        return player;
    }

    if (worldMap[mapY][mapX] > 0)
    {
        player.position = oldPosPlayer;
    }

    return player;
}