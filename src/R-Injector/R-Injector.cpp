#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NONSTDC_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <thread>
#include <direct.h>

std::wstring szProgram = L"RobloxPlayerBeta.exe";
PCHAR szDllPath;

DWORD FetchProcessId(const std::wstring& ProcessName);
BOOL Inject(DWORD dwProcessId, PCHAR DllPath);

int main(int argc, char* argv[])
{
	using namespace std::literals::chrono_literals;

	char cwDir[FILENAME_MAX];
	if (!_getcwd(cwDir, sizeof(cwDir)))
	{
		return errno;
	}

	cwDir[sizeof(cwDir) - 1] = '\0';
	std::string fPath = std::string(cwDir) + "\\RCONSOLE.dll";
	szDllPath = (PCHAR)fPath.c_str();

	DWORD dwMainProcessId = FetchProcessId(szProgram);
	Inject(dwMainProcessId, szDllPath);
	std::this_thread::sleep_for(7s);
	ExitProcess(EXIT_SUCCESS);
}

DWORD FetchProcessId(const std::wstring& ProcessName)
{
	PROCESSENTRY32 p32ProcessEntry;
	p32ProcessEntry.dwSize = sizeof(p32ProcessEntry);

	HANDLE TLH32Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (TLH32Snapshot == INVALID_HANDLE_VALUE) { return NULL; }

	Process32First(TLH32Snapshot, &p32ProcessEntry);
	if (!ProcessName.compare(p32ProcessEntry.szExeFile))
	{
		CloseHandle(TLH32Snapshot);
		return p32ProcessEntry.th32ProcessID;
	}

	while (Process32Next(TLH32Snapshot, &p32ProcessEntry))
	{
		if (!ProcessName.compare(p32ProcessEntry.szExeFile))
		{
			CloseHandle(TLH32Snapshot);
			return p32ProcessEntry.th32ProcessID;
		}
	}

	CloseHandle(TLH32Snapshot);
	return EXIT_SUCCESS;
}

BOOL Inject(DWORD dwProcessId, PCHAR DllPath)
{
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, FetchProcessId(szProgram));
	if (hProcess == NULL) { std::cerr << GetLastError() << std::endl; }

	LPVOID pDllPath = VirtualAllocEx(
		hProcess,
		NULL, strlen(szDllPath) + 1,
		MEM_RESERVE | MEM_COMMIT,
		PAGE_EXECUTE_READWRITE);

	if (pDllPath == nullptr) { std::cerr << GetLastError() << std::endl; }

	WriteProcessMemory(hProcess, pDllPath, (LPVOID)szDllPath, strlen(szDllPath) + 1, NULL);

	LPTHREAD_START_ROUTINE aLoadLibrary =(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA");
	HANDLE hRemoteThread = CreateRemoteThread(
		hProcess, NULL, NULL,
		aLoadLibrary, pDllPath,
		NULL, NULL);

	WaitForSingleObject(hRemoteThread, INFINITE);
	VirtualFreeEx(hProcess, pDllPath, MAX_PATH, MEM_RELEASE);

	std::cout << "Allocated -> " << std::hex << pDllPath << std::endl;
	std::cout << "Injected -> " << FetchProcessId(szProgram) << std::endl;
	return EXIT_SUCCESS;
}