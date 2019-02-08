#define UNICODE
#define _UNICODE

#include <string>
#include <windows.h>
#include <tchar.h>
#include <WinInet.h>

#pragma comment (lib, "Wininet.lib")

using namespace std;

std::wstring OutputError(const std::wstring & msg)
{
	DWORD error = GetLastError();
	std::wstring outmsg = L"Error " + std::to_wstring(error) + L": " + msg + L"\n";
	OutputDebugString(outmsg.c_str());

	LPVOID lpMsgBuf;
	DWORD dw = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf,
		0, NULL);
	OutputDebugStringW((LPWSTR)lpMsgBuf);

	return outmsg;
}

// Retrieving Headers Using a Constant
BOOL PrintRequestHeader(HINTERNET hHttp)
{
	LPVOID lpOutBuffer = NULL;
	DWORD dwSize = 0;

retry:
	// This call will fail on the first pass, because
	// no buffer is allocated. HTTP_QUERY_RAW_HEADERS_CRLF
	if (!HttpQueryInfo(hHttp, HTTP_QUERY_RAW_HEADERS_CRLF | HTTP_QUERY_FLAG_REQUEST_HEADERS,
		(LPVOID)lpOutBuffer, &dwSize, NULL))
	{
		if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND)
		{
			// Code to handle the case where the header isn't available.
			OutputError(L"HttpQuery: Header not found");
			return TRUE;
		}
		else
		{
			// Check for an insufficient buffer.
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				// Allocate the necessary buffer.
				lpOutBuffer = new char[dwSize];
				
				// Retry the call.
				goto retry;
			}
			else
			{
				// Error handling code.
				OutputError(L"HttpQueryInfo error");
				if (lpOutBuffer)
				{
					delete[] lpOutBuffer;
				}
				return FALSE;
			}
		}
	}

	if (lpOutBuffer)
	{
		OutputDebugString((wchar_t*)lpOutBuffer);
		delete[] lpOutBuffer;
	}

	return TRUE;
}


// send https request
wstring SendHTTPSRequest_GET(const wstring& _server,
	const wstring& _page,
	const wstring& _params = L"")
{
	char szData[1024];
	string out("");

	// initialize WinInet
	HINTERNET hInternet = ::InternetOpen(L"WinInet Test", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet != NULL)
	{
		// open HTTP session
		HINTERNET hConnect = ::InternetConnect(hInternet, _server.c_str(), INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
		if (hConnect != NULL)
		{
			wstring request = _page + (_params.empty() ? L"" : (L"?" + _params));
			LPCWSTR rgpszAcceptTypes[] = { L"text/*", NULL};

			// open request
			HINTERNET hRequest = ::HttpOpenRequest(hConnect, L"GET", request.c_str(), L"HTTP/1.1", NULL, rgpszAcceptTypes, INTERNET_FLAG_SECURE, 1);
			if (hRequest != NULL)
			{
				// send request
				BOOL isSend = ::HttpSendRequest(hRequest, NULL, 0, NULL, 0);
				if (isSend)
				{
					// read data
					DWORD dwByteRead;
					while (::InternetReadFile(hRequest, reinterpret_cast<void*>(szData), sizeof(szData) - 1, &dwByteRead))
					{
						// break cycle if on end
						if (dwByteRead == 0) break;

						// save result
						szData[dwByteRead] = 0;
						out.append(szData);
					}
				}
				else
				{
					OutputError(L"Send failed");
				}
				PrintRequestHeader(hRequest);

				// close request
				::InternetCloseHandle(hRequest);
			}
			// close session
			::InternetCloseHandle(hConnect);
		}
		// close WinInet
		::InternetCloseHandle(hInternet);
	}

	return wstring(out.begin(), out.end());
}


int _tmain(int argc, _TCHAR* argv[])
{
	wstring answer = SendHTTPSRequest_GET(L"api.iextrading.com", L"1.0/stock/aapl/book", L"");
	OutputDebugString(L"\n");
	OutputDebugString(answer.c_str());
	OutputDebugString(L"\n");

	return 0;
}