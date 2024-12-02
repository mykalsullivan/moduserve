//
// Created by msullivan on 12/1/24.
//

#pragma once
#include <string>
#include <common/Connection.h>

namespace IO {
    [[nodiscard]] bool sendData(Connection sender, Connection recipient, const std::string &data);
    [[nodiscard]] std::string receiveData(Connection connection);
}
