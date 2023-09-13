#include "States.h"
#include "Address.h"
#include "Utils.h"
#include "injection.h"
void __stdcall MyInitRandomSeeds();




void __stdcall MyInitRandomSeeds()
{
	VALUEV(GetAddress(0x5AE410),SeedType) = 0;
	VALUEV(GetAddress(0x5AE420),SeedType) = 0;
	VALUEV(GetAddress(0x5AE428),SeedType) = 0;
	VALUEV(GetAddress(0x5AE430),SeedType) = 0;
}

void CopyToOriginalSeeds(SeedType seeds[4])
{
	VALUEV(GetAddress(0x5AE410),SeedType) = seeds[0];
	VALUEV(GetAddress(0x5AE420),SeedType) = seeds[1];
	VALUEV(GetAddress(0x5AE428),SeedType) = seeds[2];
	VALUEV(GetAddress(0x5AE430),SeedType) = seeds[3];
}

void CopyFromOriginalSeeds(SeedType seeds[4])
{
	seeds[0]=VALUEV(GetAddress(0x5AE410),SeedType);
	seeds[1]=VALUEV(GetAddress(0x5AE420),SeedType);
	seeds[2]=VALUEV(GetAddress(0x5AE428),SeedType);
	seeds[3]=VALUEV(GetAddress(0x5AE430),SeedType);
}

void CopyFromCustomSetting(DWORD cfg[3])
{
	cfg[0] =VALUED(GetAddress(0x00608644));
	cfg[1] =VALUED(GetAddress(0x00608648));
	cfg[2] =VALUED(GetAddress(0x0060864C));
}

void CopyToCustomSetting(DWORD cfg[3])
{
	VALUED(GetAddress(0x00608644)) = cfg[0];
	VALUED(GetAddress(0x00608648)) = cfg[1];
	VALUED(GetAddress(0x0060864C)) = cfg[2];
}

void GenNextSeeds(SeedType seeds[4], SeedType seeds_out[4],int n)
{
	for (int i = 0; i < 4; i++)
	{
		SeedType s = seeds[i] + n * n + 1 + i;
		s = (s ^ 0x9630) - 25939;
		s = 4 * s + (s >> 14);
		seeds_out[i] = s;
	}
}
extern bool g_is_loading;
void __stdcall InitStage()
{
	g_is_loading = true;
}
void __stdcall PlayerState()
{
	g_is_loading = false;
}


void __stdcall InitSelection()
{
	g_is_loading = false;
}

void __stdcall DestructPlayer()
{
	g_is_loading = true;
}

void __stdcall ResetPlayer()
{
	g_is_loading = true;
}

void InjectSeedStates()
{
	Hook((LPVOID)GetAddress(0x0052076C), 7, MyInitRandomSeeds);
	Hook((LPVOID)GetAddress(0x00530EE3), 6, PlayerState);
	Hook((LPVOID)GetAddress(0x00505050), 6, InitStage);
	Hook((LPVOID)GetAddress(0x0054443D), 5, InitSelection);
	Hook((LPVOID)GetAddress(0x00504DB6), 7, DestructPlayer);
	Hook((LPVOID)GetAddress(0x00532294), 11, ResetPlayer);
}