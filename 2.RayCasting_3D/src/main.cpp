#include <raylib.h>
#include <raymath.h>

#define RAY_STEP (5)
#define RAY_COUNT (60)
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

typedef struct RayCasting
{
    Vector2 rayPos;
    Vector2 rayDir;
    float distance;
    float rayAngle;
    bool hit;

    float correctedDist;
    float wallHeight;
    Vector2 vec;
} RayCasting;

namespace Game
{
    Player control(Player player);
    Player collision(Player player, Wall wall, Vector2 oldPosPlayer);
}

int main(void)
{
    const int WIDTH_SCREEN = 800;
    const int HEIGHT_SCREEN = 600;

    InitWindow(WIDTH_SCREEN, HEIGHT_SCREEN, "First Ray Casting 3D - Zach Noland");

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
            .width = 100, 
            .height = 150 
        },
        .color = GRAY
    };

    RayCasting render;

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
        ClearBackground(BLACK);

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

                DrawRectangle(
                    render.vec.x, 
                    render.vec.y,
                    (GetScreenWidth() / RAY_COUNT) + 1,
                    render.wallHeight,
                    wall.color
                );
            }
        }

        DrawText(
            "RENDER 3D MAP VIEW",
            GET_CENTER_X_TEXT("RENDER 3D MAP VIEW", 25),
            15,
            25,
            WHITE
        );

        EndDrawing();
    }

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