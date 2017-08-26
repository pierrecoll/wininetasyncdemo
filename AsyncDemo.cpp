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
static REQUEST_CONTEXT rcContext1, rcContext2;

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
			SetDlgItemText(hX,IDC_URL2,
				LPSTR("http://home.microsoft.com"));

			//initialize the first context value
			rcContext1.hWindow = hX;
			rcContext1.nURL = IDC_URL1;
			rcContext1.nHeader = IDC_Header1;
			rcContext1.nResource = IDC_Resource1;
			sprintf(rcContext1.szMemo, "AsyncURL(%d)", 
				rcContext2.nURL);

			//initialize the second context value
			rcContext2.hWindow = hX;
			rcContext2.nURL = IDC_URL2;
			rcContext2.nHeader = IDC_Header2;
			rcContext2.nResource = IDC_Resource2;
			sprintf(rcContext2.szMemo, "AsyncURL(%d)", 
				rcContext2.nURL);

			//change the cursor back to normal
			SetCursor(LoadCursor(NULL,IDC_ARROW));
			return TRUE;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_EXIT:
					//change the cursor
					SetCursor(LoadCursor(NULL,IDC_WAIT));

					//close the dialog box
					EndDialog(hX,0);

					//return the cursor to normal
					SetCursor(LoadCursor(NULL,IDC_ARROW));
					return TRUE;
				case IDC_Download:

					hButton = GetDlgItem(hX, IDC_Download);
					EnableWindow(hButton,0);

					//reset the edit boxes
					SetDlgItemText(hX,IDC_Resource1,LPSTR(""));
					SetDlgItemText(hX,IDC_Resource2,LPSTR(""));
					SetDlgItemText(hX,IDC_Header1,LPSTR(""));
					SetDlgItemText(hX,IDC_Header2,LPSTR(""));

					//start the downloads
					AsyncDirect(&rcContext1, hOpen);
					AsyncDirect(&rcContext2, hOpen);
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
	sprintf(prcContext->szMemo, "AsyncDirect(%s)(%d):", szURL, prcContext->nURL);

	//call InternetOpenUrl
	prcContext->hResource = InternetOpenUrl(hOpen, szURL, NULL, 0, 0, (DWORD)prcContext);
	
	//check the HINTERNET handle for errors
	if (prcContext->hResource == NULL)
	{
		if (GetLastError() != ERROR_IO_PENDING)
		{
			//reuse the URL buffer for the error information
			sprintf(szURL,"Error %d encountered.",GetLastError());

			//write error to resource edit box
			SetDlgItemText(prcContext->hWindow, prcContext->nResource,
				szURL);
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
void __stdcall Juggler	(HINTERNET hInternet, DWORD dwContext,
							 DWORD dwInternetStatus,
							 LPVOID lpvStatusInformation,
							 DWORD dwStatusInformationLength)
{
	REQUEST_CONTEXT *cpContext;
	char szBuffer[256];
	cpContext= (REQUEST_CONTEXT*)dwContext;

	
	

	switch (dwInternetStatus)
	{
		case INTERNET_STATUS_CLOSING_CONNECTION:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: CLOSING_CONNECTION (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_CONNECTED_TO_SERVER:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: CONNECTED_TO_SERVER (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_CONNECTING_TO_SERVER:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: CONNECTING_TO_SERVER (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_CONNECTION_CLOSED:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: CONNECTION_CLOSED (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_HANDLE_CLOSING:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: HANDLE_CLOSING (%d)", 
				cpContext->szMemo, dwStatusInformationLength);

			sprintf(cpContext->szMemo, "Closed");
			
			//check if the both resource handles are closing
			//if so, enable the download button.
			if ((strcmp(rcContext1.szMemo,"Closed")) ||
				(strcmp(rcContext2.szMemo,"Closed")))
			{
				hButton = GetDlgItem(cpContext->hWindow, IDC_Download);
				EnableWindow(hButton,1);
			}
			break;
		case INTERNET_STATUS_HANDLE_CREATED:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: HANDLE_CREATED (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: INTERMEDIATE_RESPONSE (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_NAME_RESOLVED:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: NAME_RESOLVED (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_RECEIVING_RESPONSE:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: RECEIVEING_RESPONSE (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_RESPONSE_RECEIVED:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: RESPONSE_RECEIVED (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_REDIRECT:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: REDIRECT (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_REQUEST_COMPLETE:

			//check for errors
			if (LPINTERNET_ASYNC_RESULT(lpvStatusInformation)->dwError == 0)
			{
				//check if the completed request is from AsyncDirect
				if (strcmp(cpContext->szMemo, "AsyncDirect"))
				{
					//set the resource handle to the HINTERNET handle 
					//returned in the callback
					cpContext->hResource = HINTERNET(
						LPINTERNET_ASYNC_RESULT(lpvStatusInformation)->dwResult);

					//write the callback information to the buffer
					sprintf(szBuffer,"%s: REQUEST_COMPLETE (%d)", 
						cpContext->szMemo, dwStatusInformationLength);

					//create a thread to handle the header and 
					//resource download
					cpContext->hThread = CreateThread(NULL, 0, 
						(LPTHREAD_START_ROUTINE)Threader,LPVOID(cpContext), 0, 
						&cpContext->dwThreadID);

				}
				else
				{
					sprintf(szBuffer,"%s(%d): REQUEST_COMPLETE (%d)", 
						cpContext->szMemo,
						cpContext->nURL, dwStatusInformationLength);
				}

			}
			else
			{
				sprintf(szBuffer,
					"%s: REQUEST_COMPLETE (%d) Error (%d) encountered", 
					cpContext->szMemo, dwStatusInformationLength,
					GetLastError());
			}			
			break;
		case INTERNET_STATUS_REQUEST_SENT:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: REQUEST_SENT (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_RESOLVING_NAME:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: RESOLVING_NAME (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_SENDING_REQUEST:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: SENDING_REQUEST (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		case INTERNET_STATUS_STATE_CHANGE:
			//write the callback information to the buffer
			sprintf(szBuffer,"%s: STATE_CHANGE (%d)", 
				cpContext->szMemo, dwStatusInformationLength);
			break;
		default:
			//write the callback information to the buffer
			//pierrelc bug %s
			//sprintf(szBuffer, "%s: Unknown: Status %d Given", dwInternetStatus);
			sprintf(szBuffer, " Unknown: Status %d Given", dwInternetStatus);
			break;
	}

	//add the callback information to the callback list box
	SendDlgItemMessage(cpContext->hWindow,IDC_CallbackList,
		LB_ADDSTRING,0,(LPARAM)szBuffer);
	
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
	sprintf(cpContext->szMemo, "Header(%d)", cpContext->nURL);

	//call the header function
	Header(cpContext->hWindow, cpContext->nHeader, -1, cpContext->hResource);

	//set szMemo to the name of the function to be called
	sprintf(cpContext->szMemo, "Dump(%d)", cpContext->nURL);

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
			sprintf(szError,"Error %d encountered", GetLastError());
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
				sprintf(szError,"Error %d encountered", GetLastError());
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
			sprintf(szError,"Error %d encountered by InternetQueryDataAvailable",
				GetLastError());
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
				sprintf(szError,"Error %d encountered by InternetReadFile",
					GetLastError());
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
				strcat(lpszHolding,lpszData);

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
