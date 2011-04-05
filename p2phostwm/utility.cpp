/************************************************************/
/* [äeéÌã§í èàóù]                                           */
/*   + GetPeerAddress                                       */
/*   + GuidToString                                         */
/*   + GuidFromString                                       */
/************************************************************/
#include "stdafx.h"
#include "p2phostwm.h"

//-- äOïîéQè∆
extern std::list<PeerMap> MasterPeerMap;
extern CRITICAL_SECTION cs_MasterPeerMap;
////////////////////////////////////////////
//
// GetPeerAddress
//
////////////////////////////////////////////
void GetPeerAddress(GUID deviceId, struct sockaddr_in6* addr)
{
	DBGOUTEX2("[INFO0001]GetPeerAddress Function Starts.\n");

	::EnterCriticalSection(&cs_MasterPeerMap);
	std::list<PeerMap>::iterator it = MasterPeerMap.begin();
	while( it != MasterPeerMap.end() )
	{
		if(it->deviceId == deviceId)
		{
			*addr = it->address;
		}
		it++;
	}
	::LeaveCriticalSection(&cs_MasterPeerMap);

	DBGOUTEX2("[INFO0002]GetPeerAddress Function Ends.\n");
}

////////////////////////////////////////////
//
// IsEmptyGuid
//
////////////////////////////////////////////
BOOL IsEmptyGuid(GUID* guid)
{
	BOOL bRetValue = FALSE;
	GUID emptyGuid;

	::ZeroMemory(&emptyGuid, sizeof(emptyGuid));

	if(guid == NULL)
	{
		bRetValue = TRUE;
		goto EXIT;
	}
	if(::memcmp(guid, &emptyGuid, sizeof(emptyGuid))==0)
	{
		bRetValue = TRUE;
		goto EXIT;
	}

EXIT:
	return bRetValue;
}

////////////////////////////////////////////
//
// GuidToString
//
////////////////////////////////////////////
BOOL GuidToString(GUID guid, WCHAR* szString)
{
	DBGOUTEX2("[INFO0001]GuidToString Function Starts.\n");

	::swprintf(szString, L"%X-%X-%X-%X%X-%X%X%X%X%X%X", 
		guid.Data1, 
		guid.Data2, 
		guid.Data3, 
		guid.Data4[0],
		guid.Data4[1],
		guid.Data4[2],
		guid.Data4[3],
		guid.Data4[4],
		guid.Data4[5],
		guid.Data4[6],
		guid.Data4[7]
	);

	DBGOUTEX2("[INFO0002]GuidToString Function Ends.\n");

	return TRUE;
}

////////////////////////////////////////////
//
// GuidFromString
//
////////////////////////////////////////////
BOOL GuidFromString(GUID* guid, WCHAR* szString)
{
	DBGOUTEX2("[INFO0001]GuidFromString Function Starts.\n");

#ifndef P2PHOSTWM	
	::UuidFromString((RPC_WSTR)szString, (UUID *)guid);
#else
	::swscanf(szString,
		L"%8X-%X-%X-%2X%2X-%2X%2X%2X%2X%2X%2X",
		&guid->Data1, 
		&guid->Data2, 
		&guid->Data3, 
		&guid->Data4[0],
		&guid->Data4[1],
		&guid->Data4[2],
		&guid->Data4[3],
		&guid->Data4[4],
		&guid->Data4[5],
		&guid->Data4[6],
		&guid->Data4[7]
	);
#endif
	DBGOUTEX2("[INFO0002]GuidFromString Function Ends.\n");

	return TRUE;
}

