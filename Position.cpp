/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Position.cpp
 * Author: Peter Sollberger (peter.sollberger@hslu.ch)
 * 
 * Created on 7. Dezember 2016, 15:57
 */

#include "Position.h"

Position::Position(const Position& orig) {
    x = orig.x;
    y = orig.y;
    z = orig.z;
}

Position Position::operator+(const Position& p) {
    return Position(x + p.getX(), y + p.getY(), z + p.getZ());
}

Position Position::operator-(const Position& p) {
    return Position(x - p.getX(), y - p.getY(), z - p.getZ());
}
