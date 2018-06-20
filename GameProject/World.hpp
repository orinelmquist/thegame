//
//  World.hpp
//  GameProject
//
//  Created by Orin Elmquist on 6/12/18.
//  Copyright © 2018 Orin Elmquist. All rights reserved.
//

#ifndef World_hpp
#define World_hpp

#include <stdio.h>
#include <iostream>
#include <set>
#include <vector>
#include <algorithm>

class Room {
public:
    Room();
    void set();
    
    bool equals(Room other);
    static bool compareXY(Room i, Room j);
    
    std::vector<std::vector<int>> edges();
    
    bool valid(Room other, int offset);
    bool valid(std::vector<Room> &others, int offset);
    bool validX(Room other, int offset);
    bool validX(std::vector<Room> &others, int offset);
    bool validY(Room other, int offset);
    bool validY(std::vector<Room> &others, int offset);
    
    bool moveXY(std::vector<Room> &others, int offset);
    bool moveX(std::vector<Room> &others, int offset);
    bool moveY(std::vector<Room> &others, int offset);
    
    int num();
    std::pair<int, int> coords();
    std::pair<int, int> dim();
    
private:
    static int count, size;
    int x, y, w, h, id;
};

class Hall {
public:
    Hall(int s, int sxy, int e, int exy, int d);
    Hall();
    
    bool equals(Hall other);
    bool sameConnection(Hall other);
    
    std::pair<int, int> rooms();
    std::pair<int, int> coords();
    int dir();
    
private:
    int start, startxy, end, endxy, direction;
};

class World {
public:
    World();
    
    void clear();
    void set(int x, int y, int val);
    void swap(int x1, int y1, int x2, int y2);
    
    void buildCave();
    void buildDungeon();
    
    friend std::ostream &operator<<(std::ostream &out, const World &w);
    
private:
    std::vector<int> map;
    std::vector<Room> rooms;
    std::vector<Hall> halls;
    int size;
    
    void placeRoom(Room r);
    std::vector<Hall> setPossHalls();
    std::vector<Hall> updateHalls();
    int getRoomByEdge(int coord);
    void placeHall(Hall h);
    void flood(std::vector<bool> &visited, std::vector<bool> &connected, int x, int y, int &c);
};

#endif /* World_hpp */
