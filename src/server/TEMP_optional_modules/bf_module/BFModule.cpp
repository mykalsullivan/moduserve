//
// Created by msullivan on 11/8/24.
//

#include "BFModule.h"
#include "common/PCH.h"
#include <cassert>

BFModule::BFModule() : m_Code()
{
    // 0 out the entire tape
    for (auto &i : m_Tape)
        i = 0;}

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

void BFModule::execute()
{
    auto codeLength = strlen(m_Code);
    for (int i = 0; i < codeLength; i++) {
        switch (m_Code[i]) {
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
                std::cout << static_cast<char>(m_Tape[m_Ptr]);
                break;
            case '[':
                if (m_Tape[m_Ptr] == 0) {
                    int loopLevel = 1;
                    while (loopLevel > 0) {
                        i++;
                        if (m_Code[i] == '[') loopLevel++;
                        else if (m_Code[i] == ']') loopLevel--;
                    }
                }
                break;
            case ']':
                if (m_Tape[m_Ptr] != 0) {
                    int loopLevel = 1;
                    while (loopLevel > 0) {
                        i--;
                        if (m_Code[i] == '[') loopLevel--;
                        else if (m_Code[i] == ']') loopLevel++;
                    }
                }
                break;
            default:
                break;
        }
    }
}

void BFModule::printCurrentPointerValue() const {
    std::cout << "tape[" << m_Ptr << "]: " << m_Tape[m_Ptr] << '\n';
}

void BFModule::printCurrentPointerAddress() const
{
    std::cout << "tape[" << m_Ptr << "]\n";
}

void BFModule::printValues(int start, int offset)
{
    assert(offset >= 1 && "Offset must be greater than 0");
    for (int i = 0; i < offset; i++)
        std::cout << m_Tape[start + i] << ' ';
    std::cout << '\n';
}