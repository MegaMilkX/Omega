#pragma once

[[cppi_class]];
class AnotherClass {};


[[cppi_class_tpl]];
template<typename T>
class TActorNode {
public:
};

[[cppi_begin]];

typedef int MY_INT;

[[cppi_end]];

[[cppi_class]];
class MyActorNode : public TActorNode<MY_INT> {
public:
	[[cppi_decl]]
	MY_INT hello;
};