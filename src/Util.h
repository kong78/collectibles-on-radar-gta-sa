#pragma once

#include "CVector.h"

// https://gtaforums.com/topic/194199-documenting-gta-sa-memory-addresses/page/12/?tab=comments#comment-3271047
struct tTag // 53 bytes
{
    int pad1;
    CVector pos;
    //char pad2[37];
};

struct tTagState
{
    tTag* tag;
    unsigned char paint;
    char pad[3];
};

struct tUsj
{
    CVector start1;
    CVector start2;
    CVector land1;
    CVector land2;
    CVector cam;
    int reward;
    char done;
    char found;
    char pad[2];
};

struct tUsjPool // TODO Use pool
{
    tUsj* entries;
    unsigned char* flags; // 0x01 = in use, 0x80 = not in use // first bit is empty or not, rest is id (always 1?)
    int size; // 256
    int allocPtr; // first free

    bool isEmpty(int pos)
    {
        return true;
    }
};

class Util
{
public:
    static int& s_totalTags;
    static int& s_tags;
    static tTagState* s_tagList; // static tTag s_tagList[100]

    static int& s_usjFound;
    static int& s_usjDone;
    static tUsjPool*& s_usjPool;

    //static int& s_photographs;
    //static int& s_horseshoes;
    //static int& s_oysters;
};
