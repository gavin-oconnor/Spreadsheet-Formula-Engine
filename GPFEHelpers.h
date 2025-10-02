#ifndef GPFE_HELPERS_H
#define GPFE_HELPERS_H

#include <string>

int colLetterToNumber(const std::string &col)
{
    int result = 0;
    for (char c : col)
    {
        if (!std::isalpha(c))
            continue; // skip non-letters if you want safety
        char upper = std::toupper(c);
        int digit = (upper - 'A' + 1); // A=1, B=2, ..., Z=2
        result = result * 26 + digit;
    }
    return result;
}

#endif