//
// Created by msullivan on 11/30/24.
//

#pragma once
#include "../../server/Signal.h"
#include <vector>
#include <typeindex>

class ServerModule {
public signals:
	static Signal<ServerModule> initialized;
	static Signal<ServerModule> started;
	static Signal<ServerModule> stopped;
	static Signal<ServerModule, const std::string &> crashed;

protected:
    bool m_Initialized = false;
    bool m_Active = false;

public:
    virtual ~ServerModule() = default;
    virtual void init() = 0;
    virtual void run() = 0;
    [[nodiscard]] virtual std::vector<std::type_index> requiredDependencies() const = 0;
    [[nodiscard]] virtual std::vector<std::type_index> optionalDependencies() const = 0;
    [[nodiscard]] bool isInitialized() const { return m_Initialized; }
    [[nodiscard]] bool isActive() const { return m_Active && m_Initialized; }
    //[[nodiscard]] std::string name() const { return std::to_string(std::type_info(this)); }
};

using Connection = int;