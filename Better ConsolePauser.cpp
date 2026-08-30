#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

unsigned long long GetTickCount64Wrapper() {
	return GetTickCount64();
}

string formatDuration(const chrono::steady_clock::duration& duration) {
	auto seconds = chrono::duration_cast<chrono::seconds>(duration);
	auto milliseconds = chrono::duration_cast<chrono::milliseconds>(duration) % 1000;
	ostringstream oss;
	oss << seconds.count() << " 秒 " << milliseconds.count() << " 毫秒";
	return oss.str();
}

string formatDurationMs(long long milliseconds) {
	long long seconds = milliseconds / 1000;
	long long ms = milliseconds % 1000;
	ostringstream oss;
	oss << seconds << " 秒 " << ms << " 毫秒";
	return oss.str();
}

double GetProcessCpuUsage(HANDLE hProcess, unsigned long long* lpKernelTime, unsigned long long* lpUserTime) {
	FILETIME createTime, exitTime, kernelTime, userTime;
	if (!GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
		return 0.0;
	}
	
	unsigned long long kt = ((unsigned long long)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
	unsigned long long ut = ((unsigned long long)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;
	
	if (lpKernelTime && lpUserTime) {
		unsigned long long deltaK = kt - *lpKernelTime;
		unsigned long long deltaU = ut - *lpUserTime;
		*lpKernelTime = kt;
		*lpUserTime = ut;
		
		static unsigned long long lastSysTime = 0;
		FILETIME sysIdle, sysKernel, sysUser;
		GetSystemTimes(&sysIdle, &sysKernel, &sysUser);
		unsigned long long sysTotal = 
		((unsigned long long)sysKernel.dwHighDateTime << 32) | sysKernel.dwLowDateTime +
		((unsigned long long)sysUser.dwHighDateTime << 32) | sysUser.dwLowDateTime;
		
		if (lastSysTime == 0) {
			lastSysTime = sysTotal;
			return 0.0;
		}
		unsigned long long deltaSys = sysTotal - lastSysTime;
		lastSysTime = sysTotal;
		
		if (deltaSys == 0) return 0.0;
		return (double)(deltaK + deltaU) / (double)deltaSys * 100.0;
	}
	return 0.0;
}

string showExitMessage(DWORD exitCode) {
	string outString = "";
	switch(exitCode) {
	case 0:
		outString += "正常退出";
		break;
	case 1:
		outString += "一般错误";
		break;
	case 2:
		outString += "文件未找到";
		break;
	case 3:
		outString += "路径未找到";
		break;
	case 5:
		outString += "拒绝访问";
		break;
		case 1073741819:  // 0xC0000005
		outString += "访问违规";
		break;
		case 3221225477:  // 0xC00000FD
		outString += "栈溢出";
		break;
		case 3221225794:  // 0xC0000094
		outString += "整数除以零";
		break;
	}
	if (outString != "")
		outString = " (" + outString + ")";
	return outString;
}

int main(int argc, char* argv[]) {
	
	if (argc < 2) {
		cerr << "使用方法: BetterConsolePauser.exe <程序名> [参数...]" << endl;
		system("pause");
		return 1;
	}
	
	system("cls");
	
	string commandLine;
	for (int i = 1; i < argc; ++i) {
		if (i > 1) commandLine += " ";
		string arg = argv[i];
		if (arg.find(' ') != string::npos && arg.front() != '"' && arg.back() != '"') {
			commandLine += "\"" + arg + "\"";
		} else {
			commandLine += arg;
		}
	}
	
	STARTUPINFOA si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	
	BOOL bSuccess = CreateProcessA(
								   NULL,
								   const_cast<char*>(commandLine.c_str()),
								   NULL,
								   NULL,
								   TRUE,
								   0,
								   NULL,
								   NULL,
								   &si,
								   &pi
								   );
	
	if (!bSuccess) {
		cerr << "无法启动程序: " << commandLine << endl;
		cerr << "错误代码: " << GetLastError() << endl;
		system("pause");
		return 1;
	}
	
	unsigned long long startTime = GetTickCount64Wrapper();
	
	FILETIME createTime1, exitTime1, kernelTime1, userTime1;
	GetProcessTimes(pi.hProcess, &createTime1, &exitTime1, &kernelTime1, &userTime1);
	
	ULARGE_INTEGER kernelStart, userStart;
	kernelStart.LowPart = kernelTime1.dwLowDateTime;
	kernelStart.HighPart = kernelTime1.dwHighDateTime;
	userStart.LowPart = userTime1.dwLowDateTime;
	userStart.HighPart = userTime1.dwHighDateTime;
	LONGLONG cpuStartTime = kernelStart.QuadPart + userStart.QuadPart;
	
	SIZE_T peakMemoryKB = 0;
	SIZE_T peakMemoryBytes = 0;
	DWORD waitResult;
	
	do {
		waitResult = WaitForSingleObject(pi.hProcess, 50);
		
		if (waitResult == WAIT_TIMEOUT) {
			PROCESS_MEMORY_COUNTERS pmc;
			if (GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc))) {
				SIZE_T currentKB = pmc.WorkingSetSize / 1024;
				if (currentKB > peakMemoryKB) {
					peakMemoryKB = currentKB;
					peakMemoryBytes = pmc.PeakWorkingSetSize;
				}
			}
		}
	} while (waitResult == WAIT_TIMEOUT);
	
	unsigned long long endTime = GetTickCount64Wrapper();
	
	FILETIME createTime2, exitTime2, kernelTime2, userTime2;
	GetProcessTimes(pi.hProcess, &createTime2, &exitTime2, &kernelTime2, &userTime2);
	
	ULARGE_INTEGER kernelEnd, userEnd;
	kernelEnd.LowPart = kernelTime2.dwLowDateTime;
	kernelEnd.HighPart = kernelTime2.dwHighDateTime;
	userEnd.LowPart = userTime2.dwLowDateTime;
	userEnd.HighPart = userTime2.dwHighDateTime;
	LONGLONG cpuEndTime = kernelEnd.QuadPart + userEnd.QuadPart;
	
	LONGLONG cpuTime100ns = cpuEndTime - cpuStartTime;
	long long cpuTimeMs = cpuTime100ns / 10000;
	unsigned long long elapsedMs = endTime - startTime;
	
	DWORD exitCode;
	GetExitCodeProcess(pi.hProcess, &exitCode);
	
	MEMORYSTATUSEX memStatus;
	memStatus.dwLength = sizeof(memStatus);
	GlobalMemoryStatusEx(&memStatus);
	DWORDLONG totalPhys = memStatus.ullTotalPhys;
	DWORDLONG availPhys = memStatus.ullAvailPhys;
	
	unsigned long long lastKernel = 0, lastUser = 0;
	double cpuUsage = GetProcessCpuUsage(pi.hProcess, &lastKernel, &lastUser);
	
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	
	cout << "\n========================================" << endl;
	cout << "        Better ConsolePauser" << endl;
	cout << "========================================" << endl;
	
	cout << " 墙上时钟时间: " << formatDurationMs(elapsedMs) << endl;
	cout << " CPU 运行时间: " << formatDurationMs(cpuTimeMs) << endl;
	
	if (peakMemoryKB > 0) {
		cout << " 物理内存峰值: " << peakMemoryKB << " KB";
		if (peakMemoryKB >= 1024) {
			cout << " (" << fixed << setprecision(2) << (double)peakMemoryKB / 1024.0 << " MB)";
		}
		cout << endl;
	} else
		cout << " 物理内存峰值: 无法获取 (进程可能已瞬间退出)" << endl;
	
	cout << " 系统总物理内存: " << fixed << setprecision(2) 
	<< (double)totalPhys / (1024.0 * 1024.0 * 1024.0) << " GB" << endl;
	cout << " 系统可用物理内存: " << fixed << setprecision(2) 
	<< (double)availPhys / (1024.0 * 1024.0 * 1024.0) << " GB" << endl;
	
	if (cpuUsage >= 0.01) {
		cout << " CPU 使用率: " << fixed << setprecision(2) << cpuUsage << "%" << endl;
	} else if (cpuUsage > 0) {
		cout << " CPU 使用率: < 0.01%" << endl;
	} else {
		cout << " CPU 使用率: 0.00%" << endl;
	}
	
	cout << " 退出代码: " << exitCode;
	cout << " (十六进制: 0x" << hex << exitCode << ")";
	cout << showExitMessage(exitCode) << endl;
	
	cout << "========================================" << endl;
	
	cout << endl;
	system("pause");
	
	system("cls");
	
	return 0;
}
