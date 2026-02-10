#include <raylib.h>
#include <raymath.h>

#include <array> // Include header for tilemap

#include "include/File.hpp" // Include header for function File::getPathFile();

// #define RAY_STEP (5)
#define RAY_STEP (1)
// #define RAY_COUNT (60)
#define RAY_COUNT (240)
#define RAY_LENGTH (1000)
#define FOV (60 * DEG2RAD)
#define MAX_DISTANCE (800.0f)

#define GET_CENTER(POS) CLITERAL(POS / 2.0f)
#define GET_CENTER_X_TEXT(TEXT, SIZE) CLITERAL(GET_CENTER((GetScreenWidth() - MeasureText(TEXT, SIZE))))
#define GET_CENTER_Y_TEXT CLITERAL(GET_CENTER(GetScreenHeight()))

#define TILE_SIZE (64)
#define TILE_WIDTH (15)
#define TILE_HEIGHT (10)

typedef struct Player
{
    Vector2 spawn;
    Vector2 position;
    int radius;
    float angle;
    float speed;
    Rectangle rect;
} Player;

typedef struct StaticObject
{
    Vector2 position;
    Texture texture;
    float scale;
    float radius;
} StaticObject;

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

    float depthBuffer[RAY_COUNT];
} Render;

typedef struct RenderStaticObj
{
    float dx;
    float dy;
    float angleToObj;
    float relativeAngle;
    float distance;
    float correctedDist;
    float size;
    float screenX;
    int rayIndex;
    float spriteLeft;
    float spriteRight;
    float columnWidth;
    float texX;
} RenderStaticObj;

typedef struct RenderTextureMapping
{
    float dx;
    float dy;
    float hitX;
    Rectangle src;
    Rectangle dst;
    int texX;
    bool hitVertical;

    float shade;
    Color wallColor;
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
    Player collision(Player player, Vector2 oldPosPlayer, StaticObject obj);
}

/*
# WORLD MAP - 01
Data worldMap id:
[0] floor
[1] brick_gray
[2] brick_dark_gray
[3] brick_dark_blue
*/
std::array<std::array<int, TILE_WIDTH>, TILE_HEIGHT> worldMap = {{
    {2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1},
    {2, 0, 0, 0, 2, 3, 0, 0, 0, 3, 0, 0, 0, 0, 1},
    {2, 0, 0, 0, 2, 3, 0, 0, 0, 3, 0, 0, 0, 0, 1},
    {2, 0, 0, 0, 2, 3, 0, 0, 0, 3, 0, 0, 0, 0, 1},
    {2, 2, 0, 2, 2, 3, 3, 0, 3, 3, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
}};

int main(void)
{
    const int WIDTH_SCREEN = 800;
    const int HEIGHT_SCREEN = 600;

    InitWindow(WIDTH_SCREEN, HEIGHT_SCREEN, "Ray Casting Static Object - By Zach Noland");

    Player player = (Player)
    {
        .spawn = (Vector2)
        {
            .x = 2,
            .y = 2
        },
        .position = (Vector2)
        {
            .x = player.spawn.x * TILE_SIZE + TILE_SIZE / 2.0f,
            .y = player.spawn.y * TILE_SIZE + TILE_SIZE / 2.0f
        },
        .radius = 12,
        .angle = 1.0f,
        .speed = 3.0f,
        .rect = {0.0f, 0.0f, 0.0f, 0.0f}
    };

    // Sparate brickGrayTex texture for save many memory in GPU
    std::array<Texture, 3> wallTex = {
        LoadTexture(File::getPathFile("assets/textures/brick/brick_gray.png", false)),
        LoadTexture(File::getPathFile("assets/textures/brick/brick_darkgray.png", false)),
        LoadTexture(File::getPathFile("assets/textures/brick/brick_darkblue.png", false))
    };
    // If you want texture bilinear vibes
    // for (const auto& wall : wallTex) SetTextureFilter(wall, TEXTURE_FILTER_BILINEAR);

    Texture treePotTex = LoadTexture(File::getPathFile("assets/textures/object/pot_tree.png", false));

    StaticObject treePot = (StaticObject)
    {
        .position = (Vector2)
        {
            static_cast<float>(TILE_SIZE * 3.5f), 
            static_cast<float>(TILE_SIZE * 5.5f)
        },
        .texture = treePotTex,
        .scale = 90.0f,
        .radius = 20.0f
    };

    Tilemap map;
    Render render;
    RenderStaticObj renderStaticObj;
    RenderTextureMapping texMap;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // Save old position
        Vector2 oldPosPlayer = player.position;

        // Player control
        player = Game::control(player);

        // Player collision
        player = Game::collision(player, oldPosPlayer, treePot);

        BeginDrawing();
        // Add floor and ceil
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
            }

            if (render.hit)
            {
                // Fish-eye correction
                render.correctedDist = render.distance * cosf(render.rayAngle - player.angle);
                render.depthBuffer[i] = render.correctedDist;

                render.wallHeight = static_cast<float>(GetScreenHeight() * 50) / render.correctedDist;

                render.vec.x = i * (static_cast<float>(GetScreenWidth()) / static_cast<float>(RAY_COUNT));
                render.vec.y = (static_cast<float>(GetScreenHeight()) / 2) - (render.wallHeight / 2);

                // ===== Shading Distance =====

                texMap.shade = 1.0f - (render.correctedDist / MAX_DISTANCE);
                texMap.shade = Clamp(texMap.shade, 0.2f, 1.0f);

                texMap.wallColor = (Color)
                {
                    .r = static_cast<unsigned char>(255 * texMap.shade),
                    .g = static_cast<unsigned char>(255 * texMap.shade),
                    .b = static_cast<unsigned char>(255 * texMap.shade),
                    .a = 255
                };

                // ==== Texture Mapping =====

                Texture tex = wallTex[map.hitTile - 1];

                texMap.hitX = texMap.hitVertical ? fmodf(render.rayPos.y, TILE_SIZE) / TILE_SIZE : fmodf(render.rayPos.x, TILE_SIZE) / TILE_SIZE;
                
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
                    texMap.wallColor
                );
            }
        }

        // ===== STATIC OBJECT RENDER =====

        renderStaticObj.dx = treePot.position.x - player.position.x;
        renderStaticObj.dy = treePot.position.y - player.position.y;

        renderStaticObj.angleToObj = atan2f(renderStaticObj.dy, renderStaticObj.dx);
        renderStaticObj.relativeAngle = renderStaticObj.angleToObj - player.angle;

        while (renderStaticObj.relativeAngle > PI) renderStaticObj.relativeAngle -= 2 * PI;
        while (renderStaticObj.relativeAngle < -PI) renderStaticObj.relativeAngle += 2 * PI;

        if (fabs(renderStaticObj.relativeAngle) < FOV / 2)
        {
            renderStaticObj.distance = sqrtf(renderStaticObj.dx * renderStaticObj.dx + renderStaticObj.dy * renderStaticObj.dy);
            renderStaticObj.correctedDist = renderStaticObj.distance * cosf(renderStaticObj.relativeAngle);

            renderStaticObj.size = static_cast<float>((GetScreenHeight() * treePot.scale) / renderStaticObj.correctedDist);
            renderStaticObj.screenX = static_cast<float>((renderStaticObj.relativeAngle / (FOV / 2)) * (GetScreenWidth() / 2) + (GetScreenWidth() / 2));

            renderStaticObj.spriteLeft  = renderStaticObj.screenX - renderStaticObj.size / 2;
            renderStaticObj.spriteRight = renderStaticObj.screenX + renderStaticObj.size / 2;

            renderStaticObj.columnWidth = static_cast<float>(GetScreenWidth()) / RAY_COUNT;

            for (float x = renderStaticObj.spriteLeft; x < renderStaticObj.spriteRight; x += renderStaticObj.columnWidth)
            {
                renderStaticObj.rayIndex = static_cast<int>(x / renderStaticObj.columnWidth);
                if (renderStaticObj.rayIndex < 0 || renderStaticObj.rayIndex >= RAY_COUNT) continue;

                if (renderStaticObj.correctedDist < render.depthBuffer[renderStaticObj.rayIndex])
                {
                    renderStaticObj.texX = (x - renderStaticObj.spriteLeft) / renderStaticObj.size;
                    renderStaticObj.texX = Clamp(renderStaticObj.texX, 0.0f, 1.0f);

                    Rectangle src = (Rectangle) 
                    {
                        .x = renderStaticObj.texX * treePot.texture.width,
                        .y = 0.0f,
                        .width = treePot.texture.width / renderStaticObj.size,
                        .height = static_cast<float>(treePot.texture.height)
                    };

                    Rectangle dst = (Rectangle) 
                    {
                        .x = x,
                        .y = (GetScreenHeight() / 2) - renderStaticObj.size / 2,
                        .width = renderStaticObj.columnWidth + 1,
                        .height = renderStaticObj.size
                    };

                    DrawTexturePro(
                        treePot.texture,
                        src,
                        dst,
                        {0, 0},
                        0.0f,
                        WHITE
                    );

                }
            }
        }

        DrawText(
            "RENDER 3D",
            GET_CENTER_X_TEXT("RENDER 3D", 25),
            15,
            25,
            WHITE
        );

        EndDrawing();
    }

    // Unload wall texture
    for (const auto& tex : wallTex) UnloadTexture(tex);

    // Unload object texture
    UnloadTexture(treePotTex);

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

Player Game::collision(Player player, Vector2 oldPosPlayer, StaticObject obj)
{
    // ==== WorldMap Collision ====

    int left   = (player.position.x - player.radius) / TILE_SIZE;
    int right  = (player.position.x + player.radius) / TILE_SIZE;
    int top    = (player.position.y - player.radius) / TILE_SIZE;
    int bottom = (player.position.y + player.radius) / TILE_SIZE;

    if (left < 0 || right >= TILE_WIDTH || top < 0 || bottom >= TILE_HEIGHT)
    {
        player.position = oldPosPlayer;
        return player;
    }

    if (worldMap[top][left] > 0 || worldMap[top][right] > 0 || worldMap[bottom][left] > 0 || worldMap[bottom][right] > 0)
    {
        player.position = oldPosPlayer;
    }

    // ==== Static Object Collision ====

    float dx = player.position.x - obj.position.x;
    float dy = player.position.y - obj.position.y;

    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < player.radius + obj.radius)
    {
        player.position = oldPosPlayer;
    }

    return player;
}