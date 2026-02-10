#include <raylib.h>
#include <raymath.h>

#include <array> // Inlude static array STL for tilemap

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
    Camera2D camera;
    int radius;
    float angle;
    float speed;
    Rectangle rect;

    // Player color for Render 2D
    Color mainColor;
    // Player rays color for Render 2D
    Color rayColor;
} Player;

typedef struct StaticObject
{
    Vector2 position;
    Texture texture;
    float scale;
    float radius;
} StaticStatic;

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
    Player collision(Player player, Vector2 oldPosPlayer, StaticObject obj, std::array<std::array<int, TILE_WIDTH>, TILE_HEIGHT> worldMap);
}

namespace RayCasting
{
    Camera2D render2D(Camera2D camera, Player player, Render render, Tilemap map, std::array<std::array<int, TILE_WIDTH>, TILE_HEIGHT> worldMap);
    template<std::size_t N>
    void render3D(Player player, Render render, RenderStaticObj renderObj, StaticObject staticObj, RenderTextureMapping texMap, Tilemap map, std::array<Texture, N> wallTex, std::array<std::array<int, TILE_WIDTH>, TILE_HEIGHT> worldMap, float depthBuffer[RAY_COUNT]);
}

// Global variable toggle shade distance view
bool toggleShadeDistance = false;

int main(void)
{
    const int WIDTH_SCREEN = 800;
    const int HEIGHT_SCREEN = 600;

    InitWindow(WIDTH_SCREEN, HEIGHT_SCREEN, "Ray Casting Shading Distance - By Zach Noland");

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
        .camera = {
            (Vector2)
            {
                player.position.x + 20.0f, 
                player.position.y + 20.0f
            },
            (Vector2)
            {
                static_cast<float>(GetScreenWidth()/2.0f), 
                static_cast<float>(GetScreenHeight()/2.0f)
            },
            0.0f,
            1.0f
        },
        .radius = 12,
        .angle = 1.0f,
        .speed = 3.0f,
        .rect = {0.0f, 0.0f, 0.0f, 0.0f},
        .mainColor = BLUE,
        .rayColor = GREEN
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
    float depthBuffer[RAY_COUNT];

    Tilemap map;
    Render render;
    RenderStaticObj renderObj;
    RenderTextureMapping texMap;

    // Variable toggle map view
    bool toggleMap = false;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // Save old position
        Vector2 oldPosPlayer = player.position;

        // Player control
        player = Game::control(player);

        // Player collision
        player = Game::collision(player, oldPosPlayer, treePot, worldMap);

        // Toggle shade distance (Press N)
        if (IsKeyPressed(KEY_N)) toggleShadeDistance = !toggleShadeDistance;

        // Toggle 2d map view (Press M)
        if (IsKeyPressed(KEY_M)) toggleMap = !toggleMap;

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
        RayCasting::render3D(player, render, renderObj, treePot, texMap, map, wallTex, worldMap, depthBuffer);

        // Logic toggle render
        if (toggleMap) 
        {
            // Add dark layer like open menu in game
            DrawRectangleRec(
                (Rectangle)
                {
                    .x = 0.0f,
                    .y = 0.0f,
                    .width = static_cast<float>(GetScreenWidth()),
                    .height = static_cast<float>(GetScreenHeight())
                },
                (Color){0, 0, 0, 100}
            );

            // DRAW 2D MAP
            player.camera = RayCasting::render2D(player.camera, player, render, map, worldMap);

            // Add title in 2D map menu
            DrawText(
                "MAP 2D VIEW", 
                GET_CENTER_X_TEXT("MAP 2D VIEW", 25),
                15, 
                25, 
                WHITE
            );
        }

        // Shade distance display status
        DrawText(
            TextFormat("Shade distance: %s", toggleShadeDistance ? "True" : "False"),
            5,
            5,
            15,
            toggleShadeDistance ? BLUE : RED
        );

        EndDrawing();
    }

    // Unload wall texture
    for (const auto& tex : wallTex) UnloadTexture(tex);

    // Unload static object texture
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

Player Game::collision(Player player, Vector2 oldPosPlayer, StaticObject obj, std::array<std::array<int, TILE_WIDTH>, TILE_HEIGHT> worldMap)
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

Camera2D RayCasting::render2D(Camera2D camera,Player player, Render render, Tilemap map, std::array<std::array<int, TILE_WIDTH>, TILE_HEIGHT> worldMap)
{
    // Using camera2D render for map
    BeginMode2D(camera);
    
    // Camera follow player position
    camera.target = (Vector2)
    {
        .x = player.position.x + 20.0f,
        .y = player.position.y + 20.0f
    };
    // Camera follow player offset (Center)
    camera.offset = (Vector2)
    {
        .x = static_cast<float>(GetScreenWidth() / 2.0f),
        .y = static_cast<float>(GetScreenHeight() / 2.0f)
    };

    // Draw tilemap
    for (int i = 0; i < TILE_HEIGHT; ++i)
    {
        for (int j = 0; j < TILE_WIDTH; ++j)
        {
            // Render 2D worldMap
            if (worldMap[i][j] > 0)
            {
                DrawRectangle(
                    j * TILE_SIZE,
                    i * TILE_SIZE,
                    TILE_SIZE,
                    TILE_SIZE,
                    GRAY
                );
            }
        }
    }

    // Cast rays
    for (int i = 0; i < RAY_COUNT; i++)
    {
        render.rayAngle = player.angle - FOV / 2.0f + (float)i / RAY_COUNT * FOV;

        render.rayDir = {
            cosf(render.rayAngle),
            sinf(render.rayAngle)
        };

        render.rayPos = player.position;

        for (float j = 0; j < RAY_LENGTH; j += RAY_STEP)
        {
            render.rayPos.x += render.rayDir.x * RAY_STEP;
            render.rayPos.y += render.rayDir.y * RAY_STEP;

            map.mapX = render.rayPos.x / TILE_SIZE;
            map.mapY = render.rayPos.y / TILE_SIZE;

            if (map.mapX < 0 || map.mapY < 0 || map.mapX >= TILE_WIDTH || map.mapY >= TILE_HEIGHT) break;

            if (worldMap[map.mapY][map.mapX] > 0)
            {
                // Render 2D raycast lines
                DrawLineV(player.position, render.rayPos, player.rayColor);
                break;
            }
        }
    }

    // Render 2D player
    DrawCircleV(player.position, 6, player.mainColor);

    EndMode2D();

    return camera;
}

template<std::size_t N>
void RayCasting::render3D(Player player, Render render, RenderStaticObj renderObj, StaticObject staticObj, RenderTextureMapping texMap, Tilemap map, std::array<Texture, N> wallTex, std::array<std::array<int, TILE_WIDTH>, TILE_HEIGHT> worldMap, float depthBuffer[RAY_COUNT])
{
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
            depthBuffer[i] = render.correctedDist;

            render.wallHeight = static_cast<float>(GetScreenHeight() * 50) / render.correctedDist;

            render.vec.x = i * (static_cast<float>(GetScreenWidth()) / static_cast<float>(RAY_COUNT));
            render.vec.y = (static_cast<float>(GetScreenHeight()) / 2) - (render.wallHeight / 2);

            // ===== Shading Distance =====

            texMap.shade = 1.0f - (render.correctedDist / MAX_DISTANCE);
            texMap.shade = Clamp(texMap.shade, 0.2f, 1.0f);

            texMap.wallColor = (Color)
            {
                .r = (unsigned char)(255 * texMap.shade),
                .g = (unsigned char)(255 * texMap.shade),
                .b = (unsigned char)(255 * texMap.shade),
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
                toggleShadeDistance ? texMap.wallColor : WHITE
            );

        }
    }

    // ===== STATIC OBJECT RENDER =====

    renderObj.dx = staticObj.position.x - player.position.x;
    renderObj.dy = staticObj.position.y - player.position.y;

    renderObj.angleToObj = atan2f(renderObj.dy, renderObj.dx);
    renderObj.relativeAngle = renderObj.angleToObj - player.angle;

    while (renderObj.relativeAngle > PI)  renderObj.relativeAngle -= 2 * PI;
    while (renderObj.relativeAngle < -PI) renderObj.relativeAngle += 2 * PI;

    if (fabs(renderObj.relativeAngle) < FOV / 2)
    {
        renderObj.distance = sqrtf(renderObj.dx * renderObj.dx + renderObj.dy * renderObj.dy);
        renderObj.correctedDist = renderObj.distance * cosf(renderObj.relativeAngle);

        renderObj.size = static_cast<float>((GetScreenHeight() * staticObj.scale) / renderObj.correctedDist);
        renderObj.screenX = static_cast<float>((renderObj.relativeAngle / (FOV / 2)) * (GetScreenWidth() / 2) + (GetScreenWidth() / 2));

        renderObj.spriteLeft  = renderObj.screenX - renderObj.size / 2;
        renderObj.spriteRight = renderObj.screenX + renderObj.size / 2;

        renderObj.columnWidth = static_cast<float>(GetScreenWidth()) / RAY_COUNT;

        for (float x = renderObj.spriteLeft; x < renderObj.spriteRight; x += renderObj.columnWidth)
        {
            renderObj.rayIndex = static_cast<int>(x / renderObj.columnWidth);
            if (renderObj.rayIndex < 0 || renderObj.rayIndex >= RAY_COUNT) continue;

            if (renderObj.correctedDist < depthBuffer[renderObj.rayIndex])
            {
                renderObj.texX = (x - renderObj.spriteLeft) / renderObj.size;
                renderObj.texX = Clamp(renderObj.texX, 0.0f, 1.0f);

                Rectangle src = (Rectangle) 
                {
                    .x = renderObj.texX * staticObj.texture.width,
                    .y = 0.0f,
                    .width = staticObj.texture.width / renderObj.size,
                    .height = static_cast<float>(staticObj.texture.height)
                };

                Rectangle dst = (Rectangle) 
                {
                    .x = x,
                    .y = (GetScreenHeight() / 2) - renderObj.size / 2,
                    .width = renderObj.columnWidth + 1,
                    .height = renderObj.size
                };

                DrawTexturePro(
                    staticObj.texture,
                    src,
                    dst,
                    {0, 0},
                    0.0f,
                    WHITE
                );

            }
        }
    }
}