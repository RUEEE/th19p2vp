#include "States.h"
#include "Address.h"
#include "Utils.h"
#include "injection.h"
void __stdcall MyInitRandomSeeds();




void __stdcall MyInitRandomSeeds()
{
	VALUEV(0x5AE410,SeedType) = 0;
	VALUEV(0x5AE420,SeedType) = 0;
	VALUEV(0x5AE428,SeedType) = 0;
	VALUEV(0x5AE430,SeedType) = 0;
}

void CopyToOriginalSeeds(SeedType seeds[4])
{
	VALUEV(0x5AE410,SeedType) = seeds[0];
	VALUEV(0x5AE420,SeedType) = seeds[1];
	VALUEV(0x5AE428,SeedType) = seeds[2];
	VALUEV(0x5AE430,SeedType) = seeds[3];
}

void CopyFromOriginalSeeds(SeedType seeds[4])
{
	seeds[0]=VALUEV(0x5AE410,SeedType);
	seeds[1]=VALUEV(0x5AE420,SeedType);
	seeds[2]=VALUEV(0x5AE428,SeedType);
	seeds[3]=VALUEV(0x5AE430,SeedType);
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

void InjectSeedStates()
{
	Hook((LPVOID)0x52076C, 7, MyInitRandomSeeds);

	Hook((LPVOID)0x00530EE3, 6, PlayerState);
	Hook((LPVOID)0x00505050, 6, InitStage);
	Hook((LPVOID)0x0054443D, 5, InitSelection);
	Hook((LPVOID)0x00504DB6, 7, DestructPlayer);
}