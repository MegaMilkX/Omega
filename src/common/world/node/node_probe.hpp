#pragma once

#include "node_probe.auto.hpp"
#include "world/world.hpp"
#include "collision/collision_world.hpp"

[[cppi_class]];
class ProbeNode : public TActorNode<CollisionWorld> {
public:
	TYPE_ENABLE();

	CollisionSphereShape	shape;
	ColliderProbe			collider;

	ProbeNode();
	void onDefault() override;
	void onUpdateTransform() override;
	void onUpdate(RuntimeWorld* world, float dt) override;
	void onSpawn(CollisionWorld* world) override;
	void onDespawn(CollisionWorld* world) override;
};