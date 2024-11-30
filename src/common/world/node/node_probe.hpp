#pragma once

#include "node_probe.auto.hpp"
#include "world/world.hpp"

[[cppi_class]];
class ProbeNode : public ActorNode {
public:
	TYPE_ENABLE();

	CollisionSphereShape	shape;
	ColliderProbe			collider;

	ProbeNode();
	void onDefault() override;
	void onUpdateTransform() override;
	void onUpdate(RuntimeWorld* world, float dt) override;
	void onSpawn(RuntimeWorld* world) override;
	void onDespawn(RuntimeWorld* world) override;
};