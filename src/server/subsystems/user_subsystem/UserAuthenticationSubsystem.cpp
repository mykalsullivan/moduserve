//
// Created by msullivan on 11/11/24.
//

#include "UserAuthenticationSubsystem.h"
#include "common/Logger.h"
#include <iomanip>
#include <openssl/sha.h>

UserAuthenticationSubsystem::UserAuthenticationSubsystem() : m_DatabaseConnection(nullptr)
{}

UserAuthenticationSubsystem::~UserAuthenticationSubsystem()
{
    delete m_DatabaseConnection;
}

int UserAuthenticationSubsystem::init()
{
    std::string connectionString = "dbname=practice_server_user_db user=msullivan password=Mpwfsqli$ hostaddr=127.0.0.1 port=5432";
    try
    {
        m_DatabaseConnection = new pqxx::connection(connectionString);
        if (m_DatabaseConnection->is_open())
        {
            LOG(LogLevel::DEBUG, "Successfully connected to user database");
        }
        else
        {
            LOG(LogLevel::ERROR, "Failed to connect to user database");
            m_DatabaseConnection = nullptr;
            return 0;
        }
    }
    catch (const std::exception &e)
    {
        LOG(LogLevel::ERROR, "Error connecting to database: " + std::string(e.what()));
        m_DatabaseConnection = nullptr;
        return 1;
    }
    return 0;
}

bool UserAuthenticationSubsystem::authenticate(const std::string &username, const std::string &password)
{
    if (m_DatabaseConnection == nullptr)
    {
        LOG(LogLevel::ERROR, "Database connection is not established.");
        return false;
    }

    try
    {
        pqxx::work transaction(*m_DatabaseConnection);
        std::string query = "SELECT password_hash FROM users WHERE username = " + transaction.quote(username) + " LIMIT 1;";
        pqxx::result result = transaction.exec(query);

        if (result.empty())
        {
            LOG(LogLevel::WARNING, "\"" + username + "\" could not be found in the database");
            return false;
        }

        auto storedHash = result[0]["password_hash"].as<std::string>();
        std::string hashedPassword = hashPassword(password);

        if (storedHash == hashedPassword)
            return true; // Authentication successful

        LOG(LogLevel::WARNING, "\"" + username + "\" provided an incorrect password");
    }
    catch (const std::exception &e)
    {
        LOG(LogLevel::ERROR, "Error during authentication: " + std::string(e.what()));
    }
    return false;
}

bool UserAuthenticationSubsystem::usernameExists(const std::string &username) const
{
    if (m_DatabaseConnection == nullptr)
    {
        LOG(LogLevel::ERROR, "Database connection is not established.");
        return false;
    }

    try
    {
        pqxx::work transaction(*m_DatabaseConnection);
        std::string query = "SELECT COUNT(*) FROM users WHERE username = " + transaction.quote(username) + ";";
        pqxx::result result = transaction.exec(query);
        return result[0][0].as<int>() > 0; // Return true if the username exists
    }
    catch (const std::exception &e)
    {
        LOG(LogLevel::ERROR, "Error checking username uniqueness: " + std::string(e.what()));
    }
    return false;
}

std::string UserAuthenticationSubsystem::hashPassword(const std::string &password)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256_ctx;
    SHA256_Init(&sha256_ctx);
    SHA256_Update(&sha256_ctx, password.c_str(), password.length());
    SHA256_Final(hash, &sha256_ctx);

    std::ostringstream oss;
    for (unsigned char i : hash)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int) i;
    return oss.str();
}