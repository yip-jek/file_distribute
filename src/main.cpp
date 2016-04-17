#include <iostream>
#include "file_distribute.h"
#include "log.h"

int main(int argc, char* argv[])
{
	if ( argc != 3 )
	{
		std::cerr << "[usage] " << argv[0] << " [ccm_id] [cfg]" << std::endl;
		return -1;
	}

	FileDistribute fd;
	try
	{
		fd.Init(argv[1], argv[2]);
		fd.Distribute();
	}
	catch ( Exception& ex )
	{
		std::cerr << "[MAIN] ERROR: " << ex.What() << " (CODE: " 
			<< ex.ErrorCode() << ")" << std::endl;
		Log::Instance()->Output("[MAIN] ERROR: %s (CODE: %d)", ex.What().c_str(), ex.ErrorCode());
		return -2;
	}
	catch ( ... )
	{
		std::cerr << "[MAIN] Unkown ERROR!!!" << std::endl;
		Log::Instance()->Output("[MAIN] Unkown ERROR!!!");
		return -3;
	}

	Log::Instance()->Output("[MAIN] %s quit!", argv[0]);
	Log::Release();
	return 0;
}

