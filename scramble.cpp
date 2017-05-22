#include "scramble.h"
#include <random>

int randomRange(int first, int last)
{
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 eng(rd()); // seed the generator
    std::uniform_int_distribution<> distr(first, last); // define the range
    return distr(eng);
}

std::string selectRandomWord(VectorOfString& v)
{
    int index = randomRange(0, int(v.size()) - 1);
    return v[index];
}

void swap(std::string& str, int firstIndex, int secondIndex)
{
    char aux = str[firstIndex];
    str[firstIndex] = str[secondIndex];
    str[secondIndex] = aux;
}

std::string shuffleString(const std::string& str)
{
    std::string result = str;
    for (int index = 0; index < str.size(); ++index)
    {
        for (int counter = 0; counter < kShuffleLoop; ++counter)
        {
            int secondIndex = randomRange(0, int(str.size()) - 1);
            swap(result, index, secondIndex);
        }
    }
    return result;
}
