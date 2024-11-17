//
// Created by msullivan on 11/10/24.
//

#pragma once
#include <cassert>

struct CommandLineArgs {
    int argc = 0;
    char *argv[];

    char *operator[](int n) const
    {
        assert(n >= 0 && n < argc && "Index out of bounds");
        return argv[n];
    }
};