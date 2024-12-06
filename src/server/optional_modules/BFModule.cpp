//
// Created by msullivan on 11/8/24.
//

#include "BFModule.h"
#include <sstream>

#include "server/modules/Logger.h"
#include "server/modules/NetworkEngine.h"

static unsigned int m_Tape[4096] {};
static int m_Ptr = 0;

Signal<Connection, const std::string &> BFModule::receivedCode;

/*  Definitions:
 *      '>' : Move pointer up 1 byte
 *      '<' : Move pointer down 1 byte
 *      '+' : Increment value of address pointer is at
 *      '-' : Decrement value of address pointer is at
 *      '.' : Prints value the address pointer is at and interprets it as a char
 *      ',' : Standard input; writes to the current pointer address.
 *      '[' : Start a 'while' loop
 *      ']' : End a 'while' loop;
 *          * Note: Loop will skip if the pointer at the start of the loop is zero; will only continue to iterate
 *                  if both the address value at the start and end of the loop != 0
 */

BFModule::~BFModule() = default;

void BFModule::init()
{
    for (auto &i : m_Tape) i = 0; // 0 out the entire tape

    // Connect signals to slots
    receivedCode.connect(executeCode);
}

void BFModule::run() {}

void BFModule::executeCode(Connection connection, const std::string &code)
{
    std::string output;
    for (int i = 0; i < code.size(); i++)
    {
        switch (code[i])
        {
            case '>':
                if (m_Ptr + 1 == sizeof(m_Tape)/sizeof(unsigned int))
                    m_Ptr = 0;
                else
                    m_Ptr++;
                break;
            case '<':
                if (m_Ptr - 1 < 0)
                    m_Ptr = sizeof(m_Tape)/sizeof(unsigned int);
                else
                    m_Ptr--;
                break;
            case '+':
                m_Tape[m_Ptr]++;
                break;
            case '-':
                m_Tape[m_Ptr]--;
                break;
            case ',':
                m_Tape[m_Ptr] = std::cin.get();
                break;
            case '.':
                output += m_Tape[m_Ptr];
                break;
            case '[':
                if (m_Tape[m_Ptr] == 0)
                {
                    int loopLevel = 1;
                    while (loopLevel > 0)
                    {
                        i++;
                        if (code[i] == '[') loopLevel++;
                        else if (code[i] == ']') loopLevel--;
                    }
                }
                break;
            case ']':
                if (m_Tape[m_Ptr] != 0)
                {
                    int loopLevel = 1;
                    while (loopLevel > 0)
                    {
                        i--;
                        if (code[i] == '[') loopLevel--;
                        else if (code[i] == ']') loopLevel++;
                    }
                }
                break;
            default:
                break;
        }
    }
    std::string message = "BF: Executed command " + code;
    NetworkEngine::sendData(connection, message);
    NetworkEngine::sendData(connection, output);
}

void BFModule::retrieveCurrentPointerValue(Connection connection) {
    std::cout << "BF: tape[" << m_Ptr << "]: " << m_Tape[m_Ptr] << '\n';
}

void BFModule::retrieveCurrentPointerAddress(Connection connection)
{
    std::cout << "BF: tape[" << m_Ptr << "]\n";
}

void BFModule::retrieveTapeValues(Connection connection, unsigned int start, unsigned int offset)
{
    if (start < 0 || offset >= sizeof(m_Tape)/sizeof(unsigned int)) [[unlikely]]
    {
        Logger::log(LogLevel::Error, "Chosen values are out of bounds for the BF tape");
        return;
    }

    std::stringstream ss;
    ss << "BF: ";
    for (int i = 0; i < offset; i++)
    {
        ss << m_Tape[start + i];
        if (i != (offset - 1)) [[likely]] ss << ' ';
    }
    NetworkEngine::sendData(connection, ss.str());
}