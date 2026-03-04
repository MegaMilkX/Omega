#pragma once

#include "node_probe.auto.hpp"
#include "world/world.hpp"
#include "collision/collision_world.hpp"
#include "collision/shape/sphere.hpp"


[[cppi_class]];
class ProbeNode : public TActorNode<phyWorld> {
public:
	TYPE_ENABLE();

	phySphereShape	shape;
	phyProbe			collider;

	ProbeNode();
	void onDefault() override;
	void onSpawnActorNode(phyWorld* world) override;
	void onDespawnActorNode(phyWorld* world) override;
};