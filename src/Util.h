#pragma once

#include "CVector.h"
#include "CPool.h"

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

class Util
{
public:
    static int& s_totalTags;
    static tTagState* s_tagList; // static tTag s_tagList[100]

    static CPool<tUsj>*& ms_pUsjPool;

    //static int& s_tags;
    //static int& s_photographs;
    //static int& s_horseshoes;
    //static int& s_oysters;
    //static int& s_usjFound;
    //static int& s_usjDone;
};
