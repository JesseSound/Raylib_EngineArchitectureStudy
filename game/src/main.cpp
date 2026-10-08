#include "raylib.h"
#include "raymath.h"
#include "raygui.h"
#include <algorithm>
#include <vector>
#include <iostream>
#define MAX_WINDOW_HEIGHT 800
#define MAX_WINDOW_WIDTH 800    
#define MAX_FPS 60
#define MAX_BULLETS 30

constexpr float BALL_RADIUS = 25.0f;
constexpr Vector2 GRAVITY = { 0.0f, 19.6f };


enum BulletShape: int {
	NONE_BULLET,
	CIRCLE_BULLET,
	BOX_BULLET,
	CAPSULE_BULLET,

};
enum EntityType {
	NONE_ENTITY,
	BULLET,
	TARGET,
	PILL
};
enum EntityShape: int {
	NONE_SHAPE,
	CIRCLE,
	BOX,
	CAPSULE
};
struct Entity {
	Vector2 position{};
	Vector2 velocity{};
	float radius = 0.0f; // For circle entities, 0 init for safety
	Color color{};
	bool isActive = false;
	EntityType entity_type = NONE_ENTITY;
	EntityShape entity_collider_shape = NONE_SHAPE;
	Vector2 size = { 0.0f, 0.0f }; // For box entities, 0 init for safety
	Vector2 capsule_start{};
	Vector2 capsule_end{};
};



// Game state enumeration for state machines
enum GameState {
	MAIN,
	GAME_WIN,
	GAME_LOSS
};

enum WeaponType {
	TARGETSHOT,
	WIDESHOT,
	
};

void SetupCalls() {
	// Setup any necessary calls or initializations here
    InitWindow(MAX_WINDOW_WIDTH, MAX_WINDOW_HEIGHT, "Made By Ur Mom");
    InitAudioDevice();
    SetTargetFPS(MAX_FPS);
}


bool OutOfBounds(const Vector2& position) {
	return (position.x < 0 || position.x > MAX_WINDOW_WIDTH || position.y < 0 || position.y > MAX_WINDOW_HEIGHT);
}

bool CollideWithGround(const Vector2& position, const Rectangle& ground) {
	return (position.y + BALL_RADIUS >= ground.y);
}
//save typing time by making code reusable 
Rectangle BoxBoundsMath(Entity& boxEntity) {
	return { boxEntity.position.x - boxEntity.size.x / 2, boxEntity.position.y - boxEntity.size.y / 2, boxEntity.size.x, boxEntity.size.y };
}
//capsule gen
void DrawCapsule2D(Vector2 startPos, Vector2 endPos, float radius, Color color)
{
	// The diameter of the capsule's rounded caps matches the line's thickness
	DrawLineEx(startPos, endPos, radius * 2.0f, color);
	DrawCircleV(startPos, radius, color);
	DrawCircleV(endPos, radius, color);
}
// Setting Up Function Tables. I 

void CircleBoxCollision(Entity& circleEntity, Entity& boxEntity) {
	if (CheckCollisionCircleRec(circleEntity.position, circleEntity.radius, BoxBoundsMath(boxEntity))) {
		circleEntity.isActive = false;
		boxEntity.isActive = false;
	}
}
void CircleCircleCollision(Entity& circleEntityA, Entity& circleEntityB) {
	if (CheckCollisionCircles(circleEntityA.position, circleEntityA.radius, circleEntityB.position, circleEntityB.radius)) {
		circleEntityA.isActive = false;
		circleEntityB.isActive = false;
	}
}
void BoxBoxCollision(Entity& boxEntityA, Entity& boxEntityB) {
	if (CheckCollisionRecs(BoxBoundsMath(boxEntityA), BoxBoundsMath(boxEntityB))) {
		boxEntityA.isActive = false;
		boxEntityB.isActive = false;
	}
}

void CapsuleCapsuleCollision(Entity& capsuleEntityA, Entity& capsuleEntityB) {
	// Placeholder for capsule collision logic
	// Implement capsule collision detection here if needed
}

void CapsuleCircleCollision(Entity& capsuleEntity, Entity& circleEntity) {
	// Placeholder for capsule-circle collision logic
	// Implement capsule-circle collision detection here if needed
}
void CapsuleBoxCollision(Entity& capsuleEntity, Entity& boxEntity) {
	// Placeholder for capsule-box collision logic
	// Implement capsule-box collision detection here if needed
}
void CircleCapsuleCollision(Entity& circleEntity, Entity& capsuleEntity) {
	// Placeholder for circle-capsule collision logic
	// Implement circle-capsule collision detection here if needed
}
void BoxCapsuleCollision(Entity& boxEntity, Entity& capsuleEntity) {
	// Placeholder for box-capsule collision logic
	// Implement box-capsule collision detection here if needed
}
void BoxCircleCollision(Entity& boxEntity, Entity& circleEntity) {
	if (CheckCollisionCircleRec(circleEntity.position, circleEntity.radius, BoxBoundsMath(boxEntity))) {
		boxEntity.isActive = false;
		circleEntity.isActive = false;
	}
}
void HitTestNone(Entity& entityA, Entity& entityB) {
	// No collision logic for NONE shape
	
}

using CollisionFunc = void(*)(Entity&, Entity&);

CollisionFunc collisionFunctionTable[4][4] = {
	// NONE     CIRCLE                  BOX                  CAPSULE
	{ HitTestNone, HitTestNone,          HitTestNone,       HitTestNone },        // NONE
	{ HitTestNone, CircleCircleCollision,  CircleBoxCollision,  CircleCapsuleCollision }, // CIRCLE
	{ HitTestNone, BoxCircleCollision,     BoxBoxCollision,     BoxCapsuleCollision },    // BOX
	{ HitTestNone, CapsuleCircleCollision, CapsuleBoxCollision, CapsuleCapsuleCollision } // CAPSULE
};

//Drawing functions for different entity shapes

void DrawNone(Entity& entity) {
	// No drawing logic for NONE shape
}

void DrawCircleEntity(Entity& entity) {
	DrawCircleV(entity.position, entity.radius, entity.color);
}

void DrawBoxEntity(Entity& entity) {
	DrawRectangleV(entity.position - entity.size / 2, entity.size, entity.color);
}
void DrawCapsuleEntity(Entity& entity) {
	DrawCapsule2D(entity.capsule_start, entity.capsule_end, entity.radius, entity.color);
}

using DrawFunc = void(*)(Entity&);
DrawFunc drawFunctionTable[4] = {
	DrawNone,          // NONE
	DrawCircleEntity,  // CIRCLE
	DrawBoxEntity,     // BOX
	DrawCapsuleEntity  // CAPSULE
};




void SetUpTargets(std::vector<Entity>& targets, int max_targets,int remaining_targets = 0) {
	for (int i = remaining_targets; i < max_targets; i++) {
		Entity new_target;
		new_target.position = { static_cast<float>(GetRandomValue(200, MAX_WINDOW_WIDTH - 50)), static_cast<float>(GetRandomValue(50, 600)) };
		if (i < 5) {
			new_target.radius = GetRandomValue(20.0f, 40.0f);
			new_target.color = RED;
			new_target.entity_collider_shape = CIRCLE;
		}
		else {
			new_target.size = { static_cast<float>(GetRandomValue(30, 60)), static_cast<float>(GetRandomValue(30, 60)) };
			new_target.color = BLUE;
			new_target.entity_collider_shape = BOX;
		}
		new_target.entity_type = TARGET;
		new_target.isActive = true;
		targets.push_back(new_target);
	}
}

int main()
{
	SetupCalls();

    Rectangle ground;
        ground.x = 0;
        ground.y = 700;
        ground.width = MAX_WINDOW_WIDTH;
        ground.height = 20;

    Rectangle platform;
        platform.x = 100;
        platform.y = ground.y - 100.0f;
        platform.width = 50;
        platform.height = 100;
    Vector2 ball_launch_position;
        ball_launch_position.x = platform.x + BALL_RADIUS;
        ball_launch_position.y = platform.y - BALL_RADIUS;
	    Vector2 ball_position = ball_launch_position;
        Vector2 ball_velocity = Vector2Zeros;

	float ball_gravity_scale = 1.0f; // Enable gravity for the ball
    
    

	std::vector<Entity> all_entities;
	int max_targets = 10;
	int remaining_targets = max_targets;
	int balls_on_screen = 0;
	float ball_launch_angle = -30.0f; // Launch angle in degrees

	//GameState
	GameState game_state = MAIN;

	//Weapon Type Definer
	WeaponType weapon_type = TARGETSHOT;

	//Tab Selection
	BulletShape bullet_shape = CIRCLE_BULLET;

	Vector3 cube_rotation = { 0.0f, 0.0f, 0.0f };

	SetUpTargets(all_entities, max_targets);

	float countdown_timer = 60.0f; // Countdown timer in seconds
    while (!WindowShouldClose())
    {
		float dt = GetFrameTime();
		switch (game_state)
		{
		case MAIN:

			countdown_timer -= dt;
			if (countdown_timer <= 0) {
				game_state = GAME_LOSS;
			}
			if (IsKeyPressed(KEY_SPACE) && balls_on_screen < MAX_BULLETS) {


				if (weapon_type == TARGETSHOT) {

					Entity new_bullet;
					new_bullet.position = ball_launch_position;

					
					new_bullet.isActive = true;
					new_bullet.color = RED;
					new_bullet.entity_type = BULLET;


					switch (bullet_shape) {
					case CIRCLE_BULLET:
						new_bullet.entity_collider_shape = CIRCLE;
						new_bullet.radius = BALL_RADIUS;
						break;
					case BOX_BULLET:
						new_bullet.entity_collider_shape = BOX;
						new_bullet.size = { 30.0f, 15.0f }; // Set size for box bullets
						break;
					}
					
					
					new_bullet.velocity = Vector2Rotate(Vector2UnitX, ball_launch_angle * DEG2RAD) * 200.0f;
					all_entities.push_back(new_bullet);
				}

				else if (weapon_type == WIDESHOT) {

					for (int i = -1; i <= 1; i++) {
						
						Entity wide_bullet;
						wide_bullet.position = ball_launch_position;

						//Handle bullet shape selection based on the current bullet_shape value
						switch (bullet_shape) {
						case CIRCLE_BULLET:
							wide_bullet.entity_collider_shape = CIRCLE;
							wide_bullet.radius = BALL_RADIUS;
							break;
						case BOX_BULLET:
							wide_bullet.entity_collider_shape = BOX;
							wide_bullet.size = { 30.0f, 15.0f }; // Set size for box bullets
							break;
						}
						
						wide_bullet.isActive = true;
						wide_bullet.color = DARKBLUE;
						wide_bullet.entity_type = BULLET;
						
						
						wide_bullet.velocity = Vector2Rotate(Vector2UnitX,(ball_launch_angle + i * 20.0f) * DEG2RAD) * 200.0f;
						all_entities.push_back(wide_bullet);
						
						
					}
				}
			}

		if (IsKeyPressed(KEY_A)) {
			SetUpTargets(all_entities, max_targets, remaining_targets);
		}
		if (IsKeyPressed(KEY_T)) {
			if (weapon_type == TARGETSHOT) {
				weapon_type = WIDESHOT;
			}
			else if (weapon_type == WIDESHOT) {
				weapon_type = TARGETSHOT;
			}
		}

		if (IsKeyDown(KEY_UP)) {
			ball_launch_angle -= 100.0f *dt ;
		}
		if (IsKeyDown(KEY_DOWN)) {
			ball_launch_angle += 100.0f *dt ;
		}

		if (IsKeyPressed(KEY_TAB)) {
			if( bullet_shape == CIRCLE_BULLET){
				bullet_shape = BOX_BULLET;
			}
			else if (bullet_shape == BOX_BULLET){
				bullet_shape = CIRCLE_BULLET;
			}
		}

        BeginDrawing();
        ClearBackground(WHITE);
		//"UI" Studd
		DrawText(TextFormat("Time: %f", countdown_timer), 10, 10, 20, DARKGRAY);
		DrawText(TextFormat("Weapon Selected: %s", weapon_type == TARGETSHOT ? "Target Shot" : "Wide Shot"), 10, 40, 20, DARKGRAY); 
		DrawText(TextFormat("Targets Remaining: %i", remaining_targets), 10, 70, 20, DARKGRAY);


		
		


		//Func Table look up

		for (int i = 0; i < all_entities.size(); ++i) {
			Entity& entityA = all_entities[i];
			if (!entityA.isActive) continue;
			for (int j = i + 1; j < all_entities.size(); ++j) {
				Entity& entityB = all_entities[j];
				if (!entityB.isActive) continue;
				CollisionFunc collisionFunc = collisionFunctionTable[entityA.entity_collider_shape][entityB.entity_collider_shape];
				if (collisionFunc) {
					collisionFunc(entityA, entityB);
				}
			}
		}


		//handle Bullet stuff, gravity, and out of bounds
        for (Entity& entity : all_entities) {
			if (entity.entity_type == BULLET) {

				if (entity.isActive) {

					entity.velocity += GRAVITY * dt * ball_gravity_scale;
					entity.position += entity.velocity * dt;
					//meme points
					SetWindowPosition(GetWindowPosition().x, GetWindowPosition().y + entity.velocity.y * dt);
				}

				bool isOutOfBounds = OutOfBounds(entity.position);
				if (isOutOfBounds) {
					entity.isActive = false;	
				}

				bool isCollidingWithGround = CollideWithGround(entity.position, ground);
				if (isCollidingWithGround) {
					entity.isActive = false;
				}

			}
			
        }


		//thing on screen counter

		//elite ball knowledge
		remaining_targets = std::count_if(all_entities.begin(), all_entities.end(), [](const Entity& e) { return e.entity_type == TARGET && e.isActive; });
		if (remaining_targets <= 0)
			game_state = GAME_WIN;
		
		balls_on_screen = std::count_if(all_entities.begin(), all_entities.end(), [](const Entity& e) { return e.entity_type == BULLET && e.isActive; });


		//handleDrawing of entities using the draw function table
		for ( Entity& entity : all_entities){
			if (entity.isActive ) {
				drawFunctionTable[entity.entity_collider_shape](entity);
			}
		}

		std::erase_if(all_entities, [](const Entity& e) { return !e.isActive; });
        //Draw Turret
        DrawRectangleRec(ground, GREEN);

        DrawRectangleRec(platform, DARKGRAY);

		

        EndDrawing();


		



		break;

		//Main Game Loop End^^

		case GAME_WIN:
			BeginDrawing();
			ClearBackground(WHITE);

			DrawText("You Win!", MAX_WINDOW_WIDTH / 2 - MeasureText("You Win!", 20) / 2, MAX_WINDOW_HEIGHT / 2 - 10, 20, BLACK);
			if (GetClipboardText() != nullptr && GetClipboardText()[0] != '\0')
			{
				DrawText("You currently have something in your clipboard. Could it be:", MAX_WINDOW_WIDTH / 2 - MeasureText("You currently have something in your clipboard. Could it be:", 20) / 2, MAX_WINDOW_HEIGHT / 2 + 10, 20, BLACK);
				DrawText(GetClipboardText(), MAX_WINDOW_WIDTH / 2 - MeasureText(GetClipboardText(), 20) / 2, MAX_WINDOW_HEIGHT / 2 + 30, 20, BLACK);
			}
			
			EndDrawing();
			break;
		case GAME_LOSS:
			BeginDrawing();
			ClearBackground(WHITE);
		

			DrawText("You Lose!", MAX_WINDOW_WIDTH / 2 - MeasureText("You Lose!", 20) / 2, MAX_WINDOW_HEIGHT / 2 - 10, 20, BLACK);
			
			EndDrawing();
			break;
		default:
			break;
		}
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
