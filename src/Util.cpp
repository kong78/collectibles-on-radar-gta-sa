#include "Util.h"

int& Util::s_totalTags = *(int*) 0xA9AD70; // total number of gang tags (100)
tTagState* Util::s_tagList = (tTagState*) 0xA9A8C0; // array of s_totalTags entries, each with 8 bytes

CPool<tUsj>*& Util::ms_pUsjPool = *(CPool<tUsj>**) 0xA9A888;

//int& Util::s_tags = *(int*) 0xA9AD74; // number of tags "completely" painted
//int& Util::s_photographs = *(int*) 0xB790B8;
//int& Util::s_horseshoes = *(int*) 0xB791E4;
//int& Util::s_oysters = *(int*) 0xB791EC;
//int& Util::s_usjFound = *(int*) 0xB79060;
//int& Util::s_usjDone = *(int*) 0xB79064;
