//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
#include "WordBatchProcessor.h"
#include "MultiServerSelfTest.h"
#include "AuthPaths.h"
//---------------------------------------------------------------------------
USEFORM("formMain.cpp", fMain);
//USEFORM("Frames\MyFrameDbg.cpp", FrameDbg); /* TFrame: File Type */
//USEFORM("Frames\FramePdtv.cpp", frPdtv); /* TFrame: File Type */
//USEFORM("Frames\FrameWay.cpp", frWay); /* TFrame: File Type */
//USEFORM("Frames\FrameAttachments.cpp", frAttachments); /* TFrame: File Type */
//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
#ifdef _DEBUG
	if (IsMultiServerSelfTestInvocation())
		return RunMultiServerSelfTestFromCommandLine();
	if (IsDocxTimeoutSelfTestInvocation())
		return RunDocxTimeoutSelfTestFromCommandLine();
	if (IsDocxCancelSelfTestInvocation())
		return RunDocxCancelSelfTestFromCommandLine();
	if (IsDocxRestartSelfTestInvocation())
		return RunDocxRestartSelfTestFromCommandLine();
	if (ParamCount() >= 1 && SameText(ParamStr(1), L"--auth-path-self-test"))
		return auth_paths::isRegularFile(
			auth_paths::fileBesideExecutable(L"admin984.bin")) ? 0 : 51;
#endif

	if (IsDocxWorkerInvocation())
		return RunDocxWorkerFromCommandLine();

	try
	{
		Application->Initialize();
		Application->MainFormOnTaskBar = true;
		Application->CreateForm(__classid(TfMain), &fMain);
		ReportMemoryLeaksOnShutdown = true;
		Application->Run();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
		return 1;
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
		return 1;
	}
	return 0;
}
//---------------------------------------------------------------------------
