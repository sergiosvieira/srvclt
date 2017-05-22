#ifndef scramble_h
#define scramble_h

#include <vector>
#include <string>

using VectorOfString = std::vector<std::string>;
static const int kShuffleLoop = 20;

static VectorOfString preset = {
    "Aragorn",
    "Gimli",
    "Legolas",
    "Gandalf",
    "Boromir",
    "Frodo",
    "Sam",
    "Peregrin",
    "Merry",
    "Bilbo",
    "Elrond",
    "Glorfindel",
    "Arwen",
    "Elladan",
    "Elrohir",
    "Thranduil",
    "Haldir",
    "Celeborn",
    "Galadriel",
    "Gwaihir",
    "Balin",
    "Gloin",
    "King Dain",
    "Faramir",
    "Denethor",
    "Beregond",
    "Treebeard",
    "Theoden",
    "Eomer",
    "Eowyn",
    "Theodred"
};

int randomRange(int first, int last);
std::string selectRandomWord(VectorOfString& v);
void swap(std::string& str, int firstIndex, int secondIndex);
std::string shuffleString(const std::string& str);

#endif /* scramble_h */
