#ifndef KT_CURVE_HPP
#define KT_CURVE_HPP

#include <assert.h>
#include "common/math/gfxm.hpp"
#include <vector>
#include <limits>

template<typename T>
struct keyframe
{
    keyframe(): time(0) {}
    keyframe(float time): time(time) {}
    void operator=(const T& value)
    { this->value = value; }
    bool operator<(const keyframe<T>& other)
    { return time < other.time; }
    float time;
    T value;
};

template<typename T>
T curve_value_interp(const T& a, const T& b, float t) {
    return T();
}
template<>
inline float curve_value_interp(const float& a, const float& b, float t) {
    return a + t * (b - a);
}
template<>
inline gfxm::vec2 curve_value_interp(const gfxm::vec2& a, const gfxm::vec2& b, float t) {
    return gfxm::vec2(
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y)
    );
}
template<>
inline gfxm::vec3 curve_value_interp(const gfxm::vec3& a, const gfxm::vec3& b, float t) {
    return gfxm::vec3(
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y),
        a.z + t * (b.z - a.z)
    );
}
template<>
inline gfxm::quat curve_value_interp(const gfxm::quat& a, const gfxm::quat& b, float t) {
    return gfxm::slerp(a, b, t);
}

template<typename T>
T curve_value_delta(const T& a, const T& b) {
    return b - a;
}
template<>
inline float curve_value_delta(const float& a, const float& b) {
    return b - a;
}
template<>
inline gfxm::vec2 curve_value_delta(const gfxm::vec2& a, const gfxm::vec2& b) {
    return b - a;
}
template<>
inline gfxm::vec3 curve_value_delta(const gfxm::vec3& a, const gfxm::vec3& b) {
    return b - a;
}
template<>
inline gfxm::quat curve_value_delta(const gfxm::quat& a, const gfxm::quat& b) {
    return b * gfxm::inverse(a);
}

template<typename T>
T curve_value_add(const T& a, const T& b) {
    return a + b;
}
template<>
inline gfxm::quat curve_value_add(const gfxm::quat& a, const gfxm::quat& b) {
    return b * a;
}

template<typename T>
class curve {
public:
    typedef keyframe<T> keyframe_t;

    T at(float time, T def = T()) {
        if(keyframes.empty())
            return def;

        int left = 0;
        int right = keyframes.size() - 1;
        float t = .0f;
        if(time > keyframes.back().time) {
            left = keyframes.size() - 1;
            right = left;
        } else if(time < 0) {
            left = 0;
            right = 0;
        } else {
            while(right - left > 1) {
                int center = left + (right - left) / 2;
                if(time < keyframes[center].time) {
                    right = center;
                } else {
                    left = center;
                }
            }
            t = (time - keyframes[left].time) / (keyframes[right].time - keyframes[left].time);
        }

        T a = keyframes[left].value;
        T b = keyframes[right].value;
        
        return curve_value_interp(a, b, t);
    }
    T delta(float from, float to) {
        if(from < to) {
            T at0 = at(from);
            T at1 = at(to);
            return curve_value_delta(at0, at1);
        } else if(from > to) {
            T at_from0 = at(from);
            T at_from1 = at(keyframes[keyframes.size() - 1].time);
            T at_to0 = at(keyframes[0].time);
            T at_to1 = at(to);
            return curve_value_add(
                curve_value_delta(at_from0, at_from1),
                curve_value_delta(at_to0, at_to1)
            );
        } else {
            return T();
        }
    }
    keyframe<T>& operator[](float time) {
        assert(time == time);
        assert(time != std::numeric_limits<float>::infinity());
        for(unsigned i = 0; i < keyframes.size(); ++i)
        {
            if(fabsf(keyframes[i].time - time) < FLT_EPSILON)
            {
                return keyframes[i];
            }
        }        
        keyframes.push_back(keyframe<T>(time));
        std::sort(keyframes.begin(), keyframes.end());
        return operator[](time);
    }
    void set_keyframes(const std::vector<keyframe<T>> keyframes) {
        this->keyframes = keyframes;
        std::sort(this->keyframes.begin(), this->keyframes.end());
    }
    std::vector<keyframe<T>>& get_keyframes() {
        return keyframes;
    }
    float get_length() {
        if(keyframes.empty()) return .0f;
        return keyframes.back().time;
    }
private:
    std::vector<keyframe<T>> keyframes;
};


#endif