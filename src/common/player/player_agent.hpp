#pragma once


class IPlayer;
class IPlayerAgent {
    friend IPlayer;
    friend void playerLinkAgent(IPlayer* player, IPlayerAgent* agent);

    IPlayer* current_player = 0;
public:
    virtual ~IPlayerAgent();

    virtual void onPlayerAttach(IPlayer* player) = 0;
    virtual void onPlayerDetach(IPlayer* player) = 0;

    void attachPlayer(IPlayer* player);
    void detachPlayer();
};