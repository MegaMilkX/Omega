#pragma once

#include <vector>
#include <memory>
#include "animation/animation.hpp"
//#include "animation/hitbox_sequence/hitbox_sequence.hpp"
#include "animation/hitbox_sequence_2/hitbox_sequence.hpp"
#include "animation/audio_sequence/audio_sequence.hpp"
#include "animation/event_sequence/event_sequence.hpp"


enum SEQ_ED_TRACK_TYPE {
    SEQ_ED_TRACK_EVENT,
    SEQ_ED_TRACK_HITBOX
};

struct SeqEdItem {
    virtual ~SeqEdItem() {}
};

struct SeqEdEventTrack;
struct SeqEdEvent : public SeqEdItem {
    SeqEdEventTrack* track;
    int frame;
    // TODO:
};
struct SeqEdEventTrack {
    std::set<SeqEdEvent*> events;
    ~SeqEdEventTrack() {
        for (auto& e : events) {
            delete e;
        }
        events.clear();
    }
};

struct SeqEdHitboxTrack;
struct SeqEdHitbox : public SeqEdItem {
    SeqEdHitboxTrack* track;
    int frame;
    int length;
    // TODO:
};
struct SeqEdHitboxTrack {
    std::set<SeqEdHitbox*> hitboxes;
    ~SeqEdHitboxTrack() {
        for (auto& h : hitboxes) {
            delete h;
        }
        hitboxes.clear();
    }
};
struct SequenceEditorProject : public IDesignAsset {
    RHSHARED<animEventSequence> event_seq;
    RHSHARED<animHitboxSequence> hitbox_seq;

    std::vector<std::unique_ptr<SeqEdEventTrack>> event_tracks;
    std::vector<std::unique_ptr<SeqEdHitboxTrack>> hitbox_tracks;

    SequenceEditorProject() {
        event_seq.reset_acquire();
        hitbox_seq.reset_acquire();
    }

    // Events
    SeqEdEventTrack* eventTrackAdd() {
        auto trk = new SeqEdEventTrack;
        event_tracks.push_back(std::unique_ptr<SeqEdEventTrack>(trk));
        // No need to recompile event sequence since new track is empty
        return trk;
    }
    void eventTrackRemove(SeqEdEventTrack* trk) {
        for (int i = 0; i < event_tracks.size(); ++i) {
            if (event_tracks[i].get() == trk) {
                event_tracks.erase(event_tracks.begin() + i);
                compileEventSequence();
                break;
            }
        }
    }
    SeqEdEvent* eventAdd(SeqEdEventTrack* track, int frame) {
        auto e = new SeqEdEvent;
        e->frame = frame;
        e->track = track;
        track->events.insert(e);
        compileEventSequence();
        return e;
    }
    void eventRemove(SeqEdEventTrack* track, SeqEdEvent* item) {
        track->events.erase(item);
        delete item;
        compileEventSequence();
    }
    void eventMove(SeqEdEvent* event, SeqEdEventTrack* destination_track, int destination_frame) {
        event->track->events.erase(event);
        destination_track->events.insert(event);
        event->track = destination_track;
        event->frame = destination_frame;
        compileEventSequence();
    }

    void compileEventSequence() {
        event_seq->clear();
        for (auto& trk : event_tracks) {
            for (auto& e : trk->events) {
                event_seq->insert(e->frame, 0/* TODO */);
            }
        }
    }

    // Hitboxes
    SeqEdHitboxTrack* hitboxTrackAdd() {
        auto trk = new SeqEdHitboxTrack;
        hitbox_tracks.push_back(std::unique_ptr<SeqEdHitboxTrack>(trk));
        return trk;
    }
    void hitboxTrackRemove(SeqEdHitboxTrack* trk) {
        for (int i = 0; i < hitbox_tracks.size(); ++i) {
            if (hitbox_tracks[i].get() == trk) {
                hitbox_tracks.erase(hitbox_tracks.begin() + i);
                compileHitboxSequence();
                break;
            }
        }
    }
    SeqEdHitbox* hitboxAdd(SeqEdHitboxTrack* track, int frame, int len) {
        auto hb = new SeqEdHitbox;
        hb->frame = frame;
        hb->length = len;
        hb->track = track;
        track->hitboxes.insert(hb);
        compileHitboxSequence();
        return hb;
    }
    void hitboxRemove(SeqEdHitboxTrack* track, SeqEdHitbox* item) {
        track->hitboxes.erase(item);
        delete item;
        compileHitboxSequence();
    }
    void hitboxMoveResize(SeqEdHitbox* hitbox, SeqEdHitboxTrack* dest_track, int dest_frame, int dest_len) {
        hitbox->track->hitboxes.erase(hitbox);
        dest_track->hitboxes.insert(hitbox);
        hitbox->track = dest_track;
        hitbox->frame = dest_frame;
        hitbox->length = dest_len;
        compileHitboxSequence();
    }
    void compileHitboxSequence() {
        hitbox_seq->clear();
        for (auto& trk : hitbox_tracks) {
            for (auto& h : trk->hitboxes) {
                hitbox_seq->insert(h->frame, h->length, ANIM_HITBOX_SPHERE);
            }
        }
    }

    void serializeJson(nlohmann::json& json) const override {
        json["not_implemented"] = "";
    }
    bool deserializeJson(const nlohmann::json& json) override {
        return true;
    }
};
