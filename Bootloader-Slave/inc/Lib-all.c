/*
 * Lib_all.c
 *
 * Created: 4.2.2016 16:27:09
 *  Author: Lukas
 */ 

#include "AllInit.h"

char Cteni_pinu(char Jaky_Port, char Jaky_Pin)
{
	if ((Jaky_Port & Jaky_Pin) == Jaky_Pin)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}