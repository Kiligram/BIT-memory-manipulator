#include <Windows.h>
#include<TlHelp32.h>
#include <iostream>
#include <tchar.h> // _tcscmp
#include <vector>
#include "memory_search.cpp"

const std::string AssaultCubePName = "ac_client.exe";

// inspired by https://www.youtube.com/watch?v=mxS7_TVATYo&t=5s
DWORD GetModuleBaseAddress(const TCHAR* lpszModuleName, DWORD pID) {
    DWORD dwModuleBaseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pID); // make snapshot of all modules within process
    MODULEENTRY32 ModuleEntry32 = { 0 };
    ModuleEntry32.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(hSnapshot, &ModuleEntry32)) //store first Module in ModuleEntry32
    {
        do {
            if (_tcscmp(ModuleEntry32.szModule, lpszModuleName) == 0) // if Found Module matches Module we look for -> done!
            {
                dwModuleBaseAddress = (DWORD)ModuleEntry32.modBaseAddr;
                break;
            }
        } while (Module32Next(hSnapshot, &ModuleEntry32)); // go through Module entries in Snapshot and store in ModuleEntry32


    }
    CloseHandle(hSnapshot);

    return dwModuleBaseAddress;
}

// inspired by https://stackoverflow.com/questions/865152/how-can-i-get-a-process-handle-by-its-name-in-c
DWORD GetProcessIdByName(const TCHAR* processName) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    DWORD pID = NULL;

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            //std::wcout << entry.szExeFile << std::endl;
            if (_tcsicmp(entry.szExeFile, processName) == 0)
            {
                pID = entry.th32ProcessID;
            }
        }
    }

    CloseHandle(snapshot);

    return pID;
}

void printAddresses(const std::vector<void*> foundValues) {
    if (!foundValues.empty()) {
        std::cout << "Found " << std::dec << foundValues.size() << " addresses. Show them? y/N: ";
        std::string showConsent;
        fseek(stdin, 0, SEEK_END);
        std::getline(std::cin, showConsent);

        if (showConsent == "y" || showConsent == "yes") {
            for (size_t i = 0; i < foundValues.size(); i++) {
                std::stringstream ss;
                std::cout << "0x" << std::hex << ((unsigned long long)foundValues[i]) << std::endl;
            }
        }
    }
    else {
        std::cout << "No addresses found" << std::endl;
    }
}

int main() {
    std::cout << "Memory manipulator" << std::endl;
    std::cout << "Enter process name to attach: ";
    std::string processNameString;
    std::getline(std::cin, processNameString);
    std::wstring widestr = std::wstring(processNameString.begin(), processNameString.end());

    const TCHAR* processName = widestr.c_str();
    
    DWORD pID = GetProcessIdByName(processName);
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);

    if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL) {
        std::cout << "Failed to open process" << std::endl;
        return 1;
    }

    DWORD processBaseAddress = GetModuleBaseAddress(processName, pID);
    DWORD offsetFromModuleBase = 0x00183828;
    std::vector<DWORD> ammoOffsets{ 0x8,0x56C,0x64,0x68,0x30,0x2BC };
    DWORD baseAddress = NULL;

    // get value at first offset
    ReadProcessMemory(processHandle, (LPVOID)(processBaseAddress + offsetFromModuleBase), &baseAddress, sizeof(baseAddress), NULL);

    DWORD ammoAddress = baseAddress; // the adress we need -> it will change while going through offsets
    for (size_t i = 0; i < ammoOffsets.size() - 1; i++) // -1 because we dont want the value at the last offset
    {
        ReadProcessMemory(processHandle, (LPVOID)(ammoAddress + ammoOffsets.at(i)), &ammoAddress, sizeof(ammoAddress), NULL);
    }
    ammoAddress += ammoOffsets.at(ammoOffsets.size() - 1); // add last offset -> done!!


    const char* helpList =
        "\n0: exit\n"
        "1: increase ammunition amount in AssaultCube\n"
        "2: first scan for a value\n"
        "3: next scan for a value\n"
        "4: modify value on address\n";

    std::vector<void*> foundValues;
    std::string command;

    while (true) {
        std::cout << helpList << std::endl;
        std::cout << "> ";
        fseek(stdin, 0, SEEK_END);
        std::getline(std::cin, command);

        if (command == "0") {
            return 0;
        }

        if (command == "1") {
            if (processNameString != AssaultCubePName) {
                std::cout << "This function is available for AssaultCube only" << std::endl;
                continue;
            }
            std::cout << "How much ammo do you want?: ";
            int newAmmo = 0;
            std::cin >> newAmmo;
            WriteProcessMemory(processHandle, (LPVOID)(ammoAddress), &newAmmo, sizeof(newAmmo), NULL);
        }

        if (command == "2") {
            foundValues.clear();
            std::cout << "New scan" << std::endl;
            std::cout << "Enter integer value to find: ";
            
            int inValue = 0;
            std::cin >> inValue;

            foundValues = findBytePatternInProcessMemory(processHandle, (void*)&inValue, sizeof(inValue));
            printAddresses(foundValues);
        }

        if (command == "3") {
            std::cout << "Next scan" << std::endl;
            std::cout << "Enter integer value to find: ";
            
            int inValue = 0;
            std::cin >> inValue;
            
            refindBytePatternInProcessMemory(processHandle, (void*)&inValue, sizeof(inValue), foundValues);
            printAddresses(foundValues);
        }

        if (command == "4") {
            DWORD address = 0;
            int newValue = 0;

            std::cout << "Enter address to write to (hex): ";
            std::cin >> std::hex >> address;

            std::cout << "Enter value to write (decimal): ";
            std::cin >> std::dec >> newValue;

            WriteProcessMemory(processHandle, (LPVOID)(address), &newValue, sizeof(newValue), NULL);
        }
    }

}