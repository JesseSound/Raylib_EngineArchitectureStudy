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
#define TESTING true	
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
	Vector2 capsule_direction{};
	float capsule_half_length = 0.0f;
	float capsule_length = 0.0f;
	bool follows_mouse = false;
	void OutOfBounds(const Vector2& position) {
		bool CheckPosition = (position.x < 0 || position.x > MAX_WINDOW_WIDTH || position.y < 0 || position.y > MAX_WINDOW_HEIGHT);
		if (CheckPosition) {
			isActive = false;
		}	
	}

	void RunEntityLogic() {
		if (entity_type == BULLET && isActive) {
			OutOfBounds(position);
		}
	}

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


RMAPI Vector2 Vector2ProjectPointLine(Vector2 point, Vector2 A, Vector2 B)
{
	Vector2 AB = (B - A);
	float t = Vector2DotProduct((point - A), AB) / Vector2DotProduct(AB, AB);
	return A + (AB * Clamp(t, 0.0f, 1.0f));
}
void NearestPointsOnLines(Vector2 bot1, Vector2 top1, Vector2 bot2, Vector2 top2, Vector2* nearestPoint1, Vector2* nearestPoint2) {

	

	{
		Vector2 A = Vector2ProjectPointLine(top1, bot2, top2);
		Vector2 B = Vector2ProjectPointLine(bot1, bot2, top2);
		Vector2 C = Vector2ProjectPointLine(A, bot1, top1);
		Vector2 D = Vector2ProjectPointLine(B, bot1, top1);
		*nearestPoint1 = Vector2LengthSqr(A - C) < Vector2LengthSqr(B - D) ? C : D; //Point on line 1 nearest to line 2
	}


	{
		Vector2 A = Vector2ProjectPointLine(top2, bot1, top1);
		Vector2 B = Vector2ProjectPointLine(bot2, bot1, top1);
		Vector2 C = Vector2ProjectPointLine(A, bot2, top2);
		Vector2 D = Vector2ProjectPointLine(B, bot2, top2);
		*nearestPoint2 = Vector2LengthSqr(A - C) < Vector2LengthSqr(B - D) ? C : D; //Point on line 2 nearest to line 1
	}

	if (TESTING) {
		DrawLineEx(*nearestPoint1, *nearestPoint2, 2.0f, RED);
		DrawCircleV(*nearestPoint1, 5.0f, GREEN);
		DrawCircleV(*nearestPoint2, 5.0f, BLUE);
	}
	

}
//save typing time by making code reusable 
Rectangle BoxBoundsMath(Entity& boxEntity) {
	return { boxEntity.position.x - boxEntity.size.x / 2, boxEntity.position.y - boxEntity.size.y / 2, boxEntity.size.x, boxEntity.size.y };
}
void NearestVerticesOnBoxToCapsule(Entity& boxEntity, Entity& capsuleEntity, Vector2* nearestVertex, Vector2* nearestPointOnCapsule) {
	// Calculate the top and bottom points of the capsule
	Vector2 top = capsuleEntity.position + capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;
	Vector2 bottom = capsuleEntity.position - capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;
	// Get the box bounds
	Rectangle boxBounds = BoxBoundsMath(boxEntity);
	// Find the nearest point on the capsule to the box
	Vector2 closestPointOnCapsule = Vector2ProjectPointLine(boxEntity.position, bottom, top);
	//Give me some vertices to work with
	Vector2 vertices[4] = {
	  { boxBounds.x, boxBounds.y },
	  { boxBounds.x + boxBounds.width, boxBounds.y },
	  { boxBounds.x + boxBounds.width, boxBounds.y + boxBounds.height },
	  { boxBounds.x, boxBounds.y + boxBounds.height }
	};

	float nearestDistanceSquared = FLT_MAX;

	for (int i = 0; i < 4; i++) {

		Vector2 projectedPoint =
			Vector2ProjectPointLine(vertices[i], bottom, top);

		float distanceSquared =
			Vector2DistanceSqr(vertices[i], projectedPoint);

		if (distanceSquared < nearestDistanceSquared) {
			nearestDistanceSquared = distanceSquared;
			*nearestVertex = vertices[i];
			*nearestPointOnCapsule = projectedPoint;
		}
	}

	



}

//seperated due to length
bool CheckCapsuleBox(Entity& capsuleEntity, Entity& boxEntity) {

	Vector2 top = capsuleEntity.position + capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;

	Vector2 bottom = capsuleEntity.position - capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;

	Rectangle box = BoxBoundsMath(boxEntity);

	Vector2 boxMin = { box.x, box.y };
	Vector2 boxMax = { box.x + box.width, box.y + box.height };

	Vector2 nearestVertex;
	Vector2 nearestCapsulePoint;

	NearestVerticesOnBoxToCapsule(boxEntity,capsuleEntity,&nearestVertex,&nearestCapsulePoint);

	float minDistanceSquared = Vector2DistanceSqr(nearestVertex, nearestCapsulePoint);

	
	Vector2 closestTop = Vector2Clamp(top, boxMin, boxMax);
	Vector2 closestBottom = Vector2Clamp(bottom, boxMin, boxMax);

	minDistanceSquared = std::min(minDistanceSquared,Vector2DistanceSqr(top, closestTop));

	minDistanceSquared = std::min(minDistanceSquared,Vector2DistanceSqr(bottom, closestBottom));

	
	if (CheckCollisionPointRec(top, box) ||CheckCollisionPointRec(bottom, box)) {
		return true;
	}

	Vector2 corners[4] = {
		{ box.x, box.y },
		{ box.x + box.width, box.y },
		{ box.x + box.width, box.y + box.height },
		{ box.x, box.y + box.height }
	};

	for (int i = 0; i < 4; i++) {
		Vector2 intersection;

		if (CheckCollisionLines(bottom, top,corners[i], corners[(i + 1) % 4],&intersection)) {
			return true;
		}
	}

	
	return minDistanceSquared <=capsuleEntity.radius * capsuleEntity.radius;
}


bool CollideWithGround(const Vector2& position, const Rectangle& ground) {
	return (position.y + BALL_RADIUS >= ground.y);
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

	Vector2 topA = capsuleEntityA.position + capsuleEntityA.capsule_direction * capsuleEntityA.capsule_half_length;
	Vector2 bottomA = capsuleEntityA.position - capsuleEntityA.capsule_direction * capsuleEntityA.capsule_half_length;
	Vector2 topB = capsuleEntityB.position + capsuleEntityB.capsule_direction * capsuleEntityB.capsule_half_length;
	Vector2 bottomB = capsuleEntityB.position - capsuleEntityB.capsule_direction * capsuleEntityB.capsule_half_length;
	Vector2 np1 = capsuleEntityA.position;
	Vector2 np2 = capsuleEntityB.position;
	NearestPointsOnLines(bottomA, topA, bottomB, topB, &np1, &np2);
	
	if (CheckCollisionCircles(np1, capsuleEntityA.radius, np2, capsuleEntityB.radius)) {
		capsuleEntityA.isActive = false;
		capsuleEntityB.isActive = false;
	}
}


void CapsuleCircleCollision(Entity& capsuleEntity, Entity& circleEntity) {
	//1)
	Vector2 top = capsuleEntity.position + capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;
	//2)
	Vector2 bottom = capsuleEntity.position - capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;

	Vector2 closestPoint = Vector2ProjectPointLine(circleEntity.position, bottom, top);
	float distanceSquared = Vector2DistanceSqr(circleEntity.position, closestPoint);
	if (distanceSquared <= (circleEntity.radius + capsuleEntity.radius) * (circleEntity.radius + capsuleEntity.radius)) {
		circleEntity.isActive = false;
		capsuleEntity.isActive = false;
	}
	
}
void CapsuleBoxCollision(Entity& capsuleEntity, Entity& boxEntity) {
	if (CheckCapsuleBox(capsuleEntity, boxEntity)) {
		capsuleEntity.isActive = false;
		boxEntity.isActive = false;
	}
}

void BoxCapsuleCollision(Entity& boxEntity, Entity& capsuleEntity) {
	CapsuleBoxCollision(capsuleEntity, boxEntity);
}
void CircleCapsuleCollision(Entity& circleEntity, Entity& capsuleEntity) {
	//1)
	Vector2 top = capsuleEntity.position + capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;
	//2)
	Vector2 bottom = capsuleEntity.position - capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;

	Vector2 closestPoint = Vector2ProjectPointLine(circleEntity.position, bottom, top);
	float distanceSquared = Vector2DistanceSqr(circleEntity.position, closestPoint);
	if (distanceSquared <= (circleEntity.radius + capsuleEntity.radius) * (circleEntity.radius + capsuleEntity.radius)) {
		circleEntity.isActive = false;
		capsuleEntity.isActive = false;
	}
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
	

	float rotation_angle = Vector2Angle(Vector2UnitX, entity.capsule_direction) * RAD2DEG;

	Vector2 top = entity.position + entity.capsule_direction * entity.capsule_half_length;
	Vector2 bottom = entity.position - entity.capsule_direction * entity.capsule_half_length;
	
	

	Rectangle rec;
	rec.x = entity.position.x;
	rec.y = entity.position.y;
	rec.width = entity.capsule_half_length * 2.0f;
	rec.height = entity.radius * 2.0f;



	DrawRectanglePro(rec, { entity.capsule_half_length, entity.radius }, rotation_angle, entity.color);
	DrawCircleV(top, entity.radius, entity.color);
	DrawCircleV(bottom, entity.radius, entity.color);

	if (TESTING) {
		DrawLineEx(entity.position, top, entity.capsule_half_length *0.1f, BLACK);
		DrawLineEx(top, top + Vector2Rotate(entity.capsule_direction, 45.0f * DEG2RAD) * entity.radius, entity.radius * 0.1f, SKYBLUE);
		DrawCircleV(top, entity.radius * 0.25f, BLUE);
		DrawCircleV(bottom, entity.radius * 0.25f, BLUE);
	}




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


	//JUST FOR LAB 4
	Entity CapsuleEntity;
	
	CapsuleEntity.radius = 20.0f;
	CapsuleEntity.capsule_half_length = 60.0f;
	CapsuleEntity.color = PURPLE;
	CapsuleEntity.entity_type = PILL;
	CapsuleEntity.entity_collider_shape = CAPSULE;
	CapsuleEntity.isActive = true;
	CapsuleEntity.follows_mouse = true;
	CapsuleEntity.capsule_direction = Vector2Normalize(Vector2Subtract(Vector2Zero(), ball_launch_position));

	
	Entity testCapsule;
	testCapsule.position = { 400.0f, 400.0f };
	testCapsule.capsule_direction = { 0.0f, 1.0 };
	testCapsule.color = YELLOW;
	testCapsule.capsule_half_length = 50.0f;
	testCapsule.radius = 20.0f;
	testCapsule.entity_type = PILL;
	testCapsule.entity_collider_shape = CAPSULE;
	testCapsule.isActive = true;
	Entity testBox;
	testBox.position = { 500.0f, 400.0f };
	testBox.size = { 100.0f, 100.0f };	
	testBox.color = ORANGE;
	testBox.entity_type = TARGET;
	testBox.entity_collider_shape = BOX;
	testBox.isActive = true;
	Entity testCircle;
	testCircle.radius = 20.0f;
	testCircle.color = GREEN;
	testCircle.entity_collider_shape = CIRCLE;
	testCircle.position = { 150.0f,200.f };
	testCircle.entity_collider_shape = CIRCLE;
	testCircle.isActive = true;
	all_entities.push_back(testCircle);
	all_entities.push_back(testBox);
	all_entities.push_back(testCapsule);
	all_entities.push_back(CapsuleEntity);

	bool isColliding = false;
	//ABOVE IS JUST FOR LAB 4



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


						wide_bullet.velocity = Vector2Rotate(Vector2UnitX, (ball_launch_angle + i * 20.0f) * DEG2RAD) * 200.0f;
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
				ball_launch_angle -= 100.0f * dt;
			}
			if (IsKeyDown(KEY_DOWN)) {
				ball_launch_angle += 100.0f * dt;
			}

			if (IsKeyPressed(KEY_TAB)) {
				if (bullet_shape == CIRCLE_BULLET) {
					bullet_shape = BOX_BULLET;
				}
				else if (bullet_shape == BOX_BULLET) {
					bullet_shape = CIRCLE_BULLET;
				}
			}

			BeginDrawing();
			ClearBackground(WHITE);
			//"UI" Studd
			DrawText(TextFormat("Time: %f", countdown_timer), 10, 10, 20, DARKGRAY);
			DrawText(TextFormat("Weapon Selected: %s", weapon_type == TARGETSHOT ? "Target Shot" : "Wide Shot"), 10, 40, 20, DARKGRAY);
			DrawText(TextFormat("Targets Remaining: %i", remaining_targets), 10, 70, 20, DARKGRAY);
			


		//collision checking
		for (int i = 0; i < all_entities.size(); ++i) {
			Entity& entityA = all_entities[i];
			if (!entityA.isActive) continue;
			for (int j = i + 1; j < all_entities.size(); ++j) {
				Entity& entityB = all_entities[j];
				if (!entityB.isActive) continue;
	
				if (entityA.entity_type == BULLET && entityB.entity_type == BULLET) {
					continue;
				}
				if (entityA.entity_type == TARGET && entityB.entity_type == TARGET) {
					continue;
				}
				CollisionFunc collisionFunc = collisionFunctionTable[entityA.entity_collider_shape][entityB.entity_collider_shape];
				if (collisionFunc) {
					collisionFunc(entityA, entityB);
				}

				if (!entityA.isActive) break;
			}
		}


		for (Entity& entity : all_entities) {

			if (!entity.isActive)
				continue;

			if (entity.follows_mouse) {
				entity.position = GetMousePosition();
				continue;
			}

			if (entity.entity_type == BULLET) {

				entity.velocity += GRAVITY * dt * ball_gravity_scale;
				entity.position += entity.velocity * dt;

				entity.RunEntityLogic();

				if (CollideWithGround(entity.position, ground)) {
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
