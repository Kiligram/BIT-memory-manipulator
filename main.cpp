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
    return dwModuleBaseAddress;
}


int main() {
    std::vector<void*> foundValues;
    HWND hGameWindow = FindWindow(NULL, L"AssaultCube");
    if (hGameWindow == NULL) {
        std::cout << "Start the game!" << std::endl;
        return 0;
    }
    DWORD pID = NULL; // ID of our Game
    GetWindowThreadProcessId(hGameWindow, &pID);
    HANDLE processHandle = NULL;
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
    if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL) { // error handling
        std::cout << "Failed to open process" << std::endl;
        return 0;
    }

    TCHAR gameName[] = L"ac_client.exe";
    DWORD gameBaseAddress = GetModuleBaseAddress(gameName, pID);
    DWORD offsetGameToBaseAdress = 0x00183828;
    std::vector<DWORD> pointsOffsets{ 0x8,0x56C,0x64,0x68,0x30,0x2BC };
    DWORD baseAddress = NULL;
    //Get value at gamebase+offset -> store it in baseAddress
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
    while (true) {
        Sleep(50);
        if (GetAsyncKeyState(VK_NUMPAD0)) { // Exit
            return 0;
        }
        if (GetAsyncKeyState(VK_NUMPAD1)) {//Mouseposition
            std::cout << "How many points you want?" << std::endl;
            int newPoints = 0;
            std::cin >> newPoints;
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &newPoints, 4, 0);
        }
        if (GetAsyncKeyState(VK_NUMPAD2)) {//Mouseposition
            std::cout << "First scan" << std::endl;
            std::cout << "Enter integer value to find" << std::endl;
            
            int inValue = 0;
            std::cin >> inValue;

            // Cast the integer value to a void pointer
            void* voidPtr = (void*)&inValue;
            std::vector<char*> foundValuesText;

            std::vector<void*> foundValues = findBytePatternInProcessMemory(processHandle, voidPtr, sizeof(inValue));
            
            if (!foundValues.empty())
            {
                std::cout << "Found " << foundValues.size() << " addresses. Show them? y/N: ";
                char show;
                std::cin >> show;
                if (show == 'y') {

                    for (int i = 0; i < foundValues.size(); i++)
                    {
                        //if (i >= foundValuesText.size())
                        //{
                        //    foundValuesText.push_back(new char[17]);
                        //}

                        std::stringstream ss;
                        std::cout << "0x" << std::hex << ((unsigned long long)foundValues[i]) << std::endl;

                        //strcpy(foundValuesText[i], ss.str().c_str());

                    }
                }
                //for (const char* value : foundValuesText) {
                //    std::cout << value << std::endl;
                //}

            }

        }
    }

}