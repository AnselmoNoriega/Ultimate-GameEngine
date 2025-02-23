#include "nrpch.h"
#include "Physics2D.h"

// Box2D
#include <box2d/box2d.h>

namespace NR
{
	struct Box2DWorldComponent
	{
		std::unique_ptr<b2World> World;
	};

	std::vector<Raycast2DResult> Physics2D::Raycast(Ref<Scene> scene, const glm::vec2& point0, const glm::vec2& point1)
	{
		class RayCastCallback : public b2RayCastCallback
		{
		public:
			RayCastCallback(std::vector<Raycast2DResult>& results, glm::vec2 p0, glm::vec2 p1)
				: mResults(results), mPoint0(p0), mPoint1(p1)
			{

			}
			virtual float ReportFixture(b2Fixture* fixture, const b2Vec2& point,
				const b2Vec2& normal, float fraction)
			{
				Entity& entity = *(Entity*)fixture->GetBody()->GetUserData().pointer;

				float distance = glm::distance(mPoint0, mPoint1) * fraction;
				mResults.emplace_back(entity, glm::vec2(point.x, point.y), glm::vec2(normal.x, normal.y), distance);
				return 1.0f;
			}
		private:
			std::vector<Raycast2DResult>& mResults;
			glm::vec2 mPoint0, mPoint1;
		};

		auto& b2dWorld = scene->mRegistry.get<Box2DWorldComponent>(scene->mSceneEntity).World;
		std::vector<Raycast2DResult> results;
		RayCastCallback callback(results, point0, point1);
		b2dWorld->RayCast(&callback, { point0.x, point0.y }, { point1.x, point1.y });
		return results;
	}

}