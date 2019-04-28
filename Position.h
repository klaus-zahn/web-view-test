/* Copyright 2016 Hochschule Luzern - Informatik */
#ifndef POSITION_H
#define POSITION_H

#include <cstdint>
#include <cstdlib>

/**
 * x/y/z Position. All values are in mm.
 * @author Peter Sollberger (peter.sollberger@hslu.ch)
 */
class Position {
public:

    Position(void) : x(0), y(0), z(0) {
    }

    Position(int16_t ax, int16_t ay, int16_t az) : x(ax), y(ay), z(az) {
    }
    Position(const Position& orig);

    virtual ~Position() {
    }

    inline int16_t getX() const {
        return x;
    }

    inline int16_t getY() const {
        return y;
    }

    inline int16_t getZ() const {
        return z;
    }

    Position operator+(const Position& p);
    Position operator-(const Position& p);

    inline void operator=(const Position& p) {
        x = p.x;
        y = p.y;
        z = p.z;
    }

    friend inline bool operator==(const Position& p1, const Position& p2) {
        return (std::abs(p1.getX() - p2.getX()) < 10) &&
               (std::abs(p1.getY() - p2.getY()) < 10) &&
               (std::abs(p1.getZ() - p2.getZ()) < 10);
    }

    friend inline bool operator!=(const Position& p1, const Position& p2) {
        return !(p1 == p2);
    }

private:
    int16_t x, y, z;
};

#endif /* POSITION_H */
