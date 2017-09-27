/* Microsoft Corporation Copyright 1999 - 2000 */
/********************************************************************

	ProjectName	:	AsyncDemo

	Purpose		:	This sample demonstrates how to submit two 
					WinInet requests, using InternentOpenUrl,
					asynchronously.

	Notes		:	This sample does not handle any authentication.
					To properly handle authentication, the 
					functions that handle specific protocols (like
					HttpOpenRequest/HttpSendRequest) would need to 
					be used instead of InternetOpenUrl.  

	Last Updated:	February 2, 1998
					


*********************************************************************/


#include <windows.h>
#include <iostream>
using namespace std;
#include <string.h>
#include <stdio.h>
#include <wininet.h>
#include "resource.h"

//*******************************************************************
//					Global Variable Declarations
//*******************************************************************

LPSTR lpszAgent = "Asynchronous WinInet Demo Program";

//root HINTERNET handle
HINTERNET hOpen;

//instance of the callback function
INTERNET_STATUS_CALLBACK iscCallback;	

//structure to be passed as the dwContext value
typedef struct {
	HWND		hWindow;	//main window handle
	int			nURL;		//ID of the Edit Box w/ the URL
	int			nHeader;	//ID of the Edit Box for the header info
	int			nResource;	//ID of the Edit Box for the resource
	HINTERNET	hOpen;		//HINTERNET handle created by InternetOpen
	HINTERNET	hResource;	//HINTERNET handle created by InternetOpenUrl
	char		szMemo[512];//string to store status memo
	HANDLE		hThread;	//thread handle
	DWORD		dwThreadID;	//thread ID
} REQUEST_CONTEXT;


//two instances of the structure
static REQUEST_CONTEXT rcContext;

HWND hButton;

//*******************************************************************
//						Function Declarations
//*******************************************************************

//dialog box functions
BOOL CALLBACK AsyncURL(HWND, UINT, WPARAM, LPARAM);

//callback function
void __stdcall Juggler(HINTERNET, DWORD , DWORD , LPVOID, DWORD);

//thread function
DWORD Threader(LPVOID);

//functions
int WINAPI Dump(HWND, int, HINTERNET);
int WINAPI Header(HWND,int, int, HINTERNET);
void AsyncDirect (REQUEST_CONTEXT*, HINTERNET);




//*******************************************************************
//						    Main Program
//*******************************************************************
int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst, 
				   LPSTR lpszArgs, int nWinMode)
{

	//create the root HINTERNET handle using the systems default
	//settings.
	hOpen = InternetOpen(lpszAgent, INTERNET_OPEN_TYPE_PRECONFIG,
						NULL, NULL, INTERNET_FLAG_ASYNC);

	//check if the root HINTERNET handle has been created
	if (hOpen == NULL)
	{
		return FALSE;
	}
	else
	{
		//sets the callback function
		iscCallback = InternetSetStatusCallback(hOpen, (INTERNET_STATUS_CALLBACK)Juggler);

		//creates the dialog box
		DialogBox(hThisInst,"DB_ASYNCDEMO", HWND_DESKTOP,(DLGPROC)AsyncURL);

		//closes the root HINTERNET handle
		return InternetCloseHandle(hOpen);
	}


}


//*******************************************************************
//					       Dialog Functions
//*******************************************************************

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
BOOL CALLBACK AsyncURL(HWND hX, UINT message, WPARAM wParam, 
					   LPARAM lParam)
{
	

	switch(message)
	{
		case WM_INITDIALOG:		
			//change the cursor to indicate something is happening
			SetCursor(LoadCursor(NULL,IDC_WAIT));

			//set the default web sites
			SetDlgItemText(hX,IDC_URL1,
				LPSTR("http://www.microsoft.com"));


			//initialize the context value
			rcContext.hWindow = hX;
			rcContext.nURL = IDC_URL1;
			rcContext.nHeader = IDC_Header1;
			rcContext.nResource = IDC_Resource1;
			sprintf_s(rcContext.szMemo, "AsyncURL(%d)", rcContext.nURL);

			//change the cursor back to normal
			SetCursor(LoadCursor(NULL,IDC_ARROW));
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case 2:
				case IDC_EXIT2:
					//change the cursor
					SetCursor(LoadCursor(NULL,IDC_WAIT));

					//close the dialog box
					EndDialog(hX,0);

					//return the cursor to normal
					SetCursor(LoadCursor(NULL,IDC_ARROW));
					return TRUE;
				case IDC_Download:
					//Resets the callback list
					SendDlgItemMessage(hX, IDC_CallbackList, LB_RESETCONTENT, 0, NULL);

					hButton = GetDlgItem(hX, IDC_Download);
					EnableWindow(hButton,0);

					//reset the edit boxes
					SetDlgItemText(hX,IDC_Resource1,LPSTR(""));
					SetDlgItemText(hX,IDC_Header1,LPSTR(""));

					//start the downloads
					AsyncDirect(&rcContext, hOpen);
					return TRUE;
			}
			return FALSE;
		case WM_DESTROY:
			//change the cursor
			SetCursor(LoadCursor(NULL,IDC_WAIT));

			//close the dialog box
			EndDialog(hX,0);

			//return the cursor to normal
			SetCursor(LoadCursor(NULL,IDC_ARROW));
			return TRUE;
		default:
			return FALSE;

	}

}

//*******************************************************************
//					        Other Functions
//*******************************************************************

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  AsyncDirect handles the initial download request using 
  InternetOpenUrl.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void AsyncDirect (REQUEST_CONTEXT *prcContext, HINTERNET hOpen)
{
	//dim a buffer to hold the URL
	char szURL[256];

	//retrieve the URL from the designated edit box
	GetDlgItemText(prcContext->hWindow,prcContext->nURL,szURL,256);
	
	//update the memo in the REQUEST_CONTEXT structure
	sprintf_s(prcContext->szMemo, "AsyncDirect(%s)(%d):", szURL, prcContext->nURL);

	//call InternetOpenUrl
	prcContext->hResource = InternetOpenUrl(hOpen, szURL, NULL, 0, 0, (DWORD)prcContext);
	
	//check the HINTERNET handle for errors
	if (prcContext->hResource == NULL)
	{
		if (GetLastError() != ERROR_IO_PENDING)
		{
			//reuse the URL buffer for the error information
			sprintf_s(szURL,"Error %d encountered.",GetLastError());

			//write error to resource edit box
			SetDlgItemText(prcContext->hWindow, prcContext->nResource, szURL);
		}
	}

}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Juggler is the callback function that is registered using 
  InternetSetStatusCallback.

  Juggler displays the current callback in a list box with the ID,
  IDC_CallbackList.  The information displayed uses szMemo (which 
  contains the last function that was called), the 
  dwStatusInformationLength (to monitor the size of the information
  being returned),

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void __stdcall Juggler(HINTERNET hInternet, DWORD dwContext,
	DWORD dwInternetStatus,
	LPVOID lpvStatusInformation,
	DWORD dwStatusInformationLength)
{
	REQUEST_CONTEXT *cpContext;
	char szBuffer[256 + INTERNET_MAX_URL_LENGTH];
	cpContext = (REQUEST_CONTEXT*)dwContext;


	switch (dwInternetStatus)
	{
	case INTERNET_STATUS_CLOSING_CONNECTION:
		//Closing the connection to the server. The lpvStatusInformation parameter is NULL.
		sprintf_s(szBuffer, "CLOSING_CONNECTION");
		break;

	case INTERNET_STATUS_CONNECTED_TO_SERVER:
		//Successfully connected to the socket address (SOCKADDR) pointed to by lpvStatusInformation.
		//points to sa_data value directly 
		{
			SOCKADDR *lpSockAddr = (SOCKADDR *)lpvStatusInformation;
			if (lpSockAddr)
			{
				sprintf_s(szBuffer, "CONNECTED_TO_SERVER SockAddr : %s", (char *)lpvStatusInformation);
			}
			else
			{
				sprintf_s(szBuffer, "CONNECTED_TO_SERVER");
			}
		}
		break;

	case INTERNET_STATUS_CONNECTING_TO_SERVER:
		//Connecting to the socket address (SOCKADDR) pointed to by lpvStatusInformation.
		// points to sa_data value directly 
		{
			SOCKADDR *lpSockAddr = (SOCKADDR *)lpvStatusInformation;
			if (lpSockAddr)
			{
				sprintf_s(szBuffer, "CONNECTING_TO_SERVER SockAddr : %s", (char *)lpvStatusInformation);
			}
			else
			{
				sprintf_s(szBuffer, "CONNECTED_TO_SERVER ");
			}
		}
		break;

	case INTERNET_STATUS_CONNECTION_CLOSED:
		//Successfully closed the connection to the server. The lpvStatusInformation parameter is NULL.
		sprintf_s(szBuffer, "CONNECTION_CLOSED ");
		break;

	case INTERNET_STATUS_COOKIE_RECEIVED:
		//Indicates the number of cookies that were accepted, rejected,
		//downgraded (changed from persistent to session cookies), or
		//leashed (will be sent out only in 1st party context).
		//The lpvStatusInformation parameter is a DWORD with the number of cookies received.
		{
			if (lpvStatusInformation)
			{
				sprintf_s(szBuffer, "COOKIE_RECEIVED. Number : %u", *(DWORD *)lpvStatusInformation);
			}
			else
			{
				sprintf_s(szBuffer, "COOKIE_RECEIVED ");
			}
		}
		break;

	case INTERNET_STATUS_COOKIE_SENT:
		//Indicates the number of cookies that were either sent or suppressed, when a request is sent.
		//The lpvStatusInformation parameter is a DWORD with the number of cookies sent or suppressed.
		//WRONG : it is a pointer to the number
		{
			if (lpvStatusInformation)
			{
				sprintf_s(szBuffer, "COOKIE_SENT. Number : %u", *(DWORD *)lpvStatusInformation);
			}
			else
			{
				sprintf_s(szBuffer, "COOKIE_SENT ");
			}
		}
		break;

	case INTERNET_STATUS_COOKIE_HISTORY:
		//Retrieving content from the cache. Contains data about past cookie events for the URL such
		//as if cookies were accepted, rejected, downgraded, or leashed.
		//The lpvStatusInformation parameter is a pointer to an InternetCookieHistory structure.
		{
			if (lpvStatusInformation)
			{
				//The InternetCookieHistory structure contains the cookie hsitory.
				//typedef struct _InternetCookieHistory
				//{  BOOL fAccepted;  BOOL fLeashed;  BOOL fDowngraded;  BOOL fRejected;}
				//InternetCookieHistory,  *PInternetCookieHistory;
				//Members
				//fAccepted If true, the cookies was accepted.
				//fLeashed If true, the cookies was leashed.
				//fDowngraded If true, the cookies was downgraded.
				//fRejected If true, the cookies was rejected.

				InternetCookieHistory *ICH = (InternetCookieHistory*)lpvStatusInformation;
				sprintf_s(szBuffer, "COOKIE_HISTORY (%X)", (unsigned int)lpvStatusInformation);
			}
			else
			{
				sprintf_s(szBuffer, "COOKIE_HISTORY ");
			}
		}
		break;

	case INTERNET_STATUS_CTL_RESPONSE_RECEIVED:
		//Not implemented.
		sprintf_s(szBuffer, "CTL_RESPONSE_RECEIVED ");
		break;

	case INTERNET_STATUS_DETECTING_PROXY:
		//Notifies the client application that a proxy has been detected.
		sprintf_s(szBuffer, "DETECTING_PROXY ");
		break;

	case INTERNET_STATUS_HANDLE_CLOSING:
		//This handle value has been terminated.
		{
			INTERNET_ASYNC_RESULT *lpInternetAsyncResult = (INTERNET_ASYNC_RESULT*)lpvStatusInformation;

			if (lpInternetAsyncResult != NULL)
			{
				sprintf_s(szBuffer, "HANDLE_CLOSING Result: %X Error : %X", lpInternetAsyncResult->dwResult, lpInternetAsyncResult->dwError);
			}
			else
			{
				sprintf_s(szBuffer, "HANDLE_CLOSING (%d)",  dwStatusInformationLength);
			}
			sprintf_s(cpContext->szMemo, "Closed");
			hButton = GetDlgItem(cpContext->hWindow, IDC_Download);	
			EnableWindow(hButton, 1);
		}
		break;
	case INTERNET_STATUS_HANDLE_CREATED:
		//Used by InternetConnect to indicate it has created the new handle.
		//This lets the application call InternetCloseHandle from another thread, if the connect is taking too long.
		//The lpvStatusInformation parameter contains the address of an INTERNET_ASYNC_RESULT structure.
		/*
					
		// INTERNET_ASYNC_RESULT - this structure is returned to the application via
		// the callback with INTERNET_STATUS_REQUEST_COMPLETE. It is not sufficient to
		// just return the result of the async operation. If the API failed then the
		// app cannot call GetLastError() because the thread context will be incorrect.
		// Both the value returned by the async API and any resultant error code are
		// made available. The app need not check dwError if dwResult indicates that
		// the API succeeded (in this case dwError will be ERROR_SUCCESS)
		//

		typedef struct {

			//
			// dwResult - the HINTERNET, DWORD or BOOL return code from an async API
			//

			DWORD_PTR dwResult;

			//
			// dwError - the error code if the API failed
			//

			DWORD dwError;
		} INTERNET_ASYNC_RESULT, * LPINTERNET_ASYNC_RESULT;
		*/

		{
			INTERNET_ASYNC_RESULT *lpInternetAsyncResult = (INTERNET_ASYNC_RESULT*)lpvStatusInformation;
			if (lpInternetAsyncResult != NULL)
			{
				sprintf_s(szBuffer, "HANDLE_CREATED Result: %X Error : %X", lpInternetAsyncResult->dwResult, lpInternetAsyncResult->dwError);
			}
			else
			{
				sprintf_s(szBuffer, "HANDLE_CREATED ");
			}
		}
		break;
	case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
		//Received an intermediate (100 level) status code message from the server.
		sprintf_s(szBuffer, "INTERMEDIATE_RESPONSE ");
		break;

	case INTERNET_STATUS_NAME_RESOLVED:
		{
			//Successfully found the IP address of the name contained in lpvStatusInformation.
			if (lpvStatusInformation)
			{
				sprintf_s(szBuffer, "NAME_RESOLVED : %s", (char *)lpvStatusInformation);
			}
			else
			{
				sprintf_s(szBuffer, "NAME_RESOLVED ");
			}
		}
		break;

	case INTERNET_STATUS_P3P_HEADER:
		//The response has a P3P header in it.
		sprintf_s(szBuffer, "P3P_HEADER ");
		if (lpvStatusInformation)
		{
			sprintf_s(szBuffer, "P3P_HEADER : %s", (char *)lpvStatusInformation);
		}
		else
		{
			sprintf_s(szBuffer, "P3P_HEADER ");
		}
		break;

	case INTERNET_STATUS_P3P_POLICYREF:
		//Not implemented
		sprintf_s(szBuffer, "P3P_POLICYREF ");
		break;

	case INTERNET_STATUS_PREFETCH:
		//Not implemented
		sprintf_s(szBuffer, "PREFETCH ");
		break;

	case INTERNET_STATUS_PRIVACY_IMPACTED:
		//Not implemented
		sprintf_s(szBuffer, "PRIVACY_IMPACTED (%d)",
			 dwStatusInformationLength);
		break;

	case INTERNET_STATUS_RECEIVING_RESPONSE:
		//Waiting for the server to respond to a request. The lpvStatusInformation parameter is NULL.
		sprintf_s(szBuffer, "RECEIVEING_RESPONSE",  dwStatusInformationLength);
		break;

	case INTERNET_STATUS_RESPONSE_RECEIVED:
		{
		//Successfully received a response from the server.
		//The lpvStatusInformation parameter points to a DWORD value that contains the number, in bytes, received.
			if (lpvStatusInformation)
			{
				sprintf_s(szBuffer, "RESPONSE_RECEIVED : %d bytes", *(DWORD *)lpvStatusInformation);
			}
			else
			{
				sprintf_s(szBuffer, "RESPONSE_RECEIVED ");
			}
		}
		break;

	case INTERNET_STATUS_REDIRECT:
		{
			//An HTTP request is about to automatically redirect the request.
			//The lpvStatusInformation parameter points to the new URL.
			//At this point, the application can read any data returned by the server with the redirect
			//response and can query the response headers.
			//It can also cancel the operation by closing the handle.
			//This callback is not made if the original request specified INTERNET_FLAG_NO_AUTO_REDIRECT.
			if (lpvStatusInformation)
			{
				sprintf_s(szBuffer, "STATUS_REDIRECT . New URL: %s", (char *)lpvStatusInformation);
			}
			else
			{
				sprintf_s(szBuffer, "STATUS_REDIRECT ");
			}
		}
		break;

	case INTERNET_STATUS_REQUEST_COMPLETE:
		{
			if (lpvStatusInformation)
			{
				//write the callback information to the buffer
				sprintf_s(szBuffer, "REQUEST_COMPLETE Result:%X Error: %X", LPINTERNET_ASYNC_RESULT(lpvStatusInformation)->dwResult,LPINTERNET_ASYNC_RESULT(lpvStatusInformation)->dwError);
				//check for errors
				if (LPINTERNET_ASYNC_RESULT(lpvStatusInformation)->dwError == 0)
				{
					//check if the completed request is from AsyncDirect
					char *next_token1 = NULL;

					if (strtok_s(cpContext->szMemo, "AsyncDirect", &next_token1))
					{
						//set the resource handle to the HINTERNET handle 
						//returned in the callback
						cpContext->hResource = HINTERNET(LPINTERNET_ASYNC_RESULT(lpvStatusInformation)->dwResult);

						//create a thread to handle the header and 
						//resource download
						cpContext->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Threader, LPVOID(cpContext), 0, &cpContext->dwThreadID);
						//Threader(LPVOID(cpContext));
					}
					else
					{
						sprintf_s(szBuffer, "(%d): REQUEST_COMPLETE (%d)", cpContext->nURL, dwStatusInformationLength);
					}
				}
			}
			else
			{
				sprintf_s(szBuffer, "REQUEST_COMPLETE (%d) Error (%d) encountered", dwStatusInformationLength, GetLastError());
			}
		}
		break;

	case INTERNET_STATUS_REQUEST_SENT:
		//Successfully sent the information request to the server.
		//The lpvStatusInformation parameter points to a DWORD value that contains the number of bytes sent.
		if (lpvStatusInformation)
		{
			sprintf_s(szBuffer, "REQUEST_SENT : %d bytes", *(DWORD *)lpvStatusInformation);
		}
		else
		{
			sprintf_s(szBuffer, "REQUEST_SENT ");
		}
		break;

	case INTERNET_STATUS_RESOLVING_NAME:
		{
			//Looking up the IP address of the name contained in lpvStatusInformation.
			if (lpvStatusInformation)
			{
				sprintf_s(szBuffer, "RESOLVING_NAME : %s", (char *)lpvStatusInformation);
			}
			else
			{
				sprintf_s(szBuffer, "RESOLVING_NAME ");
			}
		}
		break;

	case INTERNET_STATUS_SENDING_REQUEST:
		//Sending the information request to the server. The lpvStatusInformation parameter is NULL.
		sprintf_s(szBuffer, "SENDING_REQUEST ");
		break;

	case INTERNET_STATUS_STATE_CHANGE:
		
	{
			//Moved between a secure (HTTPS) and a nonsecure (HTTP) site.
			//The user must be informed of this change; otherwise, the user is at risk of disclosing
			//sensitive information involuntarily. When this flag is set, the lpvStatusInformation parameter
			//points to a status DWORD that contains additional flags.
			//INTERNET_STATE_CONNECTED
			//Connected state. Mutually exclusive with disconnected state.
			//INTERNET_STATE_DISCONNECTED
			//Disconnected state. No network connection could be established.
			//INTERNET_STATE_DISCONNECTED_BY_USER
			//Disconnected by user request.
			//INTERNET_STATE_IDLE
			//No network requests are being made by Windows Internet.
			//INTERNET_STATE_BUSY
			//Network requests are being made by Windows Internet.
			//INTERNET_STATUS_USER_INPUT_REQUIRED
			//The request requires user input to be completed.
			DWORD dwStatus = *(DWORD*)lpvStatusInformation;


			switch (dwStatus)
			{
			case INTERNET_STATE_CONNECTED:
				sprintf_s(szBuffer, "STATE_CHANGE  : STATE_CONNECTED");
				break;
			case INTERNET_STATE_DISCONNECTED:
				sprintf_s(szBuffer, "STATE_CHANGE  : STATE_DISCONNECTED");
				break;
			case INTERNET_STATE_DISCONNECTED_BY_USER:
				sprintf_s(szBuffer, "STATE_CHANGE  : STATE_DISCONNECTED_BY_USER");
				break;
			case INTERNET_STATE_BUSY:
				sprintf_s(szBuffer, "STATE_CHANGE  : STATE_BUSY");
				break;

			case INTERNET_STATUS_USER_INPUT_REQUIRED:
				sprintf_s(szBuffer, "STATE_CHANGE  : STATUS_USER_INPUT_REQUIRED");
				break;
			default:
				sprintf_s(szBuffer, "STATE_CHANGE  : unknown status");
				break;
			}
		}
		break;

	default:
		//write the callback information to the buffer
		//pierrelc bug %s
		//sprintf_s(szBuffer, "Unknown: Status %d Given", dwInternetStatus);
		sprintf_s(szBuffer, " Unknown: Status %d Given", dwInternetStatus);
		break;
	}
	szBuffer[strlen(szBuffer)] = '\0';
	char szExtraInformation[128];
	if (lpvStatusInformation) 
	{
		sprintf_s(szExtraInformation, " ( %X / %X / %x) ", (unsigned int)lpvStatusInformation, *(unsigned*)lpvStatusInformation, (unsigned int)dwStatusInformationLength);
		szExtraInformation[strlen(szExtraInformation)] = '\0';
		strcat_s(szBuffer, sizeof(szBuffer), szExtraInformation);
	}

	//add the callback information to the callback list box
	LRESULT index= SendDlgItemMessage(cpContext->hWindow, IDC_CallbackList,LB_ADDSTRING, 0, (LPARAM)szBuffer);
	SendDlgItemMessage(cpContext->hWindow, IDC_CallbackList, LB_SETTOPINDEX, index, 0);

	if (iscCallback && (iscCallback != INTERNET_INVALID_STATUS_CALLBACK))
	{
		return (iscCallback(hInternet, dwContext,
			dwInternetStatus,
			lpvStatusInformation,
			dwStatusInformationLength));
	}
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Threader is a thread function to handle the retrieval of the header
  and resource information.  Since the WinInet functions involved in
  both of these operations work synchronously, using a separate 
  thread will help avoid any blocking.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
DWORD Threader(LPVOID lpvContext)
{
	REQUEST_CONTEXT *cpContext;
	
	//copy the pointer to a REQUEST_CONTEXT pointer to ease calls
	cpContext= (REQUEST_CONTEXT*)lpvContext;

	//set szMemo to the name of the function to be called
	sprintf_s(cpContext->szMemo, "Header(%d)", cpContext->nURL);

	//call the header function
	Header(cpContext->hWindow, cpContext->nHeader, -1, cpContext->hResource);

	//set szMemo to the name of the function to be called
	sprintf_s(cpContext->szMemo, "Dump(%d)", cpContext->nURL);

	//call the dump function
	Dump(cpContext->hWindow, cpContext->nResource, cpContext->hResource);

	return S_OK;

}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Header handles the retrieval of the HTTP header information.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
int WINAPI Header(HWND hX,int intControl, int intCustom, HINTERNET hHttp)
{
	DWORD dwHeaderType=-1;
	DWORD dwSize=0;
	LPVOID lpOutBuffer = NULL;

	char szError[80];

	//change the cursor
	SetCursor(LoadCursor(NULL,IDC_WAIT));


	//set the header type to be requested, which is all headers in
	//this case
	dwHeaderType = HTTP_QUERY_RAW_HEADERS_CRLF;	



retry:

	//call HttpQueryInfo.
	//first time with a zero buffer size to get the size of the buffer
	//needed and to check if the header exists
	if(!HttpQueryInfo(hHttp,dwHeaderType,(LPVOID)lpOutBuffer,&dwSize,NULL))
	{
		//check if the header was not found
		if (GetLastError()==ERROR_HTTP_HEADER_NOT_FOUND)
		{
			sprintf_s(szError,"Error %d encountered", GetLastError());
			SetDlgItemText(hX,intControl, szError);
			SetCursor(LoadCursor(NULL,IDC_ARROW));
			return TRUE;
		}
		else
		{
			//check if the buffer was insufficient
			if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
			{
				//allocate the buffer
				lpOutBuffer = new char[dwSize+1];
				goto retry;				
			}
			else
			{
				//display other errors in header edit box
				sprintf_s(szError,"Error %d encountered", GetLastError());
				SetDlgItemText(hX,intControl, szError);
				SetCursor(LoadCursor(NULL,IDC_ARROW));
				return FALSE;
			}
		}
	}


	SetDlgItemText(hX,intControl,(LPSTR)lpOutBuffer);
	delete[]lpOutBuffer;
	SetCursor(LoadCursor(NULL,IDC_ARROW));
	return 1;

}	
	

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
int WINAPI Dump(HWND hX, int intCtrlID, HINTERNET hResource)
{
	
	LPSTR	lpszData;		// buffer for the data
	DWORD	dwSize;			// size of the data available
	DWORD	dwDownloaded;	// size of the downloaded data
	DWORD	dwSizeSum=0;	// size of the data in the textbox
	LPSTR	lpszHolding;	// buffer to merge the textbox data and buffer

	char	szError[80];	// buffer for error information
	// Set the cursor to an hourglass.
	SetCursor(LoadCursor(NULL,IDC_WAIT));

	int		nCounter;		// counter used to display something while
							// I/O is pending.


	// This loop handles reading the data.  
	do
	{

try_again:
		// The call to InternetQueryDataAvailable determines the amount of 
		// data available to download.
		if (!InternetQueryDataAvailable(hResource,&dwSize,0,0))
		{
			if (GetLastError()== ERROR_IO_PENDING)
			{
				nCounter = 0;
				
				while(GetLastError()==ERROR_IO_PENDING)
				{
					nCounter++;

					if (nCounter==2000)
						break;
				}
				goto try_again;
			}
			sprintf_s(szError,"Error %d encountered by InternetQueryDataAvailable", GetLastError());
			SetDlgItemText(hX,intCtrlID, szError);
			SetCursor(LoadCursor(NULL,IDC_ARROW));
			break;
		}
		else
		{	
			// Allocates a buffer of the size returned by InternetQueryDataAvailable
			lpszData = new char[dwSize+1];

			// Reads the data from the HINTERNET handle.
			if(!InternetReadFile(hResource,(LPVOID)lpszData,dwSize,&dwDownloaded))
			{
				if (GetLastError()== ERROR_IO_PENDING)
				{
					nCounter = 0;
					
					while(GetLastError()==ERROR_IO_PENDING)
					{
						nCounter++;
					}
					goto keep_going;
				}
				sprintf_s(szError,"Error %d encountered by InternetReadFile", GetLastError());
				SetDlgItemText(hX,intCtrlID, szError);
				delete[] lpszData;
				break;
			}
			else
			{

keep_going:				
				// Adds a null terminator to the end of the data buffer
				lpszData[dwDownloaded]='\0';

				// Allocates the holding buffer
				lpszHolding = new char[dwSizeSum + dwDownloaded + 1];
				
				// Checks if there has been any data written to the textbox
				if (dwSizeSum != 0)
				{
					// Retrieves the data stored in the textbox if any
					GetDlgItemText(hX,intCtrlID,(LPSTR)lpszHolding,dwSizeSum);
					
					// Adds a null terminator at the end of the textbox data
					lpszHolding[dwSizeSum]='\0';
				}
				else
				{
					// Make the holding buffer an empty string. 
					lpszHolding[0]='\0';
				}

				// Adds the new data to the holding buffer
				strcat_s(lpszHolding, dwSizeSum + dwDownloaded + 1, (const char*)lpszData);

				// Writes the holding buffer to the textbox
				SetDlgItemText(hX,intCtrlID,(LPSTR)lpszHolding);

				// Delete the two buffers
				delete[] lpszHolding;
				delete[] lpszData;

				// Add the size of the downloaded data to the textbox data size
				dwSizeSum = dwSizeSum + dwDownloaded + 1;

				// Check the size of the remaining data.  If it is zero, break.
				if (dwDownloaded == 0)
					break;
			}
		}
	}
	while(TRUE);

	// Close the HINTERNET handle
	InternetCloseHandle(hResource);

	// Set the cursor back to an arrow
	SetCursor(LoadCursor(NULL,IDC_ARROW));

	// Return
	return TRUE;
}
					



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
