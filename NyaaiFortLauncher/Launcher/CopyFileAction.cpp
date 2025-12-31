#include "CopyFileAction.h"

GENERATE_BASE_CPP(NCopyFileAction)

void NCopyFileAction::Execute()
{
	if (!std::filesystem::exists(From))
	{
		Log(Error, L"Can't copy file \"{}\" as it doesn't exist.", From.wstring());
	}

	if (std::filesystem::exists(To))
	{
		std::filesystem::remove(To);
	}

	std::filesystem::copy(From, To);
	Log(Info, L"Copied file \"{}\" to \"{}\"", From.wstring(), To.wstring());
}
