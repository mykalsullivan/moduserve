//
// Created by msullivan on 11/8/24.
//

#pragma once
#include "server/modules/ServerModule.h"

class BFModule : public ServerModule {
    const char *m_Code;
    unsigned int m_Tape[4096] {};
    int m_Ptr = 0;

public:
    BFModule();
    ~BFModule() override;

    constexpr int init() override { return 0; };

    void execute();

private:
    void printCurrentPointerAddress() const;
    void printCurrentPointerValue() const;
    void printValues(int start, int offset);
};
