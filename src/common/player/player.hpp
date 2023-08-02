#pragma once


class IPlayer {
public:
    virtual ~IPlayer() {}
};

class LocalPlayer : public IPlayer {
public:
};

class NetworkPlayer : public IPlayer {
public:
};

class AiPlayer : public IPlayer {
public:
};

class ReplayPlayer : public IPlayer {
public:
};
