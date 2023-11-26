#include <Windows.h>
#include<TlHelp32.h>
#include <iostream>
#include <tchar.h> // _tcscmp
#include <vector>
#include "memory_search.cpp"

DWORD GetModuleBaseAddress(TCHAR* lpszModuleName, DWORD pID) {
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

    std::cout << std::hex << dwModuleBaseAddress << std::endl;
    return dwModuleBaseAddress;
}

DWORD GetProcessIdByName(TCHAR* processName) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    DWORD pID = NULL;

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (_tcsicmp(entry.szExeFile, processName) == 0)
            {
                pID = entry.th32ProcessID;
            }
        }
    }

    CloseHandle(snapshot);

    return pID;
}

int main() {
    //HWND hProcessWindow = FindWindow(NULL, L"AssaultCube");
    //if (hProcessWindow == NULL) {
    //    std::cout << "Process not found" << std::endl;
    //    return 0;
    //}

    //DWORD pID = NULL; // ID of the process
    //GetWindowThreadProcessId(hProcessWindow, &pID);
    //HANDLE processHandle = NULL;
    //processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);

    //if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL) {
    //    std::cout << "Failed to open process" << std::endl;
    //    return 0;
    //}

    //TCHAR processName1[] = L"ac_client.exe";
    TCHAR processName[] = L"ac_client.exe";
    
    DWORD pID = GetProcessIdByName(processName);
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);

    DWORD gameBaseAddress = GetModuleBaseAddress(processName, pID);
    DWORD offsetGameToBaseAdress = 0x00183828;
    std::vector<DWORD> pointsOffsets{ 0x8,0x56C,0x64,0x68,0x30,0x2BC };
    DWORD baseAddress = NULL;

    //Get value at gamebase+offset
    ReadProcessMemory(processHandle, (LPVOID)(gameBaseAddress + offsetGameToBaseAdress), &baseAddress, sizeof(baseAddress), NULL);
    std::cout << "debugginfo: baseaddress = " << std::hex << baseAddress << std::endl;
    DWORD pointsAddress = baseAddress; //the Adress we need -> change now while going through offsets
    for (int i = 0; i < pointsOffsets.size() - 1; i++) // -1 because we dont want the value at the last offset
    {
        ReadProcessMemory(processHandle, (LPVOID)(pointsAddress + pointsOffsets.at(i)), &pointsAddress, sizeof(pointsAddress), NULL);
        std::cout << "debugginfo: Value at offset = " << std::hex << pointsAddress << std::endl;
    }
    pointsAddress += pointsOffsets.at(pointsOffsets.size() - 1); //Add Last offset -> done!!

    //"UI"
    std::cout << "Memory manipulator" << std::endl;
    std::cout << "Press Numpad 0 to EXIT" << std::endl;
    std::cout << "Press Numpad 1 to set Points" << std::endl;

    std::vector<void*> foundValues;

    while (true) {
        Sleep(50);
        if (GetAsyncKeyState(VK_NUMPAD0)) { // Exit
            return 0;
        }
        if (GetAsyncKeyState(VK_NUMPAD1)) {//Mouseposition
            std::cout << "How many points you want?" << std::endl;
            int newPoints = 0;
            std::cin >> newPoints;
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &newPoints, sizeof(newPoints), NULL);
        }
        if (GetAsyncKeyState(VK_NUMPAD2)) {//Mouseposition
            foundValues.clear();
            std::cout << "New scan" << std::endl;
            std::cout << "Enter integer value to find" << std::endl;
            
            int inValue = 0;
            std::cin >> inValue;

            // Cast the integer value to a void pointer
            void* voidPtr = (void*)&inValue;

            foundValues = findBytePatternInProcessMemory(processHandle, voidPtr, sizeof(inValue));
            
            if (!foundValues.empty()) {
                std::cout << "Found " << foundValues.size() << " addresses. Show them? y/N: ";
                std::string showConsent;
                fseek(stdin, 0, SEEK_END);
                std::getline(std::cin, showConsent);

                if (showConsent == "y" || showConsent == "yes") {
                    for (int i = 0; i < foundValues.size(); i++) {
                        std::stringstream ss;
                        std::cout << "0x" << std::hex << ((unsigned long long)foundValues[i]) << std::endl;
                    }
                }
            }
        }
        if (GetAsyncKeyState(VK_NUMPAD3)) {//Mouseposition
            std::cout << "Next scan" << std::endl;
            std::cout << "Enter integer value to find" << std::endl;
            int inValue = 0;
            std::cin >> inValue;
            void* voidPtr = (void*)&inValue;
            refindBytePatternInProcessMemory(processHandle, &inValue, sizeof(inValue), foundValues);

            if (!foundValues.empty()) {
                std::cout << "Found " << foundValues.size() << " addresses. Show them? y/N: ";
                char show;
                std::cin >> show;
                if (show == 'y') {
                    for (int i = 0; i < foundValues.size(); i++) {
                        std::stringstream ss;
                        std::cout << "0x" << std::hex << ((unsigned long long)foundValues[i]) << std::endl;
                    }
                }
            }
        }


        if (GetAsyncKeyState(VK_NUMPAD4)) {//Mouseposition
            DWORD address = 0;
            int newValue = 0;

            std::cout << "Enter address to write to (hex)" << std::endl;
            std::cin >> std::hex >> address;

            std::cout << "Enter value to write (decimal)" << std::endl;
            std::cin >> newValue;

            WriteProcessMemory(processHandle, (LPVOID)(address), &newValue, sizeof(newValue), NULL);

        }
    }

}