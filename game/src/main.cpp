#include "raylib.h"
#include "raymath.h"
#include "raygui.h"
#include <algorithm>
#include <vector>
#include <iostream>
#include <map>


#define MAX_WINDOW_HEIGHT 800
#define MAX_WINDOW_WIDTH 800    
#define MAX_FPS 60
#define MAX_BULLETS 30
#define TESTING false	
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
enum Collider: int {
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
	Collider entity_collider_type = NONE_SHAPE;
	Vector2 size = { 0.0f, 0.0f }; // For box entities, 0 init for safety
	Vector2 capsule_direction{};
	float capsule_half_length = 0.0f;
	float capsule_length = 0.0f;
	bool follows_mouse = false;
	std::string name{};
	bool apply_mtv = false;


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

void CircleBoxCollision(Entity& circleEntity, Entity& boxEntity, Vector2* mtv) {

	if (!circleEntity.apply_mtv && !boxEntity.apply_mtv) {
		circleEntity.isActive = false;
		boxEntity.isActive = false;
		return;
	}
	// Box positions represent their centers.
	Rectangle box = BoxBoundsMath(boxEntity);

	Vector2 boxMin = { box.x, box.y };
	Vector2 boxMax = { box.x + box.width, box.y + box.height };

	Vector2 closestPoint = Vector2Clamp(
		circleEntity.position, boxMin, boxMax
	);

	Vector2 difference = Vector2Subtract(
		circleEntity.position, closestPoint
	);

	float distanceSquared = Vector2LengthSqr(difference);
	float radius = circleEntity.radius;

	if (distanceSquared > radius * radius) {
		return;
	}



	Vector2 direction{};
	float overlap = 0.0f;

	if (distanceSquared > 0.0f) {
		float distance = sqrtf(distanceSquared);
		direction = Vector2Scale(difference, 1.0f / distance);
		overlap = radius - distance;
	}
	else {

		float nearestFace = circleEntity.position.x - boxMin.x;
		direction = { -1.0f, 0.0f };

		float right = boxMax.x - circleEntity.position.x;
		if (right < nearestFace) {
			nearestFace = right;
			direction = { 1.0f, 0.0f };
		}

		float top = circleEntity.position.y - boxMin.y;
		if (top < nearestFace) {
			nearestFace = top;
			direction = { 0.0f, -1.0f };
		}

		float bottom = boxMax.y - circleEntity.position.y;
		if (bottom < nearestFace) {
			nearestFace = bottom;
			direction = { 0.0f, 1.0f };
		}

		overlap = nearestFace + radius;
	}

	Vector2 correction = Vector2Scale(direction, overlap);

	if (mtv != nullptr) {
		*mtv = correction;
	}

	bool moveBoth = circleEntity.apply_mtv && boxEntity.apply_mtv;
	Vector2 movement = Vector2Scale(correction, moveBoth ? 0.5f : 1.0f);

	if (circleEntity.apply_mtv) {
		circleEntity.position = circleEntity.position + movement; 
	}

	if (boxEntity.apply_mtv) {
		boxEntity.position = boxEntity.position - movement;
	}
}
void CircleCircleCollision(Entity& circleEntityA, Entity& circleEntityB, Vector2* mtv) {
	if (!circleEntityA.apply_mtv && !circleEntityB.apply_mtv) {
		
		if (CheckCollisionCircles(circleEntityA.position, circleEntityA.radius, circleEntityB.position, circleEntityB.radius)) {
			circleEntityA.isActive = false;
			circleEntityB.isActive = false;
		}
		
	} else{ 
		float distance = Vector2Distance(circleEntityA.position, circleEntityB.position);
		float radiiSum = circleEntityA.radius + circleEntityB.radius;
		Vector2 mtv_direction = Vector2Subtract(circleEntityA.position, circleEntityB.position);
		if (Vector2Length(mtv_direction) > 0) {
			mtv_direction = Vector2Normalize(mtv_direction);
		}
		float magnitude = radiiSum - distance;
		bool collision = distance < radiiSum;
		if (collision && mtv != nullptr) {
			*mtv = Vector2Scale(mtv_direction, magnitude);
			circleEntityA.position = Vector2Add(circleEntityA.position, *mtv);
			circleEntityB.position = Vector2Subtract(circleEntityB.position, *mtv);
		}
	}
}
void BoxBoxCollision(Entity& boxEntityA, Entity& boxEntityB, Vector2* mtv) {

	if (CheckCollisionRecs(BoxBoundsMath(boxEntityA), BoxBoundsMath(boxEntityB))) {
		if (!boxEntityA.apply_mtv && !boxEntityB.apply_mtv) {
			boxEntityA.isActive = false;
			boxEntityB.isActive = false;
		}
		else {
			Vector2 difference = Vector2Subtract(boxEntityA.position, boxEntityB.position);

			float overlapX = (boxEntityA.size.x + boxEntityB.size.x) * 0.5f - fabsf(difference.x);
			float overlapY = (boxEntityA.size.y + boxEntityB.size.y) * 0.5f - fabsf(difference.y);

			Vector2 correction{};

			if (overlapX < overlapY) {
				correction.x = difference.x < 0.0f ? -overlapX : overlapX;
			}
			else {
				correction.y = difference.y < 0.0f ? -overlapY : overlapY;
			}

			if (mtv != nullptr) {
				*mtv = correction;
			}
			Vector2 movement = Vector2Scale(correction, 0.5f);
			boxEntityA.position = Vector2Add(boxEntityA.position, movement);
			boxEntityB.position = Vector2Subtract(boxEntityB.position, movement);
			
		}
	}
}

void CapsuleCapsuleCollision(Entity& capsuleEntityA, Entity& capsuleEntityB, Vector2* mtv) {

	Vector2 topA = capsuleEntityA.position + capsuleEntityA.capsule_direction * capsuleEntityA.capsule_half_length;
	Vector2 bottomA = capsuleEntityA.position - capsuleEntityA.capsule_direction * capsuleEntityA.capsule_half_length;
	Vector2 topB = capsuleEntityB.position + capsuleEntityB.capsule_direction * capsuleEntityB.capsule_half_length;
	Vector2 bottomB = capsuleEntityB.position - capsuleEntityB.capsule_direction * capsuleEntityB.capsule_half_length;
	Vector2 np1 = capsuleEntityA.position;
	Vector2 np2 = capsuleEntityB.position;
	NearestPointsOnLines(bottomA, topA, bottomB, topB, &np1, &np2);
	
	if (CheckCollisionCircles(np1, capsuleEntityA.radius, np2, capsuleEntityB.radius)) {
		if (capsuleEntityA.apply_mtv && capsuleEntityB.apply_mtv && mtv != nullptr) {
			Vector2 mtv_direction = Vector2Subtract(np1, np2);
			if (Vector2Length(mtv_direction) > 0) {
				mtv_direction = Vector2Normalize(mtv_direction);
			}
			float distance = Vector2Distance(np1, np2);
			float radiiSum = capsuleEntityA.radius + capsuleEntityB.radius;
			float magnitude = radiiSum - distance;
			*mtv = Vector2Scale(mtv_direction, magnitude);
			capsuleEntityA.position = Vector2Add(capsuleEntityA.position, *mtv);
			capsuleEntityB.position = Vector2Subtract(capsuleEntityB.position, *mtv);
		}
		else {
			capsuleEntityA.isActive = false;
			capsuleEntityB.isActive = false;
		}
	}
}


void CapsuleCircleCollision(Entity& capsuleEntity, Entity& circleEntity, Vector2* mtv) {
	//1)
	Vector2 top = capsuleEntity.position + capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;
	//2)
	Vector2 bottom = capsuleEntity.position - capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;

	Vector2 closestPoint = Vector2ProjectPointLine(circleEntity.position, bottom, top);
	float distanceSquared = Vector2DistanceSqr(circleEntity.position, closestPoint);
	float radiiSum = circleEntity.radius + capsuleEntity.radius;
	if (distanceSquared <= radiiSum * radiiSum) {

		float overlap = radiiSum - distanceSquared;
		Vector2 mtv_direction = Vector2Normalize(Vector2Subtract(closestPoint, circleEntity.position));
		if (!capsuleEntity.apply_mtv && !circleEntity.apply_mtv) {
			//circleEntity.isActive = false;
			//capsuleEntity.isActive = false;
		}
		else
		{
			*mtv = Vector2Scale(mtv_direction, overlap);
			capsuleEntity.position = Vector2Add(capsuleEntity.position, *mtv);
			circleEntity.position = Vector2Subtract(circleEntity.position, *mtv);
		}
	}
	
}
void CapsuleBoxCollision(Entity& capsuleEntity, Entity& boxEntity, Vector2* mtv) {

	
	if (CheckCapsuleBox(capsuleEntity, boxEntity)) {
		if (!capsuleEntity.apply_mtv && !boxEntity.apply_mtv) {
			capsuleEntity.isActive = false;
			boxEntity.isActive = false;
			return;
		}
		else {
			Vector2 top = capsuleEntity.position + capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;
			Vector2 bottom = capsuleEntity.position - capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;

			Rectangle box = BoxBoundsMath(boxEntity);
			Vector2 boxMin = { box.x, box.y };
			Vector2 boxMax = { box.x + box.width, box.y + box.height };

			bool insideBox = CheckCollisionPointRec(top, box) || CheckCollisionPointRec(bottom, box);

			Vector2 corners[4] = {
				{ boxMin.x, boxMin.y },
				{ boxMax.x, boxMin.y },
				{ boxMax.x, boxMax.y },
				{ boxMin.x, boxMax.y }
			};

			for (int i = 0; i < 4; i++) {
				Vector2 intersection{};

				if (CheckCollisionLines(bottom, top, corners[i], corners[(i + 1) % 4], &intersection)) {
					insideBox = true;
					break;
				}
			}

			Vector2 correction{};

			if (insideBox) {
				// Find the outside edges of the capsule.
				float capsuleLeft = std::min(top.x, bottom.x) - capsuleEntity.radius;
				float capsuleRight = std::max(top.x, bottom.x) + capsuleEntity.radius;
				float capsuleTop = std::min(top.y, bottom.y) - capsuleEntity.radius;
				float capsuleBottom = std::max(top.y, bottom.y) + capsuleEntity.radius;

				// Find the shortest push out through a box side.
				float overlap = capsuleRight - boxMin.x;
				Vector2 direction = { -1.0f, 0.0f };

				float right = boxMax.x - capsuleLeft;
				if (right < overlap) {
					overlap = right;
					direction = { 1.0f, 0.0f };
				}

				float up = capsuleBottom - boxMin.y;
				if (up < overlap) {
					overlap = up;
					direction = { 0.0f, -1.0f };
				}

				float down = boxMax.y - capsuleTop;
				if (down < overlap) {
					overlap = down;
					direction = { 0.0f, 1.0f };
				}

				correction = Vector2Scale(direction, overlap);
			}
			else {
				Vector2 nearestBoxPoint{};
				Vector2 nearestCapsulePoint{};

				NearestVerticesOnBoxToCapsule(boxEntity, capsuleEntity, &nearestBoxPoint, &nearestCapsulePoint);

				float minDistanceSqrd = Vector2DistanceSqr(nearestBoxPoint, nearestCapsulePoint);

				// Compare the top against the box
				Vector2 closestTop = Vector2Clamp(top, boxMin, boxMax);
				float topDistanceSquared = Vector2DistanceSqr(top, closestTop);

				if (topDistanceSquared < minDistanceSqrd) {
					minDistanceSqrd = topDistanceSquared;
					nearestBoxPoint = closestTop;
					nearestCapsulePoint = top;
				}

				// Compare the bottom against the box
				Vector2 closestBottom = Vector2Clamp(bottom, boxMin, boxMax);
				float bottomDistanceSquared = Vector2DistanceSqr(bottom, closestBottom);

				if (bottomDistanceSquared < minDistanceSqrd) {
					minDistanceSqrd = bottomDistanceSquared;
					nearestBoxPoint = closestBottom;
					nearestCapsulePoint = bottom;
				}

				if (minDistanceSqrd <= 0.0f) {
					return;
				}

				float distance = sqrtf(minDistanceSqrd);
				float overlap = capsuleEntity.radius - distance;

				if (overlap <= 0.0f) {
					return;
				}

				Vector2 direction = Vector2Scale(Vector2Subtract(nearestCapsulePoint, nearestBoxPoint), 1.0f / distance);
				correction = Vector2Scale(direction, overlap);
			}

			//if i dont wrap it I get a "de-referencing nullptr" error??? 
			//wrap it before you tap it
			if (mtv != nullptr) {
				*mtv = correction;
			}
				Vector2 movement = Vector2Scale(correction, 0.5f);
				capsuleEntity.position = Vector2Add(capsuleEntity.position, movement);
				boxEntity.position = Vector2Subtract(boxEntity.position, movement);
			
		}
	}
}

void BoxCapsuleCollision(Entity& boxEntity, Entity& capsuleEntity, Vector2* mtv) {
	CapsuleBoxCollision(capsuleEntity, boxEntity, mtv);
}
void CircleCapsuleCollision(Entity& circleEntity, Entity& capsuleEntity, Vector2* mtv) {
	//1)
	Vector2 top = capsuleEntity.position + capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;
	//2)
	Vector2 bottom = capsuleEntity.position - capsuleEntity.capsule_direction * capsuleEntity.capsule_half_length;

	Vector2 closestPoint = Vector2ProjectPointLine(circleEntity.position, bottom, top);
	float distanceSquared = Vector2DistanceSqr(circleEntity.position, closestPoint);
	float radiiSum = circleEntity.radius + capsuleEntity.radius;
	if (distanceSquared <= radiiSum * radiiSum){



		float distance = sqrtf(distanceSquared);
		float overlap = radiiSum - distance;
		Vector2 mtv_direction = Vector2Normalize(Vector2Subtract(circleEntity.position, closestPoint));
		if (!capsuleEntity.apply_mtv && !circleEntity.apply_mtv) {
			circleEntity.isActive = false;
			capsuleEntity.isActive = false;
		}
		else
		{
			*mtv = Vector2Scale(mtv_direction, overlap);
			capsuleEntity.position = Vector2Subtract(capsuleEntity.position, *mtv);
			circleEntity.position = Vector2Add(circleEntity.position, *mtv);
		}
	}
}

void BoxCircleCollision(Entity& boxEntity, Entity& circleEntity, Vector2* mtv) {
	CircleBoxCollision(circleEntity, boxEntity, mtv);
}
void HitTestNone(Entity& entityA, Entity& entityB, Vector2* mtv) {
	// No collision logic for NONE shape
	
}

using CollisionFunc = void(*)(Entity&, Entity&, Vector2* mtv);

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
			new_target.entity_collider_type = CIRCLE;
		}
		else {
			new_target.size = { static_cast<float>(GetRandomValue(30, 60)), static_cast<float>(GetRandomValue(30, 60)) };
			new_target.color = BLUE;
			new_target.entity_collider_type = BOX;
		}
		new_target.entity_type = TARGET;
		new_target.isActive = true;
		targets.push_back(new_target);
	}
}


void SwapUserTestEntity(Entity& user_test_entity, Collider new_collider = NONE_SHAPE) { //auto define as none_shape so we never face issues lol

	if (new_collider == NONE_SHAPE) {
		std::cout << "Not a valid Option";
	}
	else {
		user_test_entity.entity_collider_type = new_collider;
	}

}


struct GUIButtons {
	std::string button_text{};
	float width{};
	float height{};
	int array_pointer{};
	Vector2 size{};
	Vector2 position{};
	Rectangle box{};
	Collider collider_to_use{}; //assignment 1 specific but will be using some elite ball knowledge in a moment
	int gui_state = 0; // based on rayGUI
};

/*typedef enum {
	STATE_NORMAL = 0,
	STATE_FOCUSED,
	STATE_PRESSED,
	STATE_DISABLED
} GuiState;*/

int main()
{
	SetupCalls();


    
	std::vector<std::string> names = {
	"Alex",
	"Jordan",
	"Taylor",
	"Morgan",
	"Casey",
	"Riley",
	"Cameron",
	"Dylan",
	"Avery",
	"Logan",
	"Parker",
	"Quinn",
	"Blake",
	"Reese",
	"Jamie",
	"Skyler",
	"Charlie",
	"Dakota",
	"Rowan",
	"Finley"
	};



	// map because Elite Ball Knowledge
	//That Ball knowledge being how much you hate maps
	std::map<Collider, std::string> collider_to_map = { {NONE_SHAPE, "None"}, {CIRCLE, "Circle"}, {BOX, "Box"}, { CAPSULE, "Capsule" } };


	std::vector<Entity> all_entities;
	all_entities.resize(11);



	//GameState
	GameState game_state = MAIN;

	//LMFAO Moment I realized I drifted too far from what youve been teaching this semester.
	//If you dock marks for this crap I can't even be mad
	//is it better to write bad code and learn from it or write good code and not learn anything? 
	//probably better to just not write code at midnight.
	Entity user_test_entity;
		user_test_entity.position = { 400.0f, 400.0f };
		user_test_entity.radius = 20.0f;
		user_test_entity.color = GREEN;
		user_test_entity.entity_collider_type = CIRCLE;
		user_test_entity.entity_type = BULLET;
		user_test_entity.isActive = true;
		user_test_entity.follows_mouse = true;
		user_test_entity.apply_mtv = true;
		user_test_entity.capsule_direction = Vector2Normalize({ 1.0f, 1.0f });
		user_test_entity.capsule_half_length = 30.0f;
		user_test_entity.capsule_length = 60.0f;
		user_test_entity.size = { 40.0f, 40.0f };
		user_test_entity.name = "SmileyUser";
	all_entities.push_back(user_test_entity);
			

	std::vector<GUIButtons> all_buttons;
	std::vector<GUIButtons> top_row;
	std::vector<GUIButtons> bot_row;
	all_buttons.resize(8);

	for (int i = 0; i < all_buttons.size(); i++) {
		GUIButtons& newButton = all_buttons[i];
		std::string append_letter{};
		newButton.height = 80.0f;
		newButton.width = 120.0f;
		if (i < 4) {
			newButton.position = { 100.0f + (i * 160), 20.0f };
			newButton.collider_to_use = (Collider)i;
			append_letter = " A";
			top_row.push_back(newButton);
		}
		else {
			newButton.position = { 100.0f + ((i-4) * 160), 100.0f };
			newButton.collider_to_use = (Collider)(i-4);
			append_letter = " B";
		}
		newButton.button_text = collider_to_map[newButton.collider_to_use] + append_letter;
		
		newButton.size = { newButton.width, newButton.height };
		newButton.box = { newButton.position.x,newButton.position.y, newButton.size.x, newButton.size.y };
		newButton.array_pointer = i; //I might use this later, not sure
		if (i < 4) { //bad coding but last minute decision
			top_row.push_back(newButton);
		}
		else {
			bot_row.push_back(newButton);
		}
	}

    while (!WindowShouldClose())
    {
		float dt = GetFrameTime();
		switch (game_state)
		{
		case MAIN:
			
			
			




			BeginDrawing();
			ClearBackground(WHITE);
			





			
			//mousePos needs to be done before collisions to avoid clipping

			for (Entity& entity : all_entities) {

				if (!entity.isActive)
					continue;

				if (entity.follows_mouse) {
					entity.position = GetMousePosition(); 
					continue;
				}

			}


		//collision checking
		for (int i = 0; i < all_entities.size(); ++i) {
			Entity& entityA = all_entities[i];
			if (!entityA.isActive) continue;
			for (int j = i + 1; j < all_entities.size(); ++j) {
				Entity& entityB = all_entities[j];
				if (!entityB.isActive) continue;
				Vector2 mtv = Vector2Zeros;
				CollisionFunc collisionFunc = collisionFunctionTable[entityA.entity_collider_type][entityB.entity_collider_type];
				if (collisionFunc) {
					collisionFunc(entityA, entityB, &mtv);
				}

				if (!entityA.isActive) break;
			}
		}


		




		//handleDrawing of entities using the draw function table
		for ( Entity& entity : all_entities){
			if (entity.isActive ) {
				drawFunctionTable[entity.entity_collider_type](entity);
				DrawText(TextFormat("%s", entity.name.c_str()), entity.position.x - 20, entity.position.y - 20, 10, BLACK);
			}
		}


		//starting to feel less like elite ball knowledge
		for (GUIButtons& buttons : top_row) {
			GuiSetState(buttons.gui_state);
			if (GuiButton(buttons.box, buttons.button_text.c_str())) {
				//I call it "other" but really im just resetting everything lol
				for (GUIButtons& other : top_row) { 
					other.gui_state = STATE_NORMAL;
				}
				buttons.gui_state = STATE_PRESSED;
			};
		}
		for (GUIButtons& buttons : bot_row) {
			GuiSetState(buttons.gui_state);
			if (GuiButton(buttons.box, buttons.button_text.c_str())) {
				for (GUIButtons& other : bot_row) {
					other.gui_state = STATE_NORMAL;
				}
				buttons.gui_state = STATE_PRESSED;
			};
		}



		std::erase_if(all_entities, [](const Entity& e) { return !e.isActive; });

		

        EndDrawing();


		break;

		//Main Game Loop End^^

		case GAME_WIN:
			BeginDrawing();

			
			EndDrawing();
			break;
		case GAME_LOSS:
			BeginDrawing();
			ClearBackground(WHITE);
			
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
