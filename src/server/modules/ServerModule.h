//
// Created by msullivan on 11/30/24.
//

#pragma once

class ServerModule {
public:
    virtual ~ServerModule() = default;
    virtual int init() = 0;

protected:
    bool initialized = false;
    bool active = false;
};