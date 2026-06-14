#include "protocol.h"

const char *Protocol_Process(char cmd)
{
	switch(cmd)
	{
	case 'A':
		return "Apple\r\n";

	case 'B':
		return "Banana\r\n";

	case 'C':
		return "Cherry\r\n";

	case 'D':
		return "Date\r\n";

	default:
		return "Unknown command\r\n";
	}
}
