#ifndef DECODERDATA_H
#define DECODERDATA_H

#include <string>
#include <map>

struct DecoderCandidate {
    short tag;
    short x;
    short y;
    float xRotation;
    float yRotation;
    float zRotation;
    short score;
    unsigned short bee;
};

struct DecoderData {
    long long id;
    std::string timestamp;
    std::string offset;
    short x;
    short y;
    short camID;
    std::map<int, DecoderCandidate> candidate;
};

#endif // DECODERDATA_H
